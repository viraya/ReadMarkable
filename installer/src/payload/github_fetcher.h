#pragma once

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QUrl>
#include <functional>

namespace rm::installer::payload {

struct ReleaseAsset {
    QString name;
    QUrl downloadUrl;
    qint64 size = 0;
};

struct ReleaseInfo {
    QString tag;
    QString name;
    QList<ReleaseAsset> assets;
};

class GithubFetcher : public QObject {
    Q_OBJECT
 public:
    explicit GithubFetcher(QObject* parent = nullptr);
    ~GithubFetcher() override;

    bool fetchLatestRelease(const QString& ownerRepo, ReleaseInfo& out,
                            const QString& token = {}, QString* err = nullptr);

    using ProgressFn = std::function<void(qint64 bytes, qint64 total)>;

    bool downloadAssetWithSha256(const QUrl& url, const QUrl& sha256Url,
                                 const QString& destPath, ProgressFn onProgress,
                                 QString* err = nullptr);

 private:
    QNetworkAccessManager* nam_;
};

}
