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

    component FieldLabel: RowLayout {
        id: fieldLabel
        property alias text: label.text
        property string help: ""
        property string formula: ""
        readonly property bool twoColumns: parent && parent.columns > 1
        Layout.preferredWidth: twoColumns ? Math.max(140, parent.width * 0.35) : -1
        Layout.fillWidth: !twoColumns
        Layout.alignment: Qt.AlignVCenter
        spacing: 4

        Label {
            id: label
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Theme.onSurface(Material.theme)
        }
        ToolButton {
            visible: fieldLabel.help.length > 0
            Layout.alignment: Qt.AlignVCenter
            font.family: Icons.familySolid
            font.pixelSize: AppScale.fs(16)
            text: Icons.faCircleInfo
            Accessible.name: qsTr("Help: %1").arg(label.text)
            onClicked: helpDialog.openFor(label.text, fieldLabel.help, fieldLabel.formula)
        }
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

    // ComboBox resets currentIndex to 0 when its model is replaced. qsTr() in a
    // JS model array rebuilds that model on retranslate, so without an explicit
    // re-sync the closed label jumps to the first option (e.g. Gauge "Both" →
    // "Темп речи") even though the stored setting is unchanged.
    component SettingComboBox: ComboBox {
        id: combo
        // Not named currentValue: ComboBox already has that as a final property.
        property var selectedValue
        property string valueKey: "id"

        textRole: "name"
        Layout.fillWidth: true
        displayText: {
            // Depend on language so the closed label updates after retranslate.
            void (root.settingsApi ? root.settingsApi.language : "")
            if (currentIndex < 0 || !model || currentIndex >= model.length)
                return ""
            return model[currentIndex].name
        }

        function syncFromValue() {
            if (!model || model.length === 0)
                return
            const key = valueKey
            const v = selectedValue
            let next = 0
            for (let i = 0; i < model.length; ++i) {
                if (model[i][key] === v) {
                    next = i
                    break
                }
            }
            if (currentIndex !== next)
                currentIndex = next
        }

        Component.onCompleted: syncFromValue()
        onSelectedValueChanged: syncFromValue()
        onCountChanged: Qt.callLater(syncFromValue)

        Connections {
            target: root.settingsApi
            function onLanguageChanged() { Qt.callLater(combo.syncFromValue) }
        }
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

    SettingHelpDialog {
        id: helpDialog
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
                        SettingComboBox {
                            selectedValue: settingsApi ? settingsApi.language : "en"
                            model: [
                                { name: qsTr("English"), id: "en" },
                                { name: qsTr("Russian"), id: "ru" }
                            ]
                            onActivated: if (settingsApi)
                                settingsApi.language = model[index].id
                        }

                        FieldLabel { text: qsTr("Theme") }
                        SettingComboBox {
                            selectedValue: settingsApi ? settingsApi.theme : "system"
                            model: [
                                { name: qsTr("Light"), id: "light" },
                                { name: qsTr("Dark"), id: "dark" },
                                { name: qsTr("System"), id: "system" }
                            ]
                            onActivated: if (settingsApi)
                                settingsApi.theme = model[index].id
                        }

                        FieldLabel { text: qsTr("Primary Color") }
                        SettingComboBox {
                            selectedValue: settingsApi ? settingsApi.primaryColor : "blue"
                            model: [
                                { name: qsTr("Blue"), id: "blue" },
                                { name: qsTr("Green"), id: "green" },
                                { name: qsTr("Purple"), id: "purple" },
                                { name: qsTr("Orange"), id: "orange" },
                                { name: qsTr("Red"), id: "red" }
                            ]
                            onActivated: if (settingsApi)
                                settingsApi.primaryColor = model[index].id
                        }

                        FieldLabel { text: qsTr("Show Navigation Menu") }
                        Switch {
                            checked: settingsApi ? settingsApi.showNavigationMenu : false
                            onToggled: if (settingsApi)
                                settingsApi.showNavigationMenu = checked
                        }
                        Hint {
                            text: qsTr("Home, History, and Settings along the bottom.")
                        }

                        FieldLabel { text: qsTr("Font Size") }
                        SettingComboBox {
                            valueKey: "value"
                            selectedValue: {
                                if (!settingsApi)
                                    return 1.0
                                const val = settingsApi.fontSizeMultiplier
                                if (Math.abs(val - 1.3) < 0.1)
                                    return 1.3
                                if (Math.abs(val - 1.6) < 0.1)
                                    return 1.6
                                return 1.0
                            }
                            model: [
                                { name: qsTr("Normal"), value: 1.0 },
                                { name: qsTr("Large"), value: 1.3 },
                                { name: qsTr("Extra large"), value: 1.6 }
                            ]
                            onActivated: if (settingsApi)
                                settingsApi.fontSizeMultiplier = model[index].value
                        }

                        FieldLabel {
                            text: qsTr("Mode")
                            help: qsTr("Standard restores the shipped settings: a long analysis window and few updates, so each number covers more speech and moves less often. Fast restores the fast preset: a short window and frequent updates, so the number follows you sooner and jumps more. Language, theme, color, font size, the navigation bar, Keep recording files, and speech-detection thresholds stay as they are. Choosing a mode again restores it.")
                        }
                        SettingComboBox {
                            selectedValue: settingsApi ? settingsApi.preset : 0
                            model: [
                                { name: qsTr("Standard (rare updates, accurate result)"), id: 0 },
                                { name: qsTr("Fast (frequent updates, lower accuracy)"), id: 1 }
                            ]
                            onActivated: if (settingsApi)
                                settingsApi.preset = model[index].id
                        }
                        Hint {
                            text: qsTr("Standard updates rarely and stays steadier. Fast updates often and jumps more.")
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

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: qsTr("Additional")
                            font.bold: true
                            font.pixelSize: AppScale.fs(20)
                            color: Theme.primary(Material.theme)
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
                        text: qsTr("Measurement, detection, and formula coefficients. Turns off when you leave.")
                        color: Theme.onSurfaceVariant(Material.theme)
                        font.pixelSize: AppScale.fs(12)
                    }
                }
            }

            Frame {
                visible: settingsApi && settingsApi.advanced
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

                        FieldLabel {
                            text: qsTr("Analysis window (s)")
                            help: qsTr("Length of the recent speech used for the live numbers on Home, and for each point on the Open File tempo chart. While recording, the app joins the kept phrases of this session and the phrase in progress, keeps only the last N seconds, and runs the whole analysis on that buffer. Open File measures each chart point on the last N seconds of the file ending at that point, shorter near the start of the file. The vowel count, speaking span and pauses all come from this buffer, so a shorter window reacts faster but has fewer vowels and noisier numbers. At least 3 s, default 10 s. No upper limit.")
                            formula: "buffer = last N seconds of (kept phrases + phrase in progress)"
                        }
                        SpinBox {
                            from: 3
                            to: 2147483647
                            editable: true
                            value: settingsApi ? settingsApi.analysisWindowSec : 10
                            onValueModified: if (settingsApi)
                                settingsApi.analysisWindowSec = value
                            Layout.fillWidth: true
                        }
                        Hint {
                            text: qsTr("Recent speech used on Home and for Open File. Shorter reacts faster.")
                        }

                        FieldLabel {
                            text: qsTr("Updates per minute")
                            help: qsTr("How often the live numbers are recalculated while you speak, and how often Open File places a tempo-chart point. The interval is 60000 divided by this value, in milliseconds. Fewer updates make the tempo curve less jittery. Changing it changes only the refresh rate, not the values. If a new buffer is ready before the previous analysis finishes, the older run is dropped. With Detect speech automatically on, new snapshots start only on speech frames. At least 1 per minute (once a minute), default 60 (once a second). No upper limit. Above 60000 the wait stays 1 ms. Gauge median still smooths the last few readings, and the chart keeps one point from each update inside its window (default 300 s). Open File uses this same interval and also keeps a point at the end of the file.")
                            formula: "interval (ms) = 60000 / Updates per minute\nwait is at least 1 ms"
                        }
                        SpinBox {
                            from: 1
                            to: 2147483647
                            editable: true
                            value: settingsApi ? settingsApi.updatesPerMinute : 60
                            onValueModified: if (settingsApi)
                                settingsApi.updatesPerMinute = value
                            Layout.fillWidth: true
                        }
                        Hint {
                            text: qsTr("How often Home and Open File update. Fewer updates jitter less.")
                        }

                        FieldLabel {
                            text: qsTr("Chart window (s)")
                            help: qsTr("How many seconds of tempo the chart on Home keeps. Each live Home update adds a point. With Use all readings on, that point is the median of every reading since Start. With it off, the point is the gauge median, and the tip rests at zero when the arcs would. Older points slide off the left. Home shows this chart together with the arcs while recording, after Stop, and after Open File. Range 30 to 1800 s, default 300 s (5 min).")
                        }
                        SpinBox {
                            from: 30
                            to: 1800
                            stepSize: 30
                            editable: true
                            value: settingsApi ? settingsApi.tempoChartWindowSec : 300
                            onValueModified: if (settingsApi)
                                settingsApi.tempoChartWindowSec = value
                            Layout.fillWidth: true
                        }
                        Hint {
                            text: qsTr("How much tempo the chart keeps. Older points slide off.")
                        }

                        FieldLabel {
                            text: qsTr("Pause (s)")
                            help: qsTr("Silence that closes a phrase while Detect speech automatically is on. The phrase is saved at that pause. A phrase shorter than 1 s is dropped. A phrase longer than 15 s is cut at the next pause, or at 15 s. Stored as General/autoStopSilenceDuration in milliseconds, default 2 s.")
                        }
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
                            text: qsTr("Silence that ends and saves the phrase. Auto-detect only.")
                        }

                        FieldLabel {
                            text: qsTr("Phrasal pause (ms)")
                            help: qsTr("Shortest silent, unvoiced gap that counts as a pause. A frame is a pause frame when its loudness is below the silence level (25 dB under the 98th percentile of the intensity curve, with a noise-floor limit) and it is not voiced (autocorrelation below the voicing threshold). Consecutive pause frames form one pause only if they last at least this long, so short stop closures are not counted. Used by Direct and Window articulation, and by the Pauses value in those modes. Legacy does not use it. Range 50 to 2000 ms, default 200 ms.")
                            formula: "pause = run of silent, unvoiced frames with length >= Phrasal pause\nΣTp = sum of pause lengths, Np = number of pauses\nPauses value (Direct) = ΣTp / Np"
                        }
                        SpinBox {
                            from: 50
                            to: 2000
                            stepSize: 10
                            editable: true
                            value: settingsApi ? settingsApi.phrasalPauseMs : 200
                            onValueModified: if (settingsApi)
                                settingsApi.phrasalPauseMs = value
                            Layout.fillWidth: true
                        }
                        Hint {
                            text: qsTr("Silent, unvoiced gaps at least this long count as pauses.")
                        }

                        FieldLabel {
                            text: qsTr("Speech rate method")
                            help: qsTr("Which speech-rate formula is shown. Both are always computed, and the Whole recording card shows both. Direct (default) divides by the span from the first to the last speech frame, so silence before the first word and after the last one is not counted, while pauses between words are. Legacy divides by the whole buffer, including leading and trailing silence.")
                            formula: "Direct: Rs = K1 × Nv × 60 / T_span\nLegacy: Rs = K1 × Nv × 60 / T_s\nNv = vowel nuclei, T_s = buffer length in seconds"
                        }
                        SettingComboBox {
                            selectedValue: settingsApi ? settingsApi.speechRateMethod : 0
                            model: [
                                { name: qsTr("Direct"), id: 0 },
                                { name: qsTr("Legacy"), id: 1 }
                            ]
                            onActivated: if (settingsApi)
                                settingsApi.speechRateMethod = model[index].id
                        }
                        Hint {
                            text: qsTr("Direct leaves out silence before the first sound and after the last. Legacy uses the whole buffer.")
                        }

                        FieldLabel {
                            text: qsTr("Articulation method")
                            help: qsTr("Which articulation-rate formula is shown. All three are always computed, and the Whole recording card shows them. Direct (default) removes the measured pause time from the span between the first and last speech frame, so the denominator is speaking time only. Window divides by the buffer being measured, after removing those same pauses: while recording that buffer is the analysis window, or the shorter audio collected so far; a saved phrase or an opened file uses that recording's length. Silence before the first sound and after the last stays in that time. It is not a pause, and it is not removed, unlike Direct. Window uses Window scale (K5). Legacy estimates speaking time from the vowel and gap lengths and uses Articulation scale (K2). Direct and Legacy are never shown below the speech rate. Window can sit below it, because that end silence stays in the denominator.")
                            formula: "Direct: Ra = K1 × Nv × 60 / (T_span − ΣTp)\nWindow: Ra = K5 × Nv × 60 / (T_window − ΣTp)\nLegacy: Ra = Rs × T_s / (Tv + K2 × Tc_med × Nc)\nT_window is the analyzed buffer. Leading and trailing silence stay in it."
                        }
                        SettingComboBox {
                            selectedValue: settingsApi ? settingsApi.articulationMethod : 0
                            model: [
                                { name: qsTr("Direct"), id: 0 },
                                { name: qsTr("Legacy"), id: 1 },
                                { name: qsTr("Window"), id: 2 }
                            ]
                            onActivated: if (settingsApi)
                                settingsApi.articulationMethod = model[index].id
                        }
                        Hint {
                            text: qsTr("Window keeps silence at the ends of the buffer. Direct removes it. Legacy keeps the older estimate.")
                        }

                        FieldLabel {
                            text: qsTr("Session summary")
                            help: qsTr("How the kept phrases are combined after Stop, for speech rate, articulation, pauses, and fillers. Speech stays the total time. History uses the same choice. Changing it updates the stopped session and History. Median, the default, takes the middle phrase; an even count averages the two central phrases. Weighted mean weights each phrase by its length, so a longer phrase counts more. Weighted median does the same and then takes the middle. Mean, harmonic mean, geometric mean, trimmed mean, and quadratic mean count each phrase once. A harmonic or geometric mean is zero when any phrase is zero. A trimmed mean drops the lowest and highest quarter of the phrases, and with fewer than four phrases it is the plain mean. Quadratic mean is the root mean square, so a fast phrase pulls it up. One phrase, and an opened file, stay the same number for every choice.")
                            formula: "Weighted mean = Σ(value × duration) / Σ(duration)\nMean = Σ(value) / N\nMedian = middle; an even N averages the two central values\nWeighted median = value where the running duration reaches half; a point that lands between two phrases averages them\nHarmonic mean = N / Σ(1/value)\nGeometric mean = exp(Σ ln(value) / N)\nTrimmed mean = mean after dropping the lowest and highest floor(N/4) phrases\nQuadratic mean = sqrt(Σ(value²) / N)"
                        }
                        SettingComboBox {
                            selectedValue: settingsApi ? settingsApi.summaryMethod : PhraseSummary.defaultId
                            model: {
                                void (root.settingsApi ? root.settingsApi.language : "")
                                const items = []
                                const choices = PhraseSummary.choices
                                for (let i = 0; i < choices.length; ++i)
                                    items.push({ name: choices[i].name, id: choices[i].id })
                                return items
                            }
                            onActivated: if (settingsApi)
                                settingsApi.summaryMethod = model[index].id
                        }
                        Hint {
                            text: qsTr("How the kept phrases are combined after Stop.")
                        }

                        FieldLabel {
                            text: qsTr("Detect speech automatically")
                            help: qsTr("On: recording waits for speech. A phrase starts at the first speech frame and ends at a pause of the Pause length. Off: recording starts at once and is cut into 15-second parts, with no speech detection. The detection method and thresholds are in the Speech detection stage, shown when Additional is on.")
                        }
                        Switch {
                            checked: settingsApi ? settingsApi.autoCalibrate : false
                            onToggled: if (settingsApi)
                                settingsApi.autoCalibrate = checked
                        }
                        Hint {
                            text: qsTr("On: wait for speech; a pause ends the phrase. Off: start at once, in 15-second parts.")
                        }

                        FieldLabel {
                            text: qsTr("Gauge")
                            help: qsTr("Which pace the gauge shows. Speech rate: one arc. Articulation rate: one arc. Both: speech rate on the outer arc and articulation on the inner arc. This changes only the display; the values are the same. Each arc position is the median set by Gauge median, placed on the scale from Slow to Fast.")
                        }
                        SettingComboBox {
                            selectedValue: settingsApi ? settingsApi.gaugeMode : 0
                            model: [
                                { name: qsTr("Speech rate"), id: 0 },
                                { name: qsTr("Articulation rate"), id: 1 },
                                { name: qsTr("Both"), id: 2 }
                            ]
                            onActivated: if (settingsApi)
                                settingsApi.gaugeMode = model[index].id
                        }
                        Hint {
                            text: qsTr("One pace, or both: speech rate outside, articulation inside.")
                        }

                        FieldLabel {
                            text: qsTr("Show Speech rate tile")
                            help: qsTr("Shows or hides this card after Stop, after Open File, and on a saved result. Display only: the value is the same either way. The switch is disabled when the gauge already shows this pace.")
                        }
                        Switch {
                            enabled: !!settingsApi && settingsApi.gaugeMode === 1
                            checked: settingsApi ? settingsApi.showSpeechRateTile : true
                            onToggled: if (settingsApi)
                                settingsApi.showSpeechRateTile = checked
                        }
                        FieldLabel {
                            text: qsTr("Show Articulation rate tile")
                            help: qsTr("Shows or hides this card after Stop, after Open File, and on a saved result. Display only: the value is the same either way. The switch is disabled when the gauge already shows this pace.")
                        }
                        Switch {
                            enabled: !!settingsApi && settingsApi.gaugeMode === 0
                            checked: settingsApi ? settingsApi.showArticulationRateTile : true
                            onToggled: if (settingsApi)
                                settingsApi.showArticulationRateTile = checked
                        }
                        Hint {
                            text: qsTr("Unavailable while on the gauge.")
                        }

                        FieldLabel {
                            text: qsTr("Show Fillers tile")
                            help: qsTr("Shows or hides this card after Stop, after Open File, and on a saved result. Display only: the value is the same either way.")
                        }
                        Switch {
                            checked: settingsApi ? settingsApi.showFillersTile : false
                            onToggled: if (settingsApi)
                                settingsApi.showFillersTile = checked
                        }
                        FieldLabel {
                            text: qsTr("Show Pauses tile")
                            help: qsTr("Shows or hides this card after Stop, after Open File, and on a saved result. Display only: the value is the same either way.")
                        }
                        Switch {
                            checked: settingsApi ? settingsApi.showPausesTile : true
                            onToggled: if (settingsApi)
                                settingsApi.showPausesTile = checked
                        }
                        FieldLabel {
                            text: qsTr("Show Speech tile")
                            help: qsTr("Shows or hides this card after Stop, after Open File, and on a saved result. Display only: the value is the same either way.")
                        }
                        Switch {
                            checked: settingsApi ? settingsApi.showSpeechTile : true
                            onToggled: if (settingsApi)
                                settingsApi.showSpeechTile = checked
                        }
                        FieldLabel {
                            text: qsTr("Show Whole recording tile")
                            help: qsTr("Shows or hides the whole-recording summary on Home after Stop and after Open File. Display only: the value is the same either way.")
                        }
                        Switch {
                            checked: settingsApi ? settingsApi.showWholeRecordingTile : true
                            onToggled: if (settingsApi)
                                settingsApi.showWholeRecordingTile = checked
                        }

                        FieldLabel {
                            text: qsTr("Slow (wpm)")
                            help: qsTr("Lower end of the gauge scale, in words per minute. The arc is split evenly from Slow to Fast, and the marker is clamped to this range. The printed number is not clamped. Also used as the lower bound for articulation. Default 70.")
                            formula: "marker = clamp(value, Slow, Fast)"
                        }
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
                            text: qsTr("Slow end of the gauge. The arc splits evenly up to Fast.")
                        }

                        FieldLabel {
                            text: qsTr("Fast (wpm)")
                            help: qsTr("Upper end of the gauge scale, in words per minute. See Slow. Also used as the upper bound for articulation. Default 210.")
                        }
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

                        FieldLabel {
                            text: qsTr("Use all readings")
                            help: qsTr("On: each tempo-chart point is the median of every live reading since Start. An odd count takes the middle value after sorting; an even count takes the mean of the two central values. Gauge median is not used for the chart, so that control is off. The arcs still use Gauge median. Off: each chart point is the Gauge median, and the live tip rests at zero when the arcs would. Speech rate and articulation keep separate readings, and Start clears them. Default on.")
                            formula: "chart point = median(every live reading since Start)"
                        }
                        Switch {
                            checked: settingsApi ? settingsApi.chartUseAllReadings : true
                            onToggled: if (settingsApi)
                                settingsApi.chartUseAllReadings = checked
                        }
                        Hint {
                            text: qsTr("On: each chart point uses every reading since Start.")
                        }

                        FieldLabel {
                            text: qsTr("Gauge median")
                            help: qsTr("Each pace on the gauge shows the median of the last N live readings, which smooths out jitter. The tempo chart uses this median only when Use all readings is off. An odd N takes the middle value after sorting; an even N takes the mean of the two middle values. 1 shows the current reading. Speech rate and articulation keep separate windows, and Start clears them. This control is off while Use all readings is on; the arcs still use the stored count. After Stop, the gauge shows the Session summary of the kept phrases, not this median. Range 1 to 30, default 3.")
                            formula: "gauge value = median(last N live readings)"
                        }
                        SpinBox {
                            enabled: !!settingsApi && !settingsApi.chartUseAllReadings
                            from: 1
                            to: 30
                            editable: true
                            value: settingsApi ? settingsApi.gaugeAverageCount : 3
                            onValueModified: if (settingsApi)
                                settingsApi.gaugeAverageCount = value
                            Layout.fillWidth: true
                        }
                        Hint {
                            text: settingsApi && settingsApi.chartUseAllReadings
                                  ? qsTr("Unavailable while Use all readings is on. The arcs still use this count.")
                                  : qsTr("Middle of this many latest readings. 1 is the current value.")
                        }

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
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: qsTr("These change the Home numbers, in processing order.")
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
                    text: qsTr("Keep each phrase as WAV in data/records. Off deletes them on exit.")
                    color: Theme.onSurfaceVariant(Material.theme)
                    font.pixelSize: AppScale.fs(12)
                }

                StageCard {
                    stage: 1
                    title: qsTr("Speech detection")
                    description: qsTr("Phrase start and end. Auto-detect only.")
                    columns: root.singleColumnForm ? 1 : 2

                    FieldLabel {
                        text: qsTr("Detection method")
                        help: qsTr("Which detector decides when speech starts and stops. Used only with Detect speech automatically. Energy: short-term change of amplitude (V). Autocorrelation: voicing, how strongly a frame repeats at a pitch period (U). Hybrid: speech only when both detectors agree.")
                        formula: "Energy: speech when V > Energy threshold\nAutocorrelation: speech when U > Autocorrelation threshold\nHybrid: both conditions at once"
                    }
                    SettingComboBox {
                        selectedValue: settingsApi ? settingsApi.vadMethod : 0
                        model: [
                            { name: qsTr("Energy"), id: 0 },
                            { name: qsTr("Autocorrelation"), id: 1 },
                            { name: qsTr("Hybrid"), id: 2 }
                        ]
                        onActivated: if (settingsApi)
                            settingsApi.vadMethod = model[index].id
                    }
                    Hint {
                        text: qsTr("Energy: amplitude change. Autocorrelation: voicing. Hybrid: both must agree.")
                    }

                    FieldLabel {
                        visible: root.usesEnergyVad
                        text: qsTr("Energy threshold")
                        help: qsTr("Level of the energy measure V above which a frame counts as speech. The state stays speech until V falls below 90 percent of this value, so the decision does not flicker. Calibrate sets it to 3 times the 95th percentile of V measured during the quiet pass. Default 10000.")
                        formula: "A(n) = mean of abs(sample) over 128 samples, hop 64\nU(n) = mean of A over 31 frames\nV(n) = mean of abs(A - U) over 31 frames\nspeech on: V > T; speech off: V <= 0.9 × T\nafter Calibrate: T = 3 × P95(V in silence)"
                    }
                    TextField {
                        visible: root.usesEnergyVad
                        text: settingsApi ? settingsApi.vadThreshold : ""
                        onEditingFinished: if (settingsApi)
                            settingsApi.vadThreshold = root.parseDoubleValue(text)
                        Layout.fillWidth: true
                    }
                    Hint {
                        visible: root.usesEnergyVad
                        text: qsTr("Louder than this counts as speech. Calibrate sets it from the room.")
                    }

                    FieldLabel {
                        visible: root.usesAutocorrVad
                        text: qsTr("Autocorrelation threshold")
                        help: qsTr("Voicing level U above which a frame counts as voiced. The state enters speech above this value and leaves it when U drops 0.1 below it, which keeps short dips from ending a phrase. Calibrate sets it to the average voicing of the room times the Threshold factor, clamped to 0.2 to 0.6. Default 0.3.")
                        formula: "enter speech: U > T\nleave speech: U <= T - 0.1\nafter Calibrate: T = clamp(mean U(silence) × K, 0.2, 0.6)"
                    }
                    TextField {
                        visible: root.usesAutocorrVad
                        text: settingsApi ? settingsApi.autoCorrThreshold : ""
                        onEditingFinished: if (settingsApi)
                            settingsApi.autoCorrThreshold = root.parseDoubleValue(text)
                        Layout.fillWidth: true
                    }
                    Hint {
                        visible: root.usesAutocorrVad
                        text: qsTr("How clearly a sound must repeat to count as voice.")
                    }
                    FieldLabel {
                        visible: root.usesAutocorrVad
                        text: qsTr("Threshold factor")
                        help: qsTr("Used only by Calibrate for the autocorrelation method. Calibrate measures the average voicing of the background noise and multiplies it by this factor to get the threshold. Changing the factor has no effect until you press Calibrate again. Default 1.0.")
                        formula: "T = clamp(mean U(silence) × K, 0.2, 0.6)"
                    }
                    TextField {
                        visible: root.usesAutocorrVad
                        text: settingsApi ? settingsApi.autoCorrThresholdK : ""
                        onEditingFinished: if (settingsApi)
                            settingsApi.autoCorrThresholdK = root.parseDoubleValue(text)
                        Layout.fillWidth: true
                    }
                    Hint {
                        visible: root.usesAutocorrVad
                        text: qsTr("Calibrate sets the threshold to the room average times this.")
                    }
                    FieldLabel {
                        visible: root.usesAutocorrVad
                        text: qsTr("Lowest pitch (Hz)")
                        help: qsTr("Lowest pitch the autocorrelation searches. Pitch sets the lag range: a pitch period, in samples at 8000 Hz, is searched between the value for the highest and the lowest pitch. A lower value searches longer lags, so it fits deeper voices. Default 80 Hz.")
                        formula: "lag range = [round(8000 / Highest pitch), round(8000 / Lowest pitch)] samples"
                    }
                    TextField {
                        visible: root.usesAutocorrVad
                        text: settingsApi ? settingsApi.autoCorrMinF0 : ""
                        onEditingFinished: if (settingsApi)
                            settingsApi.autoCorrMinF0 = root.parseDoubleValue(text)
                        Layout.fillWidth: true
                    }
                    Hint {
                        visible: root.usesAutocorrVad
                        text: qsTr("Lowest pitch to look for.")
                    }
                    FieldLabel {
                        visible: root.usesAutocorrVad
                        text: qsTr("Highest pitch (Hz)")
                        help: qsTr("Highest pitch the autocorrelation searches. It sets the shortest lag. Default 300 Hz.")
                    }
                    TextField {
                        visible: root.usesAutocorrVad
                        text: settingsApi ? settingsApi.autoCorrMaxF0 : ""
                        onEditingFinished: if (settingsApi)
                            settingsApi.autoCorrMaxF0 = root.parseDoubleValue(text)
                        Layout.fillWidth: true
                    }
                    Hint {
                        visible: root.usesAutocorrVad
                        text: qsTr("Highest pitch to look for.")
                    }

                    Button {
                        Layout.columnSpan: root.singleColumnForm ? 1 : 2
                        Layout.fillWidth: true
                        text: qsTr("Calibrate")
                        enabled: sessionApi && !sessionApi.sessionActive && !sessionApi.busy
                        onClicked: vadCalibrationDialog.open()
                    }
                    Hint {
                        text: qsTr("Measures the room. Stay quiet until it closes.")
                    }
                }

                StageCard {
                    stage: 2
                    title: qsTr("Intensity")
                    description: qsTr("Loudness curve and a smoothed copy. Samples are at 8000 Hz.")
                    columns: root.singleColumnForm ? 1 : 2

                    FieldLabel {
                        text: qsTr("Frame")
                        help: qsTr("Length of each loudness window, in audio samples at 8000 Hz (64 samples is 8 ms). Each intensity value is the mean absolute amplitude of this many samples. A longer window is smoother and blurs short sounds.")
                        formula: "I(j) = sum of abs(x) over the window / Frame"
                    }
                    SpinBox {
                        from: 0
                        to: 1024
                        value: settingsApi ? settingsApi.intensityFrame : 64
                        onValueModified: if (settingsApi)
                            settingsApi.intensityFrame = value
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("Length of each loudness window.") }
                    FieldLabel {
                        text: qsTr("Shift")
                        help: qsTr("Step between neighbouring loudness windows, in samples at 8000 Hz. At 32 samples one intensity point is 4 ms. Every duration in the statistics is counted in these points, so this value sets the time resolution. Smaller is finer and slower.")
                        formula: "point time (s) = Shift / 8000\nduration (s) = points × Shift / 8000"
                    }
                    SpinBox {
                        from: 0
                        to: 512
                        value: settingsApi ? settingsApi.intensityShift : 32
                        onValueModified: if (settingsApi)
                            settingsApi.intensityShift = value
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("Step to the next window. Smaller is finer.") }
                    FieldLabel {
                        text: qsTr("Smooth window")
                        help: qsTr("Number of loudness points averaged into the smoothed curve S. Vowels are the parts that rise above S. Default 64 points, which is 256 ms at the default shift. A longer window follows only slow changes in loudness, so it reacts more slowly.")
                        formula: "S(i) = mean of I_norm over about Smooth points around i\nvowel sample: I_norm(i) - S(i) > 0.009"
                    }
                    SpinBox {
                        from: 0
                        to: 1024
                        value: settingsApi ? settingsApi.intensitySmooth : 64
                        onValueModified: if (settingsApi)
                            settingsApi.intensitySmooth = value
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("Points averaged. Longer reacts more slowly.") }
                }

                StageCard {
                    stage: 3
                    title: qsTr("Vowel detection")
                    description: qsTr("Peaks above the smoothed loudness. Shorter ones are dropped.")
                    columns: root.singleColumnForm ? 1 : 2

                    FieldLabel {
                        text: qsTr("Shortest vowel (ms)")
                        help: qsTr("Vowel peaks shorter than this are ignored. Duration is the stored run, in steps of Shift/8000 (4 ms at the default shift). A peak is kept when that duration is at least this many milliseconds. 0 ms also keeps a single intensity point. The default 8 ms drops that point and a 4 ms run, and keeps a run of 8 ms or longer. Each kept peak must also reach a speech gate: its raw intensity must be at least max(80, 3 × the quiet floor), where the floor is the 10th percentile of the buffer. The same limit is used while recording and in the whole-recording summary.")
                        formula: "step (ms) = Shift / 8000 × 1000\nkeep a peak when stored length × step >= Shortest vowel\n0 ms keeps a stored length of 0"
                    }
                    SpinBox {
                        from: 0
                        to: 2000
                        stepSize: 15
                        editable: true
                        value: settingsApi ? settingsApi.segmentMinLengthMs : 8
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

                    FieldLabel {
                        text: qsTr("Long-sound weight")
                        help: qsTr("Exponent of the power mean used for the average vowel and gap lengths. 1 gives the plain average, where every sound counts equally. Higher values give long sounds more weight. Default 3. It feeds the Filler score (vowel lengths) and the Legacy pause score (gap lengths) only. Speech rate, Direct articulation, and Window articulation do not use it.")
                        formula: "M_d(L) = (mean(L_k^d))^(1/d), then truncated to seconds"
                    }
                    SpinBox {
                        from: 1
                        to: 20
                        value: settingsApi ? settingsApi.meanValueDegry : 3
                        onValueModified: if (settingsApi)
                            settingsApi.meanValueDegry = value
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("Higher makes long sounds count more. 1 treats them equally.") }
                }

                StageCard {
                    stage: 5
                    title: qsTr("Metrics")
                    description: qsTr("Formula coefficients for the Home numbers.")
                    columns: root.singleColumnForm ? 1 : 2

                    SubHeader { text: qsTr("Speech rate"); Layout.columnSpan: root.singleColumnForm ? 1 : 2 }
                    FieldLabel {
                        text: qsTr("Speech-rate scale")
                        help: qsTr("Converts vowels per minute into words per minute. It is a fixed calibration constant, not measured at runtime. Raising it raises the speech rate and Direct articulation in the same proportion. Used by both speech-rate methods and by Direct articulation. Window articulation uses K5 instead. Default 0.41.")
                        formula: "Rs = K1 × Nv × 60 / T"
                    }
                    DecimalSpinBox {
                        value: settingsApi ? Math.round(settingsApi.k1 * 100) : 41
                        onRealValueEdited: function(v) { if (settingsApi) settingsApi.k1 = v }
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("Higher raises the speech-rate number.") }

                    SubHeader { text: qsTr("Articulation"); Layout.columnSpan: root.singleColumnForm ? 1 : 2 }
                    FieldLabel {
                        text: qsTr("Articulation scale")
                        help: qsTr("Used only by the Legacy articulation method. It scales the gap term: consonants and silence between vowels are weighted by K2, so a higher value lowers the articulation rate. Ignored by Direct and Window. Default 1.2.")
                        formula: "Ra_legacy = Rs × T_s / (Tv + K2 × Tc_med × Nc)"
                    }
                    DecimalSpinBox {
                        value: settingsApi ? Math.round(settingsApi.k2 * 100) : 120
                        onRealValueEdited: function(v) { if (settingsApi) settingsApi.k2 = v }
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("Legacy articulation only. Higher lowers it.") }

                    FieldLabel {
                        text: qsTr("Window scale")
                        help: qsTr("Scales the Window articulation rate. It converts vowels per minute into words per minute for that method, the same role K1 has for Direct. Default 0.71. Raising it raises only the Window rate. K4 stays the filler scale.")
                        formula: "Ra_window = K5 × Nv × 60 / (T_window − ΣTp)"
                    }
                    DecimalSpinBox {
                        value: settingsApi ? Math.round(settingsApi.k5 * 100) : 71
                        onRealValueEdited: function(v) { if (settingsApi) settingsApi.k5 = v }
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("Window articulation only. Higher raises it.") }

                    SubHeader { text: qsTr("Pauses"); Layout.columnSpan: root.singleColumnForm ? 1 : 2 }
                    FieldLabel {
                        text: qsTr("Pause scale")
                        help: qsTr("Used only by the Legacy pause score. The score is the excess of the power-mean gap length over the median gap length, scaled by this value. A higher value raises the Pauses number in Legacy mode. Direct and Window report the measured mean pause and ignore this value. Default 0.30.")
                        formula: "P = K3 × (Tc_mean − Tc_med) / Tc_med, or 0 when Tc_mean < Tc_med"
                    }
                    DecimalSpinBox {
                        value: settingsApi ? Math.round(settingsApi.k3 * 100) : 30
                        onRealValueEdited: function(v) { if (settingsApi) settingsApi.k3 = v }
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("Legacy pause score only. Higher raises it.") }

                    SubHeader { text: qsTr("Fillers"); Layout.columnSpan: root.singleColumnForm ? 1 : 2 }
                    FieldLabel {
                        text: qsTr("Filler scale")
                        help: qsTr("Scales the filler score. The score measures how much longer the average vowel is than the typical (median) vowel, so drawn-out sounds push it up. A higher value raises the score for the same vowels. The score is then turned into a percent between the Low and High filler scores. Default 100.")
                        formula: "F = K4 × (Tv_mean − Tv_med) / Tv_med, or 0 when Tv_mean < Tv_med"
                    }
                    DecimalSpinBox {
                        value: settingsApi ? Math.round(settingsApi.k4 * 100) : 10000
                        onRealValueEdited: function(v) { if (settingsApi) settingsApi.k4 = v }
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("Higher raises the filler score.") }
                    FieldLabel {
                        text: qsTr("Low filler score")
                        help: qsTr("Filler score at or below this value shows 0 percent. Default 120.")
                        formula: "F_c = clamp(F, Low, High)\npercent = (F_c − Low) / (High − Low) × 100"
                    }
                    SpinBox {
                        from: 0
                        to: 10000
                        value: settingsApi ? settingsApi.fillerMin : 120
                        onValueModified: if (settingsApi)
                            settingsApi.fillerMin = value
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("A score at or below this is shown as 0%.") }
                    FieldLabel {
                        text: qsTr("High filler score")
                        help: qsTr("Filler score at or above this value shows 100 percent. Default 240.")
                    }
                    SpinBox {
                        from: 0
                        to: 10000
                        value: settingsApi ? settingsApi.fillerMax : 240
                        onValueModified: if (settingsApi)
                            settingsApi.fillerMax = value
                        Layout.fillWidth: true
                    }
                    Hint { text: qsTr("At or above this is 100%. In between fills 0–100%.") }
                }
            }
        }
    }
}
