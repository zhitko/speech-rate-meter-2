import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material 6.8
import by.intoncore.file 1.0
import "../utils"

Dialog {
    id: root
    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    title: qsTr("Open test example")
    width: Math.min(parent ? parent.width - 24 : 360, 480)
    height: Math.min(parent ? parent.height - 24 : 480, 520)
    standardButtons: Dialog.Cancel

    property string language: "ru"
    property var examples: []
    signal fileSelected(url file)

    function refresh() {
        examples = files.getFilesList("data/tests", [languages.currentIndex === 0 ? "ru_*.wav" : "en_*.wav"])
    }

    onAboutToShow: {
        languages.currentIndex = language.toLowerCase().indexOf("ru") === 0 ? 0 : 1
        refresh()
        exampleList.positionViewAtBeginning()
    }

    FileApi { id: files }

    contentItem: ColumnLayout {
        spacing: 12

        TabBar {
            id: languages
            Layout.fillWidth: true
            TabButton { text: "RU"; Accessible.name: "Русский" }
            TabButton { text: "EN"; Accessible.name: "English" }
            onCurrentIndexChanged: if (root.visible) root.refresh()
        }

        ListView {
            id: exampleList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.examples
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}

            delegate: ItemDelegate {
                required property var modelData
                width: exampleList.width
                text: modelData.fileName
                onClicked: {
                    var file = files.getUrlFromPath(files.getApplicationDirPath() + "/data/tests/" + modelData.filePath)
                    root.accept()
                    root.fileSelected(file)
                }
            }

            Label {
                anchors.centerIn: parent
                width: parent.width
                visible: exampleList.count === 0
                text: qsTr("No test examples found.")
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                color: Theme.onSurfaceVariant(Material.theme)
                font.pixelSize: AppScale.fs(14)
            }
        }
    }
}
