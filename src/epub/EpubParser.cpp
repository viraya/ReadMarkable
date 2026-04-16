#include "EpubParser.h"

#include <QDebug>
#include <QMap>

#include "quazip/quazip.h"
#include "quazip/quazipfile.h"

#include "pugixml.hpp"

static QByteArray readZipEntry(QuaZip &zip, const QString &entryPath)
{
    if (!zip.setCurrentFile(entryPath)) {

        zip.goToFirstFile();
        do {
            if (zip.getCurrentFileName().compare(entryPath, Qt::CaseInsensitive) == 0)
                break;
        } while (zip.goToNextFile());
        if (zip.getCurrentFileName().compare(entryPath, Qt::CaseInsensitive) != 0) {
            qWarning() << "EpubParser: file not found in archive:" << entryPath;
            return {};
        }
    }

    QuaZipFile file(&zip);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "EpubParser: failed to open entry:" << entryPath;
        return {};
    }
    return file.readAll();
}

static QString parseContainerXml(const QByteArray &data)
{
    pugi::xml_document doc;
    if (!doc.load_buffer(data.constData(), static_cast<size_t>(data.size()))) {
        qWarning() << "EpubParser: failed to parse container.xml";
        return {};
    }

    auto rootfile = doc.select_node("//rootfile");
    if (!rootfile) {
        qWarning() << "EpubParser: no <rootfile> in container.xml";
        return {};
    }
    return QString::fromUtf8(rootfile.node().attribute("full-path").value());
}

static QString opfBaseDir(const QString &opfPath)
{
    int slash = opfPath.lastIndexOf(QLatin1Char('/'));
    if (slash < 0)
        return QString();
    return opfPath.left(slash + 1);
}

static QVector<TocEntry> parseNcxNavMap(const pugi::xml_node &navMapNode, int depth = 0)
{
    QVector<TocEntry> entries;
    for (auto navPoint : navMapNode.children("navPoint")) {
        TocEntry entry;
        entry.level = depth;

        auto navLabel = navPoint.child("navLabel");
        if (navLabel) {
            auto textNode = navLabel.child("text");
            if (textNode)
                entry.label = QString::fromUtf8(textNode.text().get()).trimmed();
        }

        auto content = navPoint.child("content");
        if (content)
            entry.href = QString::fromUtf8(content.attribute("src").value());

        entry.children = parseNcxNavMap(navPoint, depth + 1);
        entries.append(entry);
    }
    return entries;
}

static QVector<TocEntry> parseNavXhtml(const QByteArray &data, int depth = 0)
{
    pugi::xml_document doc;

    auto result = doc.load_buffer(data.constData(), static_cast<size_t>(data.size()),
                                  pugi::parse_default | pugi::parse_doctype);
    if (!result) {
        qWarning() << "EpubParser: failed to parse nav.xhtml:" << result.description();
        return {};
    }

    pugi::xml_node navNode;

    auto navNodes = doc.select_nodes("//nav");
    for (auto &n : navNodes) {
        auto epubType = n.node().attribute("epub:type");
        if (!epubType) epubType = n.node().attribute("type");
        if (std::string(epubType.value()).find("toc") != std::string::npos) {
            navNode = n.node();
            break;
        }
    }
    if (!navNode) {

        if (!navNodes.empty())
            navNode = navNodes.first().node();
    }
    if (!navNode)
        return {};

    std::function<QVector<TocEntry>(const pugi::xml_node &, int)> walkOl;
    walkOl = [&walkOl](const pugi::xml_node &olNode, int d) -> QVector<TocEntry> {
        QVector<TocEntry> result;
        for (auto li : olNode.children("li")) {
            TocEntry entry;
            entry.level = d;

            auto anchor = li.child("a");
            if (anchor) {
                entry.href  = QString::fromUtf8(anchor.attribute("href").value());
                entry.label = QString::fromUtf8(anchor.text().get()).trimmed();
            } else {
                auto span = li.child("span");
                if (span)
                    entry.label = QString::fromUtf8(span.text().get()).trimmed();
            }

            auto nestedOl = li.child("ol");
            if (nestedOl)
                entry.children = walkOl(nestedOl, d + 1);
            if (!entry.label.isEmpty() || !entry.href.isEmpty())
                result.append(entry);
        }
        return result;
    };

    auto ol = navNode.child("ol");
    if (!ol)
        return {};
    return walkOl(ol, 0);
}

