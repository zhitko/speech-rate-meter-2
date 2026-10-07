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
    readonly property real slowWpm: settingsApi ? settingsApi.slowWpm : 70
    readonly property real fastWpm: settingsApi ? settingsApi.fastWpm : 210

    readonly property real layoutWidth: Math.min(scrollView.availableWidth - scrollView.effectiveScrollBarWidth - AppScale.pagePadding * 2, 1120)
    readonly property bool wideLayout: scrollView.availableWidth >= 720
                                       || (scrollView.availableWidth >= 560 && scrollView.availableWidth > height * 1.15)

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
        switch (phase()) {
        case SessionApi.Listening:
            return qsTr("Listening")
        case SessionApi.TooShort:
            return qsTr("Too short")
        case SessionApi.Measuring:
            return qsTr("Measuring")
        case SessionApi.Dropped:
            return qsTr("Not saved")
        case SessionApi.MicDenied:
            return qsTr("Microphone blocked")
        default:
            return qsTr("Ready")
        }
    }

    function phaseColor() {
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
            return qsTr("Whole-session result. Press Start to measure again.")
        case SessionApi.Listening:
            return qsTr("Listening…")
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

    Component {
        id: recordComponent

        ColumnLayout {
            spacing: 4

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
        ScrollBar.vertical.policy: (root.settingsApi && !root.settingsApi.showNavigationMenu)
                                   ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        Item {
            id: content
            width: scrollView.availableWidth
            height: Math.max(root.height - footer.height, layout.implicitHeight + AppScale.pagePadding * 2)

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
                    Layout.preferredWidth: root.wideLayout ? 6 : 1
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
                        spacing: 8

                        SpeechRateGauge {
                            id: gauge
                            Layout.fillWidth: true
                            Layout.fillHeight: root.wideLayout
                            Layout.topMargin: 12
                            Layout.preferredHeight: root.wideLayout
                                                    ? Math.max(170, Math.min(root.height - AppScale.pagePadding * 2 - 110, 440))
                                                    : Math.max(200, Math.min(root.layoutWidth * 0.62, root.height * 0.36, 330))
                            Layout.minimumHeight: 170
                            Layout.maximumHeight: 480
                            // While recording, anything but Measuring means the user is not
                            // speaking (or not enough yet), so the needle rests at zero.
                            hasValue: root.hasResult || root.active
                            value: !sessionApi || (root.active && root.phase() !== SessionApi.Measuring)
                                   ? 0 : sessionApi.speechRate
                            minimum: root.slowWpm
                            maximum: root.fastWpm
                            cardColor: gaugeCard.color
                            micActive: root.active
                            micLevel: sessionApi ? sessionApi.audioLevel : 0
                        }

                        Label {
                            Layout.fillWidth: true
                            Layout.bottomMargin: 4
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

                ColumnLayout {
                    id: sideColumn
                    Layout.fillWidth: true
                    Layout.preferredWidth: root.wideLayout ? 5 : 1
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
                            icon: Icons.faCommentDots
                            label: qsTr("Articulation")
                            value: root.hasResult ? sessionApi.articulationRate.toFixed(0) : "—"
                            unit: qsTr("wpm")
                            accent: Theme.primary(Material.theme)
                        }
                        MetricTile {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredWidth: 1
                            icon: Icons.faWaveSquare
                            label: qsTr("Fillers")
                            value: root.hasResult ? (root.fillerFraction() * 100).toFixed(0) : "—"
                            unit: "%"
                            progress: root.hasResult ? root.fillerFraction() : 0
                            accent: Theme.tertiary(Material.theme)
                        }
                        MetricTile {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredWidth: 1
                            icon: Icons.faPause
                            label: qsTr("Pauses")
                            value: root.hasResult ? sessionApi.phrasePauses.toFixed(2) : "—"
                            unit: qsTr("sec")
                            accent: Theme.secondary(Material.theme)
                        }
                        MetricTile {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.preferredWidth: 1
                            icon: Icons.faStopwatch
                            label: qsTr("Speech")
                            value: root.hasResult ? sessionApi.speechDuration.toFixed(0) : "—"
                            unit: qsTr("sec")
                            accent: Theme.zoneColor(0, Material.theme)
                        }
                    }

                    Loader {
                        Layout.alignment: Qt.AlignHCenter
                        active: root.wideLayout
                        visible: active
                        sourceComponent: recordComponent
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        visible: settingsApi && settingsApi.advanced
                        spacing: 8

                        Button {
                            text: qsTr("Details")
                            flat: true
                            visible: root.hasResult
                            enabled: sessionApi && !sessionApi.busy
                            onClicked: detailsDialog.open()
                        }

                        Button {
                            text: qsTr("Open File")
                            flat: true
                            visible: sessionApi && sessionApi.openFileAvailable && !root.active
                            enabled: sessionApi && !sessionApi.busy
                            onClicked: {
                                wavDialog.currentFolder = sessionApi.testsFolderUrl()
                                wavDialog.open()
                            }
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
