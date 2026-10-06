import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material 6.8
import "../components"
import "../utils"

Page {
    id: root
    title: sessionData.title ? sessionData.title : qsTr("History")
    padding: 0
    property string pageId: "session"
    property string sessionId: ""

    readonly property var sessionApi: ApplicationWindow.window ? ApplicationWindow.window.sessionApi : null
    property var sessionData: ({})

    function reload() {
        sessionData = sessionApi ? sessionApi.session(sessionId) : ({})
    }

    Component.onCompleted: reload()

    Connections {
        target: sessionApi
        function onSessionsChanged() { root.reload() }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: Math.max(0, scrollView.availableWidth - AppScale.pagePadding * 2)
            x: AppScale.pagePadding
            spacing: 16

            Frame {
                Layout.fillWidth: true
                Layout.topMargin: AppScale.pagePadding
                padding: 16
                background: Rectangle {
                    color: Theme.surfaceContainerLow(Material.theme)
                    radius: 16
                }

                ColumnLayout {
                    width: parent.width
                    spacing: 8

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: sessionData.title ? sessionData.title : ""
                        font.pixelSize: AppScale.fs(20)
                        font.bold: true
                        color: Theme.onSurface(Material.theme)
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("%1 wpm").arg(sessionData.speechRate !== undefined ? Number(sessionData.speechRate).toFixed(0) : "0")
                        font.pixelSize: AppScale.fs(28)
                        font.bold: true
                        color: Theme.onSurface(Material.theme)
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 16
                        rowSpacing: 8

                        Label { text: qsTr("Articulation"); color: Theme.onSurfaceVariant(Material.theme); font.pixelSize: AppScale.fs(15) }
                        Label {
                            text: (sessionData.articulationRate !== undefined ? Number(sessionData.articulationRate).toFixed(0) : "0") + " " + qsTr("wpm")
                            color: Theme.onSurface(Material.theme)
                            font.pixelSize: AppScale.fs(15)
                            Layout.alignment: Qt.AlignRight
                        }
                        Label { text: qsTr("Fillers"); color: Theme.onSurfaceVariant(Material.theme); font.pixelSize: AppScale.fs(15) }
                        Label {
                            text: (sessionData.fillerPercent !== undefined ? Number(sessionData.fillerPercent).toFixed(0) : "0") + " %"
                            color: Theme.onSurface(Material.theme)
                            font.pixelSize: AppScale.fs(15)
                            Layout.alignment: Qt.AlignRight
                        }
                        Label { text: qsTr("Pauses"); color: Theme.onSurfaceVariant(Material.theme); font.pixelSize: AppScale.fs(15) }
                        Label {
                            text: (sessionData.phrasePauses !== undefined ? Number(sessionData.phrasePauses).toFixed(2) : "0.00") + " " + qsTr("sec")
                            color: Theme.onSurface(Material.theme)
                            font.pixelSize: AppScale.fs(15)
                            Layout.alignment: Qt.AlignRight
                        }
                        Label { text: qsTr("Speech"); color: Theme.onSurfaceVariant(Material.theme); font.pixelSize: AppScale.fs(15) }
                        Label {
                            text: (sessionData.speechDuration !== undefined ? Number(sessionData.speechDuration).toFixed(0) : "0") + " " + qsTr("sec")
                            color: Theme.onSurface(Material.theme)
                            font.pixelSize: AppScale.fs(15)
                            Layout.alignment: Qt.AlignRight
                        }
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("Each point is one change shown on Home.")
                font.pixelSize: AppScale.fs(14)
                color: Theme.onSurfaceVariant(Material.theme)
            }

            MetricChart {
                Layout.fillWidth: true
                Layout.preferredHeight: 240
                chartTitle: qsTr("Speech rate")
                unit: qsTr("wpm")
                decimals: 0
                valueKey: "speechRate"
                points: sessionData.segments ? sessionData.segments : []
            }
            MetricChart {
                Layout.fillWidth: true
                chartTitle: qsTr("Articulation")
                unit: qsTr("wpm")
                decimals: 0
                valueKey: "articulationRate"
                points: sessionData.segments ? sessionData.segments : []
            }
            MetricChart {
                Layout.fillWidth: true
                chartTitle: qsTr("Fillers")
                unit: "%"
                decimals: 0
                valueKey: "fillerPercent"
                points: sessionData.segments ? sessionData.segments : []
            }
            MetricChart {
                Layout.fillWidth: true
                chartTitle: qsTr("Pauses")
                unit: qsTr("sec")
                decimals: 2
                valueKey: "phrasePauses"
                points: sessionData.segments ? sessionData.segments : []
            }
            MetricChart {
                Layout.fillWidth: true
                Layout.bottomMargin: AppScale.pagePadding
                chartTitle: qsTr("Speech")
                unit: qsTr("sec")
                decimals: 0
                valueKey: "speechDuration"
                points: sessionData.segments ? sessionData.segments : []
            }
        }
    }
}
