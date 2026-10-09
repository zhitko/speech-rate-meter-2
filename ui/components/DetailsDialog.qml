import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material 6.8
import "../utils"

Dialog {
    id: root
    modal: true
    anchors.centerIn: parent
    width: Math.min(parent ? parent.width * 0.92 : 420, 480)
    title: qsTr("Details")
    standardButtons: Dialog.Close

    property var details: ({})

    function textOf(value, digits) {
        var number = Number(value)
        if (isNaN(number))
            return ""
        if (digits === undefined)
            return number.toString()
        return number.toFixed(digits)
    }

    contentItem: Flickable {
        id: detailsFlickable
        implicitHeight: Math.min(contentColumn.implicitHeight, 420)
        contentWidth: width
        contentHeight: contentColumn.implicitHeight
        clip: true
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {
            id: detailsScrollBar
            policy: ScrollBar.AsNeeded
        }

        ColumnLayout {
            id: contentColumn
            width: Math.max(0, detailsFlickable.width
                            - (detailsScrollBar.visible ? detailsScrollBar.width + 8 : 0))
            spacing: 8

            Repeater {
                model: [
                    [qsTr("Speech time (s)"), root.textOf(root.details.speechDuration, 2)],
                    [qsTr("Consonants and silence, total (s)"), root.textOf(root.details.gapLength, 2)],
                    [qsTr("Consonants and silence, count"), root.textOf(root.details.gapCount, 0)],
                    [qsTr("Longest consonants and silence (s)"), root.textOf(root.details.gapMax, 2)],
                    [qsTr("Mean consonants and silence (s)"), root.textOf(root.details.gapMean, 2)],
                    [qsTr("Median consonants and silence (s)"), root.textOf(root.details.gapMedian)],
                    [qsTr("Vowels, total (s)"), root.textOf(root.details.vowelLength, 2)],
                    [qsTr("Vowel count"), root.textOf(root.details.vowelCount, 0)],
                    [qsTr("Longest vowel (s)"), root.textOf(root.details.vowelMax, 2)],
                    [qsTr("Mean vowel (s)"), root.textOf(root.details.vowelMean, 2)],
                    [qsTr("Median vowel (s)"), root.textOf(root.details.vowelMedian)],
                    [qsTr("Vowels per second"), root.textOf(root.details.vowelsPerSecond, 2)],
                    [qsTr("Filler score"), root.textOf(root.details.fillerScore, 3)]
                ]
                delegate: RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: modelData[0]
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        color: Theme.onSurface(Material.theme)
                        font.pixelSize: AppScale.fs(14)
                    }
                    Label {
                        text: modelData[1]
                        Layout.preferredWidth: 88
                        Layout.alignment: Qt.AlignTop | Qt.AlignRight
                        horizontalAlignment: Text.AlignRight
                        color: Theme.onSurface(Material.theme)
                        font.pixelSize: AppScale.fs(14)
                    }
                }
            }
        }
    }
}
