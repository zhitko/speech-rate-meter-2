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
        anchors.fill: parent
        visible: sessions.length > 0
        model: sessions
        clip: true
        spacing: 8
        topMargin: AppScale.pagePadding
        bottomMargin: AppScale.pagePadding
        leftMargin: AppScale.pagePadding
        rightMargin: AppScale.pagePadding

        delegate: ItemDelegate {
            width: ListView.view.width - AppScale.pagePadding * 2
            implicitHeight: textColumn.implicitHeight + 20

            contentItem: Column {
                id: textColumn
                spacing: 2
                Label {
                    width: parent.width
                    text: modelData.title
                    elide: Text.ElideRight
                    font.pixelSize: AppScale.fs(16)
                    font.bold: true
                    color: Theme.onSurface(Material.theme)
                }
                Label {
                    width: parent.width
                    text: qsTr("%1 · %2 · %3 wpm")
                        .arg(qsTr("%n phrases", "", modelData.phraseCount))
                        .arg(qsTr("%n updates", "", modelData.updateCount))
                        .arg(Number(modelData.speechRate).toFixed(0))
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

            background: Rectangle {
                radius: 12
                color: parent.hovered ? Theme.surfaceContainerHigh(Material.theme) : Theme.surfaceContainerLow(Material.theme)
            }

            onClicked: {
                var view = root.StackView.view
                if (view)
                    view.push("SessionPage.qml", { sessionId: modelData.id })
            }
        }
    }
}
