import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material 6.8
import "../utils"

Rectangle {
    id: root
    property bool pending: false
    property var summary: ({})

    readonly property var bins: summary && summary.histogram ? summary.histogram : []
    readonly property int peak: {
        var best = 1
        for (var index = 0; index < bins.length; ++index)
            best = Math.max(best, Number(bins[index].count) || 0)
        return best
    }
    readonly property int vowelCount: summary ? Number(summary.vowelCount) || 0 : 0
    readonly property int pauseCount: summary ? Number(summary.phrasalPauseCount) || 0 : 0

    function milliseconds(seconds) {
        var value = Number(seconds)
        if (isNaN(value))
            return "—"
        return Math.round(value * 1000) + " " + qsTr("ms")
    }

    implicitWidth: 280
    implicitHeight: column.implicitHeight + (AppScale.isCompact ? 24 : 28)
    radius: Theme.shapeLarge
    color: Theme.surfaceContainerLow(Material.theme)

    Accessible.role: Accessible.StaticText
    Accessible.name: pending
                     ? qsTr("Analyzing the whole recording…")
                     : qsTr("Whole recording. %1 vowels, %2 phrasal pauses.")
                       .arg(vowelCount).arg(pauseCount)

    ColumnLayout {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: AppScale.isCompact ? 12 : 14
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: qsTr("Whole recording")
            font.pixelSize: AppScale.fs(13)
            font.weight: Font.DemiBold
            color: Theme.primary(Material.theme)
        }

        Label {
            Layout.fillWidth: true
            visible: root.pending
            wrapMode: Text.Wrap
            text: qsTr("Analyzing the whole recording…")
            font.pixelSize: AppScale.fs(14)
            color: Theme.onSurfaceVariant(Material.theme)
        }

        GridLayout {
            Layout.fillWidth: true
            visible: !root.pending
            columns: 2
            columnSpacing: 12
            rowSpacing: 4

            Label {
                text: qsTr("Vowels")
                font.pixelSize: AppScale.fs(14)
                color: Theme.onSurfaceVariant(Material.theme)
            }
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                text: root.vowelCount.toString()
                font.pixelSize: AppScale.fs(16)
                font.weight: Font.DemiBold
                font.features: { "tnum": 1 }
                color: Theme.onSurface(Material.theme)
            }

            Label {
                text: qsTr("Phrasal pauses")
                font.pixelSize: AppScale.fs(14)
                color: Theme.onSurfaceVariant(Material.theme)
            }
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                text: root.pauseCount.toString()
                font.pixelSize: AppScale.fs(16)
                font.weight: Font.DemiBold
                font.features: { "tnum": 1 }
                color: Theme.onSurface(Material.theme)
            }
        }

        Label {
            Layout.fillWidth: true
            visible: !root.pending
            wrapMode: Text.Wrap
            text: qsTr("Silent or unvoiced gaps of at least %1 ms").arg(root.summary ? root.summary.pauseThresholdMs : 150)
            font.pixelSize: AppScale.fs(12)
            color: Theme.onSurfaceVariant(Material.theme)
        }

        Label {
            Layout.fillWidth: true
            visible: !root.pending && root.vowelCount > 0
            text: qsTr("Vowel durations")
            font.pixelSize: AppScale.fs(13)
            font.weight: Font.Medium
            color: Theme.onSurface(Material.theme)
        }

        Flow {
            Layout.fillWidth: true
            visible: !root.pending && root.vowelCount > 0
            spacing: 12

            Repeater {
                model: [
                    [qsTr("Mean"), root.milliseconds(root.summary ? root.summary.mean : 0)],
                    [qsTr("Median"), root.milliseconds(root.summary ? root.summary.median : 0)],
                    [qsTr("Min"), root.milliseconds(root.summary ? root.summary.min : 0)],
                    [qsTr("Max"), root.milliseconds(root.summary ? root.summary.max : 0)],
                    [qsTr("SD"), root.milliseconds(root.summary ? root.summary.stddev : 0)]
                ]
                delegate: Label {
                    text: modelData[0] + " " + modelData[1]
                    font.pixelSize: AppScale.fs(13)
                    font.features: { "tnum": 1 }
                    color: Theme.onSurface(Material.theme)
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            visible: !root.pending && root.bins.length > 0
            spacing: 4

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                spacing: 2

                Repeater {
                    model: root.bins
                    delegate: Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Rectangle {
                            anchors.bottom: parent.bottom
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: parent.width
                            height: modelData.count > 0
                                    ? Math.max(4, (modelData.count / root.peak) * parent.height)
                                    : 0
                            radius: 2
                            color: Theme.primary(Material.theme)
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: root.bins.length > 0 ? root.bins[0].startMs + " " + qsTr("ms") : ""
                    font.pixelSize: AppScale.fs(11)
                    color: Theme.onSurfaceVariant(Material.theme)
                }
                Item { Layout.fillWidth: true }
                Label {
                    text: {
                        if (root.bins.length < 1)
                            return ""
                        var last = root.bins[root.bins.length - 1]
                        if (last.openEnded)
                            return qsTr("%1 ms+").arg(last.startMs)
                        return last.endMs + " " + qsTr("ms")
                    }
                    font.pixelSize: AppScale.fs(11)
                    color: Theme.onSurfaceVariant(Material.theme)
                }
            }

            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("%1 ms bins").arg(root.summary ? root.summary.histogramBinMs : 20)
                font.pixelSize: AppScale.fs(11)
                color: Theme.onSurfaceVariant(Material.theme)
            }
        }
    }
}
