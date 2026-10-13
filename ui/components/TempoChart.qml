import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material 6.8
import "../utils"

// Live tempo chart: header, zone washes, grid, and pace lines (no fill
// under the stroke). The line grows from the left until the chart window
// is full, then scrolls. Y is fixed to Slow–Fast.
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

    readonly property bool showSpeech: gaugeMode !== 1
    readonly property bool showArticulation: gaugeMode !== 0
    readonly property real headerSpeech: tipActive ? tipSpeech
                                                   : (lastPoint ? Number(lastPoint.speechRate) : NaN)
    readonly property real headerArticulation: tipActive ? tipArticulation
                                                         : (lastPoint ? Number(lastPoint.articulationRate) : NaN)
    readonly property bool hasHeader: tipActive || !!lastPoint
    readonly property int paintTheme: Material.theme
    // Same green / yellow as the Slow and Average gauge arcs.
    readonly property color articulationColor: Theme.zoneColor(0, paintTheme)
    readonly property color speechColor: Theme.zoneColor(1, paintTheme)
    readonly property color lineColor: gaugeMode === 1 ? articulationColor : speechColor
    readonly property color axisColor: Theme.outlineVariant(paintTheme)
    readonly property color labelColor: Theme.onSurfaceVariant(paintTheme)
    // Same Slow / Average / Fast washes as the gauge track, softer on the chart.
    readonly property color zoneSlow: Qt.alpha(Theme.zoneColor(0, paintTheme), 0.09)
    readonly property color zoneAverage: Qt.alpha(Theme.zoneColor(1, paintTheme), 0.09)
    readonly property color zoneFast: Qt.alpha(Theme.zoneColor(2, paintTheme), 0.09)
    readonly property string chartIcon: gaugeMode === 1 ? Icons.faCommentDots : Icons.faGaugeHigh

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
        if (!hasHeader)
            return qsTr("No tempo yet")
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
    onZoneSlowChanged: canvas.requestPaint()
    onZoneAverageChanged: canvas.requestPaint()
    onZoneFastChanged: canvas.requestPaint()
    onPaintThemeChanged: canvas.requestPaint()

    RowLayout {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: 8

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
            visible: root.hasHeader
            Layout.maximumWidth: Math.max(120, header.width * 0.68)

            Text {
                visible: root.showArticulation && root.hasHeader
                text: qsTr("Articulation") + " " + root.paceText(root.headerArticulation)
                color: root.articulationColor
                font.pixelSize: AppScale.fs(15)
                font.weight: Font.DemiBold
                font.features: { "tnum": 1 }
            }

            Text {
                visible: root.showArticulation && root.showSpeech && root.hasHeader
                text: "/"
                color: Theme.onSurfaceVariant(Material.theme)
                font.pixelSize: AppScale.fs(15)
                font.weight: Font.DemiBold
            }

            Text {
                visible: root.showSpeech && root.hasHeader
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
        anchors.centerIn: canvas
        width: Math.max(0, canvas.width - 32)
        visible: root.plotted.length === 0 && !root.tipActive
        text: qsTr("No tempo yet")
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        color: Theme.onSurfaceVariant(Material.theme)
        font.pixelSize: AppScale.fs(14)
    }

    Canvas {
        id: canvas
        anchors.top: header.bottom
        anchors.topMargin: 12
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
            var unitSuffix = root.unit.length > 0 ? " " + root.unit : ""
            var maxLabel = yMax.toFixed(0) + unitSuffix
            var minLabel = yMin.toFixed(0) + unitSuffix
            var left = Math.max(ctx.measureText(maxLabel).width, ctx.measureText(minLabel).width) + 10
            var top = Math.max(labelSize / 2, 6) + 2
            var bottom = labelSize + 10
            var plotWidth = Math.max(1, width - left)
            var plotHeight = Math.max(1, height - top - bottom)

            function px(seconds) {
                return left + (seconds - axisStart) / span * plotWidth
            }
            function py(value) {
                return top + (1 - (value - yMin) / valueSpan) * plotHeight
            }

            // Horizontal bands match gauge arcs: Fast (top), Average, Slow (bottom).
            var bandH = plotHeight / 3
            ctx.fillStyle = root.zoneFast
            ctx.fillRect(left, top, plotWidth, bandH)
            ctx.fillStyle = root.zoneAverage
            ctx.fillRect(left, top + bandH, plotWidth, bandH)
            ctx.fillStyle = root.zoneSlow
            ctx.fillRect(left, top + 2 * bandH, plotWidth, plotHeight - 2 * bandH)

            ctx.strokeStyle = root.axisColor
            ctx.lineWidth = 1
            for (var g = 0; g <= 3; ++g) {
                var gy = Math.round(top + g / 3 * plotHeight) + 0.5
                ctx.globalAlpha = g === 3 ? 1 : 0.6
                ctx.beginPath()
                ctx.moveTo(left, gy)
                ctx.lineTo(left + plotWidth, gy)
                ctx.stroke()
            }
            ctx.globalAlpha = 1

            ctx.fillStyle = root.labelColor
            ctx.textAlign = "right"
            ctx.textBaseline = "middle"
            ctx.fillText(maxLabel, left - 8, top)
            ctx.fillText(minLabel, left - 8, top + plotHeight)

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
                    ctx.lineWidth = 2.5
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
