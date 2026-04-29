#pragma once

#include <QCryptographicHash>
#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QTimer>

#include "epub/CrEngineRenderer.h"
#include "rendering/LayoutThread.h"
#include "rendering/PageCache.h"
#include "rendering/ReadingSettings.h"

class ReaderSettingsController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString     fontFace      READ fontFace      WRITE setFontFace      NOTIFY fontFaceChanged)
    Q_PROPERTY(int         fontSize      READ fontSize      WRITE setFontSize      NOTIFY fontSizeChanged)
    Q_PROPERTY(int         marginPreset  READ marginPreset  WRITE setMarginPreset  NOTIFY marginPresetChanged)
    Q_PROPERTY(int         lineSpacing   READ lineSpacing   WRITE setLineSpacing   NOTIFY lineSpacingChanged)
    Q_PROPERTY(QStringList availableFonts READ availableFonts NOTIFY availableFontsChanged)

    Q_PROPERTY(bool fontFaceOverridden     READ fontFaceOverridden     NOTIFY overridesChanged)
    Q_PROPERTY(bool fontSizeOverridden     READ fontSizeOverridden     NOTIFY overridesChanged)
    Q_PROPERTY(bool marginPresetOverridden READ marginPresetOverridden NOTIFY overridesChanged)
    Q_PROPERTY(bool lineSpacingOverridden  READ lineSpacingOverridden  NOTIFY overridesChanged)
    Q_PROPERTY(bool hasAnyOverride         READ hasAnyOverride         NOTIFY overridesChanged)

public:

    explicit ReaderSettingsController(CrEngineRenderer *renderer,
                                      LayoutThread     *layoutThread,
                                      PageCache        *cache,
                                      QObject          *parent = nullptr);

    QString     fontFace()     const;
    void        setFontFace(const QString &face);

    int         fontSize()     const;
    void        setFontSize(int size);

    int         marginPreset() const;
    void        setMarginPreset(int preset);

    int         lineSpacing()  const;
    void        setLineSpacing(int percent);

    QStringList availableFonts() const;

    Q_INVOKABLE void refreshFontList();

    bool fontFaceOverridden()     const;
    bool fontSizeOverridden()     const;
    bool marginPresetOverridden() const;
    bool lineSpacingOverridden()  const;
    bool hasAnyOverride()         const;

    Q_INVOKABLE void loadSettingsForBook(const QString &bookPath);

    Q_INVOKABLE void resetToGlobalDefaults();

    Q_INVOKABLE void saveAsGlobalDefaults();

    Q_INVOKABLE void enterGlobalSettingsMode();

    Q_INVOKABLE void exitGlobalSettingsMode();

signals:
    void fontFaceChanged();
    void fontSizeChanged();
    void marginPresetChanged();
    void lineSpacingChanged();
    void availableFontsChanged();

    void settingsApplied();

    void overridesChanged();

private slots:

    void applySettings();

private:

    void scheduleRelayout();

    static QString bookKey(const QString &bookPath);

    void savePerBookSetting(const QString &key, const QVariant &value, const QVariant &globalValue);

    void loadGlobalDefaults();

    void saveGlobalDefaults();

    static constexpr int MARGIN_PRESETS[3][4] = {
        { 24,  32,  24,  32 },
        { 48,  64,  48,  64 },
        { 80, 100,  80, 100 },
    };

    static constexpr int FONT_SIZE_MIN  = 6;
    static constexpr int FONT_SIZE_MAX  = 72;
    static constexpr int LINE_SPACING_MIN  = 80;
    static constexpr int LINE_SPACING_MAX  = 200;
    static constexpr int LINE_SPACING_STEP = 10;
    static constexpr int DEBOUNCE_MS = 150;

    CrEngineRenderer *m_renderer;
    LayoutThread     *m_layoutThread;
    PageCache        *m_cache;

    ReadingSettings   m_settings;
    int               m_marginPreset = 1;

    ReadingSettings   m_globalDefaults;
    int               m_globalMarginPreset = 1;

    QString           m_currentBookPath;

    QTimer            m_debounceTimer;

    static const QStringList s_defaultFontList;
};
