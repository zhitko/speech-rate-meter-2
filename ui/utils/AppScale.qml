pragma Singleton
import QtQuick 6.8

QtObject {
    // ── Platform detection ──────────────────────────────────────────────────
    // On mobile (Android) the OS handles screen-density scaling, so our
    // layout transform must be an identity (factor = 1.0) to avoid making
    // the UI microscopically small.
    readonly property bool isMobile: Qt.platform.os === "android"

    // Design dimensions the UI was built for
    readonly property int designWidth: 640
    readonly property int designHeight: 950

    // Available screen real estate from the primary screen.
    // Qt.application.screens[0] is safe to use from a QtObject singleton.
    readonly property real screenWidth: Qt.application.screens.length > 0 ? Qt.application.screens[0].desktopAvailableWidth : designWidth
    readonly property real screenHeight: Qt.application.screens.length > 0 ? Qt.application.screens[0].desktopAvailableHeight : designHeight

    // Scale factor: shrink to fit if the screen is smaller than the design size,
    // but never enlarge beyond 1.0 (keeps the UI crisp on larger screens).
    // On mobile platforms the OS handles density scaling, so factor is fixed at 1.0.
    readonly property real factor: isMobile ? 1.0 : Math.min(1.0, Math.min((screenWidth - 100) / designWidth, (screenHeight - 100) / designHeight))

    // Current content area (StackView) in design-space pixels. Bound from Main.qml.
    property real viewWidth: designWidth
    property real viewHeight: designHeight

    // ── Breakpoints (phone / tablet / desktop) ──────────────────────────────
    readonly property int compactWidth: 400
    readonly property int narrowWidth: 520
    // Typical phone StackView height with toolbar + bottom nav is ~620–700.
    readonly property int shortHeight: 720

    readonly property bool isCompact: viewWidth < compactWidth
    readonly property bool isNarrow: viewWidth < narrowWidth
    readonly property bool isShort: viewHeight < shortHeight
    readonly property bool isWide: viewWidth >= 720

    readonly property int pagePadding: isCompact ? 12 : (isNarrow ? 16 : 24)
    readonly property int pageSpacing: isCompact ? 12 : (isNarrow ? 16 : 24)
    readonly property int listSpacing: isCompact ? 6 : 10
    readonly property int listItemHeight: isCompact ? 80 : 100
    readonly property int searchBarHeight: isCompact ? 48 : 56

    // Font size multiplier from user settings (1.0 = Normal, 1.5 = Big, 2.0 = Large).
    // Updated from Main.qml via Binding so all QML fs() calls react reactively.
    property double fontScale: 1.0

    // Convenience helpers -------------------------------------------------

    // Scale a design-space length (px, dp …)
    function px(value) {
        return Math.round(value * factor)
    }

    // Scale a font pixel size (minimum 8 px to stay legible).
    // Applies both the layout scale factor AND the user font size multiplier.
    function fs(pixelSize) {
        return Math.max(8, Math.round(pixelSize * fontScale))
    }

    // Legacy alias kept for backward compat (layout scale only, no font scale)
    function font(pixelSize) {
        return Math.max(8, Math.round(pixelSize * factor))
    }
}
