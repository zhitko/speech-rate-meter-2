import QtQuick
import QtQuick.Controls.Material 6.8
import "../utils"

Item {
    id: root
    property string chartTitle: ""
    property string unit: ""
    property string valueKey: "speechRate"
    property int decimals: 0
    property var points: []

    readonly property color axisColor: Theme.outlineVariant(Material.theme)
    readonly property color labelColor: Theme.onSurfaceVariant(Material.theme)
    readonly property color lineColor: Theme.primary(Material.theme)
    readonly property int paintTheme: Material.theme

    implicitHeight: 180

    onPointsChanged: canvas.requestPaint()
    onValueKeyChanged: canvas.requestPaint()
    onDecimalsChanged: canvas.requestPaint()
    onUnitChanged: canvas.requestPaint()
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()
    onAxisColorChanged: canvas.requestPaint()
    onLabelColorChanged: canvas.requestPaint()
    onLineColorChanged: canvas.requestPaint()
    onPaintThemeChanged: canvas.requestPaint()

    function hasRenderablePoints() {
        for (var i = 0; i < points.length; ++i) {
            var value = points[i] ? Number(points[i][valueKey]) : NaN
            if (isFinite(value))
                return true
        }
        return false
    }

    Text {
        id: titleLabel
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        text: root.chartTitle
        color: Theme.onSurface(Material.theme)
        font.pixelSize: AppScale.fs(16)
        font.bold: true
    }

    Text {
        anchors.centerIn: canvas
        width: Math.max(0, canvas.width - 32)
        visible: !root.hasRenderablePoints()
        text: qsTr("No measurements to chart")
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        color: Theme.onSurfaceVariant(Material.theme)
        font.pixelSize: AppScale.fs(14)
    }

    Canvas {
        id: canvas
        anchors.top: titleLabel.bottom
        anchors.topMargin: 8
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            var values = []
            var clocks = []
            for (var i = 0; i < root.points.length; ++i) {
                var point = root.points[i]
                if (!point)
                    continue
                var value = Number(point[root.valueKey])
                if (!isFinite(value))
                    continue
                values.push(value)
                clocks.push(point.clock ? point.clock : "")
            }
            if (values.length === 0)
                return

            var minValue = values[0]
            var maxValue = values[0]
            for (var n = 1; n < values.length; ++n) {
                minValue = Math.min(minValue, values[n])
                maxValue = Math.max(maxValue, values[n])
            }
            var pad = root.decimals > 0 ? Math.pow(10, -root.decimals) : 1
            if (minValue === maxValue) {
                minValue -= pad
                maxValue += pad
            }

            var left = root.unit.length > 0 ? (root.decimals > 0 ? 86 : 72)
                                             : (root.decimals > 0 ? 58 : 44)
            var right = 12
            var top = 8
            var bottom = 46
            var plotWidth = Math.max(1, width - left - right)
            var plotHeight = Math.max(1, height - top - bottom)

            ctx.strokeStyle = root.axisColor
            ctx.lineWidth = 1
            ctx.globalAlpha = 0.55
            for (var g = 1; g <= 3; ++g) {
                var gy = top + (g / 4) * plotHeight
                ctx.beginPath()
                ctx.moveTo(left, gy)
                ctx.lineTo(left + plotWidth, gy)
                ctx.stroke()
            }
            ctx.globalAlpha = 1
            ctx.beginPath()
            ctx.moveTo(left, top)
            ctx.lineTo(left, top + plotHeight)
            ctx.lineTo(left + plotWidth, top + plotHeight)
            ctx.stroke()

            ctx.fillStyle = root.labelColor
            ctx.font = AppScale.fs(11) + "px sans-serif"
            ctx.textAlign = "right"
            ctx.textBaseline = "middle"
            var unitSuffix = root.unit.length > 0 ? " " + root.unit : ""
            ctx.fillText(maxValue.toFixed(root.decimals) + unitSuffix, left - 6, top)
            ctx.fillText(minValue.toFixed(root.decimals) + unitSuffix, left - 6, top + plotHeight)

            ctx.strokeStyle = root.lineColor
            ctx.fillStyle = root.lineColor
            ctx.lineWidth = 2
            ctx.beginPath()
            for (var p = 0; p < values.length; ++p) {
                var x = left + (values.length === 1 ? plotWidth / 2 : (p / (values.length - 1)) * plotWidth)
                var y = top + (1 - (values[p] - minValue) / (maxValue - minValue)) * plotHeight
                if (p === 0)
                    ctx.moveTo(x, y)
                else
                    ctx.lineTo(x, y)
            }
            if (values.length > 1)
                ctx.stroke()
            for (var d = 0; d < values.length; ++d) {
                var dx = left + (values.length === 1 ? plotWidth / 2 : (d / (values.length - 1)) * plotWidth)
                var dy = top + (1 - (values[d] - minValue) / (maxValue - minValue)) * plotHeight
                ctx.beginPath()
                ctx.arc(dx, dy, 3.5, 0, Math.PI * 2)
                ctx.fill()
            }

            ctx.fillStyle = root.labelColor
            ctx.textBaseline = "top"
            if (clocks.length > 0) {
                ctx.textAlign = "left"
                ctx.fillText(clocks[0], left, top + plotHeight + 6)
            }
            if (clocks.length > 1) {
                ctx.textAlign = "right"
                ctx.fillText(clocks[clocks.length - 1], left + plotWidth, top + plotHeight + 6)
            }
            ctx.textAlign = "center"
            ctx.fillText(qsTr("Clock time"), left + plotWidth / 2, top + plotHeight + 22)
        }
    }
}
