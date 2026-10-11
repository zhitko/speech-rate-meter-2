import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material 6.8
import "../utils"

Rectangle {
    id: root
    property bool pending: false
    property var summary: ({})

    readonly property int vowelCount: summary ? Number(summary.vowelCount) || 0 : 0
    readonly property int pauseCount: summary ? Number(summary.phrasalPauseCount) || 0 : 0
    readonly property bool paceValid: summary ? !!summary.paceValid : false
    readonly property var pauseBins: summary && summary.pauseHistogram ? summary.pauseHistogram : []
    readonly property int pauseHistogramPeak: {
        var peak = 0
        for (var index = 0; index < pauseBins.length; ++index)
            peak = Math.max(peak, Number(pauseBins[index].count) || 0)
        return peak
    }
    readonly property bool pauseHistogramOpen: {
        if (pauseBins.length < 1)
            return false
        return !!pauseBins[pauseBins.length - 1].openEnded
    }

    function milliseconds(seconds) {
        var value = Number(seconds)
        if (isNaN(value))
            return "—"
        return Math.round(value * 1000) + " " + qsTr("ms")
    }

    function seconds(value, digits) {
        var number = Number(value)
        if (isNaN(number))
            return "—"
        return number.toFixed(digits) + " " + qsTr("s")
    }

    function wpm(value) {
        var number = Number(value)
        if (isNaN(number))
            return "—"
        return Math.round(number) + " " + qsTr("wpm")
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
            text: qsTr("Gaps of at least %1 ms").arg(root.summary ? root.summary.pauseThresholdMs : 200)
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

        Label {
            Layout.fillWidth: true
            visible: !root.pending && root.pauseCount > 0
            text: qsTr("Pause durations")
            font.pixelSize: AppScale.fs(13)
            font.weight: Font.Medium
            color: Theme.onSurface(Material.theme)
        }

        Flow {
            Layout.fillWidth: true
            visible: !root.pending && root.pauseCount > 0
            spacing: 12

            Repeater {
                model: [
                    [qsTr("Total"), root.seconds(root.summary ? root.summary.pauseTotalSec : 0, 2)],
                    [qsTr("Ratio"), ((Number(root.summary ? root.summary.pauseRatio : 0) || 0) * 100).toFixed(1) + " %"],
                    [qsTr("Mean"), root.milliseconds(root.summary ? root.summary.pauseMean : 0)],
                    [qsTr("Median"), root.milliseconds(root.summary ? root.summary.pauseMedian : 0)],
                    [qsTr("25th"), root.milliseconds(root.summary ? root.summary.pauseP25 : 0)],
                    [qsTr("75th"), root.milliseconds(root.summary ? root.summary.pauseP75 : 0)]
                ]
                delegate: Label {
                    text: modelData[0] + " " + modelData[1]
                    font.pixelSize: AppScale.fs(13)
                    font.features: { "tnum": 1 }
                    color: Theme.onSurface(Material.theme)
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            visible: !root.pending && root.pauseCount > 0 && root.pauseBins.length > 0

            Row {
                anchors.fill: parent
                spacing: 1

                Repeater {
                    model: root.pauseBins
                    delegate: Item {
                        width: Math.max(2, (parent.width - (root.pauseBins.length - 1)) / root.pauseBins.length)
                        height: parent.height

                        Rectangle {
                            anchors.bottom: parent.bottom
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: Math.max(1, parent.width - 1)
                            height: Math.max(2, parent.height * (Number(modelData.count) || 0) / Math.max(1, root.pauseHistogramPeak))
                            radius: 1
                            color: Theme.secondary(Material.theme)
                        }
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: !root.pending && root.pauseCount > 0
            wrapMode: Text.Wrap
            text: root.pauseHistogramOpen
                  ? qsTr("Each bar is %1 ms. The last bar includes longer pauses.")
                    .arg(root.summary ? root.summary.pauseHistogramBinMs : 50)
                  : qsTr("Each bar is %1 ms.")
                    .arg(root.summary ? root.summary.pauseHistogramBinMs : 50)
            font.pixelSize: AppScale.fs(12)
            color: Theme.onSurfaceVariant(Material.theme)
        }

        Label {
            Layout.fillWidth: true
            visible: !root.pending && root.paceValid
            text: qsTr("Compare")
            font.pixelSize: AppScale.fs(13)
            font.weight: Font.Medium
            color: Theme.onSurface(Material.theme)
        }

        GridLayout {
            Layout.fillWidth: true
            visible: !root.pending && root.paceValid
            columns: 2
            columnSpacing: 12
            rowSpacing: 4

            Label {
                text: qsTr("Speech rate")
                font.pixelSize: AppScale.fs(13)
                color: Theme.onSurfaceVariant(Material.theme)
            }
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                text: root.wpm(root.summary ? root.summary.speechRate : 0)
                font.pixelSize: AppScale.fs(13)
                font.features: { "tnum": 1 }
                color: Theme.onSurface(Material.theme)
            }
            Label {
                text: qsTr("Articulation, direct")
                font.pixelSize: AppScale.fs(13)
                color: Theme.onSurfaceVariant(Material.theme)
            }
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                text: root.wpm(root.summary ? root.summary.directArticulationRate : 0)
                font.pixelSize: AppScale.fs(13)
                font.features: { "tnum": 1 }
                color: Theme.onSurface(Material.theme)
            }
            Label {
                text: qsTr("Articulation, legacy")
                font.pixelSize: AppScale.fs(13)
                color: Theme.onSurfaceVariant(Material.theme)
            }
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                text: root.wpm(root.summary ? root.summary.legacyArticulationRate : 0)
                font.pixelSize: AppScale.fs(13)
                font.features: { "tnum": 1 }
                color: Theme.onSurface(Material.theme)
            }
            Label {
                text: qsTr("Mean pause, direct")
                font.pixelSize: AppScale.fs(13)
                color: Theme.onSurfaceVariant(Material.theme)
            }
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                text: root.milliseconds(root.summary ? root.summary.directPauseMeanSec : 0)
                font.pixelSize: AppScale.fs(13)
                font.features: { "tnum": 1 }
                color: Theme.onSurface(Material.theme)
            }
            Label {
                text: qsTr("Pause score, legacy")
                font.pixelSize: AppScale.fs(13)
                color: Theme.onSurfaceVariant(Material.theme)
            }
            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignRight
                text: Number(root.summary ? root.summary.legacyPauseScore : 0).toFixed(2)
                font.pixelSize: AppScale.fs(13)
                font.features: { "tnum": 1 }
                color: Theme.onSurface(Material.theme)
            }
        }
    }
}
