import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Material 6.8
import by.intoncore.session 1.0
import by.intoncore.audio 1.0
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
    readonly property var tempoPoints: sessionApi && sessionApi.tempoPoints ? sessionApi.tempoPoints : []
    readonly property int tempoWindowSec: settingsApi ? settingsApi.tempoChartWindowSec : 300
    readonly property bool chartUseAllReadings: settingsApi ? settingsApi.chartUseAllReadings : true
    // Same publish cadence as the arcs: 60 / Updates per minute seconds.
    // The chart uses this as the gap that still draws one line, so one update
    // a minute stays connected across the 300 s window.
    readonly property real tempoSampleSec: 60.0 / Math.max(1, settingsApi ? settingsApi.updatesPerMinute : 60)
    // Use all readings: hold the last chart point, which is the median of every
    // reading since Start. Otherwise the tip is the gauge median, and zero at rest.
    readonly property real tempoTipSpeech: chartUseAllReadings ? lastTempoValue("speechRate")
                                            : (!sessionApi || gaugeAtRest ? 0 : sessionApi.speechRate)
    readonly property real tempoTipArticulation: chartUseAllReadings ? lastTempoValue("articulationRate")
                                                    : (!sessionApi || gaugeAtRest ? 0 : sessionApi.gaugeArticulationRate)
    readonly property bool tempoTipActive: active && (hasResult || phase() === SessionApi.Measuring)
    // 0 tempo chart, 1 arcs — what Home shows while recording.
    readonly property int recordingView: settingsApi ? settingsApi.recordingView : 0
    readonly property bool liveUsesChart: recordingView === 0
    // Session clock for the scrolling chart. tempoFreeze keeps the right edge
    // still after Stop so the line does not jump back to the last point.
    property real tempoNow: 0
    property real tempoFreeze: 0
    // Shared beat for the recording badge dot and the card outline.
    property real recordingBlink: 1
    SequentialAnimation on recordingBlink {
        running: root.active
        loops: Animation.Infinite
        alwaysRunToEnd: true
        NumberAnimation { to: 0.25; duration: 700; easing.type: Easing.InOutSine }
        NumberAnimation { to: 1; duration: 700; easing.type: Easing.InOutSine }
    }
    readonly property bool finishedTake: !active && (hasResult || tempoPoints.length > 0)
    // While recording: chart or arcs from the setting. After a take: both.
    // Idle with no result: the same view as While recording.
    readonly property bool showTempoChart: active ? liveUsesChart
                                                 : (finishedTake || liveUsesChart)
    readonly property bool showGaugeArcs: active ? !liveUsesChart
                                                : (finishedTake || !liveUsesChart)
    readonly property bool chartAndGauge: showTempoChart && showGaugeArcs
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
    function lastTempoValue(key) {
        var pts = tempoPoints
        if (!pts || pts.length === 0)
            return 0
        var value = Number(pts[pts.length - 1][key])
        return isFinite(value) ? value : 0
    }

    readonly property string fileError: {
        var opened = sessionApi && sessionApi.errorMessage ? sessionApi.errorMessage : ""
        if (opened.length > 0)
            return opened
        return listenAudio.playbackError
    }

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
    // Below this the arc ends collide with the center label. With the chart
    // above them the arcs can sit lower; the chart has taken the top of the card.
    readonly property real gaugeFloor: chartAndGauge ? 156 : 200
    // Fixed slice of the card for the chart once the arcs are back.
    readonly property real finishedChartHeight: chartAndGauge
                                                 ? Math.max(148, Math.min(210, viewportHeight * 0.28))
                                                 : 0
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
    // While recording the chart uses the space the arcs had. After Stop the
    // arcs keep what is left under the chart.
    readonly property real liveChartHeight: Math.max(150,
                                                      (wideLayout ? fittedGaugeHeight : phoneGaugeHeight) - 24)
    readonly property real pairedGaugeHeight: chartAndGauge
                                               ? Math.max(gaugeFloor,
                                                          (wideLayout ? fittedGaugeHeight : phoneGaugeHeight)
                                                          - finishedChartHeight)
                                               : 0
    // Wide windows stack the value tiles in one column. The column stays only
    // as wide as those tiles and the Start / Open File controls, and the
    // tempo chart takes the rest of the row.
    readonly property real metricColumnWidth: Math.max(280, sideColumn.implicitWidth)
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
            return PhraseSummary.title(settingsApi ? settingsApi.summaryMethod : PhraseSummary.defaultId)
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
                   ? qsTr("Summary of the kept phrases. Press Start again.")
                   : qsTr("This recording. Press Start again.")
        case SessionApi.Listening:
            return qsTr("Speak when you are ready.")
        case SessionApi.Measuring:
            return qsTr("Last %n second(s) of speech.", "",
                        settingsApi ? settingsApi.analysisWindowSec : 10)
        case SessionApi.TooShort:
            return qsTr("Keep speaking.")
        case SessionApi.Dropped:
            return qsTr("Phrase not saved.")
        case SessionApi.MicDenied:
            return qsTr("Allow the microphone, then press Start.")
        default:
            return qsTr("Press Start and speak.")
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

    AudioApi {
        id: listenAudio
    }

    // The trace is redrawn often enough that a five-minute window still
    // reads as a slow drift, not a jump when the next point arrives.
    Timer {
        interval: 32
        repeat: true
        running: root.active && !!sessionApi
        triggeredOnStart: true
        onTriggered: root.tempoNow = sessionApi.sessionElapsedSec()
    }

    onActiveChanged: {
        if (active) {
            tempoNow = 0
            tempoFreeze = 0
        } else if (sessionApi) {
            tempoFreeze = sessionApi.sessionElapsedSec()
            tempoNow = tempoFreeze
        }
    }

    onTempoPointsChanged: {
        // A new file, or a cleared take, must not keep the previous clock.
        if (!active)
            tempoFreeze = 0
    }

    Connections {
        target: settingsApi
        function onSummaryMethodChanged() {
            if (sessionApi)
                sessionApi.refreshSummary()
        }
    }

    Connections {
        target: sessionApi
        function onListenFilesChanged() {
            if (!sessionApi || sessionApi.listenFiles.length === 0)
                listenAudio.stopPlayback()
            else
                listenAudio.extendPlaylist(sessionApi.listenFiles)
        }
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
            tempoFreeze = sessionApi.sessionElapsedSec()
            tempoNow = tempoFreeze
            sessionApi.stopSession()
            return
        }
        tempoNow = 0
        tempoFreeze = 0
        listenAudio.stopPlayback()
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

        Item {
            id: recordRow
            readonly property real gap: AppScale.isCompact ? 12 : 20
            readonly property bool showListen: !!sessionApi && !root.active && sessionApi.listenFiles.length > 0
            readonly property bool showOpenFile: !!sessionApi && sessionApi.openFileAvailable && !root.active
            readonly property real listenSpan: showListen ? listenColumn.width + gap : 0
            readonly property real openSpan: showOpenFile ? openFileColumn.width + gap : 0
            readonly property real sideSpan: Math.max(listenSpan, openSpan)
            implicitWidth: centerColumn.implicitWidth + sideSpan * 2
            implicitHeight: Math.max(centerColumn.implicitHeight,
                                     showListen ? listenColumn.implicitHeight : 0,
                                     showOpenFile ? openFileColumn.implicitHeight : 0)

            ColumnLayout {
                id: listenColumn
                readonly property real labelCap: Math.max(recordControl.buttonSize * 1.6, 108)
                readonly property real columnWidth: Math.max(recordControl.buttonSize * 0.72,
                                                              Math.min(listenLabel.implicitWidth, labelCap))
                visible: recordRow.showListen
                spacing: 4
                width: columnWidth
                height: implicitHeight
                anchors.right: centerColumn.left
                anchors.rightMargin: recordRow.gap
                anchors.top: centerColumn.top

                Item {
                    readonly property real buttonSize: recordControl.buttonSize * 0.72
                    Layout.alignment: Qt.AlignHCenter
                    implicitWidth: buttonSize
                    implicitHeight: recordControl.implicitHeight

                    AbstractButton {
                        id: listenButton
                        anchors.centerIn: parent
                        width: parent.buttonSize
                        height: width
                        hoverEnabled: true
                        focusPolicy: Qt.StrongFocus
                        Accessible.role: Accessible.Button
                        Accessible.name: listenAudio.isPlaying
                                         ? qsTr("Stop playback")
                                         : (sessionApi && sessionApi.openedPlayback
                                            ? qsTr("Listen to test")
                                            : qsTr("Listen to the recording"))
                        onClicked: {
                            if (listenAudio.isPlaying)
                                listenAudio.stopPlayback()
                            else if (sessionApi)
                                listenAudio.playFiles(sessionApi.listenFiles)
                        }
                        scale: pressed ? 0.94 : 1
                        Behavior on scale { NumberAnimation { duration: 120 } }

                        background: Rectangle {
                            radius: width / 2
                            color: listenAudio.isPlaying
                                   ? (listenButton.hovered
                                      ? Qt.lighter(Theme.errorContainer(Material.theme), 1.06)
                                      : Theme.errorContainer(Material.theme))
                                   : (listenButton.hovered
                                      ? Qt.lighter(Theme.secondaryContainer(Material.theme), 1.06)
                                      : Theme.secondaryContainer(Material.theme))
                            border.width: listenButton.visualFocus ? 3 : 0
                            border.color: Theme.onSurface(Material.theme)
                            Behavior on color { ColorAnimation { duration: 200 } }
                        }

                        contentItem: Text {
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.family: Icons.familySolid
                            font.weight: Font.Black
                            text: listenAudio.isPlaying ? Icons.faStop : Icons.faPlay
                            font.pixelSize: listenButton.width / 2.6
                            color: listenAudio.isPlaying
                                   ? Theme.onErrorContainer(Material.theme)
                                   : Theme.onSecondaryContainer(Material.theme)
                        }
                    }
                }

                Label {
                    id: listenLabel
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: listenAudio.isPlaying
                          ? qsTr("Stop")
                          : (sessionApi && sessionApi.openedPlayback ? qsTr("Listen to test") : qsTr("Listen"))
                    font.pixelSize: AppScale.fs(root.wideLayout ? 16 : 14)
                    font.weight: Font.DemiBold
                    color: Theme.onSurface(Material.theme)
                }
            }

            ColumnLayout {
                id: centerColumn
                spacing: 4
                width: implicitWidth
                height: implicitHeight
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top

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
                readonly property real columnWidth: Math.max(recordControl.buttonSize * 0.72,
                                                              Math.min(openFileLabel.implicitWidth, labelCap))
                visible: recordRow.showOpenFile
                spacing: 4
                width: columnWidth
                height: implicitHeight
                anchors.left: centerColumn.right
                anchors.leftMargin: recordRow.gap
                anchors.top: centerColumn.top

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
                    Layout.preferredWidth: root.wideLayout
                                            ? Math.max(1, root.layoutWidth - root.metricColumnWidth)
                                            : 1
                    Layout.fillHeight: root.wideLayout
                    Layout.alignment: Qt.AlignTop
                    implicitHeight: gaugeColumn.implicitHeight + 32
                    radius: Theme.shapeExtraLarge
                    color: Theme.surfaceContainerLow(Material.theme)
                    border.width: root.active ? 3 : 0
                    border.color: Qt.alpha(Theme.error(Material.theme), 0.35 + 0.65 * root.recordingBlink)

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

                                Text {
                                    text: root.phaseTitle()
                                    font.pixelSize: AppScale.fs(13)
                                    font.weight: Font.DemiBold
                                    color: root.phaseColor()
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Rectangle {
                            visible: root.active
                            implicitWidth: recordingRow.implicitWidth + 24
                            implicitHeight: 32
                            radius: height / 2
                            color: Theme.error(Material.theme)
                            Accessible.role: Accessible.StaticText
                            Accessible.name: qsTr("Recording, elapsed time %1").arg(recordingClock.text)

                            Row {
                                id: recordingRow
                                anchors.centerIn: parent
                                spacing: 8

                                Rectangle {
                                    width: 10
                                    height: 10
                                    radius: 5
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: Theme.onError(Material.theme)
                                    opacity: root.recordingBlink
                                }

                                Text {
                                    id: recordingClock
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: root.clock(root.tempoNow)
                                    font.pixelSize: AppScale.fs(18)
                                    font.weight: Font.Bold
                                    font.features: { "tnum": 1 }
                                    color: Theme.onError(Material.theme)
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        id: gaugeColumn
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: root.chartAndGauge ? 8 : 0

                        TempoChart {
                            visible: root.showTempoChart
                            Layout.fillWidth: true
                            Layout.topMargin: 36
                            Layout.fillHeight: root.wideLayout && root.showTempoChart && !root.showGaugeArcs
                            Layout.preferredHeight: !root.showTempoChart ? 0
                                                   : (root.chartAndGauge ? root.finishedChartHeight : root.liveChartHeight)
                            Layout.minimumHeight: !root.showTempoChart ? 0
                                                   : (root.chartAndGauge ? root.finishedChartHeight : 140)
                            Layout.maximumHeight: !root.showTempoChart ? 0
                                                   : (root.wideLayout && !root.showGaugeArcs
                                                      ? 10000
                                                      : Layout.preferredHeight)
                            points: root.tempoPoints
                            windowSec: root.tempoWindowSec
                            sampleInterval: root.tempoSampleSec
                            minimum: root.slowWpm
                            maximum: root.fastWpm
                            gaugeMode: root.gaugeMode
                            live: root.active
                            nowSec: root.active ? root.tempoNow : root.tempoFreeze
                            tipActive: root.tempoTipActive
                            tipSpeech: root.tempoTipSpeech
                            tipArticulation: root.tempoTipArticulation
                            firstReadingSec: sessionApi ? sessionApi.firstReadingSec : 0
                            // The waiting message in the chart already says to keep speaking.
                            caption: root.showGaugeArcs || waiting ? "" : root.hintText()
                        }

                        // The slot is the gauge's vertical budget. On a wide window
                        // that budget is often taller than the circle, so the arc and
                        // the hint are centered in it instead of sitting on the floor.
                        Item {
                            id: gaugeSlot
                            visible: root.showGaugeArcs
                            Layout.fillWidth: true
                            Layout.fillHeight: root.wideLayout
                            Layout.topMargin: root.chartAndGauge ? 0 : (28 - root.chipBand)
                            readonly property real hintBlock: 8 + gaugeHint.implicitHeight + 4
                            readonly property real gaugeBudget: root.chartAndGauge
                                                                ? root.pairedGaugeHeight
                                                                : (root.wideLayout ? root.fittedGaugeHeight
                                                                                   : root.phoneGaugeHeight)
                            Layout.preferredHeight: gaugeBudget + hintBlock
                            Layout.minimumHeight: (root.wideLayout && root.chartAndGauge)
                                                   ? root.gaugeFloor + hintBlock
                                                   : Layout.preferredHeight
                            Layout.maximumHeight: root.wideLayout ? 10000 : Layout.preferredHeight

                            SpeechRateGauge {
                                id: gauge
                                readonly property real sideBleed: root.wideLayout ? 8
                                                                     : AppScale.pagePadding + 10
                                x: -sideBleed
                                width: parent.width + sideBleed * 2
                                height: {
                                    if (!root.chartAndGauge)
                                        return root.gaugeContentHeight(width)
                                    var room = Math.max(root.gaugeFloor, parent.height - parent.hintBlock)
                                    return Math.min(root.gaugeContentHeight(width), room)
                                }
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
                    Layout.fillWidth: !root.wideLayout
                    Layout.preferredWidth: root.wideLayout ? root.metricColumnWidth : 1
                    Layout.maximumWidth: root.wideLayout ? root.metricColumnWidth : -1
                    Layout.alignment: Qt.AlignVCenter
                    Layout.fillHeight: false
                    spacing: AppScale.pageSpacing

                    Rectangle {
                        Layout.fillWidth: true
                        visible: sessionApi && (sessionApi.busy || root.fileError.length > 0)
                        implicitHeight: bannerRow.implicitHeight + 24
                        radius: Theme.shapeLarge
                        color: root.fileError.length > 0
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
                                text: root.fileError.length > 0
                                      ? root.fileError
                                      : qsTr("Analyzing audio…")
                                color: root.fileError.length > 0
                                       ? Theme.onErrorContainer(Material.theme)
                                       : Theme.onSecondaryContainer(Material.theme)
                                font.pixelSize: AppScale.fs(14)
                            }
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: false
                        columns: root.wideLayout ? 1 : (root.layoutWidth < 680 ? 2 : 4)
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
                        Layout.fillWidth: true
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
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 8
            active: !root.wideLayout
            sourceComponent: recordComponent
        }
    }
}
