import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material 6.8
import "../utils"

Rectangle {
    id: root
    property string icon: ""
    property string label: ""
    property string value: "—"
    property string unit: ""
    property string hint: ""
    property color accent: Theme.primary(Material.theme)
    // 0…1 draws a thin bar under the value; a negative value hides it.
    property real progress: -1

    implicitWidth: 150
    implicitHeight: content.implicitHeight + content.anchors.margins * 2
    radius: Theme.shapeLarge
    color: Theme.surfaceContainerLow(Material.theme)

    Accessible.role: Accessible.StaticText
    Accessible.name: label + ": " + value + (unit.length > 0 ? " " + unit : "")
    Accessible.description: hint

    ColumnLayout {
        id: content
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: AppScale.isCompact ? 12 : 14
        spacing: AppScale.isCompact ? 4 : 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                implicitWidth: 28
                implicitHeight: 28
                radius: Theme.shapeSmall
                color: Qt.alpha(root.accent, 0.14)

                Text {
                    anchors.centerIn: parent
                    text: root.icon
                    font.family: Icons.familySolid
                    font.weight: Font.Black
                    font.pixelSize: AppScale.fs(13)
                    color: root.accent
                }
            }

            Text {
                Layout.fillWidth: true
                text: root.label
                elide: Text.ElideRight
                font.pixelSize: AppScale.fs(13)
                font.weight: Font.Medium
                color: Theme.onSurfaceVariant(Material.theme)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Text {
                text: root.value
                font.pixelSize: AppScale.fs(AppScale.isCompact ? 22 : 26)
                font.weight: Font.DemiBold
                font.features: { "tnum": 1 }
                color: root.value === "—" ? Theme.outline(Material.theme) : Theme.onSurface(Material.theme)
            }

            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignBaseline
                visible: root.unit.length > 0
                text: root.unit
                elide: Text.ElideRight
                font.pixelSize: AppScale.fs(13)
                color: Theme.onSurfaceVariant(Material.theme)
            }
        }

        Text {
            Layout.fillWidth: true
            visible: root.hint.length > 0
            text: root.hint
            wrapMode: Text.Wrap
            maximumLineCount: 3
            elide: Text.ElideRight
            font.pixelSize: AppScale.fs(11)
            color: Theme.onSurfaceVariant(Material.theme)
        }

        Rectangle {
            Layout.fillWidth: true
            visible: root.progress >= 0
            implicitHeight: 4
            radius: 2
            color: Qt.alpha(root.accent, 0.16)

            Rectangle {
                width: parent.width * Math.max(0, Math.min(1, root.progress))
                height: parent.height
                radius: parent.radius
                color: root.accent
                Behavior on width { NumberAnimation { duration: 400; easing.type: Easing.OutCubic } }
            }
        }
    }
}
