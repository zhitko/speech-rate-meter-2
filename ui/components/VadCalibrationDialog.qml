import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material 6.8
import by.intoncore.audio 1.0
import "../utils"

// VAD Calibration dialog.
// Usage:
//   VadCalibrationDialog { id: myDialog; onCalibrationComplete: { ... } }
//   myDialog.open()
//
// The dialog starts calibration automatically when opened, calibrates every
// detector the current VAD method uses, stores the thresholds in settingsApi,
// closes itself, and then emits calibrationComplete().

Dialog {
    id: root

    readonly property var appWindow: ApplicationWindow.window
    readonly property var settingsApi: appWindow ? appWindow.settingsApi : null
    readonly property var sessionApi: appWindow ? appWindow.sessionApi : null

    // Emitted when calibration finishes successfully.
    signal calibrationDoneEnergy(real threshold)
    signal calibrationDoneAutocorrelation(real threshold)

    title: qsTr("VAD Calibration")
    modal: true
    anchors.centerIn: parent
    width: 380
    closePolicy: Popup.NoAutoClose

    // 0: energy, 1: autocorrelation, 2: hybrid (both detectors are calibrated)
    readonly property int method: settingsApi ? settingsApi.vadMethod : 0
    readonly property int passes: method === 2 ? 2 : 1

    // Emitted once every detector used by the current method is calibrated.
    signal calibrationComplete()

    function calibrate() {
        if (method === 1)
            _calibrationAudioApi.calibrateVadAutocorrelation();
        else
            _calibrationAudioApi.calibrateVadEnergy();
    }

    // Internal AudioApi — callers do not need to provide one.
    AudioApi {
        id: _calibrationAudioApi
        onCalibrationFinishedEnergy: function(threshold) {
            if (root.settingsApi)
                root.settingsApi.vadThreshold = threshold;
            root.calibrationDoneEnergy(threshold);
            if (root.method === 2) {
                // Let the energy result reach the UI before the second blocking pass.
                Qt.callLater(_calibrationAudioApi.calibrateVadAutocorrelation);
                return;
            }
            root.close();
            root.calibrationComplete();
        }
        onCalibrationFinishedAutocorrelation: function(threshold, energyThreshold) {
            if (root.settingsApi) {
                root.settingsApi.autoCorrThreshold = threshold;
                root.settingsApi.autoCorrEnergyThreshold = energyThreshold;
            }
            root.calibrationDoneAutocorrelation(threshold);
            root.close();
            root.calibrationComplete();
        }
        onPermissionResultReceived: function(granted) {
            if (granted) {
                root.calibrate();
            } else {
                Logger.warning("Microphone permission denied — cannot calibrate");
                if (root.sessionApi)
                    root.sessionApi.reportMicrophoneDenied();
                root.close();
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: 16
        Label {
            text: qsTr("Please stay quiet for %1 seconds so the background noise level can be measured.").arg(root.passes * (root.settingsApi ? Math.round(root.settingsApi.vadCalibrationDurationMs / 1000) : 2))
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            font.pixelSize: AppScale.fs(15)
        }
        // Animated dots to show progress
        Row {
            Layout.alignment: Qt.AlignHCenter
            spacing: 8
            Repeater {
                model: 3
                Rectangle {
                    width: 14
                    height: 14
                    radius: 7
                    color: Theme.primary(Material.theme)
                    SequentialAnimation on opacity {
                        running: root.visible
                        loops: Animation.Infinite
                        NumberAnimation { from: 0.2; to: 1.0; duration: 400 }
                        NumberAnimation { from: 1.0; to: 0.2; duration: 400 }
                        PauseAnimation { duration: index * 200 }
                    }
                }
            }
        }
    }

    onOpened: {
        // Request microphone permission first (no-op on desktop)
        if (!_calibrationAudioApi.requestAudioPermission()) {
            // Permission request is pending — will retry calibration in
            // onPermissionResultReceived callback
            return;
        }
        root.calibrate();
    }
}
