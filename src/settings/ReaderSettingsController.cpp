#include "ReaderSettingsController.h"

#include <QDebug>
#include <algorithm>

const QStringList ReaderSettingsController::s_defaultFontList = {
    QStringLiteral("Literata"),
    QStringLiteral("Merriweather"),
    QStringLiteral("Source Serif 4"),
    QStringLiteral("Lora"),
    QStringLiteral("Vollkorn"),
    QStringLiteral("Open Sans"),
    QStringLiteral("Lato"),
    QStringLiteral("Atkinson Hyperlegible Next"),
};

ReaderSettingsController::ReaderSettingsController(CrEngineRenderer *renderer,
                                                   LayoutThread     *layoutThread,
                                                   PageCache        *cache,
                                                   QObject          *parent)
    : QObject(parent)
    , m_renderer(renderer)
    , m_layoutThread(layoutThread)
    , m_cache(cache)
{
    Q_ASSERT(m_renderer);
    Q_ASSERT(m_layoutThread);
    Q_ASSERT(m_cache);

    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(DEBOUNCE_MS);
    connect(&m_debounceTimer, &QTimer::timeout,
            this, &ReaderSettingsController::applySettings);

    connect(m_renderer, &CrEngineRenderer::loadedChanged,
            this, &ReaderSettingsController::refreshFontList);

    m_globalDefaults.fontFace    = QStringLiteral("Literata");
    m_globalDefaults.fontSize    = 14;
    m_globalDefaults.marginLeft   = MARGIN_PRESETS[1][0];
    m_globalDefaults.marginTop    = MARGIN_PRESETS[1][1];
    m_globalDefaults.marginRight  = MARGIN_PRESETS[1][2];
    m_globalDefaults.marginBottom = MARGIN_PRESETS[1][3];
    m_globalDefaults.lineSpacing = 100;
    m_globalMarginPreset = 1;

    loadGlobalDefaults();

    m_settings = m_globalDefaults;
    m_marginPreset = m_globalMarginPreset;

    qInfo() << "ReaderSettingsController: initialized"
            << "fontFace=" << m_settings.fontFace
            << "fontSize=" << m_settings.fontSize
            << "marginPreset=" << m_marginPreset
            << "lineSpacing=" << m_settings.lineSpacing;
}

QString ReaderSettingsController::bookKey(const QString &bookPath)
{

    const QByteArray hash = QCryptographicHash::hash(
        bookPath.toUtf8(), QCryptographicHash::Sha1);
    return QString::fromLatin1(hash.toHex());
}

void ReaderSettingsController::loadGlobalDefaults()
{
    QSettings s;
    s.beginGroup(QStringLiteral("global"));
    m_globalDefaults.fontFace    = s.value(QStringLiteral("fontFace"),    m_globalDefaults.fontFace).toString();
    m_globalDefaults.fontSize    = s.value(QStringLiteral("fontSize"),    m_globalDefaults.fontSize).toInt();
    m_globalMarginPreset         = s.value(QStringLiteral("marginPreset"), m_globalMarginPreset).toInt();
    m_globalDefaults.marginLeft   = MARGIN_PRESETS[m_globalMarginPreset][0];
    m_globalDefaults.marginTop    = MARGIN_PRESETS[m_globalMarginPreset][1];
    m_globalDefaults.marginRight  = MARGIN_PRESETS[m_globalMarginPreset][2];
    m_globalDefaults.marginBottom = MARGIN_PRESETS[m_globalMarginPreset][3];
    m_globalDefaults.lineSpacing = s.value(QStringLiteral("lineSpacing"),  m_globalDefaults.lineSpacing).toInt();
    s.endGroup();
}

void ReaderSettingsController::saveGlobalDefaults()
{
    QSettings s;
    s.beginGroup(QStringLiteral("global"));
    s.setValue(QStringLiteral("fontFace"),    m_globalDefaults.fontFace);
    s.setValue(QStringLiteral("fontSize"),    m_globalDefaults.fontSize);
    s.setValue(QStringLiteral("marginPreset"), m_globalMarginPreset);
    s.setValue(QStringLiteral("lineSpacing"),  m_globalDefaults.lineSpacing);
    s.endGroup();
    s.sync();
}

void ReaderSettingsController::savePerBookSetting(const QString &key, const QVariant &value, const QVariant &globalValue)
{
    if (m_currentBookPath.isEmpty())
        return;

    QSettings s;
    s.beginGroup(QStringLiteral("books/") + bookKey(m_currentBookPath));
    if (value == globalValue)
        s.remove(key);
    else
        s.setValue(key, value);
    s.endGroup();
    s.sync();
}

