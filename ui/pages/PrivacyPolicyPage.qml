pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material 6.8
import by.intoncore.file 1.0
import "../utils"

Page {
    id: root
    title: qsTr("Privacy Policy")
    property string pageId: "privacy"

    readonly property var settingsApi: ApplicationWindow.window ? ApplicationWindow.window.settingsApi : null

    property string policyText: ""

    FileApi {
        id: fileApi
    }

    function loadPolicy() {
        const lang = (settingsApi && settingsApi.language === "ru") ? "ru" : "en";
        const relativePath = lang === "ru"
                ? "../../docs/privacy_policy_ru.md"
                : "../../docs/privacy_policy_en.md";
        const text = fileApi.readTextResource(Qt.resolvedUrl(relativePath));
        policyText = text.length > 0
                ? text
                : qsTr("Unable to load the privacy policy.");
    }

    Material.theme: ApplicationWindow.window ? ApplicationWindow.window.theme : Material.Light

    Component.onCompleted: loadPolicy()

    onSettingsApiChanged: if (settingsApi)
        loadPolicy()

    Connections {
        target: root.settingsApi
        function onLanguageChanged() {
            root.loadPolicy();
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
            id: policyBody
            width: scrollView.availableWidth
            padding: AppScale.pagePadding
            text: root.policyText
            textFormat: Text.MarkdownText
            wrapMode: Text.Wrap
            color: Theme.onSurface(Material.theme)
            linkColor: Theme.primary(Material.theme)
            font.pixelSize: AppScale.fs(14)
            lineHeight: 1.35
        }
    }
}
