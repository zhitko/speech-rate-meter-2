import QtQuick 6.8
import QtQuick.Controls 6.8
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 6.8
import QtQuick.Window

import by.intoncore.settings 1.0
import "utils"

ApplicationWindow {
    id: window
    width: AppScale.designWidth * AppScale.factor
    height: AppScale.designHeight * AppScale.factor
    visible: true
    title: qsTr("Speech Rate Meter 2")

    Binding on flags {
        when: Qt.platform.os === "android"
        value: Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint
    }
    topPadding: 0
    leftPadding: 0
    rightPadding: 0
    bottomPadding: 0

    readonly property real safeTop: SafeArea.margins.top / AppScale.factor
    readonly property real safeBottom: SafeArea.margins.bottom / AppScale.factor
    readonly property real safeLeft: SafeArea.margins.left / AppScale.factor
    readonly property real safeRight: SafeArea.margins.right / AppScale.factor

    SettingsApi {
        id: settingsApi
        onThemeChanged: window.theme = getTheme()
    }

    property alias settingsApi: settingsApi

    function getTheme() {
        return settingsApi.theme === "dark" ? Material.Dark
                : (settingsApi.theme === "light" ? Material.Light : Material.System);
    }

    property var theme: getTheme()

    Binding {
        target: Theme
        property: "primaryColorName"
        value: settingsApi.primaryColor
    }

    Binding {
        target: AppScale
        property: "fontScale"
        value: settingsApi ? settingsApi.fontSizeMultiplier : 1
    }

    Binding {
        target: AppScale
        property: "viewWidth"
        value: stackView.width
    }

    Binding {
        target: AppScale
        property: "viewHeight"
        value: stackView.height
    }

    Material.theme: window.theme
    Material.primary: Theme.primary(window.theme)
    Material.accent: Theme.accent(window.theme)
    Material.background: Theme.background(window.theme)

    Item {
        id: scaledRoot
        width: window.width / AppScale.factor
        height: window.height / AppScale.factor
        transformOrigin: Item.TopLeft
        transform: Scale {
            xScale: AppScale.factor
            yScale: AppScale.factor
        }

        ToolBar {
            id: toolbar
            width: parent.width
            contentHeight: AppScale.isCompact ? 56 : 64
            anchors.top: parent.top
            topPadding: window.safeTop
            leftPadding: window.safeLeft
            rightPadding: window.safeRight

            background: Rectangle {
                anchors.fill: parent
                color: Theme.surface(Material.theme)
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 1
                    color: Theme.outlineVariant(Material.theme)
                    opacity: 0.5
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 8

                ToolButton {
                    id: menuButton
                    font.family: Icons.familySolid
                    font.weight: Font.Black
                    text: Icons.faBars
                    font.pixelSize: AppScale.fs(20)
                    onClicked: drawer.open()

                    background: Rectangle {
                        implicitWidth: 48
                        implicitHeight: 48
                        radius: Theme.shapeMedium
                        color: menuButton.hovered ? Theme.surfaceContainerLow(Material.theme) : "transparent"
                    }
                }

                Label {
                    text: (stackView.currentItem && stackView.currentItem.title)
                          ? stackView.currentItem.title : qsTr("Speech Rate Meter 2")
                    font.pixelSize: AppScale.fs(22)
                    Layout.fillWidth: true
                    elide: Label.ElideRight
                    color: Theme.onSurface(Material.theme)
                    leftPadding: 8
                }

                ToolButton {
                    id: backButton
                    font.family: Icons.familySolid
                    font.weight: Font.Black
                    text: Icons.faArrowLeft
                    font.pixelSize: AppScale.fs(20)
                    onClicked: stackView.pop()
                    visible: stackView.depth > 1

                    background: Rectangle {
                        implicitWidth: 48
                        implicitHeight: 48
                        radius: Theme.shapeMedium
                        color: backButton.hovered ? Theme.surfaceContainerLow(Material.theme) : "transparent"
                    }
                }
            }
        }

        Rectangle {
            id: navigationBar
            readonly property int barContentHeight: AppScale.isCompact ? 68 : 80
            height: barContentHeight
            anchors.bottom: parent.bottom
            anchors.bottomMargin: window.safeBottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: window.safeLeft
            anchors.rightMargin: window.safeRight
            color: Theme.surfaceContainer(Material.theme)
            visible: settingsApi.showNavigationMenu

            Row {
                anchors.fill: parent

                Item {
                    width: parent.width / 2
                    height: parent.height

                    Column {
                        anchors.centerIn: parent
                        spacing: 4

                        Rectangle {
                            width: 64
                            height: 32
                            radius: 16
                            anchors.horizontalCenter: parent.horizontalCenter
                            color: stackView.depth <= 1 ? Theme.secondaryContainer(Material.theme) : "transparent"

                            Text {
                                anchors.centerIn: parent
                                font.family: Icons.familySolid
                                font.weight: Font.Black
                                font.pixelSize: AppScale.fs(20)
                                text: Icons.faHome
                                color: stackView.depth <= 1 ? Theme.onSecondaryContainer(Material.theme) : Theme.onSurfaceVariant(Material.theme)
                            }
                        }

                        Text {
                            text: qsTr("Home")
                            anchors.horizontalCenter: parent.horizontalCenter
                            font.pixelSize: AppScale.fs(12)
                            font.weight: stackView.depth <= 1 ? Font.Bold : Font.Normal
                            color: stackView.depth <= 1 ? Theme.onSurface(Material.theme) : Theme.onSurfaceVariant(Material.theme)
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            stackView.clear();
                            stackView.push("pages/HomePage.qml");
                        }
                    }
                }

                Item {
                    width: parent.width / 2
                    height: parent.height

                    Column {
                        anchors.centerIn: parent
                        spacing: 4

                        Rectangle {
                            width: 64
                            height: 32
                            radius: 16
                            anchors.horizontalCenter: parent.horizontalCenter
                            color: stackView.currentItem && stackView.currentItem.title === qsTr("Settings")
                                   ? Theme.secondaryContainer(Material.theme) : "transparent"

                            Text {
                                anchors.centerIn: parent
                                font.family: Icons.familySolid
                                font.weight: Font.Black
                                font.pixelSize: AppScale.fs(20)
                                text: Icons.faGear
                                color: stackView.currentItem && stackView.currentItem.title === qsTr("Settings")
                                       ? Theme.onSecondaryContainer(Material.theme) : Theme.onSurfaceVariant(Material.theme)
                            }
                        }

                        Text {
                            text: qsTr("Settings")
                            anchors.horizontalCenter: parent.horizontalCenter
                            font.pixelSize: AppScale.fs(12)
                            font.weight: stackView.currentItem && stackView.currentItem.title === qsTr("Settings") ? Font.Bold : Font.Normal
                            color: stackView.currentItem && stackView.currentItem.title === qsTr("Settings")
                                   ? Theme.onSurface(Material.theme) : Theme.onSurfaceVariant(Material.theme)
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (stackView.currentItem && stackView.currentItem.title !== qsTr("Settings"))
                                stackView.push("pages/SettingsPage.qml");
                        }
                    }
                }
            }
        }

        StackView {
            id: stackView
            anchors {
                top: toolbar.bottom
                bottom: settingsApi.showNavigationMenu ? navigationBar.top : parent.bottom
                bottomMargin: settingsApi.showNavigationMenu ? 0 : window.safeBottom
                left: parent.left
                right: parent.right
                leftMargin: window.safeLeft
                rightMargin: window.safeRight
            }

            initialItem: "pages/HomePage.qml"
        }
    }

    Drawer {
        id: drawer
        width: Math.min(window.width * 0.8, 360 * AppScale.factor)
        height: window.height
        z: 2

        background: Rectangle {
            color: Theme.surface(Material.theme)
            radius: 16 * AppScale.factor
        }

        Flickable {
            anchors.fill: parent
            anchors.topMargin: window.SafeArea.margins.top
            anchors.bottomMargin: window.SafeArea.margins.bottom
            anchors.leftMargin: window.SafeArea.margins.left
            contentHeight: (drawerLayout.implicitHeight + 24) * AppScale.factor
            clip: true
            ScrollBar.vertical: ScrollBar {}

            ColumnLayout {
                id: drawerLayout
                width: (drawer.width - 24 * AppScale.factor) / AppScale.factor
                x: 12 * AppScale.factor
                y: 12 * AppScale.factor
                spacing: 0
                transformOrigin: Item.TopLeft
                transform: Scale {
                    xScale: AppScale.factor
                    yScale: AppScale.factor
                }

                Label {
                    text: qsTr("Speech Rate Meter 2")
                    font.pixelSize: AppScale.fs(14)
                    font.weight: Font.Medium
                    color: Theme.onSurfaceVariant(Material.theme)
                    Layout.topMargin: 16
                    Layout.leftMargin: 16
                    Layout.bottomMargin: 16
                }

                Repeater {
                    model: [
                        { text: qsTr("Home"), icon: Icons.faHome, page: "pages/HomePage.qml", clear: true },
                        { text: qsTr("Settings"), icon: Icons.faGear, page: "pages/SettingsPage.qml", clear: false },
                        { text: qsTr("User Guide"), icon: Icons.faBookOpen, page: "pages/UserGuidePage.qml", clear: false },
                        { text: qsTr("Privacy Policy"), icon: Icons.faShieldHalved, page: "pages/PrivacyPolicyPage.qml", clear: false },
                        { text: qsTr("Open-source licences"), icon: Icons.faScaleBalanced, page: "pages/LicensesPage.qml", clear: false }
                    ]

                    delegate: ItemDelegate {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 56

                        contentItem: RowLayout {
                            spacing: 12
                            Text {
                                font.family: Icons.familySolid
                                font.weight: Font.Black
                                text: modelData.icon
                                font.pixelSize: AppScale.fs(18)
                                color: parent.parent.highlighted ? Theme.onSecondaryContainer(Material.theme) : Theme.onSurfaceVariant(Material.theme)
                                Layout.leftMargin: 4
                            }
                            Label {
                                text: modelData.text
                                font.pixelSize: AppScale.fs(14)
                                font.weight: parent.parent.highlighted ? Font.Bold : Font.Normal
                                color: parent.parent.highlighted ? Theme.onSecondaryContainer(Material.theme) : Theme.onSurface(Material.theme)
                                Layout.fillWidth: true
                            }
                        }

                        highlighted: {
                            if (modelData.text === qsTr("Home"))
                                return stackView.depth <= 1;
                            return stackView.currentItem && stackView.currentItem.title === modelData.text;
                        }

                        background: Rectangle {
                            radius: 28
                            color: parent.highlighted ? Theme.secondaryContainer(Material.theme)
                                   : (parent.hovered ? Theme.surfaceContainerLow(Material.theme) : "transparent")
                        }

                        onClicked: {
                            if (modelData.clear)
                                stackView.clear();
                            stackView.push(modelData.page);
                            drawer.close();
                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
                    Layout.preferredHeight: 40
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.outlineVariant(Material.theme)
                    Layout.bottomMargin: 8
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.margins: 16
                    Label {
                        text: qsTr("Dark Mode")
                        Layout.fillWidth: true
                        color: Theme.onSurface(Material.theme)
                    }
                    Switch {
                        checked: window.theme === Material.Dark
                        onCheckedChanged: {
                            settingsApi.theme = checked ? "dark" : "light";
                            settingsApi.save();
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 16
                    text: qsTr("Version %1").arg(Qt.application.version)
                    font.pixelSize: AppScale.fs(12)
                    color: Theme.onSurfaceVariant(Material.theme)
                    opacity: 0.7
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }
}
