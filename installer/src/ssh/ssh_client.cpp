#include "ssh/ssh_client.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QString>
#include <QtGlobal>

#include <atomic>
#include <cstring>
#include <string>
#include <vector>

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
using socket_t = SOCKET;
#  define SOCK_INVALID INVALID_SOCKET
#  define SOCK_CLOSE closesocket
#else
#  include <arpa/inet.h>
#  include <netdb.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <unistd.h>
using socket_t = int;
#  define SOCK_INVALID (-1)
#  define SOCK_CLOSE ::close
#endif

#include <libssh2.h>
#include <libssh2_sftp.h>

namespace rm::installer::ssh {

namespace {

class GlobalInit {
 public:
    GlobalInit() {
#ifdef _WIN32
        WSADATA w;
        WSAStartup(MAKEWORD(2, 2), &w);
#endif
        libssh2_init(0);
    }
    ~GlobalInit() {
        libssh2_exit();
#ifdef _WIN32
        WSACleanup();
#endif
    }
};

void ensureGlobalInit() {
    static GlobalInit g;
    (void)g;
}

socket_t openTcp(const QString& host, quint16 port, int timeoutMs, QString* err) {
    addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* res = nullptr;
    const std::string host_s = host.toStdString();
    const std::string port_s = std::to_string(port);
    if (getaddrinfo(host_s.c_str(), port_s.c_str(), &hints, &res) != 0 || !res) {
        if (err) *err = QStringLiteral("DNS lookup failed for %1").arg(host);
        return SOCK_INVALID;
    }

    socket_t sock = SOCK_INVALID;
    for (addrinfo* p = res; p != nullptr; p = p->ai_next) {
        socket_t s = static_cast<socket_t>(::socket(p->ai_family, p->ai_socktype,
                                                    p->ai_protocol));
        if (s == SOCK_INVALID) continue;

#ifdef _WIN32
        DWORD tv = static_cast<DWORD>(timeoutMs);
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv),
                   sizeof(tv));
        setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&tv),
                   sizeof(tv));
#else
        timeval tv{};
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif

        if (::connect(s, p->ai_addr, static_cast<int>(p->ai_addrlen)) == 0) {
            sock = s;
            break;
        }
        SOCK_CLOSE(s);
    }
    freeaddrinfo(res);
    if (sock == SOCK_INVALID && err) {
        *err = QStringLiteral("TCP connect to %1:%2 failed").arg(host).arg(port);
    }
    return sock;
}

QString sha256FingerprintBase64(LIBSSH2_SESSION* session) {
    const char* hash = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA256);
    if (!hash) return {};
    const QByteArray raw(hash, 32);
    QByteArray b64 = raw.toBase64(QByteArray::Base64Option::OmitTrailingEquals);
    return QStringLiteral("SHA256:") + QString::fromUtf8(b64);
}

QString libssh2Error(LIBSSH2_SESSION* session) {
    char* msg = nullptr;
    int len = 0;
    libssh2_session_last_error(session, &msg, &len, 0);
    if (!msg || len == 0) return QStringLiteral("unknown libssh2 error");
    return QString::fromUtf8(msg, len);
}

}

struct SshClient::Impl {
    QString host;
    quint16 port = 22;
    socket_t sock = SOCK_INVALID;
    LIBSSH2_SESSION* session = nullptr;
    QString fingerprint;
    bool authed = false;
    std::atomic<bool> aborting{false};
    int sessionTimeoutMs = 300000;

    ~Impl() { tearDown(); }

    void tearDown() {
        if (session) {
            libssh2_session_disconnect(session, "bye");
            libssh2_session_free(session);
            session = nullptr;
        }
        if (sock != SOCK_INVALID) {
            SOCK_CLOSE(sock);
            sock = SOCK_INVALID;
        }
        authed = false;
        fingerprint.clear();
    }
};

SshClient::SshClient(QObject* parent) : QObject(parent), d_(std::make_unique<Impl>()) {
    ensureGlobalInit();
}

SshClient::~SshClient() = default;

