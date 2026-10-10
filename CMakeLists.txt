cmake_minimum_required(VERSION 3.16)

# versionName. Increment QT_ANDROID_VERSION_CODE for each Android upload.
project(speech-rate-meter-2 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 6.12 REQUIRED COMPONENTS Quick Multimedia LinguistTools)

# ---------------------------------------------------------------------------
# OpenMP: on Android the NDK ships libomp (clang runtime) but CMake's
# FindOpenMP won't discover it automatically when cross-compiling.
# We locate it manually and create a thin imported target that wraps it.
# ---------------------------------------------------------------------------
if(ANDROID)
    # Use CMAKE_ANDROID_ARCH_ABI to find the correct libomp for the target architecture
    find_library(OMP_LIBRARY
        NAMES omp
        PATHS "${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/lib/clang/*/lib/linux/${CMAKE_ANDROID_ARCH_ABI_TRIPLET}"
        NO_DEFAULT_PATH
    )
    if(NOT OMP_LIBRARY)
        # Fallback: try with ANDROID_ABI
        find_library(OMP_LIBRARY
            NAMES omp
            PATHS "${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/lib/clang/*/lib/linux/${ANDROID_ABI}"
            NO_DEFAULT_PATH
        )
    endif()
    if(NOT OMP_LIBRARY)
        message(WARNING
            "libomp not found in NDK. OpenMP support will be disabled on Android. "
            "Set OMP_LIBRARY manually to the path of libomp.so for your ABI."
        )
        set(ANDROID_OMP_FLAGS "")
    else()
        message(STATUS "Android OpenMP library: ${OMP_LIBRARY}")
        set(ANDROID_OMP_FLAGS "-fopenmp")
        add_library(AndroidOMP SHARED IMPORTED)
        set_target_properties(AndroidOMP PROPERTIES
            IMPORTED_LOCATION "${OMP_LIBRARY}"
        )
    endif()
else()
    find_package(OpenMP REQUIRED)
endif()

if(ANDROID)
    # Keep DWARF in Release objects so AGP can embed Play native debug symbols
    # (android/build.gradle: ndk.debugSymbolLevel = FULL). Packaged .so files
    # are still stripped; symbols go into AAB BUNDLE-METADATA.
    add_compile_options("$<$<CONFIG:Release>:-g>")
endif()

qt_standard_project_setup(REQUIRES 6.8 I18N_TRANSLATED_LANGUAGES en ru)

qt_add_executable(appspeech-rate-meter-2
    main.cpp
    src/mingw_libstdcxx_compat.cpp
    src/api/audioapi.h
    src/api/audioapi.cpp
    src/api/fileapi.h
    src/api/fileapi.cpp
    src/api/settingsapi.h
    src/api/settingsapi.cpp
    src/api/sessionapi.h
    src/api/sessionapi.cpp
    src/services/speechrateanalysis.h
    src/services/speechrateanalysis.cpp
    src/services/sessionstore.h
    src/services/sessionstore.cpp
    src/api/qmllogger.h
    src/api/qmllogger.cpp
    src/api/helpers/settings.h
    src/api/helpers/settings.cpp
    src/api/helpers/beepplayer.h
    src/services/wavfileservice.h
    src/services/wavfileservice.cpp
    src/services/vadenergryservice.h
    src/services/vadenergryservice.cpp
    src/services/vadautocorrelationservice.h
    src/services/vadautocorrelationservice.cpp
    src/services/pitchservice.h
    src/services/pitchservice.cpp
    src/services/specservice.h
    src/services/specservice.cpp
    src/services/amplitudeservice.h
    src/services/amplitudeservice.cpp
    src/services/umpservice.h
    src/services/umpservice.cpp
    src/services/dpservice.h
    src/services/dpservice.cpp
    src/services/cdtwservice.h
    src/services/cdtwservice.cpp
    src/services/helpers/wavFile.h
    src/services/helpers/wavFile.cpp
    src/services/helpers/vectorutils.h
    src/services/helpers/vectorutils.cpp
    src/services/helpers/vectorutils_normalization.cpp
    src/services/helpers/vectorutils_interpolation.cpp
    src/services/helpers/vectorutils_smoothing.cpp
)

target_include_directories(appspeech-rate-meter-2 PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/alglib-cpp/src
)

