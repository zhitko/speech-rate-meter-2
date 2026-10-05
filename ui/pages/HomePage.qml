import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material 6.8
import by.intoncore.audio 1.0
import "../components"
import "../utils"

Page {
    id: root
    title: qsTr("Home")
    padding: 0

    property string lastRecordedFile: ""
    property bool saveOnStop: false

    readonly property var settingsApi: ApplicationWindow.window ? ApplicationWindow.window.settingsApi : null

    AudioApi {
        id: audioApi
        onPermissionResultReceived: function(granted) {
            if (granted)
                audioApi.startRecording();
            else {
                root.saveOnStop = false;
                Logger.warning("Microphone permission denied");
            }
        }
        onIsRecordingChanged: {
            if (!audioApi.isRecording && root.saveOnStop) {
                root.saveOnStop = false;
                root.lastRecordedFile = audioApi.saveWavFile();
            }
        }
    }

    VadCalibrationDialog {
        id: calibrationDialog
        onCalibrationDoneEnergy: function(threshold) {
            if (root.settingsApi)
                root.settingsApi.vadThreshold = threshold;
            root.beginRecording();
        }
        onCalibrationDoneAutocorrelation: function(threshold) {
            if (root.settingsApi)
                root.settingsApi.autoCorrThreshold = threshold;
            root.beginRecording();
        }
    }

    function beginRecording() {
        lastRecordedFile = "";
        saveOnStop = true;
        if (!audioApi.requestAudioPermission())
            return;
        audioApi.startRecording();
    }

    function toggleRecording() {
        if (audioApi.isRecording) {
            audioApi.stopRecording();
            return;
        }
        if (settingsApi && settingsApi.autoCalibrate) {
            calibrationDialog.open();
            return;
        }
        beginRecording();
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

            Label {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                Layout.topMargin: AppScale.pagePadding
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: qsTr("Speech Rate Meter 2")
                font.weight: Font.Bold
                font.pixelSize: AppScale.fs(AppScale.isCompact ? 24 : 30)
                color: Theme.onSurface(Material.theme)
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: qsTr("Record speech and save it on this device.")
                font.pixelSize: AppScale.fs(16)
                color: Theme.onSurfaceVariant(Material.theme)
            }

            RoundButton {
                id: recordButton
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 24
                Layout.preferredWidth: AppScale.isCompact ? 96 : 112
                Layout.preferredHeight: Layout.preferredWidth
                radius: width / 2
                hoverEnabled: true
                onClicked: root.toggleRecording()

                background: Rectangle {
                    radius: recordButton.radius
                    color: audioApi.isRecording ? Theme.error(Material.theme) : Theme.primary(Material.theme)
                    Label {
                        anchors.centerIn: parent
                        font.family: Icons.familySolid
                        font.weight: Font.Black
                        text: audioApi.isRecording ? Icons.faStop : Icons.faMicrophone
                        color: audioApi.isRecording ? Theme.onError(Material.theme) : Theme.onPrimary(Material.theme)
                        font.pixelSize: recordButton.width / 2.4
                    }
                }
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: audioApi.isRecording ? qsTr("Recording…") : qsTr("Record")
                font.pixelSize: AppScale.fs(16)
                color: Theme.onSurface(Material.theme)
            }

            ProgressBar {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 160
                from: 0
                to: 1
                value: audioApi.audioLevel
                visible: audioApi.isRecording
            }

            PlayButton {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 56
                Layout.preferredHeight: 56
                file: root.lastRecordedFile
                showLabel: true
            }

            Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                visible: root.lastRecordedFile.length > 0
                text: qsTr("Saved: %1").arg(root.lastRecordedFile)
                font.pixelSize: AppScale.fs(13)
                color: Theme.onSurfaceVariant(Material.theme)
            }
        }
    }
}
