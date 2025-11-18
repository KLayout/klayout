#---------------------------------------------------------------------------------------
# File: src/mac_no_agl.pri
#
# Aims: To avoid linking "AGL" when building with macOS SDK >= 26 (Xcode 26 or later)
#
# Refs: https://github.com/KLayout/klayout/issues/2159
#
# Usage: Include this file in all leaf "*.pro" files, for example,
#        ---> src/tl/tl/tl.pro
#               include($$PWD/../../lib.pri)
#               include($$PWD/../../mac_no_agl.pri) <===
#        --- src/tl/unit_tests/unit_tests.pro
#               include($$PWD/../../lib_ut.pri)
#               include($$PWD/../../mac_no_agl.pri) <===
#---------------------------------------------------------------------------------------

macx {
    # Prevent qmake from injecting dependencies from .prl (most reliable protection)
    CONFIG -= link_prl

    # QMAKE_MAC_SDK examples: "macosx26.0", "macosx26", "macosx27.1"
    SDK_TAG        = $$QMAKE_MAC_SDK
    SDK_VER_STR    = $$replace(SDK_TAG, "macosx", "")
    SDK_VER_MAJOR  = $$section(SDK_VER_STR, ., 0, 0)

    # Fallback: when parsing fails, also match explicit "macosx26"
    contains(SDK_TAG, "macosx26") {
        SDK_VER_MAJOR = 26
    }

    # --- fetch actual SDK info when QMAKE_MAC_SDK only gives "macosx" ---
    SDK_PATH     = $$system("/usr/bin/xcrun --sdk macosx --show-sdk-path")
    SDK_VER_STR  = $$system("/usr/bin/xcrun --sdk macosx --show-sdk-version")

    # Backup extraction: derive version from SDK path (e.g., MacOSX26.0.sdk → 26.0)
    isEmpty(SDK_VER_STR) {
        SDK_BASE      = $$basename($$SDK_PATH)          # MacOSX26.0.sdk
        SDK_VER_STR   = $$replace(SDK_BASE, "MacOSX", "")
        SDK_VER_STR   = $$replace(SDK_VER_STR, ".sdk", "")
    }

    # Extract only the major version number (e.g., 26.0 → 26)
    SDK_VER_MAJOR = $$section(SDK_VER_STR, ., 0, 0)

    # Debug output
    message("DEBUG: SDK_PATH      = $$SDK_PATH")
    message("DEBUG: SDK_VER_STR   = $$SDK_VER_STR")
    message("DEBUG: SDK_VER_MAJOR = $$SDK_VER_MAJOR")

    # Apply AGL removal if SDK version >= 26
    greaterThan(SDK_VER_MAJOR, 25) {
        message("Detected macOS SDK >= 26 ($$SDK_VER_STR). Adjusting flags...")

        # Aggressively remove AGL in case it’s inserted by Qt or manually
        LIBS -= -framework
        LIBS -= AGL
        QMAKE_LIBS_OPENGL -= -framework
        QMAKE_LIBS_OPENGL -= AGL
        QMAKE_LIBS_OPENGL = -framework OpenGL

        # Set consistent minimum deployment target for modern macOS/Apple Silicon
        QMAKE_CXXFLAGS -= -mmacosx-version-min=10.13
        QMAKE_LFLAGS   -= -mmacosx-version-min=10.13

        QMAKE_MACOSX_DEPLOYMENT_TARGET = 12.0
        QMAKE_CXXFLAGS += -mmacosx-version-min=12.0
        QMAKE_LFLAGS  += -mmacosx-version-min=12.0
    }

    # --- stop execution after printing ---
    #error("DEBUG STOP: printed all mac_no_agl.pri variables, stopping qmake.")
}