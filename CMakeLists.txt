cmake_minimum_required(VERSION 3.16)
project(ReadMarkable VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 DBus)
find_package(Qt6 OPTIONAL_COMPONENTS Test)

add_subdirectory(thirdparty)

add_library(sqlite3_readmarkable STATIC src/vendor/sqlite3/sqlite3.c)
target_include_directories(sqlite3_readmarkable PUBLIC src/vendor/sqlite3)
target_compile_definitions(sqlite3_readmarkable PRIVATE
    SQLITE_ENABLE_FTS5=1
    SQLITE_THREADSAFE=1
    SQLITE_OMIT_LOAD_EXTENSION=1
)

qt_add_executable(readmarkable
    src/main.cpp
    src/diagnostic/DiagRunner.cpp
    src/diagnostic/DebugController.cpp
    src/display/DisplayManager.cpp
    src/display/WaveformStrategy.cpp
    src/input/InputDiscovery.cpp
    src/input/TouchHandler.cpp
    src/input/PenReader.cpp
    src/input/PenInputController.cpp
    src/platform/XochitlGuard.cpp
    src/platform/LauncherRecovery.cpp
    src/platform/LifecycleManager.cpp
    src/vendor/epaper/epframebuffer.cpp
    src/epub/EpubDocument.cpp
    src/epub/EpubParser.cpp
    src/epub/CrEngineRenderer.cpp
    src/epub/PositionRepository.cpp
    src/rendering/PageCache.cpp
    src/rendering/LayoutThread.cpp
    src/qml/ReaderImageProvider.cpp
    src/settings/ReaderSettingsController.cpp
    src/navigation/NavigationController.cpp
    src/navigation/TocModel.cpp
    src/navigation/BookmarkModel.cpp
    src/navigation/BookmarkRepository.cpp
    src/navigation/ReadingTimeEstimator.cpp
    src/library/LibraryModel.cpp
    src/library/LibraryScanner.cpp
    src/library/LibrarySortProxy.cpp
    src/library/CoverImageProvider.cpp
    src/library/XochitlMetadataParser.cpp
    src/library/XochitlLibrarySource.cpp
    src/annotation/SelectionController.cpp
    src/annotation/AnnotationRepository.cpp
    src/annotation/AnnotationService.cpp
    src/annotation/AnnotationModel.cpp
    src/annotation/MarginNoteRepository.cpp
    src/annotation/MarginNoteService.cpp
    src/dictionary/DictionaryDatabase.cpp
    src/dictionary/DictionaryService.cpp
    src/search/SearchDatabase.cpp
    src/search/SearchService.cpp
    src/export/StrokeRenderer.cpp
    src/export/ExportService.cpp
    thirdparty/stb_image_impl.cpp
)

