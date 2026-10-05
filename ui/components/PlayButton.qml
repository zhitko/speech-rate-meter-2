import QtQuick
import QtQuick.Controls
import by.intoncore.audio 1.0
import QtQuick.Controls.Material 6.8

RoundButton {
    id: control
    property string file: ""
    property bool showLabel: false

    AudioApi {
        id: audioApi
    }

    width: 50
    height: width
    radius: width / 2
    visible: file !== ""

    background: Rectangle {
        radius: control.radius
        anchors.fill: parent
        color: control.hovered ? Theme.surfaceContainerLow(Material.theme) : "transparent"
        Label {
            id: label
            font.family: Icons.familySolid
            font.weight: Font.Black
            text: audioApi.isPlaying ? (Icons.faStop + " " + (control.showLabel ? qsTr("Stop") : "")) : (Icons.faPlay + " " + (control.showLabel ? qsTr("Play") : ""))
            color: audioApi.isPlaying ? Theme.error(Material.theme) : Theme.primary(Material.theme)
            anchors.left: parent.left
            anchors.leftMargin: parent.width / 3
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            font.pixelSize: (parent.width / 2) + (control.hovered ? 4 : 0)
            verticalAlignment: Label.AlignVCenter
            Behavior on font.pixelSize {
                NumberAnimation { duration: 100 }
            }
        }
    }

    onClicked: {
        if (audioApi.isPlaying) {
            audioApi.stopPlayback();
        } else {
            audioApi.play(file);
        }
    }
}
