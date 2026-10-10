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

    readonly property bool active: !!sessionApi && sessionApi.sessionActive
    readonly property bool hasResult: !!sessionApi && sessionApi.hasResult
    readonly property bool showingMean: !!sessionApi && sessionApi.showingMean && !active
    readonly property real slowWpm: settingsApi ? settingsApi.slowWpm : 70
    readonly property real fastWpm: settingsApi ? settingsApi.fastWpm : 210
    // 0 speech rate, 1 articulation rate, 2 both on the gauge.
    readonly property int gaugeMode: settingsApi ? settingsApi.gaugeMode : 0
    // A pace on the gauge is not repeated as a tile. Fillers stays off until chosen.
    readonly property bool showSpeechRateTile: gaugeMode === 1
                                                && (settingsApi ? settingsApi.showSpeechRateTile : true)
    readonly property bool showArticulationRateTile: gaugeMode === 0
                                                      && (settingsApi ? settingsApi.showArticulationRateTile : true)
    readonly property bool showFillersTile: settingsApi ? settingsApi.showFillersTile : false
    readonly property bool showPausesTile: settingsApi ? settingsApi.showPausesTile : true
    readonly property bool showSpeechTile: settingsApi ? settingsApi.showSpeechTile : true
    readonly property bool showWholeRecordingTile: settingsApi ? settingsApi.showWholeRecordingTile : true
    readonly property bool gaugeAtRest: !sessionApi || (active && phase() !== SessionApi.Measuring)

    // With the navigation bar visible the page is shorter. Size the column from the
    // full width so a scrollbar that we are about to remove cannot change the layout.
    readonly property bool fitNavigation: !!settingsApi && settingsApi.showNavigationMenu
    readonly property real layoutInnerWidth: fitNavigation ? scrollView.width
                                                           : scrollView.availableWidth - scrollView.effectiveScrollBarWidth
    readonly property real layoutWidth: Math.min(layoutInnerWidth - AppScale.pagePadding * 2, 1120)
    readonly property bool wideLayout: layoutInnerWidth >= 720
                                       || (layoutInnerWidth >= 560 && layoutInnerWidth > height * 1.15)
    readonly property real viewportHeight: Math.max(0, height - footer.height)
    // Baseline from before the arcs grew. The gauge aims for 1.5× this and
    // stops at the space above the pinned button, so a short phone does not
    // push the arc ends off the screen.
    readonly property real legacyGaugeHeight: wideLayout
                                              ? Math.max(170, Math.min(height - AppScale.pagePadding * 2 - 110, 440))
                                              : Math.max(200, Math.min(layoutWidth * 0.62, height * 0.36, 330))
    // The phase chip is a corner overlay. This is how far the gauge rises into
    // that band so the arc can use the space instead of sitting below it.
    readonly property real chipBand: 40
    // Card margins (16 + 16), column spacing (8), hint bottom margin (4), and the
    // gauge top margin (28 minus the chip band the arc now fills).
    readonly property real gaugeSurround: 72 - chipBand + (gaugeHint.implicitHeight > 0 ? gaugeHint.implicitHeight : 0)
    // Below this the arc ends collide with the center label.
    readonly property real gaugeFloor: 200
    readonly property real gaugeRoom: viewportHeight - AppScale.pagePadding * 2 - 4 - gaugeSurround
    // The legacy caps were measured with the arc below the chip. gaugeRoom
    // already includes the reclaimed band, so it is not added a second time.
    readonly property real naturalGaugeHeight: Math.min(legacyGaugeHeight * 1.5 + chipBand,
                                                         Math.max(legacyGaugeHeight + chipBand, gaugeRoom))
    // Phone column width is known up front. Wide cards are not: the tiles can
    // take more than their share, so that height is measured from the real width.
    function gaugeContentHeight(gaugeWidth) {
        var labelSpace = AppScale.fs(12) + 10
        var byWidth = Math.max(20, (Math.max(0, gaugeWidth) - 4) / 2.15)
        var content = byWidth * 1.66 + labelSpace + 4
        return Math.min(fittedGaugeHeight, Math.max(gaugeFloor, content))
    }
    readonly property real phoneGaugeHeight: gaugeContentHeight(layoutWidth - 32
                                                                + 2 * (AppScale.pagePadding + 10))
    // Wider windows give the gauge more of the row, up to 74%, and leave the
    // tiles about 300 px so their labels still fit.
    readonly property real gaugeWeight: {
        if (!wideLayout)
            return 1
        var share = (layoutWidth - 300) / Math.max(1, layoutWidth)
        return Math.min(0.74, Math.max(6 / 11, share))
    }
    readonly property real fittedGaugeHeight: {
        if (!fitNavigation)
            return naturalGaugeHeight
        // A few spare pixels keep rounding from bringing the scrollbar back.
        var budget = viewportHeight - AppScale.pagePadding * 2 - 8
        var room = wideLayout
                ? budget - gaugeSurround
                : budget - sideColumn.implicitHeight - layout.rowSpacing - gaugeSurround
        var fitted = Math.min(naturalGaugeHeight, room)
        return Math.max(Math.min(gaugeFloor, naturalGaugeHeight), fitted)
    }
    readonly property bool navigationLayoutFits: fitNavigation
            && layout.implicitHeight + AppScale.pagePadding * 2 <= viewportHeight + 1

    function clock(totalSeconds) {
        var seconds = Math.max(0, Math.floor(totalSeconds))
        var minutes = Math.floor(seconds / 60)
        var remain = seconds % 60
        return (minutes < 10 ? "0" : "") + minutes + ":" + (remain < 10 ? "0" : "") + remain
    }

    function phase() {
        return sessionApi ? sessionApi.phase : SessionApi.IdleEmpty
    }

    function phaseTitle() {
        if (showingMean)
            return qsTr("Mean values")
        switch (phase()) {
        case SessionApi.Listening:
            return qsTr("Listening")
        case SessionApi.TooShort:
            return qsTr("Too short")
        case SessionApi.Measuring:
            return qsTr("Measuring")
        case SessionApi.Dropped:
            return qsTr("Too short to save")
        case SessionApi.MicDenied:
            return qsTr("Microphone blocked")
        default:
            return qsTr("Ready")
        }
    }

    function phaseColor() {
        if (showingMean)
            return Theme.primary(Material.theme)
        switch (phase()) {
        case SessionApi.Listening:
        case SessionApi.Measuring:
            return Theme.primary(Material.theme)
        case SessionApi.TooShort:
            return Theme.tertiary(Material.theme)
        case SessionApi.MicDenied:
            return Theme.error(Material.theme)
        default:
            return Theme.onSurfaceVariant(Material.theme)
        }
    }

    function hintText() {
        switch (phase()) {
        case SessionApi.IdleReady:
            return showingMean
                   ? qsTr("Averages for the whole session. Speech is the total time. Press Start to measure again.")
                   : qsTr("Result for this recording. Press Start to measure again.")
        case SessionApi.Listening:
            return qsTr("Silence is not counted. Speak when you are ready.")
        case SessionApi.Measuring:
            return qsTr("The numbers follow your last %n second(s) of speech.", "",
                        settingsApi ? settingsApi.analysisWindowSec : 10)
        case SessionApi.TooShort:
            return qsTr("Keep speaking. There is not enough speech to measure yet.")
        case SessionApi.Dropped:
            return qsTr("That phrase was too short and was not saved.")
        case SessionApi.MicDenied:
            return qsTr("The microphone is blocked. Allow access in the system settings, then press Start again.")
        default:
            return qsTr("Press Start and speak naturally. The numbers follow your recent speech.")
        }
    }

    function fillerFraction() {
        if (!sessionApi || !settingsApi)
            return 0
        var minValue = settingsApi.fillerMin
        var maxValue = settingsApi.fillerMax
        if (!(maxValue > minValue))
            return 0
        var clamped = Math.min(maxValue, Math.max(minValue, sessionApi.fillerScore))
        return (clamped - minValue) / (maxValue - minValue)
    }

    FileDialog {
        id: wavDialog
        title: qsTr("Open File")
        nameFilters: [qsTr("WAV files (*.wav)")]
        fileMode: FileDialog.OpenFile
        onAccepted: if (sessionApi && !sessionApi.busy)
            sessionApi.openWavFile(selectedFile)
    }

    VadCalibrationDialog {
        id: startCalibrationDialog
        onCalibrationComplete: {
            if (sessionApi && !sessionApi.sessionActive && !sessionApi.busy)
                sessionApi.startSession()
        }
    }

    DetailsDialog {
        id: detailsDialog
        details: sessionApi ? sessionApi.details : ({})
    }

    function toggleRecording() {
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

    function openRecording() {
        if (!sessionApi || sessionApi.busy || sessionApi.sessionActive || !sessionApi.openFileAvailable)
            return
        wavDialog.currentFolder = sessionApi.testsFolderUrl()
        wavDialog.open()
    }

    Component {
        id: recordComponent

        RowLayout {
            id: recordRow
            spacing: AppScale.isCompact ? 12 : 20
            readonly property bool showOpenFile: !!sessionApi && sessionApi.openFileAvailable && !root.active

            ColumnLayout {
                spacing: 4
                Layout.alignment: Qt.AlignTop

                Item {
                    id: recordControl
                    readonly property real buttonSize: root.wideLayout ? (AppScale.isShort ? 80 : 96) : (AppScale.isShort || AppScale.isCompact ? 68 : 80)
                    readonly property real level: root.active && sessionApi
                                                  ? Math.max(0, Math.min(1, sessionApi.audioLevel)) : 0
                    readonly property color tone: root.active ? Theme.error(Material.theme)
                                                              : Theme.primary(Material.theme)
                    Layout.alignment: Qt.AlignHCenter
                    implicitWidth: buttonSize * 1.5
                    implicitHeight: buttonSize * 1.25

                    Rectangle {
                        anchors.centerIn: parent
                        width: recordControl.buttonSize
                        height: width
                        radius: width / 2
                        color: recordControl.tone
                        opacity: root.active ? 0.22 : 0
                        scale: 1 + recordControl.level * 0.45
                        Accessible.role: Accessible.ProgressBar
                        Accessible.name: qsTr("Microphone level")
                        Accessible.description: qsTr("%1 percent").arg(Math.round(100 * recordControl.level))
                        Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
                        Behavior on opacity { NumberAnimation { duration: 200 } }
                    }

                    Rectangle {
                        id: pulseRing
                        anchors.centerIn: parent
                        width: recordControl.buttonSize
                        height: width
                        radius: width / 2
                        color: "transparent"
                        border.width: 2
                        border.color: recordControl.tone
                        opacity: 0
                        visible: root.active

                        ParallelAnimation {
                            running: pulseRing.visible
                            loops: Animation.Infinite
                            NumberAnimation { target: pulseRing; property: "scale"; from: 1; to: 1.4; duration: 1600; easing.type: Easing.OutCubic }
                            NumberAnimation { target: pulseRing; property: "opacity"; from: 0.5; to: 0; duration: 1600; easing.type: Easing.OutCubic }
                        }
                    }

                    AbstractButton {
                        id: recordButton
                        anchors.centerIn: parent
                        width: recordControl.buttonSize
                        height: width
                        enabled: sessionApi && !sessionApi.busy
                        hoverEnabled: true
                        focusPolicy: Qt.StrongFocus
                        Accessible.role: Accessible.Button
                        Accessible.name: root.active ? qsTr("Stop recording") : qsTr("Start recording")
                        onClicked: root.toggleRecording()
                        scale: pressed ? 0.94 : 1
                        Behavior on scale { NumberAnimation { duration: 120 } }

                        background: Rectangle {
                            radius: width / 2
                            color: !recordButton.enabled
                                   ? Theme.surfaceContainerHighest(Material.theme)
                                   : (recordButton.hovered ? Qt.lighter(recordControl.tone, 1.08) : recordControl.tone)
                            border.width: recordButton.visualFocus ? 3 : 0
                            border.color: Theme.onSurface(Material.theme)
                            Behavior on color { ColorAnimation { duration: 200 } }
                        }

                        contentItem: Text {
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.family: Icons.familySolid
                            font.weight: Font.Black
                            text: root.active ? Icons.faStop : Icons.faMicrophone
                            font.pixelSize: recordControl.buttonSize / 2.8
                            color: !recordButton.enabled
                                   ? Theme.onSurfaceVariant(Material.theme)
                                   : (root.active ? Theme.onError(Material.theme) : Theme.onPrimary(Material.theme))
                        }
                    }
                }

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: root.active ? qsTr("Stop") : qsTr("Start")
                    font.pixelSize: AppScale.fs(root.wideLayout ? 16 : 14)
                    font.weight: Font.DemiBold
                    color: Theme.onSurface(Material.theme)
                }
            }

            ColumnLayout {
                id: openFileColumn
                readonly property real labelCap: Math.max(recordControl.buttonSize * 1.6, 108)
                visible: recordRow.showOpenFile
                spacing: 4
                Layout.alignment: Qt.AlignTop
                Layout.preferredWidth: Math.max(recordControl.buttonSize * 0.72,
                                                Math.min(openFileLabel.implicitWidth, labelCap))
                Layout.maximumWidth: Math.max(recordControl.buttonSize * 0.72, labelCap)

                Item {
                    readonly property real buttonSize: recordControl.buttonSize * 0.72
                    Layout.alignment: Qt.AlignHCenter
                    implicitWidth: buttonSize
                    implicitHeight: recordControl.implicitHeight

                    AbstractButton {
                        id: openFileButton
                        anchors.centerIn: parent
                        width: parent.buttonSize
                        height: width
                        enabled: sessionApi && !sessionApi.busy
                        hoverEnabled: true
                        focusPolicy: Qt.StrongFocus
                        Accessible.role: Accessible.Button
                        Accessible.name: qsTr("Open File")
                        onClicked: root.openRecording()
                        scale: pressed ? 0.94 : 1
                        Behavior on scale { NumberAnimation { duration: 120 } }

                        background: Rectangle {
                            radius: width / 2
                            color: !openFileButton.enabled
                                   ? Theme.surfaceContainerHighest(Material.theme)
                                   : (openFileButton.hovered
                                      ? Qt.lighter(Theme.secondaryContainer(Material.theme), 1.06)
                                      : Theme.secondaryContainer(Material.theme))
                            border.width: openFileButton.visualFocus ? 3 : 0
                            border.color: Theme.onSurface(Material.theme)
                            Behavior on color { ColorAnimation { duration: 200 } }
                        }

                        contentItem: Text {
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.family: Icons.familySolid
                            font.weight: Font.Black
                            text: Icons.faFolderOpen
                            font.pixelSize: openFileButton.width / 2.6
                            color: !openFileButton.enabled
                                   ? Theme.onSurfaceVariant(Material.theme)
                                   : Theme.onSecondaryContainer(Material.theme)
                        }
                    }
                }

                Label {
                    id: openFileLabel
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: qsTr("Open File")
                    font.pixelSize: AppScale.fs(root.wideLayout ? 16 : 14)
                    font.weight: Font.DemiBold
                    color: Theme.onSurface(Material.theme)
                }
            }
        }
    }

    ScrollView {
        id: scrollView
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: root.wideLayout ? parent.bottom : footer.top
        contentWidth: availableWidth
        contentHeight: content.height
        clip: true
        ScrollBar.vertical.policy: root.fitNavigation
                                   ? (root.navigationLayoutFits ? ScrollBar.AlwaysOff : ScrollBar.AsNeeded)
                                   : ScrollBar.AlwaysOn
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        Item {
            id: content
            width: scrollView.availableWidth
            height: Math.max(root.viewportHeight, layout.implicitHeight + AppScale.pagePadding * 2)

            GridLayout {
                id: layout
                width: root.layoutWidth
                x: (parent.width - scrollView.effectiveScrollBarWidth - width) / 2
                anchors.verticalCenter: parent.verticalCenter
                columns: root.wideLayout ? 2 : 1
                columnSpacing: AppScale.pageSpacing
                rowSpacing: AppScale.pageSpacing

                Rectangle {
                    id: gaugeCard
                    Layout.fillWidth: true
                    Layout.preferredWidth: root.wideLayout ? root.gaugeWeight : 1
                    Layout.fillHeight: root.wideLayout
                    Layout.alignment: Qt.AlignTop
                    implicitHeight: gaugeColumn.implicitHeight + 32
                    radius: Theme.shapeExtraLarge
                    color: Theme.surfaceContainerLow(Material.theme)

                    RowLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 14
                        z: 1
                        spacing: 8

                        Rectangle {
                            implicitWidth: phaseRow.implicitWidth + 24
                            implicitHeight: 32
                            radius: height / 2
                            color: Qt.alpha(root.phaseColor(), root.active ? 0.14 : 0.08)

                            Row {
                                id: phaseRow
                                anchors.centerIn: parent
                                spacing: 8

                                Rectangle {
                                    id: phaseDot
                                    visible: root.active
                                    width: 8
                                    height: 8
                                    radius: 4
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: Theme.error(Material.theme)

                                    SequentialAnimation on opacity {
                                        running: phaseDot.visible
                                        loops: Animation.Infinite
                                        alwaysRunToEnd: true
                                        NumberAnimation { to: 0.25; duration: 700; easing.type: Easing.InOutSine }
                                        NumberAnimation { to: 1; duration: 700; easing.type: Easing.InOutSine }
                                    }
                                }

                                Text {
                                    text: root.phaseTitle()
                                    font.pixelSize: AppScale.fs(13)
                                    font.weight: Font.DemiBold
                                    color: root.phaseColor()
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Text {
                            visible: root.active
                            text: root.clock(sessionApi ? sessionApi.phraseSeconds : 0)
                            font.pixelSize: AppScale.fs(16)
                            font.weight: Font.DemiBold
                            font.features: { "tnum": 1 }
                            color: Theme.onSurface(Material.theme)
                            Accessible.name: qsTr("Phrase time %1").arg(text)
                        }
                    }

                    ColumnLayout {
                        id: gaugeColumn
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 0

                        // The slot is the gauge's vertical budget. On a wide window
                        // that budget is often taller than the circle, so the arc and
                        // the hint are centered in it instead of sitting on the floor.
                        Item {
                            id: gaugeSlot
                            Layout.fillWidth: true
                            Layout.fillHeight: root.wideLayout
                            Layout.topMargin: 28 - root.chipBand
                            readonly property real hintBlock: 8 + gaugeHint.implicitHeight + 4
                            Layout.preferredHeight: (root.wideLayout ? root.fittedGaugeHeight
                                                                      : root.phoneGaugeHeight)
                                                     + hintBlock
                            Layout.minimumHeight: Layout.preferredHeight
                            Layout.maximumHeight: root.wideLayout ? 10000 : Layout.preferredHeight

                            SpeechRateGauge {
                                id: gauge
                                readonly property real sideBleed: root.wideLayout ? 8
                                                                     : AppScale.pagePadding + 10
                                x: -sideBleed
                                width: parent.width + sideBleed * 2
                                height: root.gaugeContentHeight(width)
                                // Center the arc and the hint together. A phone slot is
                                // already the circle's height, so this stays at the top.
                                y: {
                                    if (!root.wideLayout)
                                        return 0
                                    var block = height + parent.hintBlock
                                    return Math.max(0, (parent.height - block) / 2)
                                }
                                // While recording, anything but Measuring means the user is not
                                // speaking (or not enough yet), so the needle rests at zero.
                                hasValue: root.hasResult || root.active
                                showSecond: root.gaugeMode === 2
                                // Both draws articulation on the outer arc (value, top number)
                                // and speech rate on the inner arc. A single arc uses the pace
                                // the Gauge setting names.
                                metricLabel: root.gaugeMode === 0 ? qsTr("Speech rate") : qsTr("Articulation")
                                secondMetricLabel: qsTr("Speech rate")
                                value: root.gaugeAtRest ? 0
                                       : (root.gaugeMode === 0 ? sessionApi.speechRate : sessionApi.gaugeArticulationRate)
                                secondValue: root.gaugeAtRest ? 0 : sessionApi.speechRate
                                minimum: root.slowWpm
                                maximum: root.fastWpm
                                cardColor: gaugeCard.color
                                micActive: root.active
                                micLevel: sessionApi ? sessionApi.audioLevel : 0
                            }

                            Label {
                                id: gaugeHint
                                width: parent.width
                                y: gauge.y + gauge.height + 8
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.Wrap
                                text: root.hintText()
                                font.pixelSize: AppScale.fs(14)
                                color: root.phase() === SessionApi.MicDenied
                                       ? Theme.error(Material.theme)
                                       : Theme.onSurfaceVariant(Material.theme)
                            }
                        }
                    }
                }

                ColumnLayout {
                    id: sideColumn
                    Layout.fillWidth: true
                    Layout.preferredWidth: root.wideLayout ? (1 - root.gaugeWeight) : 1
                    Layout.alignment: Qt.AlignVCenter
                    Layout.fillHeight: false
                    spacing: AppScale.pageSpacing

                    Rectangle {
                        Layout.fillWidth: true
                        visible: sessionApi && (sessionApi.busy || sessionApi.errorMessage.length > 0)
                        implicitHeight: bannerRow.implicitHeight + 24
                        radius: Theme.shapeLarge
                        color: sessionApi && sessionApi.errorMessage.length > 0
                               ? Theme.errorContainer(Material.theme)
                               : Theme.secondaryContainer(Material.theme)

                        RowLayout {
                            id: bannerRow
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 12

                            BusyIndicator {
                                visible: sessionApi && sessionApi.busy
                                running: visible
                                Layout.preferredWidth: 28
                                Layout.preferredHeight: 28
                                Accessible.name: qsTr("Analyzing audio")
                            }

                            Label {
                                Layout.fillWidth: true
                                wrapMode: Text.Wrap
                                text: sessionApi && sessionApi.errorMessage.length > 0
                                      ? sessionApi.errorMessage
                                      : qsTr("Analyzing audio…")
                                color: sessionApi && sessionApi.errorMessage.length > 0
                                       ? Theme.onErrorContainer(Material.theme)
                                       : Theme.onSecondaryContainer(Material.theme)
                                font.pixelSize: AppScale.fs(14)
                            }
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: false
                        columns: root.wideLayout || root.layoutWidth < 680 ? 2 : 4
                        columnSpacing: AppScale.isCompact ? 8 : 12
                        rowSpacing: AppScale.isCompact ? 8 : 12

                        MetricTile {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredWidth: 1
                            visible: root.showSpeechRateTile
                            icon: Icons.faGaugeHigh
                            label: qsTr("Speech rate")
                            hint: qsTr("Overall pace, pauses included")
                            value: !root.hasResult ? "—" : sessionApi.speechRate.toFixed(0)
                            unit: qsTr("wpm")
                            accent: Theme.primary(Material.theme)
                        }
                        MetricTile {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredWidth: 1
                            visible: root.showArticulationRateTile
                            icon: Icons.faCommentDots
                            label: qsTr("Articulation")
                            hint: qsTr("Pace while speaking, gaps left out")
                            value: !root.hasResult ? "—" : sessionApi.articulationRate.toFixed(0)
                            unit: qsTr("wpm")
                            accent: Theme.primary(Material.theme)
                        }
                        MetricTile {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredWidth: 1
                            visible: root.showFillersTile
                            icon: Icons.faWaveSquare
                            label: qsTr("Fillers")
                            hint: qsTr("Drawn-out sounds, not words")
                            value: root.hasResult ? (root.fillerFraction() * 100).toFixed(0) : "—"
                            unit: "%"
                            progress: root.hasResult ? root.fillerFraction() : 0
                            accent: Theme.tertiary(Material.theme)
                        }
                        MetricTile {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredWidth: 1
                            visible: root.showPausesTile
                            icon: Icons.faPause
                            label: qsTr("Pauses")
                            hint: qsTr("Longer gaps, not all silence")
                            value: root.hasResult ? sessionApi.phrasePauses.toFixed(2) : "—"
                            unit: qsTr("sec")
                            accent: Theme.secondary(Material.theme)
                        }
                        MetricTile {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredWidth: 1
                            visible: root.showSpeechTile
                            icon: Icons.faStopwatch
                            label: qsTr("Speech")
                            hint: qsTr("Total time counted as speech")
                            value: root.hasResult ? sessionApi.speechDuration.toFixed(0) : "—"
                            unit: qsTr("sec")
                            accent: Theme.zoneColor(0, Material.theme)
                        }
                    }

                    RecordingSummaryCard {
                        Layout.fillWidth: true
                        visible: root.showWholeRecordingTile && !root.active && !!sessionApi
                                 && (sessionApi.recordingSummaryPending || sessionApi.hasRecordingSummary)
                        pending: !!sessionApi && sessionApi.recordingSummaryPending && !sessionApi.hasRecordingSummary
                        summary: sessionApi ? sessionApi.recordingSummary : ({})
                    }

                    Loader {
                        Layout.alignment: Qt.AlignHCenter
                        active: root.wideLayout
                        visible: active
                        sourceComponent: recordComponent
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        visible: settingsApi && settingsApi.advanced && root.hasResult
                        spacing: 8

                        Button {
                            text: qsTr("Details")
                            flat: true
                            enabled: sessionApi && !sessionApi.busy
                            onClicked: detailsDialog.open()
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: footer
        visible: !root.wideLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: visible ? footerLoader.implicitHeight + 12 : 0
        color: Theme.background(Material.theme)

        Rectangle {
            anchors.bottom: parent.top
            width: parent.width
            height: 16
            gradient: Gradient {
                GradientStop { position: 0; color: Qt.alpha(footer.color, 0) }
                GradientStop { position: 1; color: footer.color }
            }
        }

        Loader {
            id: footerLoader
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 8
            active: !root.wideLayout
            sourceComponent: recordComponent
        }
    }
}
