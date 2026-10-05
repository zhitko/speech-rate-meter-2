pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material 6.8
import by.intoncore.file 1.0
import "../utils"

Page {
    id: root
    title: qsTr("User Guide")

    readonly property var settingsApi: ApplicationWindow.window ? ApplicationWindow.window.settingsApi : null

    property string guideText: ""

    FileApi {
        id: fileApi
    }

    function loadGuide() {
        const lang = (settingsApi && settingsApi.language === "ru") ? "ru" : "en";
        const relativePath = lang === "ru"
                ? "../../docs/user_guide_ru.md"
                : "../../docs/user_guide_en.md";
        const text = fileApi.readTextResource(Qt.resolvedUrl(relativePath));
        guideText = text.length > 0
                ? text
                : qsTr("Unable to load the user guide.");
    }

    Material.theme: ApplicationWindow.window ? ApplicationWindow.window.theme : Material.Light

    Component.onCompleted: loadGuide()

    onSettingsApiChanged: if (settingsApi)
        loadGuide()

    Connections {
        target: root.settingsApi
        function onLanguageChanged() {
            root.loadGuide();
        }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        ScrollBar.vertical.policy: (ApplicationWindow.window && ApplicationWindow.window.settingsApi
                                    && !ApplicationWindow.window.settingsApi.showNavigationMenu)
                                   ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        Text {
            id: guideBody
            width: scrollView.availableWidth
            padding: AppScale.pagePadding
            text: root.guideText
            textFormat: Text.MarkdownText
            wrapMode: Text.Wrap
            color: Theme.onSurface(Material.theme)
            linkColor: Theme.primary(Material.theme)
            font.pixelSize: AppScale.fs(14)
            lineHeight: 1.35
        }
    }
}
