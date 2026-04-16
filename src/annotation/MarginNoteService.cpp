#include "MarginNoteService.h"
#include "../epub/CrEngineRenderer.h"

#include "lvdocview.h"
#include "lvtinydom.h"

#include <QCryptographicHash>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QVariantMap>

MarginNoteService::MarginNoteService(QObject *parent)
    : QObject(parent)
{

    const QString dataDir = QStringLiteral("/home/root/.readmarkable/data");
    QDir dir;
    if (!dir.mkpath(dataDir)) {
        qWarning() << "MarginNoteService: failed to create data directory" << dataDir;
    }

    const QString dbPath = dataDir + QStringLiteral("/annotations.db");
    m_repo = std::make_unique<MarginNoteRepository>(dbPath);

    if (!m_repo->isOpen()) {
        qCritical() << "MarginNoteService: database failed to open at" << dbPath;
    } else {
        qDebug() << "MarginNoteService: ready, database at" << dbPath;
    }
}

void MarginNoteService::setRenderer(CrEngineRenderer *renderer)
{
    m_renderer = renderer;
}

int MarginNoteService::marginNoteCount() const
{
    return m_currentNotes.size();
}

QVariantList MarginNoteService::currentPageNotes() const
{
    return m_currentPageNotes;
}

void MarginNoteService::setCurrentBook(const QString &bookPath)
{
    if (bookPath == m_currentBookPath)
        return;

    m_currentBookPath = bookPath;
    refreshCache();

    qDebug() << "MarginNoteService: current book set to"
             << bookPath << "-" << m_currentNotes.size() << "margin notes";

    emit marginNotesChanged();
}

int MarginNoteService::saveMarginNote(const QString &paragraphXPtr,
                                      const QByteArray &strokeData)
{
    if (!m_repo || m_currentBookPath.isEmpty()) {
        qWarning() << "MarginNoteService::saveMarginNote: no current book";
        return -1;
    }
    if (paragraphXPtr.isEmpty() || strokeData.isEmpty()) {
        qWarning() << "MarginNoteService::saveMarginNote: empty XPointer or strokeData";
        return -1;
    }

    QString sha;
    {
        const QByteArray hash = QCryptographicHash::hash(
            QFileInfo(m_currentBookPath).absoluteFilePath().toUtf8(),
            QCryptographicHash::Sha256).toHex();
        sha = QString::fromLatin1(hash);
    }

    MarginNoteEntry entry;
    entry.bookSha           = sha;
    entry.paragraphXPointer = paragraphXPtr;
    entry.strokeData        = strokeData;
    entry.createdAt         = QDateTime::currentSecsSinceEpoch();

    const int64_t id = m_repo->insert(entry);
    if (id < 0) {
        qWarning() << "MarginNoteService::saveMarginNote: insert failed";
        return -1;
    }

    refreshCache();
    emit marginNotesChanged();

    qDebug() << "MarginNoteService: saved margin note id" << id
             << "xptr:" << paragraphXPtr.left(40);
    return static_cast<int>(id);
}

void MarginNoteService::updateMarginNoteStrokes(int noteId,
                                                const QByteArray &strokeData)
{
    if (!m_repo) return;

    const bool ok = m_repo->updateStrokes(static_cast<int64_t>(noteId), strokeData);
    if (!ok) {
        qWarning() << "MarginNoteService::updateMarginNoteStrokes: update failed for id"
                   << noteId;
        return;
    }

    refreshCache();
    emit marginNotesChanged();
}

void MarginNoteService::deleteMarginNote(int noteId)
{
    if (!m_repo) return;

    const bool ok = m_repo->remove(static_cast<int64_t>(noteId));
    if (!ok) {
        qWarning() << "MarginNoteService::deleteMarginNote: remove failed for id" << noteId;
        return;
    }

    refreshCache();
    emit marginNotesChanged();
}

QVariantList MarginNoteService::marginNotesForCurrentBook() const
{
    QVariantList result;
    result.reserve(m_currentNotes.size());

    for (const MarginNoteEntry &e : m_currentNotes) {
        QVariantMap map;
        map[QStringLiteral("id")]                 = static_cast<int>(e.id);
        map[QStringLiteral("paragraphXPointer")]  = e.paragraphXPointer;
        map[QStringLiteral("strokeData")]         = e.strokeData;
        map[QStringLiteral("createdAt")]          = static_cast<int>(e.createdAt);
        map[QStringLiteral("paragraphY")]         = paragraphYPosition(e.paragraphXPointer);
        result.append(map);
    }

    return result;
}

void MarginNoteService::loadNotesForPage()
{
    if (!m_renderer || !m_renderer->isLoaded()) {
        m_currentPageNotes.clear();
        emit currentPageNotesChanged();
        return;
    }

    const int pageH = m_renderer->isLandscape() ? 954 : 1696;

    QVariantList result;

    for (const MarginNoteEntry &e : m_currentNotes) {
        const int y = paragraphYPosition(e.paragraphXPointer);

        if (y < 0 || y >= pageH)
            continue;

        QVariantMap map;
        map[QStringLiteral("id")]                = static_cast<int>(e.id);
        map[QStringLiteral("paragraphXPointer")] = e.paragraphXPointer;
        map[QStringLiteral("strokeData")]        = e.strokeData;
        map[QStringLiteral("paragraphY")]        = y;
        map[QStringLiteral("thumbnailRects")]    = generateThumbnailRects(e.strokeData);
        result.append(map);
    }

    m_currentPageNotes = result;
    emit currentPageNotesChanged();

    qDebug() << "MarginNoteService::loadNotesForPage:"
             << result.size() << "notes on current page";
}

