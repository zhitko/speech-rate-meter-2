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
    readonly property bool singleColumnForm: width < 720 || AppScale.fontScale > 1.0

    readonly property var settingsApi: ApplicationWindow.window ? ApplicationWindow.window.settingsApi : null
    readonly property var sessionApi: ApplicationWindow.window ? ApplicationWindow.window.sessionApi : null

    readonly property bool usesEnergyVad: !settingsApi || settingsApi.vadMethod !== 1
    readonly property bool usesAutocorrVad: !settingsApi || settingsApi.vadMethod !== 0

    component StageCard: Frame {
        id: card
        property int stage: 0
        property string title: ""
        property string description: ""
        property alias columns: fields.columns
        default property alias fieldData: fields.data

        Layout.fillWidth: true
        padding: 16

        background: Rectangle {
            color: Theme.surfaceContainerLow(card.Material.theme)
            radius: 16
        }

        ColumnLayout {
            width: parent.width
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Rectangle {
                    Layout.alignment: Qt.AlignTop
                    implicitWidth: 28
                    implicitHeight: 28
                    radius: 14
                    color: Theme.primaryContainer(card.Material.theme)

                    Text {
                        anchors.centerIn: parent
                        text: card.stage
                        font.pixelSize: AppScale.fs(13)
                        font.weight: Font.Bold
                        color: Theme.onPrimaryContainer(card.Material.theme)
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        Layout.fillWidth: true
                        text: card.title
                        wrapMode: Text.Wrap
                        font.pixelSize: AppScale.fs(17)
                        font.weight: Font.DemiBold
                        color: Theme.onSurface(card.Material.theme)
                    }
                    Label {
                        Layout.fillWidth: true
                        visible: card.description.length > 0
                        text: card.description
                        wrapMode: Text.Wrap
                        font.pixelSize: AppScale.fs(12)
                        color: Theme.onSurfaceVariant(card.Material.theme)
                    }
                }
            }

            GridLayout {
                id: fields
                Layout.fillWidth: true
                columnSpacing: 20
                rowSpacing: 10
            }
        }
    }

    component FieldLabel: Label {
        readonly property bool twoColumns: parent && parent.columns > 1
        Layout.preferredWidth: twoColumns ? Math.max(140, parent.width * 0.35) : -1
        Layout.fillWidth: !twoColumns
        Layout.alignment: Qt.AlignVCenter
        wrapMode: Text.Wrap
        color: Theme.onSurface(Material.theme)
    }

    component SubHeader: Label {
        Layout.fillWidth: true
        Layout.topMargin: 4
        font.pixelSize: AppScale.fs(13)
        font.weight: Font.DemiBold
        color: Theme.primary(Material.theme)
    }

    component Hint: Label {
        Layout.columnSpan: parent && parent.columns ? parent.columns : 1
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: Theme.onSurfaceVariant(Material.theme)
        font.pixelSize: AppScale.fs(12)
    }

    function parseDoubleValue(text) {
        return parseFloat(String(text).replace(",", "."));
    }

    VadCalibrationDialog {
        id: vadCalibrationDialog
    }

    ConfirmDialog {
        id: confirmationDialog
        titleText: qsTr("Delete user data")
        messageText: qsTr("This permanently deletes saved sessions on this device and any recording files still kept in data/records.")
        confirmText: qsTr("Delete")
        cancelText: qsTr("Cancel")
        isDestructive: true
        onAccepted: if (sessionApi && !sessionApi.sessionActive && !sessionApi.busy)
            sessionApi.clearUserData()
    }

    Material.theme: ApplicationWindow.window ? ApplicationWindow.window.theme : Material.Light

    ScrollView {
        id: settingsScroll
        anchors.fill: parent
        contentWidth: availableWidth
        ScrollBar.vertical.policy: (root.settingsApi && !root.settingsApi.showNavigationMenu)
                                   ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: settingsScroll.availableWidth
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
                        columns: root.singleColumnForm ? 1 : 2
                        columnSpacing: 20
                        rowSpacing: 10
                        Layout.fillWidth: true

                        FieldLabel { text: qsTr("Language") }
                        ComboBox {
                            textRole: "name"
                            model: [
                                { name: qsTr("English"), id: "en" },
                                { name: qsTr("Russian"), id: "ru" }
                            ]
                            currentIndex: {
                                if (!settingsApi)
                                    return 0
                                for (var i = 0; i < model.length; ++i) {
                                    if (model[i].id === settingsApi.language)
                                        return i
                                }
                                return 0
                            }
                            onActivated: if (settingsApi)
                                settingsApi.language = model[index].id
                            Layout.fillWidth: true
                        }

                        FieldLabel { text: qsTr("Theme") }
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

                        FieldLabel { text: qsTr("Primary Color") }
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

                        FieldLabel { text: qsTr("Show Navigation Menu") }
                        Switch {
                            checked: settingsApi ? settingsApi.showNavigationMenu : false
                            onToggled: if (settingsApi)
                                settingsApi.showNavigationMenu = checked
                        }
                        Hint {
                            text: qsTr("Shows Home, History, and Settings along the bottom. The menu button stays in the toolbar.")
                        }

                        FieldLabel { text: qsTr("Font Size") }
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
                        enabled: sessionApi && !sessionApi.sessionActive && !sessionApi.busy
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
                        text: qsTr("Measurement")
                        font.bold: true
                        font.pixelSize: AppScale.fs(20)
                        color: Theme.primary(Material.theme)
                    }

                    GridLayout {
                        columns: root.singleColumnForm ? 1 : 2
                        columnSpacing: 20
                        rowSpacing: 10
                        Layout.fillWidth: true

                        FieldLabel { text: qsTr("Analysis window (s)") }
                        SpinBox {
                            from: 3
                            to: 30
                            editable: true
                            value: settingsApi ? settingsApi.analysisWindowSec : 10
                            onValueModified: if (settingsApi)
                                settingsApi.analysisWindowSec = value
                            Layout.fillWidth: true
                        }
                        Hint {
                            text: qsTr("While recording, Home shows the pace of this much recent speech. Shorter reacts faster but jumps more.")
                        }

                        FieldLabel { text: qsTr("Updates per minute") }
                        SpinBox {
                            from: 6
                            to: 240
                            editable: true
                            value: settingsApi ? settingsApi.updatesPerMinute : 60
                            onValueModified: if (settingsApi)
                                settingsApi.updatesPerMinute = value
                            Layout.fillWidth: true
                        }
                        Hint {
                            text: qsTr("How often the numbers on Home are recalculated while you speak.")
                        }

                        FieldLabel { text: qsTr("Pause (s)") }
                        SpinBox {
                            from: 0
                            to: 60
                            editable: true
                            value: settingsApi ? settingsApi.pauseSec : 2
                            onValueModified: if (settingsApi)
                                settingsApi.pauseSec = value
                            Layout.fillWidth: true
                        }
                        Hint {
                            text: qsTr("Silence this long ends the phrase and saves it. Used only when Detect speech automatically is on.")
                        }

                        FieldLabel { text: qsTr("Phrasal pause (ms)") }
                        SpinBox {
                            from: 50
                            to: 2000
                            stepSize: 10
                            editable: true
                            value: settingsApi ? settingsApi.phrasalPauseMs : 150
                            onValueModified: if (settingsApi)
                                settingsApi.phrasalPauseMs = value
                            Layout.fillWidth: true
                        }
                        Hint {
                            text: qsTr("Silent or unvoiced gaps at least this long count as phrasal pauses in the whole-recording summary.")
                        }

                        FieldLabel { text: qsTr("Detect speech automatically") }
                        Switch {
                            checked: settingsApi ? settingsApi.autoCalibrate : false
                            onToggled: if (settingsApi)
                                settingsApi.autoCalibrate = checked
                        }
                        Hint {
                            text: qsTr("After Start, measures background noise, then waits for you to speak. A pause ends the phrase. Off starts measuring when you press Start, a pause does not end the phrase, and the recording is saved in 15-second parts.")
                        }

                        FieldLabel { text: qsTr("Gauge") }
                        ComboBox {
                            textRole: "name"
                            model: [
                                { name: qsTr("Speech rate"), id: 0 },
                                { name: qsTr("Articulation rate"), id: 1 },
                                { name: qsTr("Both"), id: 2 }
                            ]
                            currentIndex: settingsApi ? Math.max(0, Math.min(2, settingsApi.gaugeMode)) : 0
                            onActivated: if (settingsApi)
                                settingsApi.gaugeMode = model[index].id
                            Layout.fillWidth: true
                        }
                        Hint {
                            text: qsTr("Speech rate draws that pace on the gauge and leaves articulation in the card. Articulation rate swaps them. Both draws articulation on the outer arc and speech rate on the inner arc, and leaves that card out.")
                        }

                        FieldLabel { text: qsTr("Slow (wpm)") }
                        SpinBox {
                            from: 0
                            to: 1000
                            editable: true
                            value: settingsApi ? settingsApi.slowWpm : 70
                            onValueModified: if (settingsApi)
                                settingsApi.slowWpm = value
                            Layout.fillWidth: true
                        }
                        Hint {
                            text: qsTr("The slow end of the gauge. From here to Fast, the arc is split equally into Slow, Average, and Fast.")
                        }

                        FieldLabel { text: qsTr("Fast (wpm)") }
                        SpinBox {
                            from: 0
                            to: 1000
                            editable: true
                            value: settingsApi ? settingsApi.fastWpm : 210
                            onValueModified: if (settingsApi)
                                settingsApi.fastWpm = value
                            Layout.fillWidth: true
                        }
                        Hint {
                            text: qsTr("The fast end of the gauge.")
                        }

                        FieldLabel { text: qsTr("Gauge median") }
                        SpinBox {
                            from: 1
                            to: 30
                            editable: true
                            value: settingsApi ? settingsApi.gaugeAverageCount : 3
                            onValueModified: if (settingsApi)
                                settingsApi.gaugeAverageCount = value
                            Layout.fillWidth: true
                        }
                        Hint {
                            text: qsTr("The gauge shows the middle of this many latest readings of each pace it draws, so one jump moves it less. 1 shows the current reading.")
                        }
                    }

                    CheckBox {
                        text: qsTr("Advanced")
                        checked: settingsApi ? settingsApi.advanced : false
                        onToggled: if (settingsApi)
                            settingsApi.advanced = checked
                        Layout.fillWidth: true
                    }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: qsTr("How speech is detected, how loudness is measured, and the coefficients behind the numbers. The checkbox turns off when you leave the app.")
                        color: Theme.onSurfaceVariant(Material.theme)
                        font.pixelSize: AppScale.fs(12)
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: AppScale.pagePadding
                Layout.rightMargin: AppScale.pagePadding
                Layout.bottomMargin: AppScale.pagePadding
                visible: settingsApi && settingsApi.advanced
                spacing: AppScale.pageSpacing

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 4
                    spacing: 2

                    Label {
                        text: qsTr("Advanced")
                        font.bold: true
                        font.pixelSize: AppScale.fs(20)
                        color: Theme.primary(Material.theme)
                    }
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: qsTr("These change how the numbers on Home are calculated. The groups follow the order a phrase is processed.")
                        color: Theme.onSurfaceVariant(Material.theme)
                        font.pixelSize: AppScale.fs(13)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: qsTr("Keep recording files")
                        color: Theme.onSurface(Material.theme)
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                    }
                    Switch {
                        checked: settingsApi ? settingsApi.keepRecordingFiles : false
                        onToggled: if (settingsApi)
                            settingsApi.keepRecordingFiles = checked
                    }
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    text: qsTr("Leaves each phrase as a WAV file in data/records. Off deletes that file once its numbers are saved.")
                    color: Theme.onSurfaceVariant(Material.theme)
                    font.pixelSize: AppScale.fs(12)
                }

                StageCard {
                    stage: 1
                    title: qsTr("Speech detection")
                    description: qsTr("Finds where each phrase starts and ends. Used only when Detect speech automatically is on.")
                    columns: root.singleColumnForm ? 1 : 2

                    FieldLabel { text: qsTr("Detection method") }
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
                    Hint {
                        text: qsTr("Energy uses loudness. Autocorrelation uses pitch. Hybrid requires both.")
                    }

                    FieldLabel { visible: root.usesEnergyVad; text: qsTr("Energy threshold") }
                    TextField {
                        visible: root.usesEnergyVad
                        text: settingsApi ? settingsApi.vadThreshold : ""
                        onEditingFinished: if (settingsApi)
                            settingsApi.vadThreshold = root.parseDoubleValue(text)
                        Layout.fillWidth: true
                    }
                    Hint {
                        visible: root.usesEnergyVad
                        text: qsTr("Sound louder than this counts as speech. Higher ignores more of a quiet voice. Calibrate sets it from the room.")
                    }

                    FieldLabel { visible: root.usesAutocorrVad; text: qsTr("Autocorrelation threshold") }
                    TextField {
                        visible: root.usesAutocorrVad
                        text: settingsApi ? settingsApi.autoCorrThreshold : ""
                        onEditingFinished: if (settingsApi)
                            settingsApi.autoCorrThreshold = root.parseDoubleValue(text)
                        Layout.fillWidth: true
                    }
                    Hint {
                        visible: root.usesAutocorrVad
                        text: qsTr("How clearly a sound must repeat to count as a voice. Higher ignores more noise.")
                    }
                    FieldLabel { visible: root.usesAutocorrVad; text: qsTr("Threshold factor") }
                    TextField {
                        visible: root.usesAutocorrVad
                        text: settingsApi ? settingsApi.autoCorrThresholdK : ""
                        onEditingFinished: if (settingsApi)
                            settingsApi.autoCorrThresholdK = root.parseDoubleValue(text)
                        Layout.fillWidth: true
                    }
                    Hint {
                        visible: root.usesAutocorrVad
                        text: qsTr("Used when you calibrate. The threshold becomes the room average times this number. Higher asks for a clearer voice.")
                    }
                    FieldLabel { visible: root.usesAutocorrVad; text: qsTr("Lowest pitch (Hz)") }
                    TextField {
                        visible: root.usesAutocorrVad
                        text: settingsApi ? settingsApi.autoCorrMinF0 : ""
                        onEditingFinished: if (settingsApi)
                            settingsApi.autoCorrMinF0 = root.parseDoubleValue(text)
                        Layout.fillWidth: true
                    }
                    Hint {
                        visible: root.usesAutocorrVad
                        text: qsTr("Lowest voice pitch to look for, in hertz.")
                    }
                    FieldLabel { visible: root.usesAutocorrVad; text: qsTr("Highest pitch (Hz)") }
                    TextField {
                        visible: root.usesAutocorrVad
                        text: settingsApi ? settingsApi.autoCorrMaxF0 : ""
                        onEditingFinished: if (settingsApi)
                            settingsApi.autoCorrMaxF0 = root.parseDoubleValue(text)
                        Layout.fillWidth: true
                    }
                    Hint {
                        visible: root.usesAutocorrVad
                        text: qsTr("Highest voice pitch to look for, in hertz.")
                    }

                    Button {
                        Layout.columnSpan: root.singleColumnForm ? 1 : 2
                        Layout.fillWidth: true
                        text: qsTr("Calibrate")
                        enabled: sessionApi && !sessionApi.sessionActive && !sessionApi.busy
                        onClicked: vadCalibrationDialog.open()
                    }
                    Hint {
                        text: qsTr("Measures the room and fills in the thresholds. Stay quiet until it closes.")
                    }
                }

                StageCard {
                    stage: 2
                    title: qsTr("Intensity")
                    description: qsTr("Turns the phrase into a loudness curve and a smoothed copy. Frame and shift are audio samples at 8000 Hz, so 8000 is one second. The smooth window counts points on the curve.")
                    columns: root.singleColumnForm ? 1 : 2

                    FieldLabel { text: qsTr("Frame") }
                    SpinBox {
                        from: 0
                        to: 1024
                        value: settingsApi ? settingsApi.intensityFrame : 240
                        onValueModified: if (settingsApi)
                            settingsApi.intensityFrame = value
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("Length of each loudness window.") }
                    FieldLabel { text: qsTr("Shift") }
                    SpinBox {
                        from: 0
                        to: 512
                        value: settingsApi ? settingsApi.intensityShift : 120
                        onValueModified: if (settingsApi)
                            settingsApi.intensityShift = value
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("How far the next window moves. Smaller gives a finer curve.") }
                    FieldLabel { text: qsTr("Smooth window") }
                    SpinBox {
                        from: 0
                        to: 1024
                        value: settingsApi ? settingsApi.intensitySmooth : 120
                        onValueModified: if (settingsApi)
                            settingsApi.intensitySmooth = value
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("How many points are averaged. Longer follows the loudness more slowly.") }
                }

                StageCard {
                    stage: 3
                    title: qsTr("Vowel detection")
                    description: qsTr("Vowels are where the loudness curve rises above its smoothed copy. Shorter peaks are dropped.")
                    columns: root.singleColumnForm ? 1 : 2

                    FieldLabel { text: qsTr("Shortest vowel (ms)") }
                    SpinBox {
                        from: 0
                        to: 2000
                        value: settingsApi ? settingsApi.segmentMinLengthMs : 5
                        onValueModified: if (settingsApi)
                            settingsApi.segmentMinLengthMs = value
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("Vowel peaks shorter than this are ignored.") }
                }

                StageCard {
                    stage: 4
                    title: qsTr("Statistics")
                    description: qsTr("Averages the vowel and gap durations.")
                    columns: root.singleColumnForm ? 1 : 2

                    FieldLabel { text: qsTr("Long-sound weight") }
                    SpinBox {
                        from: 1
                        to: 20
                        value: settingsApi ? settingsApi.meanValueDegry : 3
                        onValueModified: if (settingsApi)
                            settingsApi.meanValueDegry = value
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("1 treats every sound equally. A higher number makes long sounds count more.") }
                }

                StageCard {
                    stage: 5
                    title: qsTr("Metrics")
                    description: qsTr("Coefficients of the formulas that turn the statistics into the values on Home.")
                    columns: root.singleColumnForm ? 1 : 2

                    SubHeader { text: qsTr("Speech rate"); Layout.columnSpan: root.singleColumnForm ? 1 : 2 }
                    FieldLabel { text: qsTr("Speech-rate scale") }
                    DecimalSpinBox {
                        value: settingsApi ? Math.round(settingsApi.k1 * 100) : 71
                        onRealValueEdited: function(v) { if (settingsApi) settingsApi.k1 = v }
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("Higher raises the speech-rate number.") }

                    SubHeader { text: qsTr("Articulation"); Layout.columnSpan: root.singleColumnForm ? 1 : 2 }
                    FieldLabel { text: qsTr("Articulation scale") }
                    DecimalSpinBox {
                        value: settingsApi ? Math.round(settingsApi.k2 * 100) : 120
                        onRealValueEdited: function(v) { if (settingsApi) settingsApi.k2 = v }
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("Higher usually lowers articulation.") }

                    SubHeader { text: qsTr("Pauses"); Layout.columnSpan: root.singleColumnForm ? 1 : 2 }
                    FieldLabel { text: qsTr("Pause scale") }
                    DecimalSpinBox {
                        value: settingsApi ? Math.round(settingsApi.k3 * 100) : 30
                        onRealValueEdited: function(v) { if (settingsApi) settingsApi.k3 = v }
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("Higher raises the Pauses number.") }

                    SubHeader { text: qsTr("Fillers"); Layout.columnSpan: root.singleColumnForm ? 1 : 2 }
                    FieldLabel { text: qsTr("Filler scale") }
                    DecimalSpinBox {
                        value: settingsApi ? Math.round(settingsApi.k4 * 100) : 10000
                        onRealValueEdited: function(v) { if (settingsApi) settingsApi.k4 = v }
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("Higher raises the filler score before it is shown as a percent.") }
                    FieldLabel { text: qsTr("Low filler score") }
                    SpinBox {
                        from: 0
                        to: 10000
                        value: settingsApi ? settingsApi.fillerMin : 120
                        onValueModified: if (settingsApi)
                            settingsApi.fillerMin = value
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("A score at or below this is shown as 0%.") }
                    FieldLabel { text: qsTr("High filler score") }
                    SpinBox {
                        from: 0
                        to: 10000
                        value: settingsApi ? settingsApi.fillerMax : 240
                        onValueModified: if (settingsApi)
                            settingsApi.fillerMax = value
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("A score at or above this is shown as 100%. Scores between the low and high values fill in 0–100%.") }
                }
            }
        }
    }
}