bool SshClient::connectToHost(const QString& host, quint16 port, int timeoutMs,
                              QString* err) {
    d_->tearDown();
    d_->host = host;
    d_->port = port;
    d_->sock = openTcp(host, port, timeoutMs, err);
    if (d_->sock == SOCK_INVALID) return false;

    d_->session = libssh2_session_init();
    if (!d_->session) {
        if (err) *err = QStringLiteral("libssh2_session_init failed");
        d_->tearDown();
        return false;
    }
    libssh2_session_set_blocking(d_->session, 1);
    libssh2_session_set_timeout(d_->session, timeoutMs);

    const int rc = libssh2_session_handshake(d_->session, d_->sock);
    if (rc != 0) {
        if (err) *err = QStringLiteral("SSH handshake failed: %1")
                            .arg(libssh2Error(d_->session));
        d_->tearDown();
        return false;
    }
    d_->fingerprint = sha256FingerprintBase64(d_->session);
    libssh2_session_set_timeout(d_->session, d_->sessionTimeoutMs);
    return true;
}

bool SshClient::isConnected() const { return d_->session != nullptr; }

void SshClient::disconnectFromHost() { d_->tearDown(); }

QString SshClient::serverFingerprint() const { return d_->fingerprint; }

bool SshClient::authenticatePassword(const QString& user, const QString& password,
                                     QString* err) {
    if (!d_->session) {
        if (err) *err = QStringLiteral("not connected");
        return false;
    }
    const QByteArray u = user.toUtf8();
    const QByteArray p = password.toUtf8();
    const int rc = libssh2_userauth_password_ex(
        d_->session, u.constData(), static_cast<unsigned int>(u.size()),
        p.constData(), static_cast<unsigned int>(p.size()), nullptr);
    if (rc != 0) {
        if (err) *err = QStringLiteral("password auth failed: %1")
                            .arg(libssh2Error(d_->session));
        return false;
    }
    d_->authed = true;
    return true;
}

bool SshClient::authenticatePrivateKeyFile(const QString& user,
                                           const QString& privateKeyPath,
                                           const QString& passphrase,
                                           QString* err) {
    if (!d_->session) {
        if (err) *err = QStringLiteral("not connected");
        return false;
    }
    const QByteArray u = user.toUtf8();
    const QByteArray priv = privateKeyPath.toUtf8();
    const QByteArray pub = (privateKeyPath + QStringLiteral(".pub")).toUtf8();
    const QByteArray pass = passphrase.toUtf8();

    const char* pubPtr = QFileInfo::exists(privateKeyPath + QStringLiteral(".pub"))
                             ? pub.constData() : nullptr;
    const int rc = libssh2_userauth_publickey_fromfile_ex(
        d_->session, u.constData(), static_cast<unsigned int>(u.size()),
        pubPtr, priv.constData(),
        passphrase.isEmpty() ? nullptr : pass.constData());
    if (rc != 0) {
        if (err) *err = QStringLiteral("publickey auth failed: %1")
                            .arg(libssh2Error(d_->session));
        return false;
    }
    d_->authed = true;
    return true;
}

bool SshClient::isAuthenticated() const { return d_->authed; }

void SshClient::abort() {
    d_->aborting = true;

    if (d_->sock != SOCK_INVALID) {
        SOCK_CLOSE(d_->sock);
        d_->sock = SOCK_INVALID;
    }
}

namespace {

void splitAndEmit(SshClient* self, Stream stream, QByteArray& buffer) {
    int start = 0;
    while (true) {
        const int nl = buffer.indexOf('\n', start);
        if (nl < 0) break;
        QByteArray line = buffer.mid(start, nl - start);
        if (line.endsWith('\r')) line.chop(1);
        emit self->lineReceived(stream, line);
        start = nl + 1;
    }
    if (start > 0) {
        buffer.remove(0, start);
    }
}

}

