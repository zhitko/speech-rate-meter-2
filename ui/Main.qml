pragma ComponentBehavior: Bound

import QtQuick 6.8
import QtQuick.Controls 6.8
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 6.8
import QtQuick.Window

import by.intoncore.settings 1.0
import by.intoncore.session 1.0
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

    SessionApi {
        id: sessionApi
    }

    property alias settingsApi: settingsApi
    property alias sessionApi: sessionApi

    function goHome() {
        if (stackView.depth === 1 && stackView.currentItem && stackView.currentItem.pageId === "home")
            return
        stackView.clear()
        stackView.push("pages/HomePage.qml")
    }

    function navigateTo(id, page, properties) {
        if (id === "settings" && sessionApi.sessionActive)
            sessionApi.stopSession()

        if (id === "home") {
            goHome()
            return
        }

        if (id === "history" && pageId() === "session") {
            stackView.pop()
            return
        }

        if (pageId() !== id)
            stackView.push(page, properties || {})
    }

    function pageId() {
        return stackView.currentItem && stackView.currentItem.pageId ? stackView.currentItem.pageId : ""
    }

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

                Button {
                    id: recordingChip
                    visible: sessionApi.sessionActive
                    flat: true
                    padding: 0
                    implicitHeight: 32
                    implicitWidth: chipRow.implicitWidth + 20
                    focusPolicy: Qt.StrongFocus
                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Recording in progress. Return to Home")
                    onClicked: window.navigateTo("home", "pages/HomePage.qml")

                    background: Rectangle {
                        radius: height / 2
                        color: recordingChip.activeFocus
                               ? Theme.secondaryContainer(Material.theme)
                               : Theme.errorContainer(Material.theme)
                        border.width: recordingChip.activeFocus ? 2 : 0
                        border.color: Theme.primary(Material.theme)
                    }

                    Row {
                        id: chipRow
                        anchors.centerIn: parent
                        spacing: 6
                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            anchors.verticalCenter: parent.verticalCenter
                            color: Theme.error(Material.theme)
                        }
                        Label {
                            text: qsTr("Recording")
                            font.pixelSize: AppScale.fs(13)
                            color: Theme.onErrorContainer(Material.theme)
                        }
                    }
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

                Repeater {
                    model: [
                        { text: qsTr("Home"), icon: Icons.faHome, id: "home", page: "pages/HomePage.qml" },
                        { text: qsTr("History"), icon: Icons.faClockRotateLeft, id: "history", page: "pages/HistoryPage.qml" },
                        { text: qsTr("Settings"), icon: Icons.faGear, id: "settings", page: "pages/SettingsPage.qml" }
                    ]

                    Button {
                        id: navigationButton
                        required property var modelData
                        width: navigationBar.width / 3
                        height: navigationBar.height
                        flat: true
                        padding: 0
                        focusPolicy: Qt.StrongFocus
                        readonly property bool selected: modelData.id === "history"
                                                         ? (window.pageId() === "history" || window.pageId() === "session")
                                                         : window.pageId() === modelData.id
                        Accessible.role: Accessible.Button
                        Accessible.name: modelData.text
                        Accessible.description: selected ? qsTr("Current page") : ""
                        onClicked: window.navigateTo(modelData.id, modelData.page)

                        contentItem: Column {
                            anchors.centerIn: parent
                            spacing: 4

                            Rectangle {
                                width: 64
                                height: 32
                                radius: 16
                                anchors.horizontalCenter: parent.horizontalCenter
                                color: navigationButton.selected || navigationButton.activeFocus
                                       ? Theme.secondaryContainer(Material.theme) : "transparent"

                                Text {
                                    anchors.centerIn: parent
                                    font.family: Icons.familySolid
                                    font.weight: Font.Black
                                    font.pixelSize: AppScale.fs(20)
                                    text: navigationButton.modelData.icon
                                    color: navigationButton.selected
                                           ? Theme.onSecondaryContainer(Material.theme)
                                           : Theme.onSurfaceVariant(Material.theme)
                                }
                            }

                            Text {
                                text: navigationButton.modelData.text
                                anchors.horizontalCenter: parent.horizontalCenter
                                font.pixelSize: AppScale.fs(12)
                                font.weight: navigationButton.selected ? Font.Bold : Font.Normal
                                color: navigationButton.selected ? Theme.onSurface(Material.theme) : Theme.onSurfaceVariant(Material.theme)
                            }
                        }

                        background: Item {}
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
                        { text: qsTr("Home"), icon: Icons.faHome, page: "pages/HomePage.qml", id: "home" },
                        { text: qsTr("History"), icon: Icons.faClockRotateLeft, page: "pages/HistoryPage.qml", id: "history" },
                        { text: qsTr("Settings"), icon: Icons.faGear, page: "pages/SettingsPage.qml", id: "settings" },
                        { text: qsTr("User Guide"), icon: Icons.faBookOpen, page: "pages/UserGuidePage.qml", id: "guide" },
                        { text: qsTr("Privacy Policy"), icon: Icons.faShieldHalved, page: "pages/PrivacyPolicyPage.qml", id: "privacy" },
                        { text: qsTr("Open-source licences"), icon: Icons.faScaleBalanced, page: "pages/LicensesPage.qml", id: "licenses" }
                    ]

                    delegate: ItemDelegate {
                        id: drawerButton
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 56

                        contentItem: RowLayout {
                            spacing: 12
                            Text {
                                font.family: Icons.familySolid
                                font.weight: Font.Black
                                text: drawerButton.modelData.icon
                                font.pixelSize: AppScale.fs(18)
                                color: parent.parent.highlighted ? Theme.onSecondaryContainer(Material.theme) : Theme.onSurfaceVariant(Material.theme)
                                Layout.leftMargin: 4
                            }
                            Label {
                                text: drawerButton.modelData.text
                                font.pixelSize: AppScale.fs(14)
                                font.weight: parent.parent.highlighted ? Font.Bold : Font.Normal
                                color: parent.parent.highlighted ? Theme.onSecondaryContainer(Material.theme) : Theme.onSurface(Material.theme)
                                Layout.fillWidth: true
                            }
                        }

                        highlighted: {
                            if (modelData.id === "history")
                                return window.pageId() === "history" || window.pageId() === "session"
                            return window.pageId() === modelData.id
                        }

                        background: Rectangle {
                            radius: 28
                            color: parent.highlighted ? Theme.secondaryContainer(Material.theme)
                                   : (parent.hovered ? Theme.surfaceContainerLow(Material.theme) : "transparent")
                        }

                        onClicked: {
                            window.navigateTo(modelData.id, modelData.page)
                            drawer.close()
                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
                    Layout.preferredHeight: 40
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
