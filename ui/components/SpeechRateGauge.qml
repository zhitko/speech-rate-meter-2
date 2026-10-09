import QtQuick
import QtQuick.Shapes
import QtQuick.Layouts
import QtQuick.Controls.Material 6.8
import "../utils"

Item {
    id: root
    property real value: 0
    property real secondValue: 0
    // Both paces: an inner arc and a second number. The outer arc stays `value`.
    property bool showSecond: false
    property string metricLabel: qsTr("Speech rate")
    property string secondMetricLabel: qsTr("Articulation")
    property real minimum: 70
    property real maximum: 210
    // Without a value the arc stays empty and the readout shows a dash.
    property bool hasValue: true
    property string unit: qsTr("wpm")
    property color cardColor: Theme.surfaceContainerLow(Material.theme)
    // Live input meter. Home sets these while a session is open.
    property real micLevel: 0
    property bool micActive: false

    implicitWidth: 320
    implicitHeight: 260

    Accessible.role: Accessible.Indicator
    Accessible.name: showSecond ? qsTr("Speech rate and articulation") : metricLabel
    Accessible.description: {
        if (!hasValue)
            return qsTr("No measurement yet")
        var first = value.toFixed(0) + " " + unit + ", " + zoneLabels[targetZone]
        if (!showSecond)
            return first
        return metricLabel + " " + first + ". "
                + secondMetricLabel + " " + secondValue.toFixed(0) + " " + unit
                + ", " + zoneLabels[secondTargetZone]
    }

    readonly property int paintTheme: Material.theme
    readonly property var zoneLabels: [qsTr("Slow"), qsTr("Average"), qsTr("Fast")]

    // The arc opens at the bottom: 240° clockwise from the lower-left corner.
    readonly property real startAngle: 150
    readonly property real sweepAngle: 240

    readonly property real range: maximum > minimum ? maximum - minimum : 1
    property real animatedValue: hasValue ? value : minimum
    Behavior on animatedValue {
        NumberAnimation { duration: 650; easing.type: Easing.OutCubic }
    }
    property real animatedSecond: hasValue ? secondValue : minimum
    Behavior on animatedSecond {
        NumberAnimation { duration: 650; easing.type: Easing.OutCubic }
    }

    readonly property real micClamped: Math.max(0, Math.min(1, micLevel))
    // Square root so a quiet microphone still moves the bar.
    readonly property real micTarget: micActive ? Math.sqrt(micClamped) : 0
    property real micShown: micTarget
    Behavior on micShown {
        NumberAnimation { duration: 90; easing.type: Easing.OutQuad }
    }

    readonly property real fraction: Math.max(0, Math.min(1, (animatedValue - minimum) / range))
    readonly property real valueAngle: startAngle + fraction * sweepAngle
    readonly property int zone: Math.min(2, Math.floor(fraction * 3))
    readonly property int targetZone: Theme.zoneForValue(value, minimum, maximum)
    readonly property color zoneColor: Theme.zoneColor(zone, paintTheme)

    readonly property bool dual: showSecond
    readonly property real secondFraction: Math.max(0, Math.min(1, (animatedSecond - minimum) / range))
    readonly property real secondValueAngle: startAngle + secondFraction * sweepAngle
    readonly property int secondZone: Math.min(2, Math.floor(secondFraction * 3))
    readonly property int secondTargetZone: Theme.zoneForValue(secondValue, minimum, maximum)
    readonly property color secondZoneColor: Theme.zoneColor(secondZone, paintTheme)

    readonly property real labelSpace: AppScale.fs(12) + 10
    readonly property real stroke: Math.max(10, radius * 0.15)
    readonly property real innerStroke: Math.max(8, stroke * 0.68)
    readonly property real dualGap: Math.max(10, stroke * 0.85)
    readonly property real radius: Math.max(20, Math.min((width - 16) / 2.15,
                                                         (height - labelSpace - 8) / 1.62))
    readonly property real innerRadius: dual
                                        ? Math.max(16, radius - (stroke + innerStroke) / 2 - dualGap)
                                        : radius
    readonly property real innerGapAngle: innerStroke / Math.max(1, innerRadius) * 180 / Math.PI + 3
    readonly property real innerSegmentSweep: (sweepAngle - 2 * innerGapAngle) / 3
    readonly property real dualTextWidth: Math.max(48, (innerRadius - innerStroke) * 1.65)
    readonly property real cx: width / 2
    readonly property real cy: (height - labelSpace - radius * 1.5) / 2 + radius + stroke * 0.1

    // Gap between zones wide enough that the rounded caps never touch.
    readonly property real gapAngle: stroke / radius * 180 / Math.PI + 3
    readonly property real segmentSweep: (sweepAngle - 2 * gapAngle) / 3

    function segmentStart(index) {
        return startAngle + index * (segmentSweep + gapAngle)
    }

    function filledSweep(index) {
        return Math.max(0, Math.min(segmentSweep, valueAngle - segmentStart(index)))
    }

    function innerSegmentStart(index) {
        return startAngle + index * (innerSegmentSweep + innerGapAngle)
    }

    function innerFilledSweep(index) {
        return Math.max(0, Math.min(innerSegmentSweep, secondValueAngle - innerSegmentStart(index)))
    }

    function pointAt(angleDeg, r) {
        var a = angleDeg * Math.PI / 180
        return Qt.point(cx + Math.cos(a) * r, cy + Math.sin(a) * r)
    }

    component Arc: ShapePath {
        id: arc
        required property var gauge
        property real ringRadius: gauge.radius
        property real ringStroke: gauge.stroke
        property real arcStart: 0
        property real arcSweep: 0
        property color arcColor: "transparent"
        fillColor: "transparent"
        strokeColor: arcSweep > 0.25 ? arcColor : "transparent"
        strokeWidth: ringStroke
        capStyle: ShapePath.RoundCap

        PathAngleArc {
            centerX: arc.gauge.cx
            centerY: arc.gauge.cy
            radiusX: arc.ringRadius
            radiusY: arc.ringRadius
            startAngle: arc.arcStart
            sweepAngle: Math.max(0.01, arc.arcSweep)
        }
    }

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        Arc { gauge: root; arcStart: root.segmentStart(0); arcSweep: root.segmentSweep; arcColor: Qt.alpha(Theme.zoneColor(0, root.paintTheme), 0.18) }
        Arc { gauge: root; arcStart: root.segmentStart(1); arcSweep: root.segmentSweep; arcColor: Qt.alpha(Theme.zoneColor(1, root.paintTheme), 0.18) }
        Arc { gauge: root; arcStart: root.segmentStart(2); arcSweep: root.segmentSweep; arcColor: Qt.alpha(Theme.zoneColor(2, root.paintTheme), 0.18) }

        Arc { gauge: root; arcStart: root.segmentStart(0); arcSweep: root.hasValue ? root.filledSweep(0) : 0; arcColor: Theme.zoneColor(0, root.paintTheme) }
        Arc { gauge: root; arcStart: root.segmentStart(1); arcSweep: root.hasValue ? root.filledSweep(1) : 0; arcColor: Theme.zoneColor(1, root.paintTheme) }
        Arc { gauge: root; arcStart: root.segmentStart(2); arcSweep: root.hasValue ? root.filledSweep(2) : 0; arcColor: Theme.zoneColor(2, root.paintTheme) }

        Arc {
            gauge: root
            ringRadius: root.innerRadius
            ringStroke: root.innerStroke
            arcStart: root.innerSegmentStart(0)
            arcSweep: root.dual ? root.innerSegmentSweep : 0
            arcColor: Qt.alpha(Theme.zoneColor(0, root.paintTheme), 0.18)
        }
        Arc {
            gauge: root
            ringRadius: root.innerRadius
            ringStroke: root.innerStroke
            arcStart: root.innerSegmentStart(1)
            arcSweep: root.dual ? root.innerSegmentSweep : 0
            arcColor: Qt.alpha(Theme.zoneColor(1, root.paintTheme), 0.18)
        }
        Arc {
            gauge: root
            ringRadius: root.innerRadius
            ringStroke: root.innerStroke
            arcStart: root.innerSegmentStart(2)
            arcSweep: root.dual ? root.innerSegmentSweep : 0
            arcColor: Qt.alpha(Theme.zoneColor(2, root.paintTheme), 0.18)
        }
        Arc {
            gauge: root
            ringRadius: root.innerRadius
            ringStroke: root.innerStroke
            arcStart: root.innerSegmentStart(0)
            arcSweep: root.dual && root.hasValue ? root.innerFilledSweep(0) : 0
            arcColor: Theme.zoneColor(0, root.paintTheme)
        }
        Arc {
            gauge: root
            ringRadius: root.innerRadius
            ringStroke: root.innerStroke
            arcStart: root.innerSegmentStart(1)
            arcSweep: root.dual && root.hasValue ? root.innerFilledSweep(1) : 0
            arcColor: Theme.zoneColor(1, root.paintTheme)
        }
        Arc {
            gauge: root
            ringRadius: root.innerRadius
            ringStroke: root.innerStroke
            arcStart: root.innerSegmentStart(2)
            arcSweep: root.dual && root.hasValue ? root.innerFilledSweep(2) : 0
            arcColor: Theme.zoneColor(2, root.paintTheme)
        }
    }

    Repeater {
        model: 13

        Rectangle {
            required property int index
            readonly property bool major: index % 4 === 0
            readonly property real angle: root.startAngle + index / 12 * root.sweepAngle
            readonly property real tickRadius: root.radius - root.stroke / 2 - 8 - height / 2
            readonly property point center: root.pointAt(angle, tickRadius)
            visible: !root.dual
            width: 2
            height: major ? 10 : 6
            radius: 1
            x: center.x - width / 2
            y: center.y - height / 2
            rotation: angle + 90
            antialiasing: true
            color: root.hasValue && angle <= root.valueAngle
                   ? Theme.onSurfaceVariant(Material.theme)
                   : Theme.outlineVariant(Material.theme)
        }
    }

    Rectangle {
        id: innerKnob
        readonly property point center: root.pointAt(root.secondValueAngle, root.innerRadius)
        visible: root.dual && root.hasValue
        width: root.innerStroke * 1.45
        height: width
        radius: width / 2
        x: center.x - width / 2
        y: center.y - height / 2
        color: root.cardColor
        border.width: Math.max(2, root.innerStroke * 0.32)
        border.color: root.secondZoneColor
    }

    Rectangle {
        id: knob
        readonly property point center: root.pointAt(root.valueAngle, root.radius)
        visible: root.hasValue
        width: root.stroke * 1.55
        height: width
        radius: width / 2
        x: center.x - width / 2
        y: center.y - height / 2
        color: root.cardColor
        border.width: Math.max(3, root.stroke * 0.32)
        border.color: root.zoneColor
    }

    Column {
        id: readout
        anchors.horizontalCenter: parent.horizontalCenter
        y: root.cy - height / 2 + (root.dual ? 0 : root.radius * 0.08)
        spacing: root.dual ? 0 : 2

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.hasValue ? root.animatedValue.toFixed(0) : "– –"
            font.pixelSize: root.dual
                             ? Math.max(16, Math.min(30, root.innerRadius * 0.36))
                             : Math.max(24, root.radius * 0.44)
            font.weight: root.hasValue ? Font.DemiBold : Font.Light
            font.features: { "tnum": 1 }
            color: root.hasValue ? Theme.onSurface(Material.theme) : Theme.outline(Material.theme)
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.dual
            width: root.dualTextWidth
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            text: root.metricLabel
            font.pixelSize: AppScale.fs(11)
            font.weight: Font.DemiBold
            color: root.hasValue && root.value > 0 ? root.zoneColor : Theme.onSurfaceVariant(Material.theme)
            Accessible.ignored: true
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.dual
            topPadding: 3
            text: root.hasValue ? root.animatedSecond.toFixed(0) : "– –"
            font.pixelSize: Math.max(16, Math.min(30, root.innerRadius * 0.36))
            font.weight: root.hasValue ? Font.DemiBold : Font.Light
            font.features: { "tnum": 1 }
            color: root.hasValue ? Theme.onSurface(Material.theme) : Theme.outline(Material.theme)
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.dual
            width: root.dualTextWidth
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            text: root.secondMetricLabel
            font.pixelSize: AppScale.fs(11)
            font.weight: Font.DemiBold
            color: root.hasValue && root.secondValue > 0
                   ? root.secondZoneColor
                   : Theme.onSurfaceVariant(Material.theme)
            Accessible.ignored: true
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.unit
            font.pixelSize: AppScale.fs(root.dual ? 12 : 14)
            color: Theme.onSurfaceVariant(Material.theme)
        }

        Item { width: 1; height: root.dual ? 2 : 6 }

        RowLayout {
            id: micRow
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.micActive
            width: implicitWidth
            height: implicitHeight
            spacing: 6
            Accessible.role: Accessible.ProgressBar
            Accessible.name: qsTr("Microphone level")
            Accessible.description: qsTr("%1 percent").arg(Math.round(root.micClamped * 100))

            Text {
                Layout.alignment: Qt.AlignVCenter
                font.family: Icons.familySolid
                font.pixelSize: AppScale.fs(13)
                text: Icons.faMicrophone
                color: root.micShown > 0.04
                       ? Theme.primary(root.paintTheme)
                       : Theme.onSurfaceVariant(root.paintTheme)
                Behavior on color { ColorAnimation { duration: 120 } }
            }

            Rectangle {
                id: micTrack
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: Math.max(root.dual ? 56 : 72, Math.min(root.dual ? 96 : 128, root.radius * (root.dual ? 0.48 : 0.72)))
                Layout.preferredHeight: 10
                implicitWidth: Layout.preferredWidth
                implicitHeight: 10
                radius: height / 2
                color: Qt.alpha(Theme.primary(root.paintTheme), 0.18)

                Rectangle {
                    width: Math.min(parent.width,
                                    root.micShown <= 0 ? 0 : Math.max(height, parent.width * root.micShown))
                    height: parent.height
                    radius: height / 2
                    color: Theme.primary(root.paintTheme)
                }
            }
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: !root.dual
            implicitWidth: zoneText.implicitWidth + 24
            implicitHeight: zoneText.implicitHeight + 8
            radius: height / 2
            opacity: root.hasValue && root.value > 0 ? 1 : 0
            color: Qt.alpha(root.zoneColor, 0.16)
            Behavior on opacity { NumberAnimation { duration: 200 } }

            Text {
                id: zoneText
                anchors.centerIn: parent
                text: root.zoneLabels[root.zone]
                font.pixelSize: AppScale.fs(13)
                font.weight: Font.DemiBold
                color: root.zoneColor
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: !root.dual
            text: root.metricLabel
            font.pixelSize: AppScale.fs(12)
            font.weight: Font.DemiBold
            color: Theme.onSurface(Material.theme)
            Accessible.ignored: true
        }
    }

    Text {
        readonly property point end: root.pointAt(root.startAngle, root.radius)
        x: end.x - width / 2
        y: end.y + root.stroke / 2 + 4
        text: root.minimum.toFixed(0)
        font.pixelSize: AppScale.fs(12)
        color: Theme.onSurfaceVariant(Material.theme)
    }

    Text {
        readonly property point end: root.pointAt(root.startAngle + root.sweepAngle, root.radius)
        x: end.x - width / 2
        y: end.y + root.stroke / 2 + 4
        text: root.maximum.toFixed(0)
        font.pixelSize: AppScale.fs(12)
        color: Theme.onSurfaceVariant(Material.theme)
    }
}
