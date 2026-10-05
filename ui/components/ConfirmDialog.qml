import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material 6.8
import "../utils"

Dialog {
    property string titleText: ""
    property string messageText: ""
    property string confirmText: qsTr("Yes")
    property string cancelText: qsTr("No")
    property bool isDestructive: false
    
    anchors.centerIn: parent
    modal: true

    header: Label {
        text: titleText
        font.pixelSize: AppScale.fs(20)
        font.bold: true
        padding: 24
        bottomPadding: 0
        color: Theme.onSurface(Material.theme)
    }

    contentItem: Text {
        text: messageText
        color: Theme.onSurfaceVariant(Material.theme)
        font.pixelSize: AppScale.fs(16)
        padding: 24
    }

    footer: DialogButtonBox {
        Button {
            text: confirmText
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            Material.foreground: isDestructive ? Theme.error(Material.theme) : Theme.primary(Material.theme)
        }
        Button {
            text: cancelText
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            Material.foreground: Theme.primary(Material.theme)
        }
    }
}
