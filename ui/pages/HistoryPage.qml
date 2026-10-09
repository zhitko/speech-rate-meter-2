import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material 6.8
import "../utils"

Page {
    id: root
    title: qsTr("History")
    padding: 0
    property string pageId: "history"

    readonly property var sessionApi: ApplicationWindow.window ? ApplicationWindow.window.sessionApi : null
    readonly property var settingsApi: ApplicationWindow.window ? ApplicationWindow.window.settingsApi : null

    property var sessions: []

    function reload() {
        sessions = sessionApi ? sessionApi.sessions() : []
    }

    Component.onCompleted: reload()
    onVisibleChanged: if (visible)
        reload()

    Connections {
        target: sessionApi
        function onSessionsChanged() { root.reload() }
    }
    Connections {
        target: settingsApi
        function onUserDataCleared() { root.reload() }
    }

    function sessionLine(session) {
        return qsTr("Articulation %1 · Fillers %2 · Pauses %3 · Speech %4")
                .arg(Number(session.articulationRate).toFixed(0) + " " + qsTr("wpm"))
                .arg(Number(session.fillerPercent).toFixed(0) + " %")
                .arg(Number(session.phrasePauses).toFixed(2) + " " + qsTr("sec"))
                .arg(Number(session.speechDuration).toFixed(0) + " " + qsTr("sec"))
    }

    Label {
        anchors.centerIn: parent
        width: parent.width - AppScale.pagePadding * 2
        visible: sessions.length === 0
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("No sessions yet. Start on Home and speak.")
        color: Theme.onSurfaceVariant(Material.theme)
        font.pixelSize: AppScale.fs(16)
    }

    ListView {
        id: listView
        readonly property real sideMargin: Math.max(AppScale.pagePadding, (width - 880) / 2)
        anchors.fill: parent
        visible: sessions.length > 0
        model: sessions
        clip: true
        spacing: AppScale.listSpacing
        topMargin: AppScale.pagePadding
        bottomMargin: AppScale.pagePadding
        leftMargin: sideMargin
        rightMargin: sideMargin

        delegate: ItemDelegate {
            id: sessionDelegate
            readonly property color zoneColor: Theme.zoneColor(
                Theme.zoneForValue(Number(modelData.speechRate),
                                   settingsApi ? settingsApi.slowWpm : 70,
                                   settingsApi ? settingsApi.fastWpm : 210),
                Material.theme)
            width: ListView.view.width - listView.sideMargin * 2
            implicitHeight: Math.max(textColumn.implicitHeight, rateColumn.implicitHeight) + 28
            leftPadding: 14
            rightPadding: 14

            Accessible.name: qsTr("Open session %1, speech rate %2 %3")
                    .arg(modelData.title)
                    .arg(Number(modelData.speechRate).toFixed(0))
                    .arg(qsTr("wpm"))

            contentItem: RowLayout {
                spacing: 14

                Column {
                    id: rateColumn
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 4

                    Rectangle {
                        id: rateBadge
                        anchors.horizontalCenter: parent.horizontalCenter
                        implicitWidth: AppScale.isCompact ? 56 : 64
                        implicitHeight: implicitWidth
                        radius: width / 2
                        color: Qt.alpha(sessionDelegate.zoneColor, 0.16)

                        Column {
                            anchors.centerIn: parent
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: Number(modelData.speechRate).toFixed(0)
                                font.pixelSize: AppScale.fs(AppScale.isCompact ? 17 : 19)
                                font.weight: Font.Bold
                                color: sessionDelegate.zoneColor
                            }
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: qsTr("wpm")
                                font.pixelSize: AppScale.fs(10)
                                color: sessionDelegate.zoneColor
                            }
                        }
                    }

                    Text {
                        width: rateBadge.width + 8
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        text: qsTr("Speech rate")
                        font.pixelSize: AppScale.fs(10)
                        color: Theme.onSurfaceVariant(Material.theme)
                        Accessible.ignored: true
                    }
                }

                Column {
                    id: textColumn
                    Layout.fillWidth: true
                    spacing: 2
                    Label {
                        width: parent.width
                        text: modelData.title
                        elide: Text.ElideRight
                        font.pixelSize: AppScale.fs(16)
                        font.weight: Font.DemiBold
                        color: Theme.onSurface(Material.theme)
                    }
                    Label {
                        width: parent.width
                        text: qsTr("%1 · %2")
                            .arg(qsTr("%n phrases", "", modelData.phraseCount))
                            .arg(qsTr("numbers changed %n times", "", modelData.updateCount))
                        elide: Text.ElideRight
                        font.pixelSize: AppScale.fs(14)
                        color: Theme.onSurface(Material.theme)
                    }
                    Label {
                        width: parent.width
                        text: root.sessionLine(modelData)
                        wrapMode: Text.Wrap
                        font.pixelSize: AppScale.fs(13)
                        color: Theme.onSurfaceVariant(Material.theme)
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignVCenter
                    text: Icons.faChevronRight
                    font.family: Icons.familySolid
                    font.weight: Font.Black
                    font.pixelSize: AppScale.fs(14)
                    color: Theme.onSurfaceVariant(Material.theme)
                }
            }

            background: Rectangle {
                radius: Theme.shapeLarge
                color: sessionDelegate.down ? Theme.surfaceContainerHighest(Material.theme)
                     : (sessionDelegate.hovered ? Theme.surfaceContainerHigh(Material.theme)
                                                : Theme.surfaceContainerLow(Material.theme))
                border.width: sessionDelegate.visualFocus ? 2 : 0
                border.color: Theme.primary(Material.theme)
            }

            onClicked: {
                var view = root.StackView.view
                if (view)
                    view.push("SessionPage.qml", { sessionId: modelData.id })
            }
        }
    }
}
