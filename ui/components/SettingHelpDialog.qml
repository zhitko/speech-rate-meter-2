import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material 6.8
import "../utils"

// Shows the full explanation and, when there is one, the formula for a setting.
// Opened from the info button next to each setting label in SettingsPage.
Dialog {
    id: root

    property string helpText: ""
    property string formulaText: ""

    function openFor(title, help, formula) {
        root.title = title
        root.helpText = help
        root.formulaText = formula
        root.open()
    }

    modal: true
    anchors.centerIn: parent
    width: Math.min(560, Overlay.overlay ? Overlay.overlay.width - 48 : 480)

    footer: DialogButtonBox {
        Button {
            text: qsTr("Close")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            Material.foreground: Theme.primary(Material.theme)
        }
    }

    contentItem: ScrollView {
        id: scroll
        clip: true
        contentWidth: availableWidth
        implicitHeight: Math.min(body.implicitHeight,
                                 (Overlay.overlay ? Overlay.overlay.height : 800) * 0.6)

        ColumnLayout {
            id: body
            width: scroll.availableWidth
            spacing: 12

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: root.helpText
                font.pixelSize: AppScale.fs(15)
                color: Theme.onSurface(Material.theme)
            }

            Rectangle {
                visible: root.formulaText.length > 0
                Layout.fillWidth: true
                implicitHeight: formula.implicitHeight + 24
                radius: 12
                color: Theme.surfaceContainerHigh(Material.theme)

                Label {
                    id: formula
                    x: 12
                    y: 12
                    width: parent.width - 24
                    wrapMode: Text.WrapAnywhere
                    text: root.formulaText
                    font.family: "monospace"
                    font.pixelSize: AppScale.fs(13)
                    color: Theme.onSurface(Material.theme)
                }
            }
        }
    }
}
