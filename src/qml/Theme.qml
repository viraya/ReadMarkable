pragma Singleton
import QtQuick

QtObject {

    readonly property string uiFontFamily: "Noto Sans"

    readonly property int textXL:   50
    readonly property int textLG:   38
    readonly property int textMD:   32
    readonly property int textSM:   28
    readonly property int textXS:   20

    readonly property int textToolbar:    32
    readonly property int textChapter:    30
    readonly property int textListTitle:  26

    readonly property int textLibraryGridTitle: 30

    readonly property int spaceHair:   2
    readonly property int spaceMicro:  4
    readonly property int spaceTight:  6
    readonly property int spaceXXS:    8
    readonly property int spaceXS:    12
    readonly property int spaceSM:    18
    readonly property int spaceMD:    24
    readonly property int spaceLG:    36
    readonly property int spaceXL:    48

    readonly property color ink:       "#000000"
    readonly property color paper:     "#ffffff"
    readonly property color separator: "#606060"
    readonly property color subtle:    "#e0e0e0"
    readonly property color muted:     "#888888"
    readonly property color faint:     "#444444"
    readonly property color dimmed:    "#666666"

    readonly property color panelBorder: ink
    readonly property color overlayDim:       "#26000000"
    readonly property color highlightShadow:  "#40000000"
    readonly property color selectionOverlay: "#60808080"
    readonly property color strokeOverlay:    "#60404040"

    readonly property color highlightYellow: "#FFD700"
    readonly property color highlightGreen:  "#32CD32"
    readonly property color highlightBlue:   "#6495ED"
    readonly property color highlightRed:    "#FF6347"
    readonly property color highlightOrange: "#FFA500"
    readonly property color highlightGray:   "#808080"

    readonly property color highlightYellowOverlay: "#90FFD700"
    readonly property color highlightGreenOverlay:  "#8032CD32"
    readonly property color highlightBlueOverlay:   "#806495ED"
    readonly property color highlightRedOverlay:    "#80FF6347"
    readonly property color highlightOrangeOverlay: "#80FFA500"
    readonly property color highlightGrayOverlay:   "#70808080"

    readonly property int borderRadius:   0
    readonly property int borderWidth:    3
    readonly property int touchTarget:   88
    readonly property int toolbarHeight: 150
    readonly property int readerTopBarHeight:    180
    readonly property int readerBottomBarHeight:  90

    readonly property int  toolbarRowHeight:       60
    readonly property int  toolbarTitleRowHeight: 100
    readonly property int  toolbarActionRowHeight: 80
    readonly property int  progressBarHeight:    8
    readonly property int  progressLineHeight:   2
    readonly property real drawerWidthFraction:  0.60
    readonly property int  panelHeaderHeight:  100
    readonly property real popupWidthFraction:   0.70
    readonly property real popupHeightFraction:  0.80

    readonly property int iconButton:           48
    readonly property int iconButtonMd:         40
    readonly property int iconButtonSm:         32
    readonly property int actionRowHeight:      44
    readonly property int toolbarButtonHeight:  52
    readonly property int footnoteActionHeight: 36
    readonly property int bookmarkRowHeight:    64
    readonly property int annotationRowHeight:  80
    readonly property int thumbnailLg:          96
    readonly property int styleIndicatorSize:   36
    readonly property int fontPickerHeight:     56
    readonly property int searchHighlightHeight: 3
    readonly property int marginNoteIconSize:   24
    readonly property int handleBarWidth:        2
    readonly property int selectionHandleWidth: 24
    readonly property int selectionHandleHeight: 36
    readonly property int selectionHandleHitSlop: 20
    readonly property int selectionHandleBarInset: 8
    readonly property int loadingBoxWidth:     300
    readonly property int loadingBoxHeight:    100
    readonly property int actionButtonWidth:   120
    readonly property int numericFieldWidth:   100
    readonly property int chipRadius:            6
    readonly property int popupRadius:           4
    readonly property int buttonRadius:          8

    readonly property int libraryListRowHeight:       104
    readonly property int libraryThumbWidth:           70
    readonly property int libraryThumbHeight:          98
    readonly property int libraryListProgressBarHeight: 4
    readonly property int folderGridColumns:            4
    readonly property int folderIconSize:              64
    readonly property int sortMenuWidth:              200

    readonly property int folderPillHeight:   touchTarget
    readonly property int folderPillRadius:   buttonRadius
    readonly property int folderPillGlyphSize:         48
    readonly property int folderPillHPadding: spaceMD

    readonly property int libraryFolderHeaderColumns:   3

    readonly property real libraryGridCellAspect:    1.85

    readonly property int continueHeroHeight:         200
    readonly property int continueHeroThumbWidth:     120
    readonly property int continueHeroThumbHeight:    168

    readonly property int continueHeroTitleSize:   textMD
    readonly property int continueHeroChapterSize: textXS
    readonly property int continueHeroCoverLeftMargin: 0

    readonly property int coverDetailWidth:                  300
    readonly property int coverDetailHeight:                 420
    readonly property int bookDetailProgressContainerHeight:  20
    readonly property int bookDetailProgressTrackHeight:       6
    readonly property int bookDetailOkButtonHeight:           50

    readonly property int animDuration: 0
}
