import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material 6.8
import "../components"
import "../utils"

Page {
    id: root
    title: qsTr("Settings")
    property string pageId: "settings"

    readonly property var settingsApi: ApplicationWindow.window ? ApplicationWindow.window.settingsApi : null
    readonly property var sessionApi: ApplicationWindow.window ? ApplicationWindow.window.sessionApi : null

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
        messageText: qsTr("This permanently deletes saved sessions on this device. Recorded audio is already gone.")
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
                                { name: qsTr("Normal"), value: 1.0 },
                                { name: qsTr("Large"), value: 1.3 },
                                { name: qsTr("Extra large"), value: 1.6 }
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
                        flat: true
                        Material.foreground: Theme.error(Material.theme)
                        background: Rectangle {
                            radius: 20
                            color: parent.down ? Theme.errorContainer(Material.theme) : "transparent"
                            border.width: 1
                            border.color: Theme.error(Material.theme)
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
                        text: qsTr("Phrase")
                        font.bold: true
                        font.pixelSize: AppScale.fs(20)
                        color: Theme.primary(Material.theme)
                    }

                    GridLayout {
                        columns: 2
                        columnSpacing: 20
                        rowSpacing: 10
                        Layout.fillWidth: true

                        Label { text: qsTr("Shortest phrase (s)"); color: Theme.onSurface(Material.theme) }
                        SpinBox {
                            from: 0
                            to: 600
                            editable: true
                            value: settingsApi ? settingsApi.shortestPhraseSec : 1
                            onValueModified: if (settingsApi)
                                settingsApi.shortestPhraseSec = value
                            Layout.fillWidth: true
                        }
                        Label {
                            Layout.columnSpan: 2
                            wrapMode: Text.Wrap
                            text: qsTr("Shorter speech is ignored.")
                            color: Theme.onSurfaceVariant(Material.theme)
                            font.pixelSize: AppScale.fs(12)
                        }

                        Label { text: qsTr("Longest phrase (s)"); color: Theme.onSurface(Material.theme) }
                        SpinBox {
                            from: 1
                            to: 600
                            editable: true
                            value: settingsApi ? settingsApi.longestPhraseSec : 15
                            onValueModified: if (settingsApi)
                                settingsApi.longestPhraseSec = value
                            Layout.fillWidth: true
                        }
                        Label {
                            Layout.columnSpan: 2
                            wrapMode: Text.Wrap
                            text: qsTr("A longer stretch is split even without a pause.")
                            color: Theme.onSurfaceVariant(Material.theme)
                            font.pixelSize: AppScale.fs(12)
                        }

                        Label { text: qsTr("Pause (s)"); color: Theme.onSurface(Material.theme) }
                        SpinBox {
                            from: 0
                            to: 60
                            editable: true
                            value: settingsApi ? settingsApi.pauseSec : 2
                            onValueModified: if (settingsApi)
                                settingsApi.pauseSec = value
                            Layout.fillWidth: true
                        }
                        Label {
                            Layout.columnSpan: 2
                            wrapMode: Text.Wrap
                            text: qsTr("Silence that ends a phrase.")
                            color: Theme.onSurfaceVariant(Material.theme)
                            font.pixelSize: AppScale.fs(12)
                        }

                        Label {
                            text: qsTr("Use Speech Autodetection")
                            color: Theme.onSurface(Material.theme)
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                        Switch {
                            checked: settingsApi ? settingsApi.autoCalibrate : false
                            onToggled: if (settingsApi)
                                settingsApi.autoCalibrate = checked
                        }
                        Label {
                            Layout.columnSpan: 2
                            wrapMode: Text.Wrap
                            text: qsTr("After Start, measure background noise, then listen for speech. Off measures the recording from the first sample.")
                            color: Theme.onSurfaceVariant(Material.theme)
                            font.pixelSize: AppScale.fs(12)
                        }

                        Label { text: qsTr("Slow (wpm)"); color: Theme.onSurface(Material.theme) }
                        SpinBox {
                            from: 0
                            to: 1000
                            editable: true
                            value: settingsApi ? settingsApi.slowWpm : 70
                            onValueModified: if (settingsApi)
                                settingsApi.slowWpm = value
                            Layout.fillWidth: true
                        }
                        Label {
                            Layout.columnSpan: 2
                            wrapMode: Text.Wrap
                            text: qsTr("Left end of the speech-rate gauge.")
                            color: Theme.onSurfaceVariant(Material.theme)
                            font.pixelSize: AppScale.fs(12)
                        }

                        Label { text: qsTr("Fast (wpm)"); color: Theme.onSurface(Material.theme) }
                        SpinBox {
                            from: 0
                            to: 1000
                            editable: true
                            value: settingsApi ? settingsApi.fastWpm : 210
                            onValueModified: if (settingsApi)
                                settingsApi.fastWpm = value
                            Layout.fillWidth: true
                        }
                        Label {
                            Layout.columnSpan: 2
                            wrapMode: Text.Wrap
                            text: qsTr("Right end of the speech-rate gauge.")
                            color: Theme.onSurfaceVariant(Material.theme)
                            font.pixelSize: AppScale.fs(12)
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("Advanced")
                            color: Theme.onSurface(Material.theme)
                            Layout.fillWidth: true
                        }
                        Switch {
                            checked: settingsApi ? settingsApi.advanced : false
                            onToggled: if (settingsApi)
                                settingsApi.advanced = checked
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: qsTr("Calibration, coefficients, and Open File.")
                        color: Theme.onSurfaceVariant(Material.theme)
                        font.pixelSize: AppScale.fs(12)
                    }
                }
            }

            Frame {
                Layout.fillWidth: true
                Layout.leftMargin: AppScale.pagePadding
                Layout.rightMargin: AppScale.pagePadding
                Layout.bottomMargin: AppScale.pagePadding
                visible: settingsApi && settingsApi.advanced

                background: Rectangle {
                    color: Theme.surfaceContainerLow(Material.theme)
                    radius: 16
                }

                ColumnLayout {
                    width: parent.width
                    spacing: 10

                    Label {
                        text: qsTr("Advanced")
                        font.bold: true
                        font.pixelSize: AppScale.fs(20)
                        color: Theme.primary(Material.theme)
                    }

                    GridLayout {
                        columns: 2
                        columnSpacing: 20
                        rowSpacing: 10
                        Layout.fillWidth: true

                        Label { text: qsTr("Display average"); color: Theme.onSurface(Material.theme) }
                        SpinBox {
                            from: 1
                            to: 30
                            editable: true
                            value: settingsApi ? settingsApi.metricAverageCount : 4
                            onValueModified: if (settingsApi)
                                settingsApi.metricAverageCount = value
                            Layout.fillWidth: true
                        }
                        Label {
                            Layout.columnSpan: 2
                            wrapMode: Text.Wrap
                            text: qsTr("While a phrase is open, Home averages this many recent updates.")
                            color: Theme.onSurfaceVariant(Material.theme)
                            font.pixelSize: AppScale.fs(12)
                        }

                        Label { text: qsTr("Mean value degry"); color: Theme.onSurface(Material.theme) }
                        SpinBox {
                            from: 1
                            to: 20
                            value: settingsApi ? settingsApi.meanValueDegry : 3
                            onValueModified: if (settingsApi)
                                settingsApi.meanValueDegry = value
                            Layout.fillWidth: true
                        }

                        Label { text: qsTr("K1"); color: Theme.onSurface(Material.theme) }
                        DecimalSpinBox {
                            value: settingsApi ? Math.round(settingsApi.k1 * 100) : 71
                            onRealValueEdited: function(v) { if (settingsApi) settingsApi.k1 = v }
                            Layout.fillWidth: true
                        }
                        Label { text: qsTr("K2"); color: Theme.onSurface(Material.theme) }
                        DecimalSpinBox {
                            value: settingsApi ? Math.round(settingsApi.k2 * 100) : 120
                            onRealValueEdited: function(v) { if (settingsApi) settingsApi.k2 = v }
                            Layout.fillWidth: true
                        }
                        Label { text: qsTr("K3"); color: Theme.onSurface(Material.theme) }
                        DecimalSpinBox {
                            value: settingsApi ? Math.round(settingsApi.k3 * 100) : 30
                            onRealValueEdited: function(v) { if (settingsApi) settingsApi.k3 = v }
                            Layout.fillWidth: true
                        }
                        Label { text: qsTr("K4"); color: Theme.onSurface(Material.theme) }
                        DecimalSpinBox {
                            value: settingsApi ? Math.round(settingsApi.k4 * 100) : 10000
                            onRealValueEdited: function(v) { if (settingsApi) settingsApi.k4 = v }
                            Layout.fillWidth: true
                        }
                        Label { text: qsTr("Frame"); color: Theme.onSurface(Material.theme) }
                        SpinBox {
                            from: 0
                            to: 1024
                            value: settingsApi ? settingsApi.intensityFrame : 240
                            onValueModified: if (settingsApi)
                                settingsApi.intensityFrame = value
                            Layout.fillWidth: true
                        }
                        Label { text: qsTr("Shift"); color: Theme.onSurface(Material.theme) }
                        SpinBox {
                            from: 0
                            to: 512
                            value: settingsApi ? settingsApi.intensityShift : 120
                            onValueModified: if (settingsApi)
                                settingsApi.intensityShift = value
                            Layout.fillWidth: true
                        }
                        Label { text: qsTr("Smooth Frame"); color: Theme.onSurface(Material.theme) }
                        SpinBox {
                            from: 0
                            to: 1024
                            value: settingsApi ? settingsApi.intensitySmooth : 120
                            onValueModified: if (settingsApi)
                                settingsApi.intensitySmooth = value
                            Layout.fillWidth: true
                        }
                        Label { text: qsTr("Segment length limit (millisec)"); color: Theme.onSurface(Material.theme); wrapMode: Text.Wrap }
                        SpinBox {
                            from: 0
                            to: 2000
                            value: settingsApi ? settingsApi.segmentMinLengthMs : 5
                            onValueModified: if (settingsApi)
                                settingsApi.segmentMinLengthMs = value
                            Layout.fillWidth: true
                        }
                        Label { text: qsTr("Min FS"); color: Theme.onSurface(Material.theme) }
                        SpinBox {
                            from: 0
                            to: 10000
                            value: settingsApi ? settingsApi.fillerMin : 120
                            onValueModified: if (settingsApi)
                                settingsApi.fillerMin = value
                            Layout.fillWidth: true
                        }
                        Label { text: qsTr("Max FS"); color: Theme.onSurface(Material.theme) }
                        SpinBox {
                            from: 0
                            to: 10000
                            value: settingsApi ? settingsApi.fillerMax : 240
                            onValueModified: if (settingsApi)
                                settingsApi.fillerMax = value
                            Layout.fillWidth: true
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
                        enabled: !(sessionApi && sessionApi.sessionActive)
                        onClicked: vadCalibrationDialog.open()
                    }
                }
            }
        }
    }
}
