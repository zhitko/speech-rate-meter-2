import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material 6.8
import "../utils"

// Live tempo chart: header, zone washes, boundary lines, and pace lines
// (no fill under the stroke). The line grows from the left until the chart
// window is full, then scrolls. Y is fixed to Slow–Fast. The left side
// names the zones; the numeric ends stay on the arcs.
Item {
    id: root
    property var points: []
    property int windowSec: 300
    property real sampleInterval: 1
    property real minimum: 70
    property real maximum: 210
    property bool live: false
    property real nowSec: 0
    property bool tipActive: false
    property real tipSpeech: 0
    property real tipArticulation: 0
    // 0 speech rate, 1 articulation, 2 both. Matches the gauge.
    property int gaugeMode: 0
    property string unit: qsTr("wpm")
    property string caption: ""
    // Speech needed before the first live point, for the waiting message.
    property real firstReadingSec: 0
    readonly property bool waiting: live && (!points || points.length === 0)
    readonly property int firstReadingWholeSec: Math.max(1, Math.ceil(firstReadingSec))

    readonly property bool showSpeech: gaugeMode !== 1
    readonly property bool showArticulation: gaugeMode !== 0
    readonly property real headerSpeech: tipActive ? tipSpeech
                                                   : (lastPoint ? Number(lastPoint.speechRate) : NaN)
    readonly property real headerArticulation: tipActive ? tipArticulation
                                                         : (lastPoint ? Number(lastPoint.articulationRate) : NaN)
    readonly property bool hasHeader: tipActive || !!lastPoint
    // Pace figures stay off the chart. While recording the line moves
    // without numbers. After Stop the arc centre shows the result, which is
    // a different number from the last chart point, so the header does not
    // repeat it.
    readonly property bool showHeaderValues: false
    // Home draws the Tempo title in the card's chip row and gives the plot
    // the header's height.
    property bool compactHeader: false
    readonly property int paintTheme: Material.theme
    // Same blue / green as the slow and medium parts of the pace arc.
    readonly property color articulationColor: Theme.paceAnchor(0, paintTheme)
    readonly property color speechColor: Theme.paceAnchor(1, paintTheme)
    readonly property color lineColor: gaugeMode === 1 ? articulationColor : speechColor
    readonly property color axisColor: Theme.outlineVariant(paintTheme)
    readonly property color labelColor: Theme.onSurfaceVariant(paintTheme)
    readonly property color boundaryColor: paintTheme === Material.Dark
                                           ? Qt.rgba(0.93, 0.94, 0.96, 0.62)
                                           : Qt.rgba(0.13, 0.16, 0.22, 0.55)
    readonly property string slowLabel: qsTr("slow")
    readonly property string mediumLabel: qsTr("medium")
    readonly property string fastLabel: qsTr("fast")
    readonly property real paceLineWidth: 4
    readonly property string chartIcon: gaugeMode === 1 ? Icons.faCommentDots : Icons.faGaugeHigh
    // Left inset of the colored plot. Zone words sit in this gutter, and the
    // plot runs to the canvas edge, so overlays centered on the canvas sit
    // left of the bands.
    readonly property real zoneGutter: AppScale.fs(11) + 16

    readonly property real dataEnd: {
        var all = points
        if (!all || all.length === 0)
            return 0
        var last = Number(all[all.length - 1].seconds)
        return isFinite(last) ? last : 0
    }
    readonly property real clockEnd: {
        if (live)
            return Math.max(0, nowSec)
        if (nowSec > 0)
            return Math.max(nowSec, dataEnd)
        return dataEnd
    }
    readonly property real windowSpan: Math.max(1, windowSec)
    readonly property bool scrolling: clockEnd >= windowSpan
    readonly property real axisStart: scrolling ? clockEnd - windowSpan : 0
    readonly property real axisEnd: scrolling ? clockEnd : windowSpan
    readonly property real tipSec: tipActive ? clockEnd : -1
    readonly property var plotted: {
        var all = points
        if (!all || all.length === 0)
            return []
        var result = []
        for (var i = 0; i < all.length; ++i) {
            var point = all[i]
            if (!point)
                continue
            var seconds = Number(point.seconds)
            if (!isFinite(seconds) || seconds < root.axisStart)
                continue
            result.push(point)
        }
        return result
    }
    readonly property var lastPoint: plotted.length > 0 ? plotted[plotted.length - 1] : null

    function clampPace(value) {
        var number = Number(value)
        if (!isFinite(number))
            return number
        if (!(maximum > minimum))
            return number
        return Math.min(maximum, Math.max(minimum, number))
    }

    function paceText(value) {
        var number = Number(value)
        if (!isFinite(number))
            return ""
        return number.toFixed(0) + (unit.length > 0 ? " " + unit : "")
    }

    function formatClock(totalSeconds) {
        var seconds = Math.max(0, Math.floor(totalSeconds))
        var minutes = Math.floor(seconds / 60)
        var remain = seconds % 60
        return (minutes < 10 ? "0" : "") + minutes + ":" + (remain < 10 ? "0" : "") + remain
    }

    implicitHeight: 240

    Accessible.role: Accessible.Chart
    Accessible.name: qsTr("Tempo")
    Accessible.description: {
        if (waiting)
            return waitingTitle.text + " " + waitingBody.text
        // The figures are not painted while recording. A screen reader still
        // hears the latest pace, because the moving line is not announced.
        if (!(live && hasHeader))
            return plotted.length === 0 ? qsTr("No tempo yet") : ""
        var parts = []
        if (showArticulation)
            parts.push(qsTr("Articulation") + " " + paceText(headerArticulation))
        if (showSpeech)
            parts.push(qsTr("Speech rate") + " " + paceText(headerSpeech))
        return parts.join(" / ")
    }

    onPlottedChanged: canvas.requestPaint()
    onAxisStartChanged: canvas.requestPaint()
    onAxisEndChanged: canvas.requestPaint()
    onWindowSecChanged: canvas.requestPaint()
    onMinimumChanged: canvas.requestPaint()
    onMaximumChanged: canvas.requestPaint()
    onGaugeModeChanged: canvas.requestPaint()
    onTipActiveChanged: canvas.requestPaint()
    onTipSpeechChanged: canvas.requestPaint()
    onTipArticulationChanged: canvas.requestPaint()
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()
    onSpeechColorChanged: canvas.requestPaint()
    onArticulationColorChanged: canvas.requestPaint()
    onLineColorChanged: canvas.requestPaint()
    onAxisColorChanged: canvas.requestPaint()
    onLabelColorChanged: canvas.requestPaint()
    onBoundaryColorChanged: canvas.requestPaint()
    onSlowLabelChanged: canvas.requestPaint()
    onMediumLabelChanged: canvas.requestPaint()
    onFastLabelChanged: canvas.requestPaint()
    onPaintThemeChanged: canvas.requestPaint()

    RowLayout {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: 8
        visible: !root.compactHeader
        height: root.compactHeader ? 0 : implicitHeight

        Rectangle {
            implicitWidth: 28
            implicitHeight: 28
            radius: Theme.shapeSmall
            color: Qt.alpha(root.lineColor, 0.14)

            Text {
                anchors.centerIn: parent
                text: root.chartIcon
                font.family: Icons.familySolid
                font.weight: Font.Black
                font.pixelSize: AppScale.fs(13)
                color: root.lineColor
            }
        }

        Text {
            Layout.fillWidth: true
            text: qsTr("Tempo")
            elide: Text.ElideRight
            color: Theme.onSurface(Material.theme)
            font.pixelSize: AppScale.fs(16)
            font.weight: Font.DemiBold
        }

        Row {
            id: headerValues
            spacing: 6
            visible: root.showHeaderValues
            Layout.maximumWidth: Math.max(120, header.width * 0.68)

            Text {
                visible: root.showArticulation && root.showHeaderValues
                text: qsTr("Articulation") + " " + root.paceText(root.headerArticulation)
                color: root.articulationColor
                font.pixelSize: AppScale.fs(15)
                font.weight: Font.DemiBold
                font.features: { "tnum": 1 }
            }

            Text {
                visible: root.showArticulation && root.showSpeech && root.showHeaderValues
                text: "/"
                color: Theme.onSurfaceVariant(Material.theme)
                font.pixelSize: AppScale.fs(15)
                font.weight: Font.DemiBold
            }

            Text {
                visible: root.showSpeech && root.showHeaderValues
                text: qsTr("Speech rate") + " " + root.paceText(root.headerSpeech)
                color: root.speechColor
                font.pixelSize: AppScale.fs(15)
                font.weight: Font.DemiBold
                font.features: { "tnum": 1 }
            }
        }
    }

    Text {
        id: captionItem
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: text.length > 0 ? implicitHeight : 0
        visible: text.length > 0
        text: root.caption
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        color: Theme.onSurfaceVariant(Material.theme)
        font.pixelSize: AppScale.fs(14)
    }

    Text {
        anchors.horizontalCenter: canvas.horizontalCenter
        anchors.verticalCenter: canvas.verticalCenter
        anchors.horizontalCenterOffset: root.zoneGutter / 2
        width: Math.max(0, canvas.width - root.zoneGutter - 32)
        visible: !root.waiting && root.plotted.length === 0 && !root.tipActive
        text: qsTr("No tempo yet")
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        color: Theme.onSurfaceVariant(Material.theme)
        font.pixelSize: AppScale.fs(14)
    }

    Column {
        anchors.horizontalCenter: canvas.horizontalCenter
        anchors.verticalCenter: canvas.verticalCenter
        anchors.horizontalCenterOffset: root.zoneGutter / 2
        width: Math.max(0, canvas.width - root.zoneGutter - 32)
        visible: root.waiting
        spacing: 6
        z: 1

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            font.family: Icons.familySolid
            font.weight: Font.Black
            font.pixelSize: AppScale.fs(22)
            text: Icons.faMicrophone
            color: Theme.primary(Material.theme)
        }

        Text {
            id: waitingTitle
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: qsTr("Everything is working. Keep speaking.")
            color: Theme.onSurface(Material.theme)
            font.pixelSize: AppScale.fs(16)
            font.weight: Font.DemiBold
        }

        Text {
            id: waitingBody
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: qsTr("The tempo appears about %n second(s) after you start speaking.", "",
                       root.firstReadingWholeSec)
            color: Theme.onSurfaceVariant(Material.theme)
            font.pixelSize: AppScale.fs(14)
            // The column is centered on the plot, which lands this line on
            // the medium/slow border. Shift it clear of that line, into the
            // slow band.
            transform: Translate { y: Math.round(font.pixelSize * 2.2) }
        }
    }

    Canvas {
        id: canvas
        anchors.top: header.bottom
        anchors.topMargin: root.compactHeader ? 0 : 12
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: captionItem.top
        anchors.bottomMargin: captionItem.text.length > 0 ? 8 : 0

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)
            var data = root.plotted
            if (!data)
                data = []
            if (width < 8 || height < 8)
                return

            var yMin = root.minimum
            var yMax = root.maximum
            if (!(yMax > yMin)) {
                yMin = 0
                yMax = 1
            }

            var axisStart = root.axisStart
            var axisEnd = Math.max(root.axisEnd, axisStart + 1)
            var tipTime = root.tipSec
            var span = axisEnd - axisStart
            var valueSpan = yMax - yMin

            var labelSize = AppScale.fs(11)
            ctx.font = labelSize + "px sans-serif"
            // Rotated zone words are only as wide as the font is tall.
            var gutter = root.zoneGutter
            var left = gutter
            var top = 8
            var bottom = labelSize + 10
            var plotWidth = Math.max(1, width - left)
            var plotHeight = Math.max(1, height - top - bottom)

            function px(seconds) {
                return left + (seconds - axisStart) / span * plotWidth
            }
            function py(value) {
                return top + (1 - (value - yMin) / valueSpan) * plotHeight
            }

            // Horizontal bands match the gauge: each zone blends across its
            // third. Fast is the top band, Slow the bottom.
            var bandH = plotHeight / 3
            for (var zone = 0; zone < 3; ++zone) {
                var yTop = top + (2 - zone) * bandH
                var yBot = zone === 0 ? top + plotHeight : yTop + bandH
                var wash = ctx.createLinearGradient(0, yBot, 0, yTop)
                var stops = 4
                for (var stop = 0; stop <= stops; ++stop) {
                    var u = stop / stops
                    wash.addColorStop(u, Qt.alpha(Theme.paceColor((zone + u) / 3, root.paintTheme), 0.16))
                }
                ctx.fillStyle = wash
                ctx.fillRect(left, yTop, plotWidth, Math.max(1, yBot - yTop))
            }

            // Thin lines at both ends and at the two borders between zones.
            ctx.strokeStyle = root.boundaryColor
            ctx.lineWidth = 1
            ctx.globalAlpha = 1
            for (var g = 0; g <= 3; ++g) {
                var gy = Math.round(top + g / 3 * plotHeight) + 0.5
                ctx.beginPath()
                ctx.moveTo(left, gy)
                ctx.lineTo(left + plotWidth, gy)
                ctx.stroke()
            }

            // slow / medium / fast, reading upward, each centered on its band.
            var zoneWords = [root.fastLabel, root.mediumLabel, root.slowLabel]
            var zoneInks = [Theme.zoneColor(2, root.paintTheme),
                            Theme.zoneColor(1, root.paintTheme),
                            Theme.zoneColor(0, root.paintTheme)]
            for (var zi = 0; zi < 3; ++zi) {
                var word = zoneWords[zi]
                var size = labelSize
                ctx.font = size + "px sans-serif"
                var maxLen = Math.max(8, bandH - 8)
                while (size > 8 && ctx.measureText(word).width > maxLen) {
                    size -= 1
                    ctx.font = size + "px sans-serif"
                }
                ctx.save()
                ctx.translate(gutter * 0.42, top + (zi + 0.5) * bandH)
                ctx.rotate(-Math.PI / 2)
                ctx.fillStyle = zoneInks[zi]
                ctx.globalAlpha = 1
                ctx.textAlign = "center"
                ctx.textBaseline = "middle"
                ctx.fillText(word, 0, 0)
                ctx.restore()
            }

            var gapLimit = Math.max(0.25, root.sampleInterval) * 2.5
            var series = []
            if (root.showArticulation)
                series.push({ key: "articulationRate", color: root.articulationColor,
                              tip: root.clampPace(root.tipArticulation) })
            if (root.showSpeech)
                series.push({ key: "speechRate", color: root.speechColor,
                              tip: root.clampPace(root.tipSpeech) })

            for (var s = 0; s < series.length; ++s) {
                var spec = series[s]
                var segments = []
                var current = []
                var prevSeconds = -1
                for (var p = 0; p < data.length; ++p) {
                    var value = root.clampPace(data[p][spec.key])
                    var seconds = Number(data[p].seconds)
                    if (!isFinite(value) || !isFinite(seconds))
                        continue
                    if (current.length > 0 && seconds - prevSeconds > gapLimit) {
                        segments.push(current)
                        current = []
                    }
                    current.push({ seconds: seconds, value: value })
                    prevSeconds = seconds
                }
                if (root.tipActive && isFinite(spec.tip) && tipTime >= axisStart) {
                    if (current.length > 0 && tipTime - prevSeconds > gapLimit) {
                        segments.push(current)
                        current = []
                    }
                    current.push({ seconds: tipTime, value: spec.tip })
                }
                if (current.length > 0)
                    segments.push(current)

                for (var seg = 0; seg < segments.length; ++seg) {
                    var points = segments[seg]
                    if (points.length === 0)
                        continue

                    ctx.beginPath()
                    for (var b = 0; b < points.length; ++b) {
                        var bx = px(points[b].seconds)
                        var by = py(points[b].value)
                        if (b === 0)
                            ctx.moveTo(bx, by)
                        else
                            ctx.lineTo(bx, by)
                    }
                    ctx.strokeStyle = spec.color
                    ctx.lineWidth = root.paceLineWidth
                    ctx.lineJoin = "round"
                    ctx.lineCap = "round"
                    ctx.stroke()
                }
            }

            ctx.fillStyle = root.labelColor
            ctx.textBaseline = "bottom"
            ctx.font = labelSize + "px sans-serif"
            ctx.textAlign = "left"
            ctx.fillText(root.formatClock(Math.max(0, axisStart)), left, height)
            ctx.textAlign = "right"
            ctx.fillText(root.formatClock(Math.max(0, axisEnd)), left + plotWidth, height)
        }
    }
}
