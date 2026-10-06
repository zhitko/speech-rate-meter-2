import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Material 6.8
import by.intoncore.session 1.0
import "../components"
import "../utils"

Page {
    id: root
    title: qsTr("Home")
    padding: 0
    property string pageId: "home"

    readonly property var settingsApi: ApplicationWindow.window ? ApplicationWindow.window.settingsApi : null
    readonly property var sessionApi: ApplicationWindow.window ? ApplicationWindow.window.sessionApi : null

    function clock(totalSeconds) {
        var seconds = Math.max(0, Math.floor(totalSeconds))
        var minutes = Math.floor(seconds / 60)
        var remain = seconds % 60
        return (minutes < 10 ? "0" : "") + minutes + ":" + (remain < 10 ? "0" : "") + remain
    }

    function statusText() {
        if (!sessionApi)
            return ""
        switch (sessionApi.phase) {
        case SessionApi.IdleReady:
            return qsTr("Press Start to measure again.")
        case SessionApi.Listening:
            return qsTr("Listening…")
        case SessionApi.TooShort:
            return qsTr("Keep speaking. This phrase is still too short to count.")
        case SessionApi.Measuring:
            return clock(sessionApi.phraseSeconds)
        case SessionApi.Dropped:
            return qsTr("That phrase was too short and was not saved.")
        case SessionApi.MicDenied:
            return qsTr("The microphone is blocked. Allow access in the system settings, then press Start again.")
        default:
            return qsTr("Press Start and speak naturally. A phrase is measured when you pause.")
        }
    }

    function fillerLabel() {
        if (!sessionApi || !settingsApi)
            return ""
        var minValue = settingsApi.fillerMin
        var maxValue = settingsApi.fillerMax
        if (!(maxValue > minValue))
            return "0 %"
        var clamped = Math.min(maxValue, Math.max(minValue, sessionApi.fillerScore))
        var percent = (clamped - minValue) / (maxValue - minValue) * 100
        return percent.toFixed(0) + " %"
    }

    FileDialog {
        id: wavDialog
        title: qsTr("Open File")
        nameFilters: [qsTr("WAV files (*.wav)")]
        fileMode: FileDialog.OpenFile
        onAccepted: if (sessionApi)
            sessionApi.openWavFile(selectedFile)
    }

    VadCalibrationDialog {
        id: startCalibrationDialog
        onCalibrationDoneEnergy: function(threshold) {
            if (settingsApi)
                settingsApi.vadThreshold = threshold
            if (sessionApi && !sessionApi.sessionActive)
                sessionApi.startSession()
        }
        onCalibrationDoneAutocorrelation: function(threshold) {
            if (settingsApi)
                settingsApi.autoCorrThreshold = threshold
            if (sessionApi && !sessionApi.sessionActive)
                sessionApi.startSession()
        }
    }

    DetailsDialog {
        id: detailsDialog
        details: sessionApi ? sessionApi.details : ({})
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ScrollBar.vertical.policy: (root.settingsApi && !root.settingsApi.showNavigationMenu)
                                   ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: Math.max(0, scrollView.availableWidth - AppScale.pagePadding * 2)
            x: AppScale.pagePadding
            spacing: AppScale.pageSpacing

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: AppScale.pagePadding
                spacing: 12

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: root.statusText()
                    font.pixelSize: AppScale.fs(16)
                    color: Theme.onSurface(Material.theme)
                }

                Label {
                    visible: sessionApi && sessionApi.sessionActive && sessionApi.phase !== SessionApi.Measuring
                    text: root.clock(sessionApi ? sessionApi.phraseSeconds : 0)
                    font.pixelSize: AppScale.fs(20)
                    font.bold: true
                    color: Theme.onSurface(Material.theme)
                }
            }

            Frame {
                Layout.fillWidth: true
                padding: 12
                background: Rectangle {
                    color: Theme.surfaceContainerLow(Material.theme)
                    radius: 16
                }

                ColumnLayout {
                    width: parent.width
                    spacing: 4

                    SpeechRateGauge {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.fillWidth: true
                        Layout.preferredHeight: 230
                        visible: sessionApi && (sessionApi.hasResult || sessionApi.sessionActive)
                        value: (sessionApi && sessionApi.hasResult)
                              ? sessionApi.speechRate
                              : ((settingsApi ? settingsApi.slowWpm : 70) + (settingsApi ? settingsApi.fastWpm : 210)) / 2
                        minimum: settingsApi ? settingsApi.slowWpm : 70
                        maximum: settingsApi ? settingsApi.fastWpm : 210
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 180
                        visible: !sessionApi || (!sessionApi.hasResult && !sessionApi.sessionActive)
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        wrapMode: Text.Wrap
                        text: root.statusText()
                        font.pixelSize: AppScale.fs(18)
                        color: Theme.onSurfaceVariant(Material.theme)
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: -4
                        visible: sessionApi && sessionApi.hasResult
                        text: (sessionApi ? sessionApi.speechRate.toFixed(0) : "0") + " " + qsTr("wpm")
                        font.pixelSize: AppScale.fs(AppScale.isCompact ? 36 : 48)
                        font.bold: true
                        color: Theme.onSurface(Material.theme)
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 8
                        columns: 2
                        columnSpacing: 16
                        rowSpacing: 8
                        visible: sessionApi && sessionApi.hasResult

                        Label { text: qsTr("Articulation"); color: Theme.onSurfaceVariant(Material.theme); font.pixelSize: AppScale.fs(15) }
                        Label {
                            text: (sessionApi ? sessionApi.articulationRate.toFixed(0) : "0") + " " + qsTr("wpm")
                            color: Theme.onSurface(Material.theme)
                            font.pixelSize: AppScale.fs(15)
                            Layout.alignment: Qt.AlignRight
                        }
                        Label { text: qsTr("Fillers"); color: Theme.onSurfaceVariant(Material.theme); font.pixelSize: AppScale.fs(15) }
                        Label {
                            text: root.fillerLabel()
                            color: Theme.onSurface(Material.theme)
                            font.pixelSize: AppScale.fs(15)
                            Layout.alignment: Qt.AlignRight
                        }
                        Label { text: qsTr("Pauses"); color: Theme.onSurfaceVariant(Material.theme); font.pixelSize: AppScale.fs(15) }
                        Label {
                            text: (sessionApi ? sessionApi.phrasePauses.toFixed(2) : "0.00") + " " + qsTr("sec")
                            color: Theme.onSurface(Material.theme)
                            font.pixelSize: AppScale.fs(15)
                            Layout.alignment: Qt.AlignRight
                        }
                        Label { text: qsTr("Speech"); color: Theme.onSurfaceVariant(Material.theme); font.pixelSize: AppScale.fs(15) }
                        Label {
                            text: (sessionApi ? sessionApi.speechDuration.toFixed(0) : "0") + " " + qsTr("sec")
                            color: Theme.onSurface(Material.theme)
                            font.pixelSize: AppScale.fs(15)
                            Layout.alignment: Qt.AlignRight
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                spacing: 12
                visible: sessionApi && sessionApi.sessionActive

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Level")
                    font.pixelSize: AppScale.fs(13)
                    color: Theme.onSurfaceVariant(Material.theme)
                }

                Item {
                    Layout.fillWidth: true
                    implicitHeight: 18

                    Rectangle {
                        anchors.fill: parent
                        radius: height / 2
                        color: Theme.surfaceContainerHigh(Material.theme)
                    }

                    Rectangle {
                        width: Math.max(height, parent.width * Math.max(0, Math.min(1, sessionApi ? sessionApi.audioLevel : 0)))
                        height: parent.height
                        radius: height / 2
                        color: (sessionApi && sessionApi.audioLevel > 0.08)
                               ? Theme.primary(Material.theme)
                               : Theme.outline(Material.theme)
                    }
                }
            }

            Button {
                id: recordButton
                Layout.alignment: Qt.AlignHCenter
                flat: true
                padding: 0
                hoverEnabled: true
                implicitWidth: AppScale.isCompact ? 112 : 128
                implicitHeight: implicitWidth + AppScale.fs(32)
                onClicked: {
                    if (!sessionApi)
                        return
                    if (sessionApi.sessionActive) {
                        sessionApi.stopSession()
                        return
                    }
                    if (settingsApi && settingsApi.autoCalibrate)
                        startCalibrationDialog.open()
                    else
                        sessionApi.startSession()
                }

                contentItem: Column {
                    spacing: 8
                    width: recordButton.implicitWidth

                    Rectangle {
                        width: parent.width
                        height: width
                        radius: width / 2
                        anchors.horizontalCenter: parent.horizontalCenter
                        color: sessionApi && sessionApi.sessionActive ? Theme.error(Material.theme) : Theme.primary(Material.theme)
                        Label {
                            anchors.centerIn: parent
                            font.family: Icons.familySolid
                            font.weight: Font.Black
                            text: sessionApi && sessionApi.sessionActive ? Icons.faStop : Icons.faMicrophone
                            color: sessionApi && sessionApi.sessionActive ? Theme.onError(Material.theme) : Theme.onPrimary(Material.theme)
                            font.pixelSize: parent.width / 2.6
                        }
                    }

                    Label {
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                        text: sessionApi && sessionApi.sessionActive ? qsTr("Stop") : qsTr("Start")
                        font.pixelSize: AppScale.fs(16)
                        font.bold: true
                        color: Theme.onSurface(Material.theme)
                    }
                }

                background: Item {}
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                visible: settingsApi && settingsApi.advanced
                spacing: 12

                Button {
                    text: qsTr("Details")
                    visible: sessionApi && sessionApi.hasResult
                    onClicked: detailsDialog.open()
                }

                Button {
                    text: qsTr("Open File")
                    visible: sessionApi && sessionApi.openFileAvailable && !sessionApi.sessionActive
                    onClicked: {
                        wavDialog.currentFolder = sessionApi.testsFolderUrl()
                        wavDialog.open()
                    }
                }
            }

            Item { Layout.preferredHeight: AppScale.pagePadding }
        }
    }
}