QByteArray MarginNoteService::serializeStrokes(const QVector<QVector<QPoint>> &strokes)
{
    QByteArray bytes;
    QDataStream ds(&bytes, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);

    ds << quint16(0x4D4E);
    ds << quint8(1);
    ds << quint8(static_cast<quint8>(strokes.size()));

    for (const auto &stroke : strokes) {
        ds << quint16(static_cast<quint16>(stroke.size()));
        for (const QPoint &p : stroke) {
            ds << qint16(static_cast<qint16>(p.x()));
            ds << qint16(static_cast<qint16>(p.y()));
        }
    }

    return bytes;
}

QVector<QVector<QPoint>> MarginNoteService::deserializeStrokes(const QByteArray &data)
{
    if (data.size() < 4) {
        qWarning() << "MarginNoteService::deserializeStrokes: data too short";
        return {};
    }

    QDataStream ds(data);
    ds.setByteOrder(QDataStream::LittleEndian);

    quint16 magic = 0;
    ds >> magic;
    if (magic != 0x4D4E) {
        qWarning() << "MarginNoteService::deserializeStrokes: invalid magic"
                   << Qt::hex << magic;
        return {};
    }

    quint8 version = 0;
    ds >> version;
    if (version != 1) {
        qWarning() << "MarginNoteService::deserializeStrokes: unsupported version"
                   << version;
        return {};
    }

    quint8 strokeCount = 0;
    ds >> strokeCount;

    QVector<QVector<QPoint>> result;
    result.reserve(strokeCount);

    for (int s = 0; s < strokeCount; ++s) {
        if (ds.atEnd()) break;

        quint16 pointCount = 0;
        ds >> pointCount;

        const qint64 remaining = data.size() - ds.device()->pos();
        if (remaining < static_cast<qint64>(pointCount) * 4) {
            qWarning() << "MarginNoteService::deserializeStrokes: truncated stroke data";
            break;
        }

        QVector<QPoint> stroke;
        stroke.reserve(pointCount);
        for (int p = 0; p < pointCount; ++p) {
            qint16 x = 0, y = 0;
            ds >> x >> y;
            stroke.append(QPoint(x, y));
        }
        result.append(stroke);
    }

    return result;
}

QVariantList MarginNoteService::generateThumbnailRects(const QByteArray &strokeData,
                                                        int thumbW, int thumbH)
{
    const QVector<QVector<QPoint>> strokes = deserializeStrokes(strokeData);
    if (strokes.isEmpty())
        return {};

    int minX = std::numeric_limits<int>::max();
    int minY = std::numeric_limits<int>::max();
    int maxX = std::numeric_limits<int>::min();
    int maxY = std::numeric_limits<int>::min();

    for (const auto &stroke : strokes) {
        for (const QPoint &p : stroke) {
            if (p.x() < minX) minX = p.x();
            if (p.y() < minY) minY = p.y();
            if (p.x() > maxX) maxX = p.x();
            if (p.y() > maxY) maxY = p.y();
        }
    }

    const int rangeX = maxX - minX;
    const int rangeY = maxY - minY;

    const float scaleX = (rangeX > 0) ? float(thumbW - 4) / float(rangeX) : 1.0f;
    const float scaleY = (rangeY > 0) ? float(thumbH - 4) / float(rangeY) : 1.0f;
    const float scale  = std::min(scaleX, scaleY);

    QVariantList result;
    for (const auto &stroke : strokes) {
        for (const QPoint &p : stroke) {
            const int sx = static_cast<int>((p.x() - minX) * scale) + 2;
            const int sy = static_cast<int>((p.y() - minY) * scale) + 2;
            QVariantMap dot;
            dot[QStringLiteral("x")]      = sx;
            dot[QStringLiteral("y")]      = sy;
            dot[QStringLiteral("width")]  = 1;
            dot[QStringLiteral("height")] = 1;
            result.append(dot);
        }
    }

    return result;
}

QVariantList MarginNoteService::deserializeStrokesToRects(const QByteArray &strokeData) const
{
    const QVector<QVector<QPoint>> strokes = deserializeStrokes(strokeData);

    QVariantList result;
    for (const auto &stroke : strokes) {
        for (const QPoint &p : stroke) {
            QVariantMap dot;
            dot[QStringLiteral("x")]      = p.x();
            dot[QStringLiteral("y")]      = p.y();
            dot[QStringLiteral("width")]  = 2;
            dot[QStringLiteral("height")] = 2;
            result.append(dot);
        }
    }
    return result;
}

void MarginNoteService::refreshCache()
{
    if (!m_repo || m_currentBookPath.isEmpty()) {
        m_currentNotes.clear();
        return;
    }

    m_currentNotes = m_repo->loadForBook(m_currentBookPath);
}

int MarginNoteService::paragraphYPosition(const QString &xpointerStr) const
{
    if (!m_renderer || !m_renderer->isLoaded())
        return -1;

    LVDocView *dv = m_renderer->docView();
    if (!dv)
        return -1;

    ldomXPointer xptr = dv->getDocument()->createXPointer(
        lString32(xpointerStr.toUtf8().constData()));
    if (xptr.isNull())
        return -1;

    lvPoint pt = xptr.toPoint();
    return pt.y;
}
