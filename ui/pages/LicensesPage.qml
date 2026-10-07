pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material 6.8
import by.intoncore.file 1.0
import "../utils"

Page {
    id: root
    title: qsTr("Open-source licences")
    property string pageId: "licenses"

    property string documentTitle: ""
    property string documentText: ""

    FileApi {
        id: fileApi
    }

    function openDocument(title, relativePath) {
        documentTitle = title;
        let text = fileApi.readTextResource(Qt.resolvedUrl(relativePath));
        documentText = text.length > 0
                ? text
                : qsTr("Unable to load this licence text.");
        licenceDialog.open();
    }

    Material.theme: ApplicationWindow.window ? ApplicationWindow.window.theme : Material.Light

    ListView {
        id: licenceList
        anchors.fill: parent
        clip: true
        spacing: AppScale.px(8)
        leftMargin: AppScale.pagePadding
        rightMargin: AppScale.pagePadding
        topMargin: AppScale.pagePadding
        bottomMargin: AppScale.pagePadding
        ScrollBar.vertical: ScrollBar {
            policy: (ApplicationWindow.window && ApplicationWindow.window.settingsApi
                     && !ApplicationWindow.window.settingsApi.showNavigationMenu)
                    ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
        }

        header: ColumnLayout {
            width: licenceList.width - licenceList.leftMargin - licenceList.rightMargin
            spacing: AppScale.px(8)

            Label {
                text: qsTr("Speech Rate Meter 2 uses open-source software. Select an item to read its complete licence and notices.")
                wrapMode: Text.Wrap
                color: Theme.onSurface(Material.theme)
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("The combined executable includes GPL-licensed ALGLIB. Application source, dependency sources, build instructions, and Qt relinking information are listed in Open-source notices.")
                wrapMode: Text.Wrap
                color: Theme.onSurfaceVariant(Material.theme)
                font.pixelSize: AppScale.fs(13)
                Layout.fillWidth: true
                Layout.bottomMargin: AppScale.px(8)
            }
        }

        model: [
            {
                title: qsTr("Open-source notices and source offer"),
                subtitle: qsTr("Attributions, source links, and Qt relinking instructions"),
                path: "../../licenses/THIRD_PARTY_NOTICES.md"
            },
            {
                title: qsTr("Speech Rate Meter 2 source"),
                subtitle: qsTr("MIT License"),
                path: "../../LICENSE"
            },
            {
                title: "ALGLIB 4.06.0",
                subtitle: "GNU GPL version 2",
                path: "../../3rdparty/alglib-cpp/gpl2.txt"
            },
            {
                title: qsTr("GNU General Public License"),
                subtitle: "GNU GPL version 3",
                path: "../../3rdparty/alglib-cpp/gpl3.txt"
            },
            {
                title: "Qt 6.12.0",
                subtitle: "GNU LGPL version 3",
                path: "../../licenses/LGPL-3.0.txt"
            },
            {
                title: "SPTK 4.3",
                subtitle: "Apache License 2.0",
                path: "../../3rdparty/SPTK/LICENSE"
            },
            {
                title: "SPTK / WORLD",
                subtitle: "BSD 3-Clause-style license",
                path: "../../3rdparty/SPTK/third_party/WORLD/LICENSE"
            },
            {
                title: "SPTK / REAPER",
                subtitle: "Apache License 2.0",
                path: "../../3rdparty/SPTK/third_party/REAPER/LICENSE"
            },
            {
                title: "SPTK / SWIPE",
                subtitle: "MIT License",
                path: "../../3rdparty/SPTK/third_party/SWIPE/LICENSE"
            },
            {
                title: "SPTK / Snack",
                subtitle: qsTr("Permissive license"),
                path: "../../3rdparty/SPTK/third_party/Snack/LICENSE"
            },
            {
                title: "SPTK / ya_getopt",
                subtitle: "BSD 2-Clause-style license",
                path: "../../3rdparty/SPTK/third_party/GETOPT/LICENSE"
            },
            {
                title: "Font Awesome Free 7.2.0",
                subtitle: "SIL Open Font License 1.1",
                path: "../../licenses/Font-Awesome-Free-LICENSE.txt"
            },
            {
                title: qsTr("LLVM OpenMP runtime"),
                subtitle: "Apache License 2.0 with LLVM Exceptions",
                path: "../../licenses/LLVM-OpenMP-LICENSE.txt"
            }
        ]

        delegate: Button {
            id: licenceButton
            required property var modelData
            width: licenceList.width - licenceList.leftMargin - licenceList.rightMargin
            height: AppScale.px(64)
            leftPadding: AppScale.px(16)
            rightPadding: AppScale.px(16)

            contentItem: Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: AppScale.px(2)

                Label {
                    text: licenceButton.modelData.title
                    color: Theme.onSurface(Material.theme)
                    font.bold: true
                    width: parent.width
                    elide: Text.ElideRight
                }

                Label {
                    text: licenceButton.modelData.subtitle
                    color: Theme.onSurfaceVariant(Material.theme)
                    font.pixelSize: AppScale.fs(12)
                    width: parent.width
                    elide: Text.ElideRight
                }
            }

            background: Rectangle {
                color: licenceButton.down
                       ? Theme.primaryContainer(Material.theme)
                       : Theme.surfaceContainerLow(Material.theme)
                radius: AppScale.px(10)
            }

            onClicked: root.openDocument(licenceButton.modelData.title,
                                         licenceButton.modelData.path)
        }
    }

    Dialog {
        id: licenceDialog
        title: root.documentTitle
        modal: true
        clip: true
        // Popups reparent to Overlay (window space). Size against that, not the
        // scaled page — otherwise the dialog overflows on small desktops.
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: {
            const available = parent ? parent.width : root.width;
            return Math.min(AppScale.px(760), Math.max(0, available - AppScale.px(24)));
        }
        height: {
            const available = parent ? parent.height : root.height;
            return Math.min(AppScale.px(760), Math.max(0, available - AppScale.px(24)));
        }

        header: Label {
            text: licenceDialog.title
            visible: text.length > 0
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
            font.pixelSize: AppScale.fs(16)
            font.bold: true
            color: Theme.onSurface(Material.theme)
            padding: AppScale.px(16)
            bottomPadding: AppScale.px(8)
            width: licenceDialog.availableWidth
        }

        contentItem: Flickable {
            id: licenceFlickable
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            contentWidth: width
            contentHeight: licenceText.implicitHeight
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            TextEdit {
                id: licenceText
                width: licenceFlickable.width
                text: root.documentText
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                color: Theme.onSurface(Material.theme)
                font.family: "monospace"
                font.pixelSize: AppScale.fs(11)
            }
        }

        footer: DialogButtonBox {
            background: Rectangle {
                color: "transparent"
            }
            Button {
                text: qsTr("Close")
                font.pixelSize: AppScale.fs(14)
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
        }
    }
}