EpubDocument EpubParser::parse(const QString &epubPath)
{
    EpubDocument doc;
    doc.filePath = epubPath;

    QuaZip epub(epubPath);
    epub.setFileNameCodec("UTF-8");
    if (!epub.open(QuaZip::mdUnzip)) {
        qWarning() << "EpubParser: failed to open EPUB:" << epubPath;
        return {};
    }

    QByteArray containerData = readZipEntry(epub, QStringLiteral("META-INF/container.xml"));
    if (containerData.isEmpty())
        return {};

    QString opfPath = parseContainerXml(containerData);
    if (opfPath.isEmpty()) {
        qWarning() << "EpubParser: empty OPF path from container.xml";
        return {};
    }
    qDebug() << "EpubParser: OPF path:" << opfPath;

    QByteArray opfData = readZipEntry(epub, opfPath);
    if (opfData.isEmpty()) {
        qWarning() << "EpubParser: failed to read OPF:" << opfPath;
        return {};
    }

    pugi::xml_document opfDoc;
    if (!opfDoc.load_buffer(opfData.constData(), static_cast<size_t>(opfData.size()))) {
        qWarning() << "EpubParser: failed to parse OPF";
        return {};
    }

    const QString baseDir = opfBaseDir(opfPath);

    auto metadata = opfDoc.select_node("//*[local-name()='metadata']");
    if (metadata) {
        auto titleNode  = metadata.node().select_node("//*[local-name()='title']");
        auto authorNode = metadata.node().select_node("//*[local-name()='creator']");
        auto langNode   = metadata.node().select_node("//*[local-name()='language']");
        auto descNode   = metadata.node().select_node("//*[local-name()='description']");
        if (titleNode)
            doc.title       = QString::fromUtf8(titleNode.node().text().get()).trimmed();
        if (authorNode)
            doc.author      = QString::fromUtf8(authorNode.node().text().get()).trimmed();
        if (langNode)
            doc.language    = QString::fromUtf8(langNode.node().text().get()).trimmed();
        if (descNode)
            doc.description = QString::fromUtf8(descNode.node().text().get()).trimmed();
    }

    struct ManifestItem {
        QString href;
        QString mediaType;
        QString properties;
    };
    QMap<QString, ManifestItem> manifest;
    QString ncxId;
    QString navItemId;

    auto manifestNode = opfDoc.select_node("//*[local-name()='manifest']");
    if (manifestNode) {
        for (auto item : manifestNode.node().children("item")) {
            QString id         = QString::fromUtf8(item.attribute("id").value());
            QString href       = QString::fromUtf8(item.attribute("href").value());
            QString mediaType  = QString::fromUtf8(item.attribute("media-type").value());
            QString properties = QString::fromUtf8(item.attribute("properties").value());

            manifest[id] = ManifestItem{ href, mediaType, properties };

            if (properties.contains(QLatin1String("nav")))
                navItemId = id;
        }
    }

    auto spineNode = opfDoc.select_node("//*[local-name()='spine']");
    if (spineNode) {

        ncxId = QString::fromUtf8(spineNode.node().attribute("toc").value());

        for (auto itemref : spineNode.node().children("itemref")) {
            QString idref  = QString::fromUtf8(itemref.attribute("idref").value());
            const char *linearAttr = itemref.attribute("linear").value();
            bool linear = (linearAttr[0] == '\0' || std::string(linearAttr) != "no");

            if (!manifest.contains(idref))
                continue;
            const ManifestItem &mi = manifest[idref];

            SpineItem si;
            si.id        = idref;
            si.href      = baseDir + mi.href;
            si.mediaType = mi.mediaType;
            si.linear    = linear;
            doc.spine.append(si);
        }
    }

    auto coverMeta = opfDoc.select_node("//*[local-name()='meta'][@name='cover']");
    if (coverMeta) {
        QString coverId = QString::fromUtf8(coverMeta.node().attribute("content").value());
        if (manifest.contains(coverId))
            doc.coverImagePath = baseDir + manifest[coverId].href;
    }

    if (doc.coverImagePath.isEmpty()) {
        for (auto it = manifest.cbegin(); it != manifest.cend(); ++it) {
            if (it.value().properties.contains(QLatin1String("cover-image"))) {
                doc.coverImagePath = baseDir + it.value().href;
                break;
            }
        }
    }

    if (!navItemId.isEmpty() && manifest.contains(navItemId)) {
        QString navHref   = baseDir + manifest[navItemId].href;
        QByteArray navData = readZipEntry(epub, navHref);
        if (!navData.isEmpty()) {
            doc.toc = parseNavXhtml(navData);
            qDebug() << "EpubParser: parsed EPUB3 nav TOC, entries:" << doc.toc.size();
        }
    }

    if (doc.toc.isEmpty() && !ncxId.isEmpty() && manifest.contains(ncxId)) {
        QString ncxHref   = baseDir + manifest[ncxId].href;
        QByteArray ncxData = readZipEntry(epub, ncxHref);
        if (!ncxData.isEmpty()) {
            pugi::xml_document ncxDoc;
            if (ncxDoc.load_buffer(ncxData.constData(), static_cast<size_t>(ncxData.size()))) {
                auto navMap = ncxDoc.select_node("//*[local-name()='navMap']");
                if (navMap)
                    doc.toc = parseNcxNavMap(navMap.node());
            }
            qDebug() << "EpubParser: parsed EPUB2 NCX TOC, entries:" << doc.toc.size();
        }
    }

    epub.close();

    qInfo() << "EpubParser: parsed" << epubPath
            << "title:" << doc.title
            << "author:" << doc.author
            << "spine items:" << doc.spine.size()
            << "toc entries:" << doc.toc.size();

    return doc;
}
