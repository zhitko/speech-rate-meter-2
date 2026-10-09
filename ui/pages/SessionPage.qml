import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material 6.8
import "../components"
import "../utils"

Page {
    id: root
    title: hasSession ? sessionData.title : qsTr("History")
    padding: 0
    property string pageId: "session"
    property string sessionId: ""

    readonly property var sessionApi: ApplicationWindow.window ? ApplicationWindow.window.sessionApi : null
    readonly property var settingsApi: ApplicationWindow.window ? ApplicationWindow.window.settingsApi : null
    property var sessionData: ({})
    property bool loaded: false
    readonly property bool hasSession: !!(sessionData && sessionData.id)
    readonly property var segments: sessionData.segments ? sessionData.segments : []

    readonly property real layoutWidth: Math.min(scrollView.availableWidth - AppScale.pagePadding * 2, 1120)
    readonly property bool wideLayout: layoutWidth >= 680
    readonly property int gaugeMode: settingsApi ? settingsApi.gaugeMode : 0

    function number(key, decimals) {
        return sessionData[key] !== undefined ? Number(sessionData[key]).toFixed(decimals) : "—"
    }

    function reload() {
        sessionData = sessionApi ? sessionApi.session(sessionId) : ({})
        loaded = true
    }

    Component.onCompleted: reload()

    Connections {
        target: sessionApi
        function onSessionsChanged() { root.reload() }
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.max(0, parent.width - AppScale.pagePadding * 2)
        spacing: 12
        visible: root.loaded && !root.hasSession

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: qsTr("This session is no longer available. It may have been deleted.")
            color: Theme.onSurfaceVariant(Material.theme)
            font.pixelSize: AppScale.fs(16)
        }

        Button {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Back to History")
            onClicked: {
                var view = root.StackView.view
                if (view)
                    view.pop()
            }
        }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        visible: root.hasSession
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: root.layoutWidth
            x: (scrollView.availableWidth - width) / 2
            spacing: AppScale.pageSpacing

            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: AppScale.pagePadding
                implicitHeight: summary.implicitHeight + 32
                radius: Theme.shapeExtraLarge
                color: Theme.surfaceContainerLow(Material.theme)

                GridLayout {
                    id: summary
                    anchors.fill: parent
                    anchors.margins: 16
                    columns: root.wideLayout ? 2 : 1
                    columnSpacing: 16
                    rowSpacing: 12

                    Label {
                        Layout.fillWidth: true
                        Layout.columnSpan: root.wideLayout ? 2 : 1
                        visible: Number(sessionData.phraseCount) > 0
                        text: qsTr("Mean values")
                        font.pixelSize: AppScale.fs(13)
                        font.weight: Font.DemiBold
                        color: Theme.primary(Material.theme)
                    }

                    SpeechRateGauge {
                        Layout.fillWidth: true
                        Layout.preferredWidth: root.wideLayout ? 1 : -1
                        Layout.preferredHeight: root.wideLayout ? 240 : Math.max(190, Math.min(root.layoutWidth * 0.55, 260))
                        hasValue: sessionData.speechRate !== undefined
                        showSecond: root.gaugeMode === 2
                        metricLabel: root.gaugeMode === 1 ? qsTr("Articulation") : qsTr("Speech rate")
                        value: !hasValue ? 0
                               : (root.gaugeMode === 1 ? Number(sessionData.articulationRate) : Number(sessionData.speechRate))
                        secondValue: sessionData.articulationRate !== undefined ? Number(sessionData.articulationRate) : 0
                        minimum: settingsApi ? settingsApi.slowWpm : 70
                        maximum: settingsApi ? settingsApi.fastWpm : 210
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        Layout.preferredWidth: root.wideLayout ? 1 : -1
                        Layout.alignment: Qt.AlignVCenter
                        columns: 2
                        columnSpacing: AppScale.isCompact ? 8 : 12
                        rowSpacing: AppScale.isCompact ? 8 : 12

                        MetricTile {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredWidth: 1
                            visible: root.gaugeMode !== 2
                            color: Theme.surfaceContainer(Material.theme)
                            icon: root.gaugeMode === 1 ? Icons.faGaugeHigh : Icons.faCommentDots
                            label: root.gaugeMode === 1 ? qsTr("Speech rate") : qsTr("Articulation")
                            hint: root.gaugeMode === 1
                                  ? qsTr("Overall pace, pauses included")
                                  : qsTr("Pace while speaking, gaps left out")
                            value: root.gaugeMode === 1 ? root.number("speechRate", 0) : root.number("articulationRate", 0)
                            unit: qsTr("wpm")
                            accent: Theme.primary(Material.theme)
                        }
                        MetricTile {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredWidth: 1
                            color: Theme.surfaceContainer(Material.theme)
                            icon: Icons.faWaveSquare
                            label: qsTr("Fillers")
                            hint: qsTr("Drawn-out sounds, not words")
                            value: root.number("fillerPercent", 0)
                            unit: "%"
                            progress: sessionData.fillerPercent !== undefined ? Number(sessionData.fillerPercent) / 100 : 0
                            accent: Theme.tertiary(Material.theme)
                        }
                        MetricTile {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredWidth: 1
                            color: Theme.surfaceContainer(Material.theme)
                            icon: Icons.faPause
                            label: qsTr("Pauses")
                            hint: qsTr("Longer gaps, not all silence")
                            value: root.number("phrasePauses", 2)
                            unit: qsTr("sec")
                            accent: Theme.secondary(Material.theme)
                        }
                        MetricTile {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredWidth: 1
                            color: Theme.surfaceContainer(Material.theme)
                            icon: Icons.faStopwatch
                            label: qsTr("Speech")
                            hint: qsTr("Total time counted as speech")
                            value: root.number("speechDuration", 0)
                            unit: qsTr("sec")
                            accent: Theme.zoneColor(0, Material.theme)
                        }
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: qsTr("Each point is one moment the numbers on Home changed.")
                font.pixelSize: AppScale.fs(14)
                color: Theme.onSurfaceVariant(Material.theme)
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.bottomMargin: AppScale.pagePadding
                columns: root.wideLayout ? 2 : 1
                columnSpacing: AppScale.pageSpacing
                rowSpacing: AppScale.pageSpacing

                MetricChart {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.columnSpan: root.wideLayout ? 2 : 1
                    Layout.preferredHeight: 260
                    chartTitle: qsTr("Speech rate")
                    icon: Icons.faGaugeHigh
                    unit: qsTr("wpm")
                    decimals: 0
                    valueKey: "speechRate"
                    points: root.segments
                }
                MetricChart {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    chartTitle: qsTr("Articulation")
                    icon: Icons.faCommentDots
                    unit: qsTr("wpm")
                    decimals: 0
                    valueKey: "articulationRate"
                    points: root.segments
                }
                MetricChart {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    chartTitle: qsTr("Fillers")
                    icon: Icons.faWaveSquare
                    lineColor: Theme.tertiary(Material.theme)
                    unit: "%"
                    decimals: 0
                    valueKey: "fillerPercent"
                    points: root.segments
                }
                MetricChart {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    chartTitle: qsTr("Pauses")
                    icon: Icons.faPause
                    lineColor: Theme.secondary(Material.theme)
                    unit: qsTr("sec")
                    decimals: 2
                    valueKey: "phrasePauses"
                    points: root.segments
                }
                MetricChart {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    chartTitle: qsTr("Speech")
                    icon: Icons.faStopwatch
                    lineColor: Theme.zoneColor(0, Material.theme)
                    unit: qsTr("sec")
                    decimals: 0
                    valueKey: "speechDuration"
                    points: root.segments
                }
            }
        }
    }
}
