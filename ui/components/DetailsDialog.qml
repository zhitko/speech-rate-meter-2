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
        implicitHeight: Math.min(contentColumn.implicitHeight, 420)
        contentHeight: contentColumn.implicitHeight
        clip: true

        ColumnLayout {
            id: contentColumn
            width: parent.width
            spacing: 8

            Repeater {
                model: [
                    [qsTr("Record Length"), root.textOf(root.details.speechDuration, 2)],
                    [qsTr("Consonants & Silence Length"), root.textOf(root.details.gapLength, 2)],
                    [qsTr("Consonants & Silence Count"), root.textOf(root.details.gapCount, 0)],
                    [qsTr("Consonants & Silence Max"), root.textOf(root.details.gapMax, 2)],
                    [qsTr("Consonants & Silence Mean Duration"), root.textOf(root.details.gapMean, 2)],
                    [qsTr("Consonants & Silence Median Duration"), root.textOf(root.details.gapMedian)],
                    [qsTr("Vowels Length"), root.textOf(root.details.vowelLength, 2)],
                    [qsTr("Vowels Count"), root.textOf(root.details.vowelCount, 0)],
                    [qsTr("Vowels Max"), root.textOf(root.details.vowelMax, 2)],
                    [qsTr("Vowels Mean Duration"), root.textOf(root.details.vowelMean, 2)],
                    [qsTr("Vowels Median Duration"), root.textOf(root.details.vowelMedian)],
                    [qsTr("Vowels Speaking Rate"), root.textOf(root.details.vowelsPerSecond, 2)],
                    [qsTr("Mean Filler Sounds"), root.textOf(root.details.fillerScore, 3)]
                ]
                delegate: RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: modelData[0]
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                        color: Theme.onSurface(Material.theme)
                        font.pixelSize: AppScale.fs(14)
                    }
                    Label {
                        text: modelData[1]
                        color: Theme.onSurface(Material.theme)
                        font.pixelSize: AppScale.fs(14)
                    }
                }
            }
        }
    }
}