set_target_properties(readmarkable PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set_source_files_properties(src/qml/Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(readmarkable
    URI ReadMarkable
    VERSION 1.0
    NO_CACHEGEN
    QML_FILES
        src/qml/Theme.qml
        src/qml/Main.qml
        src/qml/ReaderView.qml
        src/qml/Toolbar.qml
        src/qml/TocPanel.qml
        src/qml/BookmarkPanel.qml
        src/qml/SettingsSheet.qml
        src/qml/FootnotePopup.qml
        src/qml/LinkPreviewPopup.qml
        src/qml/FontPicker.qml
        src/qml/diagnostic/DiagScreen.qml
        src/qml/diagnostic/DiagLine.qml
        src/qml/diagnostic/InputDiagScreen.qml
        src/qml/diagnostic/FullDiagScreen.qml
        src/qml/LibraryScreen.qml
        src/qml/BookGridCard.qml
        src/qml/BookListCard.qml
        src/qml/BookDetailScreen.qml
        src/qml/GlobalSettingsScreen.qml
        src/qml/SelectionOverlay.qml
        src/qml/SelectionHandles.qml
        src/qml/SelectionToolbar.qml
        src/qml/HighlightOverlay.qml
        src/qml/NoteEditor.qml
        src/qml/DictionaryPopup.qml
        src/qml/AnnotationsPanel.qml
        src/qml/PenStrokeOverlay.qml
        src/qml/MarginNoteIcon.qml
        src/qml/MarginNoteCanvas.qml
        src/qml/SearchPanel.qml
        src/qml/OnScreenKeyboard.qml
        src/qml/RmButton.qml
        src/qml/RmPanel.qml
        src/qml/RmHeader.qml
        src/qml/RmListRow.qml
        src/qml/ReturningSplash.qml
)

qt_add_resources(readmarkable "icons"
    PREFIX "/icons"
    BASE assets/icons
    FILES
        assets/icons/folder-filled.svg
        assets/icons/folder-outlined.svg
        assets/icons/camera.svg
        assets/icons/cross.svg
)

set(CRENGINE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/crengine/crengine/include)

set(QUAZIP_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/quazip)

set(PUGIXML_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/pugixml)

target_include_directories(readmarkable PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/vendor/epaper
    ${CMAKE_SOURCE_DIR}/thirdparty
    ${CRENGINE_INCLUDE_DIR}
    ${QUAZIP_INCLUDE_DIR}
    ${PUGIXML_INCLUDE_DIR}
)

target_link_libraries(readmarkable PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::DBus
    ${CMAKE_DL_LIBS}
    crengine_with_deps
    QuaZip::QuaZip
    pugixml
    sqlite3_readmarkable
)

include(cmake/xochitl_readonly_lint.cmake)

find_program(BASH_COMMAND bash)
find_program(GREP_COMMAND grep)
if(BASH_COMMAND AND GREP_COMMAND)
    add_custom_target(theme_lint
        COMMAND ${BASH_COMMAND} -c "\
set -e; \
EXCLUDES='-e Theme.qml -e /diagnostic/ -e BookDetailScreen.qml -e GlobalSettingsScreen.qml'; \
hex_violations=$(grep -rn --include='*.qml' -E '\"#[0-9a-fA-F]{3,8}\"' '${CMAKE_SOURCE_DIR}/src/qml/' | grep -v $EXCLUDES || true); \
if [ -n \"$hex_violations\" ]; then \
    echo 'THEME-07 VIOLATION (hex): hardcoded hex colors found in QML (reader scope):'; \
    echo \"$hex_violations\"; \
    exit 1; \
fi; \
pixel_pattern='^[[:space:]]*(width|height|implicitWidth|implicitHeight|pixelSize|radius|spacing|padding|leftPadding|rightPadding|topPadding|bottomPadding|duration|anchors\\.margins|anchors\\.leftMargin|anchors\\.rightMargin|anchors\\.topMargin|anchors\\.bottomMargin|font\\.pixelSize|border\\.width)[[:space:]]*:[[:space:]]*[2-9][0-9]*[[:space:]]*$'; \
pixel_violations=$(grep -rnE \"$pixel_pattern\" --include='*.qml' '${CMAKE_SOURCE_DIR}/src/qml/' | grep -v $EXCLUDES || true); \
if [ -n \"$pixel_violations\" ]; then \
    echo 'THEME-07 VIOLATION (raw-pixel): raw-pixel integers found in QML (reader scope):'; \
    echo \"$pixel_violations\"; \
    echo 'Use Theme.* tokens instead.'; \
    exit 1; \
fi; \
echo 'THEME-07 OK: no hardcoded hex colors or raw-pixel integers in reader-scope QML'; \
"
        COMMENT "THEME-07: checking for hardcoded hex colors AND raw-pixel integers in QML"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        VERBATIM
    )
    add_dependencies(readmarkable theme_lint)
else()
    if(NOT BASH_COMMAND)
        message(WARNING "bash not found - THEME-07 enforcement disabled")
    endif()
    if(NOT GREP_COMMAND)
        message(WARNING "grep not found - THEME-07 enforcement disabled")
    endif()
endif()

install(TARGETS readmarkable DESTINATION bin)

enable_testing()

if(Qt6Test_FOUND)
    qt_add_executable(XochitlMetadataParserTest
        tests/XochitlMetadataParserTest.cpp
        src/library/XochitlMetadataParser.cpp
    )

    target_include_directories(XochitlMetadataParserTest PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )

    target_link_libraries(XochitlMetadataParserTest PRIVATE
        Qt6::Core
        Qt6::Test
    )

    find_program(QEMU_AARCH64
        NAMES qemu-aarch64
        PATHS /opt/codex/chiappa/5.6.75/sysroots/x86_64-codexsdk-linux/usr/bin
        NO_DEFAULT_PATH
    )
    set(AARCH64_SYSROOT "/opt/codex/chiappa/5.6.75/sysroots/cortexa55-remarkable-linux")
    if(QEMU_AARCH64)
        add_test(NAME XochitlMetadataParserTest
            COMMAND ${QEMU_AARCH64} -L ${AARCH64_SYSROOT}
                    $<TARGET_FILE:XochitlMetadataParserTest>
        )
    else()
        add_test(NAME XochitlMetadataParserTest COMMAND XochitlMetadataParserTest)
        message(WARNING "qemu-aarch64 not found, ctest will attempt to run AArch64 binary directly")
    endif()
else()
    message(WARNING "Qt6::Test not found, XochitlMetadataParserTest unit test target skipped")
endif()