# alglib: AVX2/FMA/SSE2 kernels are x86-only; skip them on ARM Android
if(ANDROID)
    target_sources(appspeech-rate-meter-2 PRIVATE
        3rdparty/alglib-cpp/src/alglibinternal.cpp
        3rdparty/alglib-cpp/src/alglibmisc.cpp
        3rdparty/alglib-cpp/src/ap.cpp
        3rdparty/alglib-cpp/src/dataanalysis.cpp
        3rdparty/alglib-cpp/src/diffequations.cpp
        3rdparty/alglib-cpp/src/fasttransforms.cpp
        3rdparty/alglib-cpp/src/integration.cpp
        3rdparty/alglib-cpp/src/interpolation.cpp
        3rdparty/alglib-cpp/src/linalg.cpp
        3rdparty/alglib-cpp/src/minlp.cpp
        3rdparty/alglib-cpp/src/optimization.cpp
        3rdparty/alglib-cpp/src/solvers.cpp
        3rdparty/alglib-cpp/src/specialfunctions.cpp
        3rdparty/alglib-cpp/src/statistics.cpp
    )
else()
    target_sources(appspeech-rate-meter-2 PRIVATE
        3rdparty/alglib-cpp/src/alglibinternal.cpp
        3rdparty/alglib-cpp/src/alglibmisc.cpp
        3rdparty/alglib-cpp/src/ap.cpp
        3rdparty/alglib-cpp/src/dataanalysis.cpp
        3rdparty/alglib-cpp/src/diffequations.cpp
        3rdparty/alglib-cpp/src/fasttransforms.cpp
        3rdparty/alglib-cpp/src/integration.cpp
        3rdparty/alglib-cpp/src/interpolation.cpp
        3rdparty/alglib-cpp/src/kernels_avx2.cpp
        3rdparty/alglib-cpp/src/kernels_fma.cpp
        3rdparty/alglib-cpp/src/kernels_sse2.cpp
        3rdparty/alglib-cpp/src/linalg.cpp
        3rdparty/alglib-cpp/src/minlp.cpp
        3rdparty/alglib-cpp/src/optimization.cpp
        3rdparty/alglib-cpp/src/solvers.cpp
        3rdparty/alglib-cpp/src/specialfunctions.cpp
        3rdparty/alglib-cpp/src/statistics.cpp
    )
endif()

set(SPTK_BUILD_COMMANDS OFF CACHE BOOL "Build SPTK commands" FORCE)
add_subdirectory(3rdparty/SPTK)

set(qml_singletons
    ui/utils/AppScale.qml
    ui/utils/Icons.qml
    ui/utils/Logger.qml
    ui/utils/Theme.qml
)

