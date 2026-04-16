#include "credentials/keypair.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QString>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

#include <cstring>
#include <memory>

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  include <aclapi.h>
#  include <sddl.h>
#else
#  include <sys/stat.h>
#endif

namespace rm::installer::credentials {

namespace {

void sshWireAppendString(QByteArray& out, const char* bytes, int len);

QByteArray opensshPrivateKeyPem(const unsigned char* pub, size_t publen,
                                const unsigned char* privSeed, size_t privSeedLen,
                                const QString& comment, QString* err) {
    if (publen != 32 || privSeedLen != 32) {
        if (err) *err = QStringLiteral("Ed25519 key sizes unexpected");
        return {};
    }

    QByteArray priv;

    const quint32 checkint = 0x01020304u;
    for (int shift : {24, 16, 8, 0}) priv.append(static_cast<char>((checkint >> shift) & 0xff));
    for (int shift : {24, 16, 8, 0}) priv.append(static_cast<char>((checkint >> shift) & 0xff));
    sshWireAppendString(priv, "ssh-ed25519", 11);
    sshWireAppendString(priv, reinterpret_cast<const char*>(pub), 32);

    QByteArray seedPlusPub(reinterpret_cast<const char*>(privSeed), 32);
    seedPlusPub.append(reinterpret_cast<const char*>(pub), 32);
    sshWireAppendString(priv, seedPlusPub.constData(), seedPlusPub.size());
    const QByteArray commentUtf8 = comment.toUtf8();
    sshWireAppendString(priv, commentUtf8.constData(), commentUtf8.size());

    int pad = 1;
    while (priv.size() % 8 != 0) {
        priv.append(static_cast<char>(pad++));
    }

    QByteArray pubWire;
    sshWireAppendString(pubWire, "ssh-ed25519", 11);
    sshWireAppendString(pubWire, reinterpret_cast<const char*>(pub), 32);

    QByteArray blob;
    blob.append("openssh-key-v1", 14);
    blob.append('\0');
    sshWireAppendString(blob, "none", 4);
    sshWireAppendString(blob, "none", 4);
    sshWireAppendString(blob, "", 0);
    for (int shift : {24, 16, 8, 0})
        blob.append(static_cast<char>((1u >> shift) & 0xff));
    sshWireAppendString(blob, pubWire.constData(), pubWire.size());
    sshWireAppendString(blob, priv.constData(), priv.size());

    const QByteArray b64 = blob.toBase64();
    QByteArray out;
    out.append("-----BEGIN OPENSSH PRIVATE KEY-----\n");
    for (int i = 0; i < b64.size(); i += 70) {
        out.append(b64.mid(i, 70));
        out.append('\n');
    }
    out.append("-----END OPENSSH PRIVATE KEY-----\n");
    return out;
}

void sshWireAppendString(QByteArray& out, const char* bytes, int len) {
    quint32 n = static_cast<quint32>(len);
    char lenBuf[4] = {static_cast<char>((n >> 24) & 0xff),
                      static_cast<char>((n >> 16) & 0xff),
                      static_cast<char>((n >> 8) & 0xff),
                      static_cast<char>(n & 0xff)};
    out.append(lenBuf, 4);
    out.append(bytes, len);
}

QByteArray formatSshEd25519PublicKey(const unsigned char* pub, size_t publen,
                                     const QString& comment) {
    QByteArray wire;
    sshWireAppendString(wire, "ssh-ed25519", 11);
    sshWireAppendString(wire, reinterpret_cast<const char*>(pub),
                        static_cast<int>(publen));
    const QByteArray b64 = wire.toBase64();
    QByteArray out;
    out.append("ssh-ed25519 ");
    out.append(b64);
    if (!comment.isEmpty()) {
        out.append(' ');
        out.append(comment.toUtf8());
    }
    out.append('\n');
    return out;
}

QString sha256OfSshPublicKeyBase64Blob(const QByteArray& singleLine) {

    const int firstSpace = singleLine.indexOf(' ');
    if (firstSpace < 0) return {};
    const int secondSpace = singleLine.indexOf(' ', firstSpace + 1);
    const QByteArray b64 = (secondSpace > 0)
                               ? singleLine.mid(firstSpace + 1,
                                                secondSpace - firstSpace - 1)
                               : singleLine.mid(firstSpace + 1).trimmed();
    const QByteArray blob = QByteArray::fromBase64(b64);
    const QByteArray digest = QCryptographicHash::hash(blob,
                                                       QCryptographicHash::Sha256);
    return QStringLiteral("SHA256:") +
           QString::fromUtf8(digest.toBase64(QByteArray::OmitTrailingEquals));
}

bool restrictFilePermissions(const QString& path, QString* err) {
#ifdef _WIN32

    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        if (err) *err = QStringLiteral("OpenProcessToken failed");
        return false;
    }
    DWORD size = 0;
    GetTokenInformation(hToken, TokenUser, nullptr, 0, &size);
    std::unique_ptr<BYTE[]> buf(new BYTE[size]);
    if (!GetTokenInformation(hToken, TokenUser, buf.get(), size, &size)) {
        CloseHandle(hToken);
        if (err) *err = QStringLiteral("GetTokenInformation failed");
        return false;
    }
    CloseHandle(hToken);
    PSID userSid = reinterpret_cast<TOKEN_USER*>(buf.get())->User.Sid;