void ReaderSettingsController::loadSettingsForBook(const QString &bookPath)
{
    m_currentBookPath = bookPath;

    loadGlobalDefaults();

    m_settings     = m_globalDefaults;
    m_marginPreset = m_globalMarginPreset;

    if (!bookPath.isEmpty()) {
        QSettings s;
        const QString group = QStringLiteral("books/") + bookKey(bookPath);
        s.beginGroup(group);
        if (s.contains(QStringLiteral("fontFace")))
            m_settings.fontFace = s.value(QStringLiteral("fontFace")).toString();
        if (s.contains(QStringLiteral("fontSize")))
            m_settings.fontSize = s.value(QStringLiteral("fontSize")).toInt();
        if (s.contains(QStringLiteral("marginPreset"))) {
            m_marginPreset = s.value(QStringLiteral("marginPreset")).toInt();
            m_settings.marginLeft   = MARGIN_PRESETS[m_marginPreset][0];
            m_settings.marginTop    = MARGIN_PRESETS[m_marginPreset][1];
            m_settings.marginRight  = MARGIN_PRESETS[m_marginPreset][2];
            m_settings.marginBottom = MARGIN_PRESETS[m_marginPreset][3];
        }
        if (s.contains(QStringLiteral("lineSpacing")))
            m_settings.lineSpacing = s.value(QStringLiteral("lineSpacing")).toInt();
        s.endGroup();
    }

    emit fontFaceChanged();
    emit fontSizeChanged();
    emit marginPresetChanged();
    emit lineSpacingChanged();
    emit overridesChanged();

    scheduleRelayout();

    qInfo() << "ReaderSettingsController: loaded settings for book"
            << bookPath
            << "fontFace=" << m_settings.fontFace
            << "fontSize=" << m_settings.fontSize
            << "marginPreset=" << m_marginPreset
            << "lineSpacing=" << m_settings.lineSpacing
            << "hasAnyOverride=" << hasAnyOverride();
}

void ReaderSettingsController::resetToGlobalDefaults()
{
    if (!m_currentBookPath.isEmpty()) {

        QSettings s;
        s.beginGroup(QStringLiteral("books/") + bookKey(m_currentBookPath));
        s.remove(QString());
        s.endGroup();
        s.sync();
    }

    loadGlobalDefaults();
    m_settings     = m_globalDefaults;
    m_marginPreset = m_globalMarginPreset;

    emit fontFaceChanged();
    emit fontSizeChanged();
    emit marginPresetChanged();
    emit lineSpacingChanged();
    emit overridesChanged();

    scheduleRelayout();

    qInfo() << "ReaderSettingsController: reset to global defaults";
}

void ReaderSettingsController::saveAsGlobalDefaults()
{
    m_globalDefaults     = m_settings;
    m_globalMarginPreset = m_marginPreset;
    saveGlobalDefaults();
    emit overridesChanged();

    qInfo() << "ReaderSettingsController: saved current settings as global defaults";
}

void ReaderSettingsController::enterGlobalSettingsMode()
{

    m_currentBookPath = QString();

    loadGlobalDefaults();

    m_settings     = m_globalDefaults;
    m_marginPreset = m_globalMarginPreset;

    emit fontFaceChanged();
    emit fontSizeChanged();
    emit marginPresetChanged();
    emit lineSpacingChanged();
    emit overridesChanged();

    qInfo() << "ReaderSettingsController: entered global settings mode"
            << "fontFace=" << m_settings.fontFace
            << "fontSize=" << m_settings.fontSize
            << "marginPreset=" << m_marginPreset
            << "lineSpacing=" << m_settings.lineSpacing;
}

void ReaderSettingsController::exitGlobalSettingsMode()
{

    m_globalDefaults     = m_settings;
    m_globalMarginPreset = m_marginPreset;
    saveGlobalDefaults();
    emit overridesChanged();

    qInfo() << "ReaderSettingsController: exited global settings mode, globals saved";
}

bool ReaderSettingsController::fontFaceOverridden() const
{
    return m_settings.fontFace != m_globalDefaults.fontFace;
}

bool ReaderSettingsController::fontSizeOverridden() const
{
    return m_settings.fontSize != m_globalDefaults.fontSize;
}

bool ReaderSettingsController::marginPresetOverridden() const
{
    return m_marginPreset != m_globalMarginPreset;
}

