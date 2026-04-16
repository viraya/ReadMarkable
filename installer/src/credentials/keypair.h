#pragma once

#include <QByteArray>
#include <QString>
#include <optional>

namespace rm::installer::credentials {

struct Ed25519Keypair {
    QByteArray privateKeyPem;
    QByteArray publicKeyOpenSsh;
    QString fingerprintSha256;

    static std::optional<Ed25519Keypair> generate(const QString& comment,
                                                  QString* err = nullptr);

    bool saveToFiles(const QString& privatePath, const QString& publicPath,
                     QString* err = nullptr) const;

    static std::optional<Ed25519Keypair> loadFromFiles(const QString& privatePath,
                                                        const QString& publicPath,
                                                        QString* err = nullptr);

    static QString computeFingerprint(const QByteArray& openSshPublicKeyLine);
};

}
