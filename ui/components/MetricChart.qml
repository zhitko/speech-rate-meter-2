import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material 6.8
import "../utils"

Rectangle {
    id: root
    property string chartTitle: ""
    property string unit: ""
    property string valueKey: "speechRate"
    property int decimals: 0
    property var points: []
    property string icon: ""
    property color lineColor: Theme.primary(Material.theme)

    readonly property color axisColor: Theme.outlineVariant(Material.theme)
    readonly property color labelColor: Theme.onSurfaceVariant(Material.theme)
    readonly property int paintTheme: Material.theme
    readonly property var values: {
        var result = []
        for (var i = 0; i < points.length; ++i) {
            var point = points[i]
            var value = point ? Number(point[valueKey]) : NaN
            if (isFinite(value))
                result.push({ value: value, clock: point.clock ? point.clock : "" })
        }
        return result
    }

    implicitHeight: 240
    radius: Theme.shapeExtraLarge
    color: Theme.surfaceContainerLow(Material.theme)

    onValuesChanged: canvas.requestPaint()
    onUnitChanged: canvas.requestPaint()
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()
    onAxisColorChanged: canvas.requestPaint()
    onLabelColorChanged: canvas.requestPaint()
    onLineColorChanged: canvas.requestPaint()
    onPaintThemeChanged: canvas.requestPaint()

    RowLayout {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 16
        spacing: 8

        Rectangle {
            visible: root.icon.length > 0
            implicitWidth: 28
            implicitHeight: 28
            radius: Theme.shapeSmall
            color: Qt.alpha(root.lineColor, 0.14)

            Text {
                anchors.centerIn: parent
                text: root.icon
                font.family: Icons.familySolid
                font.weight: Font.Black
                font.pixelSize: AppScale.fs(13)
                color: root.lineColor
            }
        }

        Text {
            Layout.fillWidth: true
            text: root.chartTitle
            elide: Text.ElideRight
            color: Theme.onSurface(Material.theme)
            font.pixelSize: AppScale.fs(16)
            font.weight: Font.DemiBold
        }

        Text {
            visible: root.values.length > 0
            text: root.values.length > 0
                  ? root.values[root.values.length - 1].value.toFixed(root.decimals)
                    + (root.unit.length > 0 ? " " + root.unit : "")
                  : ""
            color: Theme.onSurface(Material.theme)
            font.pixelSize: AppScale.fs(15)
            font.weight: Font.DemiBold
            font.features: { "tnum": 1 }
        }
    }

    Text {
        anchors.centerIn: canvas
        width: Math.max(0, canvas.width - 32)
        visible: root.values.length === 0
        text: qsTr("No measurements to chart")
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
        anchors.bottom: parent.bottom
        anchors.leftMargin: 12
        anchors.rightMargin: 16
        anchors.bottomMargin: 12

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)
            var data = root.values
            if (data.length === 0)
                return

            var minValue = data[0].value
            var maxValue = data[0].value
            for (var n = 1; n < data.length; ++n) {
                minValue = Math.min(minValue, data[n].value)
                maxValue = Math.max(maxValue, data[n].value)
            }
            var pad = root.decimals > 0 ? Math.pow(10, -root.decimals) : 1
            if (minValue === maxValue) {
                minValue -= pad
                maxValue += pad
            }

            var labelSize = AppScale.fs(11)
            ctx.font = labelSize + "px sans-serif"
            var unitSuffix = root.unit.length > 0 ? " " + root.unit : ""
            var maxLabel = maxValue.toFixed(root.decimals) + unitSuffix
            var minLabel = minValue.toFixed(root.decimals) + unitSuffix
            var left = Math.max(ctx.measureText(maxLabel).width, ctx.measureText(minLabel).width) + 10
            var top = Math.max(labelSize / 2, 6) + 2
            var bottom = labelSize + 10
            var plotWidth = Math.max(1, width - left)
            var plotHeight = Math.max(1, height - top - bottom)

            var inset = 6
            function px(index) {
                return left + inset + (data.length === 1 ? (plotWidth - 2 * inset) / 2
                                                         : index / (data.length - 1) * (plotWidth - 2 * inset))
            }
            function py(value) {
                return top + (1 - (value - minValue) / (maxValue - minValue)) * plotHeight
            }

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

            if (data.length > 1) {
                var gradient = ctx.createLinearGradient(0, top, 0, top + plotHeight)
                gradient.addColorStop(0, Qt.alpha(root.lineColor, 0.28))
                gradient.addColorStop(1, Qt.alpha(root.lineColor, 0.0))
                ctx.beginPath()
                ctx.moveTo(px(0), top + plotHeight)
                for (var a = 0; a < data.length; ++a)
                    ctx.lineTo(px(a), py(data[a].value))
                ctx.lineTo(px(data.length - 1), top + plotHeight)
                ctx.closePath()
                ctx.fillStyle = gradient
                ctx.fill()

                ctx.beginPath()
                for (var p = 0; p < data.length; ++p) {
                    if (p === 0)
                        ctx.moveTo(px(p), py(data[p].value))
                    else
                        ctx.lineTo(px(p), py(data[p].value))
                }
                ctx.strokeStyle = root.lineColor
                ctx.lineWidth = 2.5
                ctx.lineJoin = "round"
                ctx.lineCap = "round"
                ctx.stroke()
            }

            var showAll = data.length <= 40
            for (var d = 0; d < data.length; ++d) {
                var last = d === data.length - 1
                if (!showAll && !last)
                    continue
                ctx.beginPath()
                ctx.arc(px(d), py(data[d].value), last ? 5 : 3.5, 0, Math.PI * 2)
                ctx.fillStyle = last ? root.lineColor : root.color
                ctx.fill()
                ctx.lineWidth = 2
                ctx.strokeStyle = last ? root.color : root.lineColor
                ctx.stroke()
            }

            ctx.fillStyle = root.labelColor
            ctx.textBaseline = "bottom"
            if (data.length > 0) {
                ctx.textAlign = data.length === 1 ? "center" : "left"
                ctx.fillText(data[0].clock, px(0), height)
            }
            if (data.length > 1) {
                ctx.textAlign = "right"
                ctx.fillText(data[data.length - 1].clock, left + plotWidth, height)
            }
        }
    }
}
