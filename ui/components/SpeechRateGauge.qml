import QtQuick
import QtQuick.Controls.Material 6.8
import "../utils"

Item {
    id: root
    property real value: 0
    property real minimum: 70
    property real maximum: 210

    implicitWidth: 320
    implicitHeight: 220

    readonly property real shown: {
        if (maximum <= minimum)
            return minimum
        return Math.max(minimum, Math.min(maximum, value))
    }

    function paintGauge() {
        canvas.requestPaint()
    }

    onValueChanged: paintGauge()
    onMinimumChanged: paintGauge()
    onMaximumChanged: paintGauge()
    onWidthChanged: paintGauge()
    onHeightChanged: paintGauge()

    Canvas {
        id: canvas
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            var cx = width / 2
            var cy = height * 0.70
            var radius = Math.min(width * 0.36, cy - 32)
            if (radius < 20)
                return

            var gradient = ctx.createLinearGradient(cx - radius, cy, cx + radius, cy)
            gradient.addColorStop(0, "#066832")
            gradient.addColorStop(0.5, "#c8b219")
            gradient.addColorStop(1, "#b8181f")

            ctx.beginPath()
            // Qt's canvas sweeps the opposite way from the flag name here, so
            // false keeps the semicircle above the baseline.
            ctx.arc(cx, cy, radius, Math.PI, 0, false)
            ctx.strokeStyle = gradient
            ctx.lineWidth = Math.max(12, radius * 0.14)
            ctx.lineCap = "round"
            ctx.stroke()

            var fraction = root.maximum <= root.minimum
                    ? 0
                    : (root.shown - root.minimum) / (root.maximum - root.minimum)
            var angle = Math.PI * (1 - fraction)
            var needle = radius * 0.78
            ctx.beginPath()
            ctx.moveTo(cx, cy)
            ctx.lineTo(cx + Math.cos(angle) * needle, cy - Math.sin(angle) * needle)
            ctx.strokeStyle = Theme.onSurface(Material.theme)
            ctx.lineWidth = 3
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(cx, cy, 5, 0, Math.PI * 2)
            ctx.fillStyle = Theme.onSurface(Material.theme)
            ctx.fill()

            ctx.fillStyle = Theme.onSurfaceVariant(Material.theme)
            ctx.font = AppScale.fs(13) + "px sans-serif"
            ctx.textAlign = "center"
            ctx.textBaseline = "middle"
            ctx.fillText(qsTr("Slow"), cx - radius * 0.78, cy - radius * 0.78)
            ctx.fillText(qsTr("Average"), cx, cy - radius - 20)
            ctx.fillText(qsTr("Fast"), cx + radius * 0.78, cy - radius * 0.78)

            ctx.fillStyle = Theme.onSurface(Material.theme)
            ctx.font = AppScale.fs(13) + "px sans-serif"
            ctx.textBaseline = "top"
            ctx.fillText(root.minimum.toFixed(0) + " " + qsTr("wpm"), cx - radius * 0.72, cy + 14)
            ctx.fillText(root.maximum.toFixed(0) + " " + qsTr("wpm"), cx + radius * 0.72, cy + 14)
        }
    }
}
