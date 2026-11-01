import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material 6.8
import QtQuick.Shapes
import "../utils"

Item {
    id: root
    property real value: 0
    property real secondValue: 0
    // Both paces: speech rate on the outer arc (`value`) and articulation one
    // ring inside it (`secondValue`). Fillers is always the innermost ring.
    property bool showSecond: false
    property string metricLabel: qsTr("Speech rate")
    property string secondMetricLabel: qsTr("Articulation")
    property real minimum: 70
    property real maximum: 210
    // Innermost arc. 0…100 percent of filler share. Drawn with the pace arcs
    // even when the fillers card is hidden.
    property real fillerValue: 0
    // Without a value the arcs stay empty and the readouts show a dash.
    property bool hasValue: true
    // Home turns this off while recording. The arcs keep moving; the integers
    // come back after Stop, Open File, and a saved result.
    property bool showNumbers: true
    property string unit: qsTr("wpm")
    property color cardColor: Theme.surfaceContainerLow(Material.theme)
    // Same colours as the tempo-chart curves. They do not follow the zone.
    property color arcColor: Theme.speechSeriesColor(Material.theme)
    property color secondArcColor: Theme.articulationSeriesColor(Material.theme)
    // Left inset shared with the tempo chart, so the pace-arc ends meet the
    // coloured plot. 0 on a page that has no chart beside this gauge.
    property real plotInset: 0
    // Live input meter. Home sets these while a session is open.
    property real micLevel: 0
    property bool micActive: false

    implicitWidth: 320
    implicitHeight: blockHeight

    Accessible.role: Accessible.Indicator
    Accessible.name: showSecond ? qsTr("Speech rate and articulation") : metricLabel
    Accessible.description: {
        var filler = qsTranslate("HomePage", "Fillers") + " "
                + (hasValue ? animatedFiller.toFixed(0) + " %" : qsTr("No measurement yet"))
        if (!hasValue)
            return qsTr("No measurement yet") + ". " + filler
        var first = value.toFixed(0) + " " + unit + ", " + zoneLabels[targetZone]
        if (!showSecond)
            return first + ". " + filler
        return metricLabel + " " + first + ". "
                + secondMetricLabel + " " + secondValue.toFixed(0) + " " + unit
                + ", " + zoneLabels[secondTargetZone] + ". " + filler
    }

    readonly property int paintTheme: Material.theme
    readonly property var zoneLabels: [qsTr("Slow"), qsTr("Average"), qsTr("Fast")]
    readonly property color fillerColor: Theme.fillerSeriesColor(paintTheme)

    // Semicircle, open at the bottom. 180° is 9 o'clock, 360° is 3 o'clock.
    readonly property real startAngle: 180
    readonly property real sweepAngle: 180

    readonly property real range: maximum > minimum ? maximum - minimum : 1
    property real animatedValue: hasValue ? value : minimum
    Behavior on animatedValue {
        NumberAnimation { duration: 650; easing.type: Easing.OutCubic }
    }
    property real animatedSecond: hasValue ? secondValue : minimum
    Behavior on animatedSecond {
        NumberAnimation { duration: 650; easing.type: Easing.OutCubic }
    }
    // root.fillerValue: an id named fillerValue would hide this property.
    property real animatedFiller: !hasValue || !(root.fillerValue > 0) ? 0
                                   : (root.fillerValue < 100 ? root.fillerValue : 100)
    Behavior on animatedFiller {
        NumberAnimation { duration: 650; easing.type: Easing.OutCubic }
    }

    readonly property real micClamped: Math.max(0, Math.min(1, micLevel))
    readonly property real micTarget: micActive ? Math.sqrt(micClamped) : 0
    property real micShown: micTarget
    Behavior on micShown {
        NumberAnimation { duration: 90; easing.type: Easing.OutQuad }
    }

    readonly property real fraction: Math.max(0, Math.min(1, (animatedValue - minimum) / range))
    readonly property real valueAngle: startAngle + fraction * sweepAngle
    readonly property int zone: Math.min(2, Math.floor(fraction * 3))
    readonly property int targetZone: Theme.zoneForValue(value, minimum, maximum)

    readonly property bool dual: showSecond
    readonly property real secondFraction: Math.max(0, Math.min(1, (animatedSecond - minimum) / range))
    readonly property real secondValueAngle: startAngle + secondFraction * sweepAngle
    readonly property int secondZone: Math.min(2, Math.floor(secondFraction * 3))
    readonly property int secondTargetZone: Theme.zoneForValue(secondValue, minimum, maximum)
    readonly property real fillerFraction: {
        var v = animatedFiller
        if (!isFinite(v) || v <= 0)
            return 0
        if (v >= 100)
            return 1
        return v / 100
    }
    readonly property real fillerAngle: startAngle + fillerFraction * sweepAngle

    readonly property real plotSpan: Math.max(48, width - plotInset)
    readonly property real radius: Theme.gaugeRadiusForSpan(plotSpan)
    readonly property real stroke: Theme.gaugeStroke(radius)
    readonly property real valueStroke: Math.max(4, stroke * 0.46)
    // Same stroke on every ring. The gap keeps the round caps from touching.
    readonly property real ringGap: Math.max(8, stroke * 0.28)
    readonly property real ringStep: stroke + ringGap
    // Outer ring meets the chart. Articulation is one step in, when shown.
    // Fillers is the next step in, always.
    readonly property real innerRadius: dual
                                        ? Math.max(16, radius - ringStep)
                                        : radius
    readonly property real fillerRadius: Math.max(16, (dual ? innerRadius : radius) - ringStep)
    readonly property real cx: plotInset + plotSpan / 2
    readonly property real cy: Theme.paceCy(radius)
    readonly property real holeRadius: Math.max(12, fillerRadius - stroke * 0.5 - 4)
    // Shared integers. The ring names sit under the arc, so the hole holds
    // numbers, the unit, and (one pace) the zone chip.
    readonly property real readoutNumberSize: {
        var paceCap = dual
                ? Math.min(28, Math.max(16, innerRadius * 0.34))
                : Math.min(40, Math.max(22, radius * 0.28))
        var unit = AppScale.fs(dual ? 12 : 13)
        var zone = dual ? 0 : (AppScale.fs(13) + 12)
        var lines = dual ? 3 : 2
        var fit = (holeRadius - unit - zone - 8) / lines
        return Math.max(14, Math.min(paceCap, fit))
    }
    readonly property real captionLine: AppScale.fs(12) + 6
    // Both paces: two rows under the ends. One pace: the two names share one row.
    readonly property int captionRows: dual ? 2 : 1
    readonly property real blockHeight: {
        var under = 2 + captionLine + 8 + captionRows * captionLine + (dual ? 2 : 0) + 2
        return cy + stroke * 0.5 + under
    }

    readonly property real gapAngle: stroke / Math.max(1, radius) * 180 / Math.PI + 2
    readonly property real segmentSweep: (sweepAngle - 2 * gapAngle) / 3
    readonly property real innerGapAngle: stroke / Math.max(1, innerRadius) * 180 / Math.PI + 2
    readonly property real innerSegmentSweep: (sweepAngle - 2 * innerGapAngle) / 3
    readonly property real fillerGapAngle: stroke / Math.max(1, fillerRadius) * 180 / Math.PI + 2
    readonly property real fillerSegmentSweep: (sweepAngle - 2 * fillerGapAngle) / 3

    function segmentStart(index) {
        return startAngle + index * (segmentSweep + gapAngle)
    }

    function innerSegmentStart(index) {
        return startAngle + index * (innerSegmentSweep + innerGapAngle)
    }

    function fillerSegmentStart(index) {
        return startAngle + index * (fillerSegmentSweep + fillerGapAngle)
    }

    function pointAt(angleDeg, r, centerY) {
        var a = angleDeg * Math.PI / 180
        var y = centerY === undefined ? cy : centerY
        return Qt.point(cx + Math.cos(a) * r, y + Math.sin(a) * r)
    }

    function paintOnCard(color, alpha) {
        var bg = cardColor
        var a = Math.max(0, Math.min(1, alpha))
        return Qt.rgba(bg.r + (color.r - bg.r) * a,
                       bg.g + (color.g - bg.g) * a,
                       bg.b + (color.b - bg.b) * a, 1)
    }

    // One step of the scale. Short flat strokes stand in for a stroke
    // gradient. The gap between zones is the thin border. The value is a
    // separate stroke in the chart colour, drawn on top.
    component ZoneSlice: ShapePath {
        id: slice
        property int zone: 0
        property int step: 0
        property int steps: 16
        // 0 speech (or the single pace), 1 articulation, 2 fillers.
        property int ring: 0
        property var gauge: root
        readonly property bool live: ring === 1 ? gauge.dual : true
        readonly property real z0: ring === 2 ? gauge.fillerSegmentStart(zone)
                                   : ring === 1 ? gauge.innerSegmentStart(zone)
                                   : gauge.segmentStart(zone)
        readonly property real zoneSweep: ring === 2 ? gauge.fillerSegmentSweep
                                         : ring === 1 ? gauge.innerSegmentSweep
                                         : gauge.segmentSweep
        readonly property real sliceSweep: zoneSweep / steps
        readonly property real a0: z0 + step * sliceSweep
        readonly property real a1: z0 + (step + 1) * sliceSweep
        readonly property bool atStart: step === 0
        readonly property bool atEnd: step === steps - 1
        readonly property real from: atStart ? a0 : a0 - 0.7
        readonly property real to: atEnd ? a1 : a1 + 0.7
        readonly property real drawn: Math.max(0.01, to - from)
        readonly property real tee: Math.max(0, Math.min(1, (zone + (step + 0.5) / steps) / 3))
        readonly property real ringRadius: ring === 2 ? gauge.fillerRadius
                                           : ring === 1 ? gauge.innerRadius
                                           : gauge.radius
        fillColor: "transparent"
        strokeWidth: live ? gauge.stroke : 0
        strokeColor: gauge.paintOnCard(Theme.paceColor(tee, gauge.paintTheme), 0.42)
        capStyle: (atStart || atEnd) ? ShapePath.RoundCap : ShapePath.FlatCap

        PathAngleArc {
            centerX: slice.gauge.cx
            centerY: slice.gauge.cy
            radiusX: slice.ringRadius
            radiusY: slice.ringRadius
            startAngle: slice.from
            sweepAngle: slice.drawn
        }
    }

    component ValueArc: ShapePath {
        id: valueArc
        property bool inner: false
        property var gauge: root
        readonly property bool live: !inner || gauge.dual
        readonly property real sweep: !gauge.hasValue ? 0
                                      : (inner ? gauge.secondValueAngle : gauge.valueAngle) - gauge.startAngle
        fillColor: "transparent"
        strokeWidth: live && sweep > 0.4 ? gauge.valueStroke : 0
        strokeColor: inner ? gauge.secondArcColor : gauge.arcColor
        capStyle: ShapePath.RoundCap

        PathAngleArc {
            centerX: valueArc.gauge.cx
            centerY: valueArc.gauge.cy
            radiusX: valueArc.inner ? valueArc.gauge.innerRadius : valueArc.gauge.radius
            radiusY: valueArc.inner ? valueArc.gauge.innerRadius : valueArc.gauge.radius
            startAngle: valueArc.gauge.startAngle
            sweepAngle: Math.max(0.01, valueArc.sweep)
        }
    }

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        ZoneSlice { zone: 0; step: 0 } ZoneSlice { zone: 0; step: 1 } ZoneSlice { zone: 0; step: 2 } ZoneSlice { zone: 0; step: 3 }
        ZoneSlice { zone: 0; step: 4 } ZoneSlice { zone: 0; step: 5 } ZoneSlice { zone: 0; step: 6 } ZoneSlice { zone: 0; step: 7 }
        ZoneSlice { zone: 0; step: 8 } ZoneSlice { zone: 0; step: 9 } ZoneSlice { zone: 0; step: 10 } ZoneSlice { zone: 0; step: 11 }
        ZoneSlice { zone: 0; step: 12 } ZoneSlice { zone: 0; step: 13 } ZoneSlice { zone: 0; step: 14 } ZoneSlice { zone: 0; step: 15 }
        ZoneSlice { zone: 1; step: 0 } ZoneSlice { zone: 1; step: 1 } ZoneSlice { zone: 1; step: 2 } ZoneSlice { zone: 1; step: 3 }
        ZoneSlice { zone: 1; step: 4 } ZoneSlice { zone: 1; step: 5 } ZoneSlice { zone: 1; step: 6 } ZoneSlice { zone: 1; step: 7 }
        ZoneSlice { zone: 1; step: 8 } ZoneSlice { zone: 1; step: 9 } ZoneSlice { zone: 1; step: 10 } ZoneSlice { zone: 1; step: 11 }
        ZoneSlice { zone: 1; step: 12 } ZoneSlice { zone: 1; step: 13 } ZoneSlice { zone: 1; step: 14 } ZoneSlice { zone: 1; step: 15 }
        ZoneSlice { zone: 2; step: 0 } ZoneSlice { zone: 2; step: 1 } ZoneSlice { zone: 2; step: 2 } ZoneSlice { zone: 2; step: 3 }
        ZoneSlice { zone: 2; step: 4 } ZoneSlice { zone: 2; step: 5 } ZoneSlice { zone: 2; step: 6 } ZoneSlice { zone: 2; step: 7 }
        ZoneSlice { zone: 2; step: 8 } ZoneSlice { zone: 2; step: 9 } ZoneSlice { zone: 2; step: 10 } ZoneSlice { zone: 2; step: 11 }
        ZoneSlice { zone: 2; step: 12 } ZoneSlice { zone: 2; step: 13 } ZoneSlice { zone: 2; step: 14 } ZoneSlice { zone: 2; step: 15 }
        ZoneSlice { zone: 0; step: 0; ring: 1 } ZoneSlice { zone: 0; step: 1; ring: 1 } ZoneSlice { zone: 0; step: 2; ring: 1 } ZoneSlice { zone: 0; step: 3; ring: 1 }
        ZoneSlice { zone: 0; step: 4; ring: 1 } ZoneSlice { zone: 0; step: 5; ring: 1 } ZoneSlice { zone: 0; step: 6; ring: 1 } ZoneSlice { zone: 0; step: 7; ring: 1 }
        ZoneSlice { zone: 0; step: 8; ring: 1 } ZoneSlice { zone: 0; step: 9; ring: 1 } ZoneSlice { zone: 0; step: 10; ring: 1 } ZoneSlice { zone: 0; step: 11; ring: 1 }
        ZoneSlice { zone: 0; step: 12; ring: 1 } ZoneSlice { zone: 0; step: 13; ring: 1 } ZoneSlice { zone: 0; step: 14; ring: 1 } ZoneSlice { zone: 0; step: 15; ring: 1 }
        ZoneSlice { zone: 1; step: 0; ring: 1 } ZoneSlice { zone: 1; step: 1; ring: 1 } ZoneSlice { zone: 1; step: 2; ring: 1 } ZoneSlice { zone: 1; step: 3; ring: 1 }
        ZoneSlice { zone: 1; step: 4; ring: 1 } ZoneSlice { zone: 1; step: 5; ring: 1 } ZoneSlice { zone: 1; step: 6; ring: 1 } ZoneSlice { zone: 1; step: 7; ring: 1 }
        ZoneSlice { zone: 1; step: 8; ring: 1 } ZoneSlice { zone: 1; step: 9; ring: 1 } ZoneSlice { zone: 1; step: 10; ring: 1 } ZoneSlice { zone: 1; step: 11; ring: 1 }
        ZoneSlice { zone: 1; step: 12; ring: 1 } ZoneSlice { zone: 1; step: 13; ring: 1 } ZoneSlice { zone: 1; step: 14; ring: 1 } ZoneSlice { zone: 1; step: 15; ring: 1 }
        ZoneSlice { zone: 2; step: 0; ring: 1 } ZoneSlice { zone: 2; step: 1; ring: 1 } ZoneSlice { zone: 2; step: 2; ring: 1 } ZoneSlice { zone: 2; step: 3; ring: 1 }
        ZoneSlice { zone: 2; step: 4; ring: 1 } ZoneSlice { zone: 2; step: 5; ring: 1 } ZoneSlice { zone: 2; step: 6; ring: 1 } ZoneSlice { zone: 2; step: 7; ring: 1 }
        ZoneSlice { zone: 2; step: 8; ring: 1 } ZoneSlice { zone: 2; step: 9; ring: 1 } ZoneSlice { zone: 2; step: 10; ring: 1 } ZoneSlice { zone: 2; step: 11; ring: 1 }
        ZoneSlice { zone: 2; step: 12; ring: 1 } ZoneSlice { zone: 2; step: 13; ring: 1 } ZoneSlice { zone: 2; step: 14; ring: 1 } ZoneSlice { zone: 2; step: 15; ring: 1 }
        ZoneSlice { zone: 0; step: 0; ring: 2 } ZoneSlice { zone: 0; step: 1; ring: 2 } ZoneSlice { zone: 0; step: 2; ring: 2 } ZoneSlice { zone: 0; step: 3; ring: 2 }
        ZoneSlice { zone: 0; step: 4; ring: 2 } ZoneSlice { zone: 0; step: 5; ring: 2 } ZoneSlice { zone: 0; step: 6; ring: 2 } ZoneSlice { zone: 0; step: 7; ring: 2 }
        ZoneSlice { zone: 0; step: 8; ring: 2 } ZoneSlice { zone: 0; step: 9; ring: 2 } ZoneSlice { zone: 0; step: 10; ring: 2 } ZoneSlice { zone: 0; step: 11; ring: 2 }
        ZoneSlice { zone: 0; step: 12; ring: 2 } ZoneSlice { zone: 0; step: 13; ring: 2 } ZoneSlice { zone: 0; step: 14; ring: 2 } ZoneSlice { zone: 0; step: 15; ring: 2 }
        ZoneSlice { zone: 1; step: 0; ring: 2 } ZoneSlice { zone: 1; step: 1; ring: 2 } ZoneSlice { zone: 1; step: 2; ring: 2 } ZoneSlice { zone: 1; step: 3; ring: 2 }
        ZoneSlice { zone: 1; step: 4; ring: 2 } ZoneSlice { zone: 1; step: 5; ring: 2 } ZoneSlice { zone: 1; step: 6; ring: 2 } ZoneSlice { zone: 1; step: 7; ring: 2 }
        ZoneSlice { zone: 1; step: 8; ring: 2 } ZoneSlice { zone: 1; step: 9; ring: 2 } ZoneSlice { zone: 1; step: 10; ring: 2 } ZoneSlice { zone: 1; step: 11; ring: 2 }
        ZoneSlice { zone: 1; step: 12; ring: 2 } ZoneSlice { zone: 1; step: 13; ring: 2 } ZoneSlice { zone: 1; step: 14; ring: 2 } ZoneSlice { zone: 1; step: 15; ring: 2 }
        ZoneSlice { zone: 2; step: 0; ring: 2 } ZoneSlice { zone: 2; step: 1; ring: 2 } ZoneSlice { zone: 2; step: 2; ring: 2 } ZoneSlice { zone: 2; step: 3; ring: 2 }
        ZoneSlice { zone: 2; step: 4; ring: 2 } ZoneSlice { zone: 2; step: 5; ring: 2 } ZoneSlice { zone: 2; step: 6; ring: 2 } ZoneSlice { zone: 2; step: 7; ring: 2 }
        ZoneSlice { zone: 2; step: 8; ring: 2 } ZoneSlice { zone: 2; step: 9; ring: 2 } ZoneSlice { zone: 2; step: 10; ring: 2 } ZoneSlice { zone: 2; step: 11; ring: 2 }
        ZoneSlice { zone: 2; step: 12; ring: 2 } ZoneSlice { zone: 2; step: 13; ring: 2 } ZoneSlice { zone: 2; step: 14; ring: 2 } ZoneSlice { zone: 2; step: 15; ring: 2 }
        ValueArc { }
        ValueArc { inner: true }

        ShapePath {
            id: fillerValueArc
            fillColor: "transparent"
            strokeWidth: root.hasValue && root.fillerFraction > 0.004 ? root.valueStroke : 0
            strokeColor: root.fillerColor
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: root.cx
                centerY: root.cy
                radiusX: root.fillerRadius
                radiusY: root.fillerRadius
                startAngle: root.startAngle
                sweepAngle: Math.max(0.01, root.fillerAngle - root.startAngle)
            }
        }
    }

    Repeater {
        model: 9

        Rectangle {
            required property int index
            readonly property bool major: index % 4 === 0
            readonly property real angle: root.startAngle + index / 8 * root.sweepAngle
            readonly property real tickRadius: root.radius - root.stroke / 2 - 6 - height / 2
            readonly property point center: root.pointAt(angle, tickRadius)
            visible: !root.dual
                     && tickRadius - height / 2 > root.fillerRadius + root.stroke / 2 + 2
            width: 2
            height: major ? 8 : 5
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
        width: root.stroke * 1.35
        height: width
        radius: width / 2
        x: center.x - width / 2
        y: center.y - height / 2
        color: root.cardColor
        border.width: Math.max(3, root.stroke * 0.28)
        border.color: root.secondArcColor
    }

    Rectangle {
        id: knob
        readonly property point center: root.pointAt(root.valueAngle, root.radius)
        visible: root.hasValue
        width: root.stroke * 1.35
        height: width
        radius: width / 2
        x: center.x - width / 2
        y: center.y - height / 2
        color: root.cardColor
        border.width: Math.max(3, root.stroke * 0.28)
        border.color: root.arcColor
    }

    Rectangle {
        id: fillerKnob
        readonly property point center: root.pointAt(root.fillerAngle, root.fillerRadius)
        visible: root.hasValue
        width: root.stroke * 1.35
        height: width
        radius: width / 2
        x: center.x - width / 2
        y: center.y - height / 2
        color: root.cardColor
        border.width: Math.max(3, root.stroke * 0.28)
        border.color: root.fillerColor
    }

    Column {
        id: readout
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: root.plotInset / 2
        y: {
            var top = Math.max(0, root.cy - root.holeRadius + 2)
            var prefer = root.cy - root.holeRadius * 0.5 - height / 2
            var aboveDiameter = root.cy - 4 - height
            return Math.max(top, Math.min(prefer, aboveDiameter))
        }
        spacing: root.dual ? 0 : 2

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: !root.hasValue || root.showNumbers
            text: root.hasValue ? root.animatedValue.toFixed(0) : "– –"
            font.pixelSize: root.readoutNumberSize
            font.weight: root.hasValue ? Font.DemiBold : Font.Light
            font.features: { "tnum": 1 }
            color: root.hasValue ? Theme.onSurface(Material.theme) : Theme.outline(Material.theme)
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.dual && (!root.hasValue || root.showNumbers)
            topPadding: 2
            text: root.hasValue ? root.animatedSecond.toFixed(0) : "– –"
            font.pixelSize: root.readoutNumberSize
            font.weight: root.hasValue ? Font.DemiBold : Font.Light
            font.features: { "tnum": 1 }
            color: root.hasValue ? Theme.onSurface(Material.theme) : Theme.outline(Material.theme)
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.unit
            font.pixelSize: AppScale.fs(root.dual ? 12 : 13)
            color: Theme.onSurfaceVariant(Material.theme)
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: !root.hasValue || root.showNumbers
            topPadding: 6
            text: root.hasValue ? root.animatedFiller.toFixed(0) : "– –"
            font.pixelSize: root.readoutNumberSize
            font.weight: root.hasValue ? Font.DemiBold : Font.Light
            font.features: { "tnum": 1 }
            color: root.hasValue ? Theme.onSurface(Material.theme) : Theme.outline(Material.theme)
        }

        Item { width: 1; height: root.dual ? 1 : 4; visible: !root.dual || root.micActive }

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
                Layout.preferredWidth: Math.max(root.dual ? 56 : 72, Math.min(root.dual ? 96 : 128, root.radius * (root.dual ? 0.42 : 0.55)))
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
            visible: !root.dual && root.showNumbers
            implicitWidth: zoneText.implicitWidth + 24
            implicitHeight: zoneText.implicitHeight + 8
            radius: height / 2
            opacity: root.hasValue && root.value > 0 ? 1 : 0
            color: Qt.alpha(Theme.zoneColor(root.zone, root.paintTheme), 0.16)
            Behavior on opacity { NumberAnimation { duration: 200 } }

            Text {
                id: zoneText
                anchors.centerIn: parent
                text: root.zoneLabels[root.zone]
                font.pixelSize: AppScale.fs(13)
                font.weight: Font.DemiBold
                color: Theme.zoneColor(root.zone, root.paintTheme)
            }
        }

    }

    Text {
        id: minLabel
        readonly property point tip: root.pointAt(root.startAngle, root.radius)
        x: tip.x - width / 2
        y: tip.y + root.stroke / 2 + 2
        height: root.captionLine
        verticalAlignment: Text.AlignVCenter
        text: root.minimum.toFixed(0)
        font.pixelSize: AppScale.fs(12)
        color: Theme.onSurfaceVariant(Material.theme)
    }

    Text {
        id: maxLabel
        readonly property point tip: root.pointAt(root.startAngle + root.sweepAngle, root.radius)
        x: tip.x - width / 2
        y: tip.y + root.stroke / 2 + 2
        height: root.captionLine
        verticalAlignment: Text.AlignVCenter
        text: root.maximum.toFixed(0)
        font.pixelSize: AppScale.fs(12)
        color: Theme.onSurfaceVariant(Material.theme)
    }

    // Ring names, under the arc and clear of 70 / 210. Both paces make a
    // triangle: speech at the lower left, articulation at the lower right,
    // fillers centred on the next line. One pace keeps the two names on
    // that first line, pace at the left and fillers at the right.
    Item {
        id: nameRow
        anchors.left: parent.left
        anchors.right: parent.right
        y: minLabel.y + minLabel.height + 8
        height: root.captionLine

        Text {
            id: nameLeft
            anchors.left: parent.left
            anchors.leftMargin: root.plotInset
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(48, (parent.width - root.plotInset) / 2 - 8)
            height: root.captionLine
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.NoWrap
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 9
            text: root.metricLabel
            font.pixelSize: AppScale.fs(12)
            font.weight: Font.DemiBold
            color: root.hasValue && root.value > 0
                   ? root.arcColor
                   : Theme.onSurfaceVariant(Material.theme)
            Accessible.ignored: true
        }

        Text {
            id: nameRight
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(48, (parent.width - root.plotInset) / 2 - 8)
            height: root.captionLine
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.NoWrap
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 9
            text: root.dual ? root.secondMetricLabel : qsTranslate("HomePage", "Fillers")
            font.pixelSize: AppScale.fs(12)
            font.weight: Font.DemiBold
            color: {
                if (root.dual)
                    return root.hasValue && root.secondValue > 0
                            ? root.secondArcColor
                            : Theme.onSurfaceVariant(Material.theme)
                return root.hasValue && root.fillerValue > 0
                        ? root.fillerColor
                        : Theme.onSurfaceVariant(Material.theme)
            }
            Accessible.ignored: true
        }
    }

    Text {
        id: nameBottom
        visible: root.dual
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: root.plotInset / 2
        y: nameRow.y + nameRow.height + 2
        width: Math.max(48, Math.min(parent.width - 16, parent.width * 0.62))
        height: root.captionLine
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.NoWrap
        fontSizeMode: Text.HorizontalFit
        minimumPixelSize: 9
        text: qsTranslate("HomePage", "Fillers")
        font.pixelSize: AppScale.fs(12)
        font.weight: Font.DemiBold
        color: root.hasValue && root.fillerValue > 0
               ? root.fillerColor
               : Theme.onSurfaceVariant(Material.theme)
        Accessible.ignored: true
    }
}
