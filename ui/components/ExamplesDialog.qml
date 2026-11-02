import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material 6.8
import QtQuick.Layouts
import by.intoncore.audio 1.0
import "../utils"

Dialog {
    id: root
    modal: true
    anchors.centerIn: parent
    width: Math.min(parent ? parent.width - 24 : 440, 480)
    padding: 0
    title: qsTr("Example tempos")

    property var sessionApi: null
    property string playingFile: ""

    AudioApi {
        id: exampleAudio
        onIsPlayingChanged: if (!isPlaying)
            root.playingFile = ""
    }

    onAboutToHide: exampleAudio.stopPlayback()

    header: ColumnLayout {
        spacing: 4
        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.topMargin: 16
            text: root.title
            font.pixelSize: AppScale.fs(20)
            font.bold: true
            color: Theme.onSurface(Material.theme)
            wrapMode: Text.Wrap
        }
        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.bottomMargin: 8
            text: qsTr("Short recordings of slow, medium, and fast speech. The files in this version are placeholders.")
            wrapMode: Text.Wrap
            font.pixelSize: AppScale.fs(13)
            color: Theme.onSurfaceVariant(Material.theme)
        }
    }

    contentItem: Flickable {
        implicitHeight: Math.min(column.implicitHeight, 520)
        contentHeight: column.implicitHeight
        clip: true
        flickableDirection: Flickable.VerticalFlick

        ColumnLayout {
            id: column
            width: root.availableWidth
            spacing: 14

            Repeater {
                model: ["slow", "medium", "fast"]

                ColumnLayout {
                    required property string modelData
                    Layout.fillWidth: true
                    spacing: 6
                    visible: groupItems.length > 0

                    readonly property string groupId: modelData
                    readonly property var groupItems: {
                        var out = []
                        var all = root.sessionApi && root.sessionApi.exampleTempos
                                  ? root.sessionApi.exampleTempos : []
                        for (var i = 0; i < all.length; ++i) {
                            if (all[i].group === groupId)
                                out.push(all[i])
                        }
                        return out
                    }

                    Label {
                        text: groupId === "slow" ? qsTr("Slow")
                              : (groupId === "medium" ? qsTr("Medium")
                                 : (groupId === "fast" ? qsTr("Fast") : groupId))
                        font.pixelSize: AppScale.fs(14)
                        font.weight: Font.DemiBold
                        color: Theme.zoneColor(groupId === "fast" ? 2 : (groupId === "medium" ? 1 : 0), Material.theme)
                    }

                    Repeater {
                        model: groupItems

                        ColumnLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 4

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Label {
                                    Layout.fillWidth: true
                                    text: modelData.title || modelData.file
                                    wrapMode: Text.Wrap
                                    font.pixelSize: AppScale.fs(15)
                                    color: Theme.onSurface(Material.theme)
                                }

                                Label {
                                    visible: !!modelData.placeholder
                                    text: qsTr("Placeholder")
                                    font.pixelSize: AppScale.fs(11)
                                    font.weight: Font.Medium
                                    color: Theme.onSurfaceVariant(Material.theme)
                                    leftPadding: 8
                                    rightPadding: 8
                                    topPadding: 2
                                    bottomPadding: 2
                                    background: Rectangle {
                                        radius: 8
                                        color: Qt.alpha(Theme.onSurfaceVariant(Material.theme), 0.12)
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Button {
                                    Layout.fillWidth: true
                                    text: root.playingFile === modelData.file && exampleAudio.isPlaying
                                          ? qsTr("Stop") : qsTr("Play")
                                    enabled: !!root.sessionApi && !root.sessionApi.busy
                                    onClicked: {
                                        if (!root.sessionApi)
                                            return
                                        if (root.playingFile === modelData.file && exampleAudio.isPlaying) {
                                            exampleAudio.stopPlayback()
                                            return
                                        }
                                        var path = root.sessionApi.materializeExample(modelData.file)
                                        if (!path || path.length === 0)
                                            return
                                        root.playingFile = modelData.file
                                        exampleAudio.play(path)
                                    }
                                }
                                Button {
                                    Layout.fillWidth: true
                                    text: qsTr("Measure")
                                    enabled: !!root.sessionApi && !root.sessionApi.busy && !root.sessionApi.sessionActive
                                    onClicked: {
                                        if (!root.sessionApi)
                                            return
                                        exampleAudio.stopPlayback()
                                        var name = modelData.file
                                        root.close()
                                        root.sessionApi.openExample(name)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Close")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
