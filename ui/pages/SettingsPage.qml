import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material 6.8
import "../components"
import "../utils"

Page {
    id: root
    title: qsTr("Settings")

    readonly property var settingsApi: ApplicationWindow.window ? ApplicationWindow.window.settingsApi : null

    function parseDoubleValue(text) {
        return parseFloat(String(text).replace(",", "."));
    }

    VadCalibrationDialog {
        id: vadCalibrationDialog
        onCalibrationDoneEnergy: function(threshold) {
            if (settingsApi)
                settingsApi.vadThreshold = threshold;
        }
        onCalibrationDoneAutocorrelation: function(threshold) {
            if (settingsApi)
                settingsApi.autoCorrThreshold = threshold;
        }
    }

    ConfirmDialog {
        id: confirmationDialog
        titleText: qsTr("Delete user data")
        messageText: qsTr("This permanently deletes saved recordings on this device.")
        confirmText: qsTr("Delete")
        cancelText: qsTr("Cancel")
        isDestructive: true
        onAccepted: if (settingsApi)
            settingsApi.clearUserData()
    }

    Material.theme: ApplicationWindow.window ? ApplicationWindow.window.theme : Material.Light

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        ScrollBar.vertical.policy: (root.settingsApi && !root.settingsApi.showNavigationMenu)
                                   ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: parent.width
            spacing: 0

            Frame {
                Layout.fillWidth: true
                Layout.margins: AppScale.pagePadding

                background: Rectangle {
                    color: Theme.surfaceContainerLow(Material.theme)
                    radius: 16
                }

                ColumnLayout {
                    width: parent.width
                    spacing: 10

                    Label {
                        text: qsTr("General")
                        font.bold: true
                        font.pixelSize: AppScale.fs(20)
                        color: Theme.primary(Material.theme)
                    }

                    GridLayout {
                        columns: 2
                        columnSpacing: 20
                        rowSpacing: 10
                        Layout.fillWidth: true

                        Label { text: qsTr("Language"); color: Theme.onSurface(Material.theme) }
                        ComboBox {
                            model: ["en", "ru"]
                            currentIndex: settingsApi ? model.indexOf(settingsApi.language) : 0
                            onActivated: if (settingsApi)
                                settingsApi.language = currentText
                            Layout.fillWidth: true
                        }

                        Label { text: qsTr("Theme"); color: Theme.onSurface(Material.theme) }
                        ComboBox {
                            textRole: "name"
                            model: [
                                { name: qsTr("Light"), id: "light" },
                                { name: qsTr("Dark"), id: "dark" },
                                { name: qsTr("System"), id: "system" }
                            ]
                            currentIndex: {
                                if (!settingsApi)
                                    return 0;
                                for (var i = 0; i < model.length; i++) {
                                    if (model[i].id === settingsApi.theme)
                                        return i;
                                }
                                return 0;
                            }
                            onActivated: if (settingsApi)
                                settingsApi.theme = model[index].id
                            Layout.fillWidth: true
                        }

                        Label { text: qsTr("Primary Color"); color: Theme.onSurface(Material.theme) }
                        ComboBox {
                            textRole: "name"
                            model: [
                                { name: qsTr("Blue"), id: "blue" },
                                { name: qsTr("Green"), id: "green" },
                                { name: qsTr("Purple"), id: "purple" },
                                { name: qsTr("Orange"), id: "orange" },
                                { name: qsTr("Red"), id: "red" }
                            ]
                            currentIndex: {
                                if (!settingsApi)
                                    return 0;
                                for (var i = 0; i < model.length; i++) {
                                    if (model[i].id === settingsApi.primaryColor)
                                        return i;
                                }
                                return 0;
                            }
                            onActivated: if (settingsApi)
                                settingsApi.primaryColor = model[index].id
                            Layout.fillWidth: true
                        }

                        Label { text: qsTr("Show Navigation Menu"); color: Theme.onSurface(Material.theme) }
                        Switch {
                            checked: settingsApi ? settingsApi.showNavigationMenu : false
                            onToggled: if (settingsApi)
                                settingsApi.showNavigationMenu = checked
                        }

                        Label { text: qsTr("Font Size"); color: Theme.onSurface(Material.theme) }
                        ComboBox {
                            textRole: "name"
                            model: [
                                { name: qsTr("Small"), value: 1.0 },
                                { name: qsTr("Normal"), value: 1.3 },
                                { name: qsTr("Big"), value: 1.6 }
                            ]
                            currentIndex: {
                                if (!settingsApi)
                                    return 0;
                                let val = settingsApi.fontSizeMultiplier;
                                if (Math.abs(val - 1.3) < 0.1)
                                    return 1;
                                if (Math.abs(val - 1.6) < 0.1)
                                    return 2;
                                return 0;
                            }
                            onActivated: if (settingsApi)
                                settingsApi.fontSizeMultiplier = model[index].value
                            Layout.fillWidth: true
                        }
                    }

                    Button {
                        text: qsTr("Delete user data")
                        Layout.fillWidth: true
                        Material.foreground: Theme.onError(Material.theme)
                        background: Rectangle {
                            color: Theme.error(Material.theme)
                            radius: 4
                        }
                        onClicked: confirmationDialog.open()
                    }
                }
            }

            Frame {
                Layout.fillWidth: true
                Layout.leftMargin: AppScale.pagePadding
                Layout.rightMargin: AppScale.pagePadding
                Layout.bottomMargin: AppScale.pagePadding

                background: Rectangle {
                    color: Theme.surfaceContainerLow(Material.theme)
                    radius: 16
                }

                ColumnLayout {
                    width: parent.width
                    spacing: 10

                    Label {
                        text: qsTr("Recording")
                        font.bold: true
                        font.pixelSize: AppScale.fs(20)
                        color: Theme.primary(Material.theme)
                    }

                    GridLayout {
                        columns: 2
                        columnSpacing: 20
                        rowSpacing: 10
                        Layout.fillWidth: true

                        Label { text: qsTr("Auto Stop Recording"); color: Theme.onSurface(Material.theme) }
                        Switch {
                            checked: settingsApi ? settingsApi.autoStopRecording : true
                            onToggled: if (settingsApi)
                                settingsApi.autoStopRecording = checked
                        }

                        Label { text: qsTr("Autocalibrate before recording"); color: Theme.onSurface(Material.theme) }
                        Switch {
                            checked: settingsApi ? settingsApi.autoCalibrate : false
                            onToggled: if (settingsApi)
                                settingsApi.autoCalibrate = checked
                        }

                        Label { text: qsTr("VAD Method"); color: Theme.onSurface(Material.theme) }
                        ComboBox {
                            textRole: "name"
                            model: [
                                { name: qsTr("Energy"), id: 0 },
                                { name: qsTr("Autocorrelation"), id: 1 },
                                { name: qsTr("Hybrid"), id: 2 }
                            ]
                            currentIndex: settingsApi ? settingsApi.vadMethod : 0
                            onActivated: if (settingsApi)
                                settingsApi.vadMethod = model[index].id
                            Layout.fillWidth: true
                        }

                        Label { text: qsTr("Silence Duration (ms)"); color: Theme.onSurface(Material.theme) }
                        TextField {
                            text: settingsApi ? settingsApi.autoStopSilenceDuration : ""
                            inputMethodHints: Qt.ImhDigitsOnly
                            onEditingFinished: if (settingsApi)
                                settingsApi.autoStopSilenceDuration = parseInt(text)
                            Layout.fillWidth: true
                        }

                        Label { text: qsTr("Calibration Duration (ms)"); color: Theme.onSurface(Material.theme) }
                        TextField {
                            text: settingsApi ? settingsApi.vadCalibrationDurationMs : ""
                            inputMethodHints: Qt.ImhDigitsOnly
                            onEditingFinished: if (settingsApi)
                                settingsApi.vadCalibrationDurationMs = parseInt(text)
                            Layout.fillWidth: true
                        }

                        Label { text: qsTr("Energy Threshold"); color: Theme.onSurface(Material.theme) }
                        TextField {
                            text: settingsApi ? settingsApi.vadThreshold : ""
                            onEditingFinished: if (settingsApi)
                                settingsApi.vadThreshold = root.parseDoubleValue(text)
                            Layout.fillWidth: true
                        }

                        Label { text: qsTr("Autocorr. Threshold"); color: Theme.onSurface(Material.theme) }
                        TextField {
                            text: settingsApi ? settingsApi.autoCorrThreshold : ""
                            onEditingFinished: if (settingsApi)
                                settingsApi.autoCorrThreshold = root.parseDoubleValue(text)
                            Layout.fillWidth: true
                        }

                        Label { text: qsTr("Autocorr. Threshold K"); color: Theme.onSurface(Material.theme) }
                        TextField {
                            text: settingsApi ? settingsApi.autoCorrThresholdK : ""
                            onEditingFinished: if (settingsApi)
                                settingsApi.autoCorrThresholdK = root.parseDoubleValue(text)
                            Layout.fillWidth: true
                        }

                        Label { text: qsTr("Autocorr Min F0 (Hz)"); color: Theme.onSurface(Material.theme) }
                        TextField {
                            text: settingsApi ? settingsApi.autoCorrMinF0 : ""
                            onEditingFinished: if (settingsApi)
                                settingsApi.autoCorrMinF0 = root.parseDoubleValue(text)
                            Layout.fillWidth: true
                        }

                        Label { text: qsTr("Autocorr Max F0 (Hz)"); color: Theme.onSurface(Material.theme) }
                        TextField {
                            text: settingsApi ? settingsApi.autoCorrMaxF0 : ""
                            onEditingFinished: if (settingsApi)
                                settingsApi.autoCorrMaxF0 = root.parseDoubleValue(text)
                            Layout.fillWidth: true
                        }
                    }

                    Button {
                        text: qsTr("Calibrate")
                        Layout.fillWidth: true
                        onClicked: vadCalibrationDialog.open()
                    }
                }
            }
        }
    }
}
