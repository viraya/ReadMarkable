#include "CoverImageProvider.h"

#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QImageReader>
#include <QLatin1String>
#include <QMutexLocker>
#include <QPainter>
#include <QRect>
#include <QString>
#include <QTextOption>
#include <QUrl>
#include <QXmlStreamReader>

#include "quazip/quazip.h"
#include "quazip/quazipfile.h"
#include "stb_image.h"

static const QString kDiskCacheDir =
    QStringLiteral("/home/root/.local/share/readmarkable/covers");

CoverImageProvider::CoverImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

static QByteArray readZipEntryBytes(QuaZip &zip, const QString &entryPath)
{
    if (entryPath.isEmpty())
        return {};

    bool found = zip.setCurrentFile(entryPath);
    if (!found) {

        if (zip.goToFirstFile()) {
            do {
                if (zip.getCurrentFileName().compare(entryPath, Qt::CaseInsensitive) == 0) {
                    found = true;
                    break;
                }
            } while (zip.goToNextFile());
        }
    }
    if (!found)
        return {};

    QuaZipFile file(&zip);
    if (!file.open(QIODevice::ReadOnly))
        return {};

    const QByteArray data = file.readAll();
    file.close();
    return data;
}

QString CoverImageProvider::readContainerXml(QuaZip &zip)
{
    const QByteArray data = readZipEntryBytes(zip, QStringLiteral("META-INF/container.xml"));
    if (data.isEmpty())
        return {};

    QXmlStreamReader reader(data);
    while (!reader.atEnd() && !reader.hasError()) {
        const auto tok = reader.readNext();
        if (tok != QXmlStreamReader::StartElement)
            continue;

        if (reader.name() == QLatin1String("rootfile")) {
            const auto attrs = reader.attributes();
            if (attrs.hasAttribute(QLatin1String("full-path"))) {
                return attrs.value(QLatin1String("full-path")).toString();
            }
        }
    }
    return {};
}

QString CoverImageProvider::readCoverHrefFromOpf(QuaZip &zip, const QString &opfPath)
{
    if (opfPath.isEmpty())
        return {};

    const QByteArray data = readZipEntryBytes(zip, opfPath);
    if (data.isEmpty())
        return {};

    QString coverImageByProperties;
    QString coverIdFromMeta;
    QHash<QString, QString> manifestById;

    QXmlStreamReader reader(data);
    while (!reader.atEnd() && !reader.hasError()) {
        const auto tok = reader.readNext();
        if (tok != QXmlStreamReader::StartElement)
            continue;

        const auto name = reader.name();
        const auto attrs = reader.attributes();

        if (name == QLatin1String("item")) {
            const QString id   = attrs.value(QLatin1String("id")).toString();
            const QString href = attrs.value(QLatin1String("href")).toString();
            if (!id.isEmpty() && !href.isEmpty())
                manifestById.insert(id, href);

            if (coverImageByProperties.isEmpty()) {
                const QString props = attrs.value(QLatin1String("properties")).toString();

                const auto tokens = props.split(QChar(' '), Qt::SkipEmptyParts);
                for (const auto &tok : tokens) {
                    if (tok == QLatin1String("cover-image")) {
                        coverImageByProperties = href;
                        break;
                    }
                }
            }
        } else if (name == QLatin1String("meta")) {

            if (attrs.value(QLatin1String("name")) == QLatin1String("cover")) {
                coverIdFromMeta = attrs.value(QLatin1String("content")).toString();
            }
        }
    }

    QString href;
    if (!coverImageByProperties.isEmpty()) {
        href = coverImageByProperties;
    } else if (!coverIdFromMeta.isEmpty()) {
        href = manifestById.value(coverIdFromMeta);

        if (href.isEmpty())
            href = coverIdFromMeta;
    }

    if (href.isEmpty())
        return {};

    QString normalizedHref = href;
    while (normalizedHref.startsWith(QLatin1String("./")))
        normalizedHref.remove(0, 2);

    const QString opfDir = QFileInfo(opfPath).path();
    QString resolved = (opfDir.isEmpty() || opfDir == QLatin1String("."))
                         ? normalizedHref
                         : QDir(opfDir).filePath(normalizedHref);
    resolved.replace(QLatin1Char('\\'), QLatin1Char('/'));

    resolved.replace(QLatin1String("/./"), QLatin1String("/"));

    while (resolved.startsWith(QLatin1String("./")))
        resolved.remove(0, 2);
    return resolved;
}

