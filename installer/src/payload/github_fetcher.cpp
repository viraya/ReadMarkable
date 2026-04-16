#include "payload/github_fetcher.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QTimer>

namespace rm::installer::payload {

namespace {

QByteArray syncGet(QNetworkAccessManager* nam, const QUrl& url,
                   const QString& token, int* httpStatus, QString* err) {
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", "ReadMarkable-Installer");
    req.setRawHeader("Accept", "application/vnd.github+json");
    if (!token.isEmpty()) {
        req.setRawHeader("Authorization",
                         QByteArray("Bearer ") + token.toUtf8());
    }
    QNetworkReply* reply = nam->get(req);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        if (err) *err = reply->errorString();
        if (httpStatus) {
            *httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute)
                              .toInt();
        }
        reply->deleteLater();
        return {};
    }
    const QByteArray data = reply->readAll();
    if (httpStatus) {
        *httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute)
                          .toInt();
    }
    reply->deleteLater();
    return data;
}

}

GithubFetcher::GithubFetcher(QObject* parent)
    : QObject(parent), nam_(new QNetworkAccessManager(this)) {}
GithubFetcher::~GithubFetcher() = default;

bool GithubFetcher::fetchLatestRelease(const QString& ownerRepo,
                                       ReleaseInfo& out, const QString& token,
                                       QString* err) {
    const QUrl url(
        QStringLiteral("https://api.github.com/repos/%1/releases/latest")
            .arg(ownerRepo));
    int status = 0;
    const QByteArray body = syncGet(nam_, url, token, &status, err);
    if (body.isEmpty()) return false;
    if (status != 200) {
        if (err) *err = QStringLiteral("GitHub API HTTP %1").arg(status);
        return false;
    }
    QJsonParseError parseErr{};
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        if (err) *err = QStringLiteral("GitHub API: malformed JSON: %1")
                            .arg(parseErr.errorString());
        return false;
    }
    const QJsonObject o = doc.object();
    out.tag = o.value(QStringLiteral("tag_name")).toString();
    out.name = o.value(QStringLiteral("name")).toString();
    out.assets.clear();
    for (const QJsonValue& v : o.value(QStringLiteral("assets")).toArray()) {
        const QJsonObject a = v.toObject();
        ReleaseAsset ra;
        ra.name = a.value(QStringLiteral("name")).toString();
        ra.downloadUrl =
            QUrl(a.value(QStringLiteral("browser_download_url")).toString());
        ra.size = static_cast<qint64>(
            a.value(QStringLiteral("size")).toDouble());
        out.assets.push_back(ra);
    }
    return true;
}

bool GithubFetcher::downloadAssetWithSha256(const QUrl& url, const QUrl& sha256Url,
                                            const QString& destPath,
                                            ProgressFn onProgress, QString* err) {

    int status = 0;
    const QByteArray expectedSha = syncGet(nam_, sha256Url, {}, &status, err);
    if (expectedSha.isEmpty() || status != 200) {
        if (err && err->isEmpty()) {
            *err = QStringLiteral("cannot fetch sha256 sidecar (%1)").arg(status);
        }
        return false;
    }

    QByteArray expected = expectedSha.simplified();
    const int space = expected.indexOf(' ');
    if (space > 0) expected = expected.left(space);
    expected = expected.toLower();

    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", "ReadMarkable-Installer");
    QNetworkReply* reply = nam_->get(req);

    QCryptographicHash hasher(QCryptographicHash::Sha256);
    QSaveFile out(destPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (err) *err = QStringLiteral("cannot open %1 for write").arg(destPath);
        reply->abort();
        reply->deleteLater();
        return false;
    }

    qint64 totalExpected = -1;

    QObject::connect(reply, &QNetworkReply::downloadProgress,
                     [&](qint64 got, qint64 total) {
                         totalExpected = total;
                         if (onProgress) onProgress(got, total);
                     });
    QObject::connect(reply, &QNetworkReply::readyRead, [&]() {
        const QByteArray chunk = reply->readAll();
        hasher.addData(chunk);
        out.write(chunk);
    });

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const bool netOk = (reply->error() == QNetworkReply::NoError);
    reply->deleteLater();
    if (!netOk) {
        if (err) *err = QStringLiteral("download failed");
        out.cancelWriting();
        return false;
    }

    const QByteArray actual = hasher.result().toHex();
    if (actual != expected) {
        if (err) {
            *err = QStringLiteral(
                       "sha256 mismatch\n  expected: %1\n  actual:   %2")
                       .arg(QString::fromUtf8(expected), QString::fromUtf8(actual));
        }
        out.cancelWriting();
        return false;
    }
    if (!out.commit()) {
        if (err) *err = QStringLiteral("commit failed: %1").arg(destPath);
        return false;
    }
    if (onProgress && totalExpected > 0) onProgress(totalExpected, totalExpected);
    return true;
}

}