bool ReaderSettingsController::lineSpacingOverridden() const
{
    return m_settings.lineSpacing != m_globalDefaults.lineSpacing;
}

bool ReaderSettingsController::hasAnyOverride() const
{
    return fontFaceOverridden()
        || fontSizeOverridden()
        || marginPresetOverridden()
        || lineSpacingOverridden();
}

QString ReaderSettingsController::fontFace() const
{
    return m_settings.fontFace;
}

void ReaderSettingsController::setFontFace(const QString &face)
{
    if (face.isEmpty() || face == m_settings.fontFace)
        return;

    m_settings.fontFace = face;
    savePerBookSetting(QStringLiteral("fontFace"), face, m_globalDefaults.fontFace);
    emit fontFaceChanged();
    emit overridesChanged();
    scheduleRelayout();
}

int ReaderSettingsController::fontSize() const
{
    return m_settings.fontSize;
}

void ReaderSettingsController::setFontSize(int size)
{
    const int clamped = std::clamp(size, FONT_SIZE_MIN, FONT_SIZE_MAX);
    if (clamped == m_settings.fontSize)
        return;

    m_settings.fontSize = clamped;
    savePerBookSetting(QStringLiteral("fontSize"), clamped, m_globalDefaults.fontSize);
    emit fontSizeChanged();
    emit overridesChanged();
    scheduleRelayout();
}

int ReaderSettingsController::marginPreset() const
{
    return m_marginPreset;
}

void ReaderSettingsController::setMarginPreset(int preset)
{
    if (preset < 0 || preset > 2 || preset == m_marginPreset)
        return;

    m_marginPreset = preset;

    m_settings.marginLeft  = MARGIN_PRESETS[preset][0];
    m_settings.marginTop   = MARGIN_PRESETS[preset][1];
    m_settings.marginRight = MARGIN_PRESETS[preset][2];
    m_settings.marginBottom= MARGIN_PRESETS[preset][3];

    savePerBookSetting(QStringLiteral("marginPreset"), preset, m_globalMarginPreset);
    emit marginPresetChanged();
    emit overridesChanged();
    scheduleRelayout();
}

int ReaderSettingsController::lineSpacing() const
{
    return m_settings.lineSpacing;
}

void ReaderSettingsController::setLineSpacing(int percent)
{

    const int snapped = std::clamp(
        (percent / LINE_SPACING_STEP) * LINE_SPACING_STEP,
        LINE_SPACING_MIN, LINE_SPACING_MAX);

    if (snapped == m_settings.lineSpacing)
        return;

    m_settings.lineSpacing = snapped;
    savePerBookSetting(QStringLiteral("lineSpacing"), snapped, m_globalDefaults.lineSpacing);
    emit lineSpacingChanged();
    emit overridesChanged();
    scheduleRelayout();
}

QStringList ReaderSettingsController::availableFonts() const
{

    return s_defaultFontList;
}

void ReaderSettingsController::refreshFontList()
{
    emit availableFontsChanged();
    qInfo() << "ReaderSettingsController: font list refreshed,"
            << availableFonts().size() << "fonts available";
}

void ReaderSettingsController::scheduleRelayout()
{

    m_debounceTimer.start();
}

void ReaderSettingsController::applySettings()
{
    qInfo() << "ReaderSettingsController: applying settings"
            << "fontFace=" << m_settings.fontFace
            << "fontSize=" << m_settings.fontSize
            << "marginPreset=" << m_marginPreset
            << "lineSpacing=" << m_settings.lineSpacing;

    const QString savedXPointer = m_renderer->currentXPointer();

    m_renderer->setFontFace(m_settings.fontFace);
    m_renderer->setFontSize(m_settings.fontSize);
    m_renderer->setPageMargins(
        m_settings.marginLeft, m_settings.marginTop,
        m_settings.marginRight, m_settings.marginBottom);
    m_renderer->setLineSpacing(m_settings.lineSpacing);

    m_renderer->relayout();

    if (!savedXPointer.isEmpty())
        m_renderer->goToXPointer(savedXPointer);

    m_cache->invalidateAll();

    const int page = m_renderer->currentPage();
    QImage img = m_renderer->renderPage(page);
    if (!img.isNull())
        m_cache->insert(page, img);

    m_renderer->bumpRenderVersion();

    m_layoutThread->applySettings(m_settings);

    m_layoutThread->requestPrerender(page);

    emit settingsApplied();

    qInfo() << "ReaderSettingsController: settings applied, pageCount="
            << m_renderer->pageCount()
            << "currentPage=" << m_renderer->currentPage();
}