    EXPLICIT_ACCESSA ea = {};
    ea.grfAccessPermissions = GENERIC_ALL;
    ea.grfAccessMode = SET_ACCESS;
    ea.grfInheritance = NO_INHERITANCE;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
    ea.Trustee.ptstrName = reinterpret_cast<LPSTR>(userSid);

    PACL newAcl = nullptr;
    if (SetEntriesInAclA(1, &ea, nullptr, &newAcl) != ERROR_SUCCESS) {
        if (err) *err = QStringLiteral("SetEntriesInAcl failed");
        return false;
    }
    const QByteArray pathUtf8 = path.toUtf8();
    const DWORD rc = SetNamedSecurityInfoA(
        const_cast<LPSTR>(pathUtf8.constData()), SE_FILE_OBJECT,
        DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, nullptr,
        nullptr, newAcl, nullptr);
    LocalFree(newAcl);
    if (rc != ERROR_SUCCESS) {
        if (err) *err = QStringLiteral("SetNamedSecurityInfo failed: %1").arg(rc);
        return false;
    }
    return true;
#else
    const QByteArray p = path.toUtf8();
    if (chmod(p.constData(), 0600) != 0) {
        if (err) *err = QStringLiteral("chmod 0600 failed");
        return false;
    }
    return true;
#endif
}

}

std::optional<Ed25519Keypair> Ed25519Keypair::generate(const QString& comment,
                                                       QString* err) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_from_name(nullptr, "ED25519", nullptr);
    if (!ctx) {
        if (err) *err = QStringLiteral("EVP_PKEY_CTX_new_from_name(ED25519) failed");
        return std::nullopt;
    }
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        if (err) *err = QStringLiteral("EVP_PKEY_keygen_init failed");
        return std::nullopt;
    }
    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0 || !pkey) {
        EVP_PKEY_CTX_free(ctx);
        if (err) *err = QStringLiteral("EVP_PKEY_keygen failed");
        return std::nullopt;
    }
    EVP_PKEY_CTX_free(ctx);

    unsigned char pub[32];
    size_t publen = sizeof(pub);
    if (EVP_PKEY_get_raw_public_key(pkey, pub, &publen) <= 0 || publen != 32) {
        EVP_PKEY_free(pkey);
        if (err) *err = QStringLiteral("EVP_PKEY_get_raw_public_key failed");
        return std::nullopt;
    }
    unsigned char privSeed[32];
    size_t privSeedLen = sizeof(privSeed);
    if (EVP_PKEY_get_raw_private_key(pkey, privSeed, &privSeedLen) <= 0
        || privSeedLen != 32) {
        EVP_PKEY_free(pkey);
        if (err) *err = QStringLiteral("EVP_PKEY_get_raw_private_key failed");
        return std::nullopt;
    }
    EVP_PKEY_free(pkey);

    Ed25519Keypair kp;
    kp.privateKeyPem = opensshPrivateKeyPem(pub, publen, privSeed, privSeedLen,
                                            comment, err);
    if (kp.privateKeyPem.isEmpty()) {
        return std::nullopt;
    }

    kp.publicKeyOpenSsh = formatSshEd25519PublicKey(pub, publen, comment);
    kp.fingerprintSha256 = sha256OfSshPublicKeyBase64Blob(kp.publicKeyOpenSsh);
    return kp;
}

bool Ed25519Keypair::saveToFiles(const QString& privatePath,
                                 const QString& publicPath, QString* err) const {
    QDir().mkpath(QFileInfo(privatePath).absolutePath());
    QDir().mkpath(QFileInfo(publicPath).absolutePath());

    {
        QSaveFile f(privatePath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (err) *err = QStringLiteral("cannot write %1").arg(privatePath);
            return false;
        }
        f.write(privateKeyPem);
        if (!f.commit()) {
            if (err) *err = QStringLiteral("commit failed: %1").arg(privatePath);
            return false;
        }
    }
    if (!restrictFilePermissions(privatePath, err)) return false;

    {
        QSaveFile f(publicPath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (err) *err = QStringLiteral("cannot write %1").arg(publicPath);
            return false;
        }
        f.write(publicKeyOpenSsh);
        if (!f.commit()) {
            if (err) *err = QStringLiteral("commit failed: %1").arg(publicPath);
            return false;
        }
    }
    return true;
}

std::optional<Ed25519Keypair> Ed25519Keypair::loadFromFiles(
    const QString& privatePath, const QString& publicPath, QString* err) {
    Ed25519Keypair kp;
    QFile priv(privatePath);
    if (!priv.open(QIODevice::ReadOnly)) {
        if (err) *err = QStringLiteral("cannot read %1").arg(privatePath);
        return std::nullopt;
    }
    kp.privateKeyPem = priv.readAll();

    QFile pub(publicPath);
    if (!pub.open(QIODevice::ReadOnly)) {
        if (err) *err = QStringLiteral("cannot read %1").arg(publicPath);
        return std::nullopt;
    }
    kp.publicKeyOpenSsh = pub.readAll();
    kp.fingerprintSha256 = sha256OfSshPublicKeyBase64Blob(kp.publicKeyOpenSsh);
    if (kp.fingerprintSha256.isEmpty()) {
        if (err) *err = QStringLiteral("public key has no parseable blob");
        return std::nullopt;
    }
    return kp;
}

QString Ed25519Keypair::computeFingerprint(const QByteArray& openSshPublicKeyLine) {
    return sha256OfSshPublicKeyBase64Blob(openSshPublicKeyLine);
}

}