QImage CoverImageProvider::readCoverImage(QuaZip &zip, const QString &coverPath,
                                          const QSize &targetSize)
{
    const QByteArray bytes = readZipEntryBytes(zip, coverPath);
    if (bytes.isEmpty())
        return {};

    const bool isPng = bytes.size() >= 4
        && static_cast<unsigned char>(bytes.at(0)) == 0x89
        && bytes.at(1) == 'P' && bytes.at(2) == 'N' && bytes.at(3) == 'G';

    QImage image;

    if (isPng) {

        int w = 0, h = 0, channels = 0;
        unsigned char *pixels = stbi_load_from_memory(
            reinterpret_cast<const unsigned char *>(bytes.constData()),
            bytes.size(), &w, &h, &channels, 4);
        if (!pixels) {
            qWarning() << "CoverImageProvider: stb_image PNG decode failed for"
                       << coverPath << ":" << stbi_failure_reason();
            return {};
        }

        image = QImage(pixels, w, h, w * 4, QImage::Format_RGBA8888,
                       [](void *p) { stbi_image_free(p); }, pixels);

        image = image.copy();
    } else {

        QBuffer buf;
        buf.setData(bytes);
        if (!buf.open(QIODevice::ReadOnly))
            return {};

        QImageReader reader(&buf);
        const QSize srcSize = reader.size();
        if (srcSize.isValid() && !srcSize.isEmpty()) {
            const QSize scaled = srcSize.scaled(targetSize, Qt::KeepAspectRatio);
            reader.setScaledSize(scaled);
        }

        image = reader.read();
        if (image.isNull())
            return {};
    }

    if (image.size() != targetSize) {
        image = image.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return image;
}

void CoverImageProvider::cacheInsert(const QString &epubPath, const QImage &image)
{
    QMutexLocker locker(&m_mutex);
    if (m_cache.contains(epubPath))
        return;
    if (m_cache.size() >= MAX_CACHE_SIZE) {

        m_cache.erase(m_cache.begin());
    }
    m_cache.insert(epubPath, image);
}

QImage CoverImageProvider::tryLoadExistingThumbnail(const QString &thumbPath,
                                                    const QSize &targetSize)
{
    if (thumbPath.isEmpty()) return {};
    const QFileInfo fi(thumbPath);
    const QString suffix = fi.suffix().toLower();
    if (!fi.exists()) return {};

    if (suffix != QLatin1String("jpg")
        && suffix != QLatin1String("jpeg")) {
        return {};
    }
    {
        QMutexLocker locker(&m_mutex);
        if (m_cache.contains(thumbPath)) {
            return m_cache.value(thumbPath);
        }
    }

    QImageReader reader(thumbPath);
    reader.setAutoDetectImageFormat(true);
    QSize srcSize = reader.size();
    qDebug() << "  reader.size=" << srcSize
             << " canRead=" << reader.canRead();
    if (srcSize.isValid() && !srcSize.isEmpty()) {
        QSize fitted = srcSize.scaled(targetSize, Qt::KeepAspectRatio);
        if (!fitted.isEmpty() && fitted != srcSize) {
            reader.setScaledSize(fitted);
        }
    }
    QImage img = reader.read();
    qDebug() << "  read returned size=" << img.size()
             << " null=" << img.isNull();
    if (img.isNull()) {
        qWarning() << "CoverImageProvider: QImageReader::read failed for"
                   << thumbPath << "err=" << reader.errorString();
        return {};
    }
    cacheInsert(thumbPath, img);
    return img;
}

QImage CoverImageProvider::requestImage(const QString &idIn, QSize *size,
                                        const QSize &requestedSize)
{
    const QSize targetSize = (requestedSize.isValid() && !requestedSize.isEmpty())
                                 ? requestedSize
                                 : DEFAULT_COVER_SIZE;

    if (size)
        *size = targetSize;

    const QString id = QUrl::fromPercentEncoding(idIn.toUtf8());

    {
        QMutexLocker locker(&m_mutex);
        if (m_cache.contains(id)) {
            QImage cached = m_cache.value(id);
            if (size) *size = cached.size();
            return cached;
        }
    }

    if (!id.isEmpty()) {
        QImage img = tryLoadExistingThumbnail(id, targetSize);
        if (!img.isNull()) {
            cacheInsert(id, img);
            if (size) *size = img.size();
            return img;
        }
    }

    if (!id.isEmpty()) {
        const QFileInfo bookFi(id);
        const QString suffix = bookFi.suffix().toLower();
        if (suffix != QLatin1String("jpg")
            && suffix != QLatin1String("jpeg")
            && suffix != QLatin1String("png")
            && !suffix.isEmpty()) {
            const QString thumbDirPath = bookFi.absolutePath()
                                         + QLatin1Char('/')
                                         + bookFi.completeBaseName()
                                         + QStringLiteral(".thumbnails");
            QDir thumbDir(thumbDirPath);
            if (thumbDir.exists()) {

                static const QStringList kJpegFilters = {
                    QStringLiteral("*.jpg"),
                    QStringLiteral("*.jpeg"),
                };
                const QFileInfoList entries = thumbDir.entryInfoList(
                    kJpegFilters, QDir::Files, QDir::Name);
                for (const QFileInfo &fi : entries) {
                    QImage img = tryLoadExistingThumbnail(
                        fi.absoluteFilePath(), targetSize);
                    if (!img.isNull()) {
                        cacheInsert(id, img);
                        if (size) *size = img.size();
                        return img;
                    }
                }
            }
        }
    }

    {
        QImage img = tryLoadFromDiskCache(id, targetSize);
        if (!img.isNull()) {
            cacheInsert(id, img);
            if (size) *size = img.size();
            return img;
        }
    }

    {
        QImage img = tryExtractEpubCover(id, targetSize);
        if (!img.isNull()) {
            cacheInsert(id, img);
            saveToDiskCache(id, img);
            if (size) *size = img.size();
            return img;
        }
    }

    if (size) *size = QSize();
    return {};
}

QString CoverImageProvider::diskCachePath(const QString &epubPath)
{
    if (epubPath.isEmpty())
        return {};
    const QByteArray hash = QCryptographicHash::hash(
        epubPath.toUtf8(), QCryptographicHash::Sha1);
    return kDiskCacheDir + QLatin1Char('/')
           + QString::fromLatin1(hash.toHex()) + QStringLiteral(".jpg");
}

QImage CoverImageProvider::tryLoadFromDiskCache(const QString &epubPath,
                                                 const QSize &targetSize)
{
    const QString path = diskCachePath(epubPath);
    if (path.isEmpty() || !QFileInfo::exists(path))
        return {};

    QImageReader reader(path);
    reader.setAutoDetectImageFormat(true);
    const QSize srcSize = reader.size();
    if (srcSize.isValid() && !srcSize.isEmpty()) {
        QSize fitted = srcSize.scaled(targetSize, Qt::KeepAspectRatio);
        if (!fitted.isEmpty() && fitted != srcSize)
            reader.setScaledSize(fitted);
    }
    QImage img = reader.read();
    if (img.isNull()) {
        qWarning() << "CoverImageProvider: disk-cache read failed for"
                   << path << "err=" << reader.errorString();
    }
    return img;
}

void CoverImageProvider::saveToDiskCache(const QString &epubPath,
                                          const QImage &image)
{
    if (image.isNull())
        return;
    const QString path = diskCachePath(epubPath);
    if (path.isEmpty())
        return;
    QDir().mkpath(kDiskCacheDir);
    if (!image.save(path, "JPEG", 85)) {
        qWarning() << "CoverImageProvider: failed to write disk cache" << path;
    }
}

QImage CoverImageProvider::tryExtractEpubCover(const QString &epubPath,
                                                const QSize &targetSize)
{
    if (epubPath.isEmpty())
        return {};

    const QString suffix = QFileInfo(epubPath).suffix().toLower();
    if (suffix != QLatin1String("epub"))
        return {};

    if (!QFileInfo::exists(epubPath))
        return {};

    QuaZip zip(epubPath);
    if (!zip.open(QuaZip::mdUnzip)) {
        qDebug() << "CoverImageProvider: cannot open EPUB" << epubPath;
        return {};
    }

    const QString opfPath = readContainerXml(zip);
    if (opfPath.isEmpty()) {
        zip.close();
        return {};
    }

    const QString coverHref = readCoverHrefFromOpf(zip, opfPath);
    if (coverHref.isEmpty()) {
        zip.close();
        return {};
    }

    QImage img = readCoverImage(zip, coverHref, targetSize);
    zip.close();

    if (!img.isNull()) {
        qDebug() << "CoverImageProvider: extracted cover from"
                 << epubPath << "entry=" << coverHref
                 << "size=" << img.size();
    }
    return img;
}

QImage CoverImageProvider::generatePlaceholder(const QString &title,
                                               const QSize &targetSize)
{
    QImage image(targetSize, QImage::Format_Grayscale8);
    image.fill(0xE8);

    QPainter painter(&image);
    if (!painter.isActive())
        return image;
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const qreal scale = qreal(targetSize.height()) / 310.0;
    const int titlePx  = qMax(10, qRound(16.0 * scale));
    const int padding  = qMax(8, qRound(14.0 * scale));
    const int textWidth = targetSize.width() - 2 * padding;

    QFont titleFont;
    titleFont.setBold(true);
    titleFont.setPixelSize(titlePx);
    painter.setFont(titleFont);
    painter.setPen(QColor(0x30, 0x30, 0x30));

    QTextOption titleOption;
    titleOption.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    titleOption.setWrapMode(QTextOption::WordWrap);

    const QRectF titleRect(padding, padding,
                           textWidth,
                           targetSize.height() - 2 * padding);
    painter.drawText(titleRect, title, titleOption);

    painter.end();
    return image;
}