ExecResult SshClient::exec(const QString& command, int timeoutMs) {
    ExecResult result;
    if (!d_->session || !d_->authed) {
        result.stdErr = "not authenticated";
        return result;
    }
    libssh2_session_set_timeout(d_->session, timeoutMs);
    d_->aborting = false;

    LIBSSH2_CHANNEL* ch = libssh2_channel_open_session(d_->session);
    if (!ch) {
        result.stdErr = libssh2Error(d_->session).toUtf8();
        return result;
    }

    const QByteArray cmdBytes = command.toUtf8();
    if (libssh2_channel_exec(ch, cmdBytes.constData()) != 0) {
        result.stdErr = libssh2Error(d_->session).toUtf8();
        libssh2_channel_free(ch);
        return result;
    }

    QByteArray stdoutPartial, stderrPartial;
    char buf[8192];
    QElapsedTimer elapsed;
    elapsed.start();
    while (true) {
        if (d_->aborting) {
            result.timedOut = true;
            break;
        }
        const ssize_t n = libssh2_channel_read(ch, buf, sizeof(buf));
        if (n > 0) {
            QByteArray chunk(buf, static_cast<int>(n));
            result.stdOut.append(chunk);
            stdoutPartial.append(chunk);
            splitAndEmit(this, Stream::StdOut, stdoutPartial);
        } else if (n == 0 && libssh2_channel_eof(ch)) {

        } else if (n < 0 && n != LIBSSH2_ERROR_EAGAIN) {
            break;
        }
        const ssize_t en = libssh2_channel_read_stderr(ch, buf, sizeof(buf));
        if (en > 0) {
            QByteArray chunk(buf, static_cast<int>(en));
            result.stdErr.append(chunk);
            stderrPartial.append(chunk);
            splitAndEmit(this, Stream::StdErr, stderrPartial);
        }

        if (libssh2_channel_eof(ch) && n <= 0 && en <= 0) {
            break;
        }
        if (elapsed.elapsed() > timeoutMs) {
            result.timedOut = true;
            break;
        }
    }

    if (!stdoutPartial.isEmpty()) {
        emit lineReceived(Stream::StdOut, stdoutPartial);
    }
    if (!stderrPartial.isEmpty()) {
        emit lineReceived(Stream::StdErr, stderrPartial);
    }

    libssh2_channel_close(ch);
    result.exitCode = libssh2_channel_get_exit_status(ch);
    libssh2_channel_free(ch);
    libssh2_session_set_timeout(d_->session, d_->sessionTimeoutMs);
    return result;
}

bool SshClient::sftpPut(const QString& localPath, const QString& remotePath,
                        int remoteMode, int timeoutMs, QString* err) {
    if (!d_->session || !d_->authed) {
        if (err) *err = QStringLiteral("not authenticated");
        return false;
    }
    QFile f(localPath);
    if (!f.open(QIODevice::ReadOnly)) {
        if (err) *err = QStringLiteral("cannot open local file %1").arg(localPath);
        return false;
    }

    libssh2_session_set_timeout(d_->session, timeoutMs);

    LIBSSH2_SFTP* sftp = libssh2_sftp_init(d_->session);
    if (!sftp) {
        if (err) *err = QStringLiteral("sftp_init: %1").arg(libssh2Error(d_->session));
        return false;
    }

    const QByteArray rp = remotePath.toUtf8();
    LIBSSH2_SFTP_HANDLE* out = libssh2_sftp_open_ex(
        sftp, rp.constData(), rp.size(),
        LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC, remoteMode,
        LIBSSH2_SFTP_OPENFILE);
    if (!out) {
        if (err) *err = QStringLiteral("sftp_open(%1): %2").arg(
                            remotePath, libssh2Error(d_->session));
        libssh2_sftp_shutdown(sftp);
        return false;
    }

    constexpr qint64 kChunk = 32 * 1024;
    while (!f.atEnd()) {
        if (d_->aborting) {
            if (err) *err = QStringLiteral("aborted");
            libssh2_sftp_close(out);
            libssh2_sftp_shutdown(sftp);
            return false;
        }
        const QByteArray chunk = f.read(kChunk);
        const char* p = chunk.constData();
        qint64 remaining = chunk.size();
        while (remaining > 0) {
            const ssize_t w = libssh2_sftp_write(out, p, remaining);
            if (w < 0) {
                if (err) *err = QStringLiteral("sftp_write: %1")
                                    .arg(libssh2Error(d_->session));
                libssh2_sftp_close(out);
                libssh2_sftp_shutdown(sftp);
                return false;
            }
            p += w;
            remaining -= w;
        }
    }

    libssh2_sftp_close(out);
    libssh2_sftp_shutdown(sftp);
    libssh2_session_set_timeout(d_->session, d_->sessionTimeoutMs);
    return true;
}

}
