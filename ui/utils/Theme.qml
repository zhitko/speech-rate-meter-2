pragma Singleton
import QtQuick 6.8
import QtQuick.Controls.Material 6.8

Item {
    // ── MD3 Shape Tokens ──────────────────────────────────────────────────────
    // Shape tokens define corner radius values for different component sizes
    readonly property int shapeExtraSmall: 4    // Extra small components (chips, small buttons)
    readonly property int shapeSmall: 8         // Small components (text fields, small cards)
    readonly property int shapeMedium: 12       // Medium components (buttons, cards, dialogs)
    readonly property int shapeLarge: 16        // Large components (large cards, sheets)
    readonly property int shapeExtraLarge: 28   // Extra large components (full-screen dialogs)
    Item {
        id: lightTheme
        readonly property color primary: "#415F91"
        readonly property color surfaceTint: "#415F91"
        readonly property color colorOnPrimary: "#FFFFFF"
        readonly property color primaryContainer: "#D6E3FF"
        readonly property color colorOnPrimaryContainer: "#284777"
        readonly property color secondary: "#565F71"
        readonly property color colorOnSecondary: "#FFFFFF"
        readonly property color secondaryContainer: "#DAE2F9"
        readonly property color colorOnSecondaryContainer: "#3E4759"
        readonly property color tertiary: "#705575"
        readonly property color colorOnTertiary: "#FFFFFF"
        readonly property color tertiaryContainer: "#FAD8FD"
        readonly property color colorOnTertiaryContainer: "#573E5C"
        readonly property color error: "#BA1A1A"
        readonly property color colorOnError: "#FFFFFF"
        readonly property color errorContainer: "#FFDAD6"
        readonly property color colorOnErrorContainer: "#93000A"
        readonly property color success: '#1f6d21'
        readonly property color background: "#F9F9FF"
        readonly property color colorOnBackground: "#191C20"
        readonly property color surface: "#F9F9FF"
        readonly property color colorOnSurface: "#191C20"
        readonly property color surfaceVariant: "#E0E2EC"
        readonly property color colorOnSurfaceVariant: "#44474E"
        readonly property color outline: "#74777F"
        readonly property color outlineVariant: "#C4C6D0"
        readonly property color shadow: "#000000"
        readonly property color scrim: "#000000"
        readonly property color inverseSurface: "#2E3036"
        readonly property color inverseOnSurface: "#F0F0F7"
        readonly property color inversePrimary: "#AAC7FF"
        readonly property color primaryFixed: "#D6E3FF"
        readonly property color colorOnPrimaryFixed: "#001B3E"
        readonly property color primaryFixedDim: "#AAC7FF"
        readonly property color colorOnPrimaryFixedVariant: "#284777"
        readonly property color secondaryFixed: "#DAE2F9"
        readonly property color colorOnSecondaryFixed: "#131C2B"
        readonly property color secondaryFixedDim: "#BEC6DC"
        readonly property color colorOnSecondaryFixedVariant: "#3E4759"
        readonly property color tertiaryFixed: "#FAD8FD"
        readonly property color colorOnTertiaryFixed: "#28132E"
        readonly property color tertiaryFixedDim: "#DDBCE0"
        readonly property color colorOnTertiaryFixedVariant: "#573E5C"
        readonly property color surfaceDim: "#D9D9E0"
        readonly property color surfaceBright: "#F9F9FF"
        readonly property color surfaceContainerLowest: "#FFFFFF"
        readonly property color surfaceContainerLow: "#F3F3FA"
        readonly property color surfaceContainer: "#EDEDF4"
        readonly property color surfaceContainerHigh: "#E7E8EE"
        readonly property color surfaceContainerHighest: "#E2E2E9"
    }

    Item {
        id: darkTheme
        readonly property color primary: "#AAC7FF"
        readonly property color surfaceTint: "#AAC7FF"
        readonly property color colorOnPrimary: "#0A305F"
        readonly property color primaryContainer: "#284777"
        readonly property color colorOnPrimaryContainer: "#D6E3FF"
        readonly property color secondary: "#BEC6DC"
        readonly property color colorOnSecondary: "#283141"
        readonly property color secondaryContainer: "#3E4759"
        readonly property color colorOnSecondaryContainer: "#DAE2F9"
        readonly property color tertiary: "#DDBCE0"
        readonly property color colorOnTertiary: "#3F2844"
        readonly property color tertiaryContainer: "#573E5C"
        readonly property color colorOnTertiaryContainer: "#FAD8FD"
        readonly property color error: "#FFB4AB"
        readonly property color colorOnError: "#690005"
        readonly property color errorContainer: "#93000A"
        readonly property color colorOnErrorContainer: "#FFDAD6"
        readonly property color success: "#81c784"
        readonly property color background: "#111318"
        readonly property color colorOnBackground: "#E2E2E9"
        readonly property color surface: "#111318"
        readonly property color colorOnSurface: "#E2E2E9"
        readonly property color surfaceVariant: "#44474E"
        readonly property color colorOnSurfaceVariant: "#C4C6D0"
        readonly property color outline: "#8E9099"
        readonly property color outlineVariant: "#44474E"
        readonly property color shadow: "#000000"
        readonly property color scrim: "#000000"
        readonly property color inverseSurface: "#E2E2E9"
        readonly property color inverseOnSurface: "#2E3036"
        readonly property color inversePrimary: "#415F91"
        readonly property color primaryFixed: "#D6E3FF"
        readonly property color colorOnPrimaryFixed: "#001B3E"
        readonly property color primaryFixedDim: "#AAC7FF"
        readonly property color colorOnPrimaryFixedVariant: "#284777"
        readonly property color secondaryFixed: "#DAE2F9"
        readonly property color colorOnSecondaryFixed: "#131C2B"
        readonly property color secondaryFixedDim: "#BEC6DC"
        readonly property color colorOnSecondaryFixedVariant: "#3E4759"
        readonly property color tertiaryFixed: "#FAD8FD"
        readonly property color colorOnTertiaryFixed: "#28132E"
        readonly property color tertiaryFixedDim: "#DDBCE0"
        readonly property color colorOnTertiaryFixedVariant: "#573E5C"
        readonly property color surfaceDim: "#111318"
        readonly property color surfaceBright: "#37393E"
        readonly property color surfaceContainerLowest: "#0C0E13"
        readonly property color surfaceContainerLow: "#191C20"
        readonly property color surfaceContainer: "#1D2024"
        readonly property color surfaceContainerHigh: "#282A2F"
        readonly property color surfaceContainerHighest: "#33353A"
    }

    property string primaryColorName: "blue"

    function getPrimaryColorName() {
        return primaryColorName;
    }

    property var palettes: {
        "blue": {
            "light": {
                "primary": "#415F91",
                "onPrimary": "#FFFFFF",
                "primaryContainer": "#D6E3FF",
                "onPrimaryContainer": "#001B3E",
                "secondary": "#565F71",
                "onSecondary": "#FFFFFF",
                "secondaryContainer": "#DAE2F9",
                "onSecondaryContainer": "#131C2B",
                "tertiary": "#705573",
                "onTertiary": "#FFFFFF",
                "tertiaryContainer": "#FAD8FD",
                "onTertiaryContainer": "#28132E",
                "error": "#BA1A1A",
                "onError": "#FFFFFF",
                "errorContainer": "#FFDAD6",
                "onErrorContainer": "#410002"
            },
            "dark": {
                "primary": "#AAC7FF",
                "onPrimary": "#0A305F",
                "primaryContainer": "#284777",
                "onPrimaryContainer": "#D6E3FF",
                "secondary": "#BEC6DC",
                "onSecondary": "#283141",
                "secondaryContainer": "#3E4759",
                "onSecondaryContainer": "#DAE2F9",
                "tertiary": "#DDBE61",
                "onTertiary": "#3E2E00",
                "tertiaryContainer": "#594400",
                "onTertiaryContainer": "#FFDF92",
                "error": "#FFB4AB",
                "onError": "#690005",
                "errorContainer": "#93000A",
                "onErrorContainer": "#FFDAD6"
            }
        },
        "green": {
            "light": {
                "primary": "#3E6837",
                "onPrimary": "#FFFFFF",
                "primaryContainer": "#BFF0B2",
                "onPrimaryContainer": "#002204",
                "secondary": "#53634F",
                "onSecondary": "#FFFFFF",
                "secondaryContainer": "#D6E8CE",
                "onSecondaryContainer": "#111F0F",
                "tertiary": "#386567",
                "onTertiary": "#FFFFFF",
                "tertiaryContainer": "#BCEBEE",
                "onTertiaryContainer": "#002021",
                "error": "#BA1A1A",
                "onError": "#FFFFFF",
                "errorContainer": "#FFDAD6",
                "onErrorContainer": "#410002"
            },
            "dark": {
                "primary": "#A4D398",
                "onPrimary": "#0F380D",
                "primaryContainer": "#274F21",
                "onPrimaryContainer": "#BFF0B2",
                "secondary": "#BBCBB3",
                "onSecondary": "#253423",
                "secondaryContainer": "#3B4B38",
                "onSecondaryContainer": "#D6E8CE",
                "tertiary": "#A0CFD2",
                "onTertiary": "#003738",
                "tertiaryContainer": "#1E4D4F",
                "onTertiaryContainer": "#BCEBEE",
                "error": "#FFB4AB",
                "onError": "#690005",
                "errorContainer": "#93000A",
                "onErrorContainer": "#FFDAD6"
            }
        },
        "purple": {
            "light": {
                "primary": "#6750A4",
                "onPrimary": "#FFFFFF",
                "primaryContainer": "#EADDFF",
                "onPrimaryContainer": "#21005E",
                "secondary": "#625B71",
                "onSecondary": "#FFFFFF",
                "secondaryContainer": "#E8DEF8",
                "onSecondaryContainer": "#1E192B",
                "tertiary": "#7D5260",
                "onTertiary": "#FFFFFF",
                "tertiaryContainer": "#FFD8E4",
                "onTertiaryContainer": "#31111D",
                "error": "#B3261E",
                "onError": "#FFFFFF",
                "errorContainer": "#F9DEDC",
                "onErrorContainer": "#410E0B"
            },
            "dark": {
                "primary": "#D0BCFF",
                "onPrimary": "#381E72",
                "primaryContainer": "#4F378B",
                "onPrimaryContainer": "#EADDFF",
                "secondary": "#CCC2DC",
                "onSecondary": "#332D41",
                "secondaryContainer": "#4A4458",
                "onSecondaryContainer": "#E8DEF8",
                "tertiary": "#EFB8C8",
                "onTertiary": "#492532",
                "tertiaryContainer": "#633B48",
                "onTertiaryContainer": "#FFD8E4",
                "error": "#F2B8B5",
                "onError": "#601410",
                "errorContainer": "#8C1D18",
                "onErrorContainer": "#F9DEDC"
            }
        },
        "orange": {
            "light": {
                "primary": "#8B5000",
                "onPrimary": "#FFFFFF",
                "primaryContainer": "#FFDCC0",
                "onPrimaryContainer": "#2D1600",
                "secondary": "#715D4D",
                "onSecondary": "#FFFFFF",
                "secondaryContainer": "#FCDDC6",
                "onSecondaryContainer": "#281A0E",
                "tertiary": "#5C633A",
                "onTertiary": "#FFFFFF",
                "tertiaryContainer": "#E1E9B4",
                "onTertiaryContainer": "#191E01",
                "error": "#BA1A1A",
                "onError": "#FFFFFF",
                "errorContainer": "#FFDAD6",
                "onErrorContainer": "#410002"
            },
            "dark": {
                "primary": "#FFB870",
                "onPrimary": "#4A2800",
                "primaryContainer": "#6A3B00",
                "onPrimaryContainer": "#FFDCC0",
                "secondary": "#DFBFAF",
                "onSecondary": "#3F2E22",
                "secondaryContainer": "#584537",
                "onSecondaryContainer": "#FCDDC6",
                "tertiary": "#C5CD9A",
                "onTertiary": "#2E3410",
                "tertiaryContainer": "#444B24",
                "onTertiaryContainer": "#E1E9B4",
                "error": "#FFB4AB",
                "onError": "#690005",
                "errorContainer": "#93000A",
                "onErrorContainer": "#FFDAD6"
            }
        },
        "red": {
            "light": {
                "primary": "#BF0031",
                "onPrimary": "#FFFFFF",
                "primaryContainer": "#FFDADA",
                "onPrimaryContainer": "#40000A",
                "secondary": "#775656",
                "onSecondary": "#FFFFFF",
                "secondaryContainer": "#FFDADA",
                "onSecondaryContainer": "#2C1516",
                "tertiary": "#755A2B",
                "onTertiary": "#FFFFFF",
                "tertiaryContainer": "#FFDFA6",
                "onTertiaryContainer": "#261900",
                "error": "#BA1A1A",
                "onError": "#FFFFFF",
                "errorContainer": "#FFDAD6",
                "onErrorContainer": "#410002"
            },
            "dark": {
                "primary": "#FFB2B8",
                "onPrimary": "#670015",
                "primaryContainer": "#910022",
                "onPrimaryContainer": "#FFDADA",
                "secondary": "#E6BDBE",
                "onSecondary": "#44292B",
                "secondaryContainer": "#5D3F41",
                "onSecondaryContainer": "#FFDADA",
                "tertiary": "#E6C38A",
                "onTertiary": "#422C05",
                "tertiaryContainer": "#5B4219",
                "onTertiaryContainer": "#FFDFA6",
                "error": "#FFB4AB",
                "onError": "#690005",
                "errorContainer": "#93000A",
                "onErrorContainer": "#FFDAD6"
            }
        },
        "stainless_steel": {
            "light": {
                "primary": "#535F70",
                "onPrimary": "#FFFFFF",
                "primaryContainer": "#D7E3F7",
                "onPrimaryContainer": "#101C2B",
                "secondary": "#565F6D",
                "onSecondary": "#FFFFFF",
                "secondaryContainer": "#DAE3F2",
                "onSecondaryContainer": "#131C28",
                "tertiary": "#6C5975",
                "onTertiary": "#FFFFFF",
                "tertiaryContainer": "#F4DAFF",
                "onTertiaryContainer": "#26172E",
                "error": "#BA1A1A",
                "onError": "#FFFFFF",
                "errorContainer": "#FFDAD6",
                "onErrorContainer": "#410002"
            },
            "dark": {
                "primary": "#B9C8DF",
                "onPrimary": "#253140",
                "primaryContainer": "#3B4858",
                "onPrimaryContainer": "#D7E3F7",
                "secondary": "#BFC8D5",
                "onSecondary": "#29313C",
                "secondaryContainer": "#3F4753",
                "onSecondaryContainer": "#DAE2F2",
                "tertiary": "#D7BEE0",
                "onTertiary": "#3B2A45",
                "tertiaryContainer": "#53405C",
                "onTertiaryContainer": "#F4DAFF",
                "error": "#FFB4AB",
                "onError": "#690005",
                "errorContainer": "#93000A",
                "onErrorContainer": "#FFDAD6"
            }
        }
    }

    function getColor(propertyName, theme) {
        let colorName = getPrimaryColorName();
        let mode = theme === Material.Dark ? "dark" : "light";
        let palette = palettes[colorName] || palettes["blue"];
        if (palette[mode] && palette[mode][propertyName]) {
            return palette[mode][propertyName];
        }
        return palettes["blue"][mode][propertyName];
    }

    // Helper functions to get colors based on theme
    function primary(theme) {
        return getColor("primary", theme);
    }
    function onPrimary(theme) {
        return getColor("onPrimary", theme);
    }
    function primaryContainer(theme) {
        return getColor("primaryContainer", theme);
    }
    function onPrimaryContainer(theme) {
        return getColor("onPrimaryContainer", theme);
    }

    function secondary(theme) {
        return getColor("secondary", theme);
    }
    function onSecondary(theme) {
        return getColor("onSecondary", theme);
    }
    function secondaryContainer(theme) {
        return getColor("secondaryContainer", theme);
    }
    function onSecondaryContainer(theme) {
        return getColor("onSecondaryContainer", theme);
    }

    function harmonize(color, theme) {
        return Qt.tint(color, Qt.alpha(primary(theme), 0.3));
    }

    function chartPalette(theme) {
        let p = primary(theme);
        // Standard high-contrast visualization colors (Tableau-inspired)
        let baseColors = ["#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd", "#8c564b", "#e377c2", "#7f7f7f", "#bcbd22", "#17becf"];

        // Blend each base color with the current primary color to "merge" them into the theme
        // 30% primary color tint usually gives good harmony without losing the original color's identity
        return baseColors.map(c => Qt.tint(c, Qt.alpha(p, 0.3)));
    }

    function tertiary(theme) {
        return getColor("tertiary", theme);
    }
    function onTertiary(theme) {
        return getColor("onTertiary", theme);
    }
    function tertiaryContainer(theme) {
        return getColor("tertiaryContainer", theme);
    }
    function onTertiaryContainer(theme) {
        return getColor("onTertiaryContainer", theme);
    }

    function error(theme) {
        return getColor("error", theme);
    }
    function onError(theme) {
        return getColor("onError", theme);
    }
    function errorContainer(theme) {
        return getColor("errorContainer", theme);
    }
    function onErrorContainer(theme) {
        return getColor("onErrorContainer", theme);
    }
    function success(theme) {
        return theme === Material.Dark ? darkTheme.success : lightTheme.success;
    }

    function background(theme) {
        return theme === Material.Dark ? darkTheme.background : lightTheme.background;
    }
    function onBackground(theme) {
        return theme === Material.Dark ? darkTheme.colorOnBackground : lightTheme.colorOnBackground;
    }

    function surface(theme) {
        return theme === Material.Dark ? darkTheme.surface : lightTheme.surface;
    }
    function onSurface(theme) {
        return theme === Material.Dark ? darkTheme.colorOnSurface : lightTheme.colorOnSurface;
    }
    function surfaceVariant(theme) {
        return theme === Material.Dark ? darkTheme.surfaceVariant : lightTheme.surfaceVariant;
    }
    function onSurfaceVariant(theme) {
        return theme === Material.Dark ? darkTheme.colorOnSurfaceVariant : lightTheme.colorOnSurfaceVariant;
    }

    function outline(theme) {
        return theme === Material.Dark ? darkTheme.outline : lightTheme.outline;
    }
    function outlineVariant(theme) {
        return theme === Material.Dark ? darkTheme.outlineVariant : lightTheme.outlineVariant;
    }

    function shadow(theme) {
        return theme === Material.Dark ? darkTheme.shadow : lightTheme.shadow;
    }
    function scrim(theme) {
        return theme === Material.Dark ? darkTheme.scrim : lightTheme.scrim;
    }

    function inverseSurface(theme) {
        return theme === Material.Dark ? darkTheme.inverseSurface : lightTheme.inverseSurface;
    }
    function inverseOnSurface(theme) {
        return theme === Material.Dark ? darkTheme.inverseOnSurface : lightTheme.inverseOnSurface;
    }
    function inversePrimary(theme) {
        return theme === Material.Dark ? darkTheme.inversePrimary : lightTheme.inversePrimary;
    }

    function primaryFixed(theme) {
        return theme === Material.Dark ? darkTheme.primaryFixed : lightTheme.primaryFixed;
    }
    function onPrimaryFixed(theme) {
        return theme === Material.Dark ? darkTheme.colorOnPrimaryFixed : lightTheme.colorOnPrimaryFixed;
    }
    function primaryFixedDim(theme) {
        return theme === Material.Dark ? darkTheme.primaryFixedDim : lightTheme.primaryFixedDim;
    }
    function onPrimaryFixedVariant(theme) {
        return theme === Material.Dark ? darkTheme.colorOnPrimaryFixedVariant : lightTheme.colorOnPrimaryFixedVariant;
    }

    function secondaryFixed(theme) {
        return theme === Material.Dark ? darkTheme.secondaryFixed : lightTheme.secondaryFixed;
    }
    function onSecondaryFixed(theme) {
        return theme === Material.Dark ? darkTheme.colorOnSecondaryFixed : lightTheme.colorOnSecondaryFixed;
    }
    function secondaryFixedDim(theme) {
        return theme === Material.Dark ? darkTheme.secondaryFixedDim : lightTheme.secondaryFixedDim;
    }
    function onSecondaryFixedVariant(theme) {
        return theme === Material.Dark ? darkTheme.colorOnSecondaryFixedVariant : lightTheme.colorOnSecondaryFixedVariant;
    }

    function tertiaryFixed(theme) {
        return theme === Material.Dark ? darkTheme.tertiaryFixed : lightTheme.tertiaryFixed;
    }
    function onTertiaryFixed(theme) {
        return theme === Material.Dark ? darkTheme.colorOnTertiaryFixed : lightTheme.colorOnTertiaryFixed;
    }
    function tertiaryFixedDim(theme) {
        return theme === Material.Dark ? darkTheme.tertiaryFixedDim : lightTheme.tertiaryFixedDim;
    }
    function onTertiaryFixedVariant(theme) {
        return theme === Material.Dark ? darkTheme.colorOnTertiaryFixedVariant : lightTheme.colorOnTertiaryFixedVariant;
    }

    function surfaceDim(theme) {
        return theme === Material.Dark ? darkTheme.surfaceDim : lightTheme.surfaceDim;
    }
    function surfaceBright(theme) {
        return theme === Material.Dark ? darkTheme.surfaceBright : lightTheme.surfaceBright;
    }
    function surfaceContainerLowest(theme) {
        return theme === Material.Dark ? darkTheme.surfaceContainerLowest : lightTheme.surfaceContainerLowest;
    }
    function surfaceContainerLow(theme) {
        return theme === Material.Dark ? darkTheme.surfaceContainerLow : lightTheme.surfaceContainerLow;
    }
    function surfaceContainer(theme) {
        return theme === Material.Dark ? darkTheme.surfaceContainer : lightTheme.surfaceContainer;
    }
    function surfaceContainerHigh(theme) {
        return theme === Material.Dark ? darkTheme.surfaceContainerHigh : lightTheme.surfaceContainerHigh;
    }
    function surfaceContainerHighest(theme) {
        return theme === Material.Dark ? darkTheme.surfaceContainerHighest : lightTheme.surfaceContainerHighest;
    }

    // Compatibility helpers
    function accent(theme) {
        return secondary(theme);
    }
    function foreground(theme) {
        return onSurface(theme);
    }
}
