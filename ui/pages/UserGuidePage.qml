pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material 6.8
import by.intoncore.file 1.0
import "../utils"

Page {
    id: root
    title: qsTr("User Guide")
    property string pageId: "guide"

    readonly property var settingsApi: ApplicationWindow.window ? ApplicationWindow.window.settingsApi : null

    property string guideText: ""

    FileApi {
        id: fileApi
    }

    function loadGuide() {
        const lang = (settingsApi && settingsApi.language === "ru") ? "ru" : "en";
        const relativePath = lang === "ru"
                ? "../../docs/SpeechRateMeter2_UserGuide_ru.md"
                : "../../docs/SpeechRateMeter2_UserGuilde_en.md";
        const text = fileApi.readTextResource(Qt.resolvedUrl(relativePath));
        guideText = text.length > 0
                ? text
                : qsTr("Unable to load the user guide.");
        guideBlocks = splitBlocks(guideText);
    }

    property var guideBlocks: []

    // Markdown Text renders images at natural size and applies lineHeight to
    // them, so standalone image lines are pulled out and shown as Image items.
    function splitBlocks(markdown) {
        const blocks = [];
        const imageLine = /^\s*!\[([^\]]*)\]\(([^)\s]+)\)\s*$/;
        let buffer = [];
        const flush = function () {
            const chunk = buffer.join("\n").trim();
            if (chunk.length > 0)
                blocks.push({ kind: "text", text: chunk, source: "", alt: "" });
            buffer = [];
        };
        const lines = markdown.split(/\r?\n/);
        for (let i = 0; i < lines.length; ++i) {
            const m = lines[i].match(imageLine);
            if (m) {
                flush();
                blocks.push({ kind: "image", text: "", source: m[2], alt: m[1] });
            } else {
                buffer.push(lines[i]);
            }
        }
        flush();
        return blocks;
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

        Column {
            id: guideBody
            width: scrollView.availableWidth
            padding: AppScale.pagePadding
            spacing: AppScale.fs(8)

            readonly property real innerWidth: width - leftPadding - rightPadding

            Repeater {
                model: root.guideBlocks

                delegate: Loader {
                    id: blockLoader
                    required property var modelData
                    width: guideBody.innerWidth
                    sourceComponent: modelData.kind === "image" ? imageBlock : textBlock

                    Component {
                        id: textBlock
                        Text {
                            width: blockLoader.width
                            text: blockLoader.modelData.text
                            textFormat: Text.MarkdownText
                            baseUrl: Qt.resolvedUrl("../../docs/")
                            wrapMode: Text.Wrap
                            color: Theme.onSurface(Material.theme)
                            linkColor: Theme.primary(Material.theme)
                            font.pixelSize: AppScale.fs(14)
                            lineHeight: 1.35
                        }
                    }

                    Component {
                        id: imageBlock
                        Image {
                            width: blockLoader.width
                            height: sourceSize.width > 0
                                    ? width * sourceSize.height / sourceSize.width
                                    : 0
                            // Image links in the guide are relative to docs/.
                            source: Qt.resolvedUrl("../../docs/" + blockLoader.modelData.source)
                            fillMode: Image.PreserveAspectFit
                            asynchronous: true
                            smooth: true
                            mipmap: true
                            Accessible.name: blockLoader.modelData.alt
                        }
                    }
                }
            }
        }
    }
}