set_source_files_properties(${qml_singletons}
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(appspeech-rate-meter-2
    URI SpeechRateMeter2
    VERSION 1.0
    QML_FILES
        ui/Main.qml
        ui/pages/HomePage.qml
        ui/pages/HistoryPage.qml
        ui/pages/SessionPage.qml
        ui/pages/SettingsPage.qml
        ui/pages/UserGuidePage.qml
        ui/pages/PrivacyPolicyPage.qml
        ui/pages/LicensesPage.qml
        ui/components/PlayButton.qml
        ui/components/ConfirmDialog.qml
        ui/components/SpeechRateGauge.qml
        ui/components/MetricChart.qml
        ui/components/MetricTile.qml
        ui/components/DetailsDialog.qml
        ui/components/RecordingSummaryCard.qml
        ui/components/DecimalSpinBox.qml
        ui/components/VadCalibrationDialog.qml
        ${qml_singletons}
    RESOURCES
        res/fonts/fa-brands-400.ttf
        res/fonts/fa-regular-400.ttf
        res/fonts/fa-solid-900.ttf
        res/icons/speechratemeter.png
        res/images/wave.png
        LICENSE
        docs/user_guide_en.md
        docs/user_guide_ru.md
        docs/privacy_policy_en.md
        docs/privacy_policy_ru.md
        licenses/THIRD_PARTY_NOTICES.md
        licenses/LGPL-3.0.txt
        licenses/Font-Awesome-Free-LICENSE.txt
        licenses/LLVM-OpenMP-LICENSE.txt
        3rdparty/alglib-cpp/gpl2.txt
        3rdparty/alglib-cpp/gpl3.txt
        3rdparty/SPTK/LICENSE
        3rdparty/SPTK/third_party/WORLD/LICENSE
        3rdparty/SPTK/third_party/REAPER/LICENSE
        3rdparty/SPTK/third_party/SWIPE/LICENSE
        3rdparty/SPTK/third_party/Snack/LICENSE
        3rdparty/SPTK/third_party/GETOPT/LICENSE
)

qt_add_translations(appspeech-rate-meter-2
    TS_FILES
        i18n/speech-rate-meter-2_en.ts
        i18n/speech-rate-meter-2_ru.ts
    QM_FILES_OUTPUT_VARIABLE
        qm_files
)

if(WIN32)
    target_sources(appspeech-rate-meter-2 PRIVATE res/icons/speechratemeter.rc)
endif()

if(APPLE)
    set_source_files_properties(res/icons/speechratemeter.icns
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(appspeech-rate-meter-2 PRIVATE res/icons/speechratemeter.icns)
endif()

set_target_properties(appspeech-rate-meter-2 PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE_ICON_FILE speechratemeter.icns
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

set_property(SOURCE main.cpp APPEND PROPERTY
    COMPILE_DEFINITIONS PROJECT_VERSION="${PROJECT_VERSION}"
)

if(ANDROID)
    set_target_properties(appspeech-rate-meter-2 PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
        QT_ANDROID_TARGET_SDK_VERSION 36
        QT_ANDROID_COMPILE_SDK_VERSION 36
        QT_ANDROID_MIN_SDK_VERSION 26
        QT_ANDROID_VERSION_CODE 1
        QT_ANDROID_VERSION_NAME "${PROJECT_VERSION}"
        QT_ANDROID_APP_NAME "Speech Rate Meter 2"
        QT_ANDROID_PACKAGE_NAME "by.intoncore.SpeechRateMeter2"
        # Uncompressed native libs, 16 KB zip-aligned by AGP 9. Required
        # so Play-generated APKs mmap .so files on 16 KB page-size devices.
        # Address Sanitizer builds should override this to TRUE.
        QT_ANDROID_LEGACY_PACKAGING FALSE
    )
    # Google Play (API 35+ / 16 KB page size): ELF LOAD segments must be
    # 16 KB aligned. NDK r27 does not default to this (r28+ does).
    target_link_options(appspeech-rate-meter-2 PRIVATE
        "-Wl,-z,max-page-size=16384"
        "-Wl,-z,common-page-size=16384"
    )
endif()

if(ANDROID)
    if(TARGET AndroidOMP)
        target_compile_options(appspeech-rate-meter-2 PRIVATE ${ANDROID_OMP_FLAGS})
        target_link_libraries(appspeech-rate-meter-2
            PRIVATE Qt6::Quick Qt6::Multimedia sptk AndroidOMP
        )
    else()
        target_link_libraries(appspeech-rate-meter-2
            PRIVATE Qt6::Quick Qt6::Multimedia sptk
        )
    endif()

    # Copy runtime resources into android/assets/ so androiddeployqt bundles
    # them into the APK (extracted to AppDataLocation on device).
    add_custom_target(copy_runtime_resources ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_SOURCE_DIR}/android/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/settings.ini"
            "${CMAKE_SOURCE_DIR}/android/assets/settings.ini"
        COMMENT "Copying settings.ini to APK assets"
    )
else()
    target_link_libraries(appspeech-rate-meter-2
        PRIVATE Qt6::Quick Qt6::Multimedia sptk OpenMP::OpenMP_CXX
    )

    # Copy runtime resources to build directory so settings.ini and data/ are
    # available next to the built executable. This runs at build time.
    add_custom_target(copy_runtime_resources ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/settings.ini"
            "${CMAKE_BINARY_DIR}/settings.ini"
        COMMENT "Copying settings.ini to build directory"
    )
endif()

add_dependencies(appspeech-rate-meter-2 copy_runtime_resources)

if(NOT ANDROID)
    enable_testing()
    add_executable(speechrate-analysis-test
        tests/speechrate_analysis_test.cpp
        src/services/speechrateanalysis.cpp
    )
    target_include_directories(speechrate-analysis-test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/services
    )
    add_test(NAME speechrate-analysis COMMAND speechrate-analysis-test)

    add_executable(sessionstore-test
        tests/sessionstore_test.cpp
        src/services/sessionstore.cpp
        src/services/speechrateanalysis.cpp
        src/services/wavfileservice.cpp
        src/services/helpers/wavFile.cpp
        src/api/helpers/settings.cpp
    )
    target_include_directories(sessionstore-test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/api
        ${CMAKE_CURRENT_SOURCE_DIR}/src/services
    )
    target_link_libraries(sessionstore-test PRIVATE Qt6::Core)
    add_test(NAME sessionstore COMMAND sessionstore-test)
endif()

include(GNUInstallDirs)
install(TARGETS appspeech-rate-meter-2
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES packaging/by.intoncore.SpeechRateMeter2.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(DIRECTORY res/icons/hicolor/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor
    FILES_MATCHING PATTERN "*.png")
