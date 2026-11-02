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
    // ring inside it (`secondValue`).
    property bool showSecond: false
    property string metricLabel: qsTr("Speech rate")
    property string secondMetricLabel: qsTr("Articulation")
    property real minimum: 70
    property real maximum: 210
    // Without a value the arcs stay empty and the readouts show a dash.
    property bool hasValue: true
    // Integers sit above the names while recording and after Stop.
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

    readonly property real plotSpan: Math.max(48, width - plotInset)
    readonly property real radius: Theme.gaugeRadiusForSpan(plotSpan)
    readonly property real stroke: Theme.gaugeStroke(radius)
    readonly property real valueStroke: Math.max(4, stroke * 0.46)
    // Same stroke on every ring. The gap keeps the round caps from touching.
    readonly property real ringGap: Math.max(8, stroke * 0.28)
    readonly property real ringStep: stroke + ringGap
    // Outer ring meets the chart. Articulation is one step in, when shown.
    readonly property real innerRadius: dual
                                        ? Math.max(16, radius - ringStep)
                                        : radius
    readonly property real cx: plotInset + plotSpan / 2
    readonly property real cy: Theme.paceCy(radius)
    readonly property real holeRadius: Math.max(12, innerRadius - stroke * 0.5 - 4)
    readonly property real blockHeight: Theme.gaugeBlockHeight(radius)
    // Value + name blocks live in the clear disc inside the innermost ring.
    // Both paces sit side by side, with the unit under them. One pace is a
    // single block, also with the unit under it. Names shrink to the chord,
    // and are never below 8 px. On a short window the disc is smaller, so
    // the type shrinks with it.
    readonly property real pairGap: holeRadius < 96 ? 10 : 14
    function holeHalf(fromTop) {
        var r = Math.max(28, holeRadius)
        var dy = Math.max(0, Math.min(r - 1, r - fromTop))
        return Math.sqrt(Math.max(0, r * r - dy * dy))
    }
    readonly property real blockWidth: {
        var fromTop = Math.max(28, holeRadius) * (dual ? 0.48 : 0.36)
        var half = holeHalf(fromTop)
        if (!dual)
            return Math.max(72, Math.min(half * 2 - 16, 200))
        return Math.max(48, (half * 2 - 10 - pairGap) / 2)
    }
    readonly property real captionPx: {
        var r = Math.max(28, holeRadius)
        // "Articulation" / "Артикуляция" is the long caption.
        var byWidth = blockWidth / (12 * 0.56)
        var byHeight = r / (dual ? 5.4 : 3.8)
        return Math.max(8, Math.min(AppScale.fs(11), Math.floor(Math.min(byWidth, byHeight))))
    }
    readonly property real numberPx: {
        var unit = Math.min(AppScale.fs(11), captionPx + 1)
        var room = holeRadius * 0.78 - captionPx - unit - (dual ? 10 : 6)
        return Math.max(12, Math.min(dual ? 32 : 42, Math.floor(room)))
    }

    function captionInk(active, series) {
        return active ? series : Theme.onSurfaceVariant(paintTheme)
    }

    // 0 speech (or the single pace), 1 articulation when both are shown.
    function ringRadius(ring) {
        return ring === 1 ? innerRadius : radius
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

    // The arc runs clockwise on screen from 9 o'clock to 3 o'clock. A conical
    // gradient runs counter-clockwise from its angle, so with angle 0 the
    // scale fraction f sits at position (1 - f) / 2. The lower half is only
    // reached by the round caps: slow below the left end, fast below the right.
    function conicPos(f) {
        return (1 - Math.max(0, Math.min(1, f))) / 2
    }

    // Pace colours along the whole scale, toned down onto the card.
    component PaceGradient: ConicalGradient {
        id: pace
        property real alpha: 0.4
        property var gauge: root
        function tone(f) {
            // Fast is a brighter yellow than a single wash would show, on the
            // scale as well as on the chart. Slow and medium stay faint.
            var a = pace.alpha
            if (f >= 0.70)
                a = pace.gauge.paintTheme === Material.Dark ? 0.88 : 0.72
            return pace.gauge.paintOnCard(Theme.paceColor(f, pace.gauge.paintTheme), a)
        }
        centerX: gauge.cx
        centerY: gauge.cy
        angle: 0
        GradientStop { position: 0.0; color: pace.tone(1) }
        GradientStop { position: pace.gauge.conicPos(0.925); color: pace.tone(0.925) }
        GradientStop { position: pace.gauge.conicPos(0.85); color: pace.tone(0.85) }
        GradientStop { position: pace.gauge.conicPos(0.775); color: pace.tone(0.775) }
        GradientStop { position: pace.gauge.conicPos(0.70); color: pace.tone(0.70) }
        GradientStop { position: pace.gauge.conicPos(0.64); color: pace.tone(0.64) }
        GradientStop { position: pace.gauge.conicPos(0.58); color: pace.tone(0.58) }
        GradientStop { position: pace.gauge.conicPos(0.49); color: pace.tone(0.49) }
        GradientStop { position: pace.gauge.conicPos(0.40); color: pace.tone(0.40) }
        GradientStop { position: pace.gauge.conicPos(0.31); color: pace.tone(0.31) }
        GradientStop { position: pace.gauge.conicPos(0.22); color: pace.tone(0.22) }
        GradientStop { position: pace.gauge.conicPos(0.11); color: pace.tone(0.11) }
        GradientStop { position: 0.5; color: pace.tone(0) }
        GradientStop { position: 0.75; color: pace.tone(0) }
        GradientStop { position: 0.7501; color: pace.tone(1) }
        GradientStop { position: 1.0; color: pace.tone(1) }
    }

    // Value stroke: pale at the scale start, full chart colour at the head.
    component TrailGradient: ConicalGradient {
        id: trail
        property color ink: "black"
        property real head: 0
        property var gauge: root
        readonly property color faded: gauge.paintOnCard(ink, 0.38)
        centerX: gauge.cx
        centerY: gauge.cy
        angle: 0
        GradientStop { position: 0.0; color: trail.ink }
        GradientStop { position: trail.gauge.conicPos(trail.head); color: trail.ink }
        GradientStop { position: 0.5; color: trail.faded }
        GradientStop { position: 0.75; color: trail.faded }
        GradientStop { position: 0.7501; color: trail.ink }
        GradientStop { position: 1.0; color: trail.ink }
    }

    // One continuous ring of the scale.
    component TrackArc: ShapePath {
        id: track
        property int ring: 0
        property var gauge: root
        readonly property bool live: ring === 1 ? gauge.dual : true
        fillColor: "transparent"
        strokeWidth: live ? gauge.stroke : 0
        strokeColor: "transparent"
        capStyle: ShapePath.RoundCap
        strokeGradient: PaceGradient {
            alpha: track.gauge.paintTheme === Material.Dark ? 0.36 : 0.42
        }

        PathAngleArc {
            centerX: track.gauge.cx
            centerY: track.gauge.cy
            radiusX: track.gauge.ringRadius(track.ring)
            radiusY: track.gauge.ringRadius(track.ring)
            startAngle: track.gauge.startAngle
            sweepAngle: track.gauge.sweepAngle
        }
    }

    // Thin cuts in the card colour at the slow/average and average/fast borders.
    component ZoneNotches: ShapePath {
        id: notches
        property int ring: 0
        property var gauge: root
        readonly property bool live: ring === 1 ? gauge.dual : true
        readonly property real r: gauge.ringRadius(ring)
        readonly property real half: gauge.stroke / 2 + 1
        readonly property real a1: gauge.startAngle + gauge.sweepAngle / 3
        readonly property real a2: gauge.startAngle + gauge.sweepAngle * 2 / 3
        readonly property point p1: gauge.pointAt(a1, r - half)
        readonly property point q1: gauge.pointAt(a1, r + half)
        readonly property point p2: gauge.pointAt(a2, r - half)
        readonly property point q2: gauge.pointAt(a2, r + half)
        fillColor: "transparent"
        strokeWidth: live ? Math.max(2, gauge.stroke * 0.09) : 0
        strokeColor: gauge.cardColor
        capStyle: ShapePath.FlatCap

        PathMove { x: notches.p1.x; y: notches.p1.y }
        PathLine { x: notches.q1.x; y: notches.q1.y }
        PathMove { x: notches.p2.x; y: notches.p2.y }
        PathLine { x: notches.q2.x; y: notches.q2.y }
    }

    component ValueArc: ShapePath {
        id: valueArc
        property int ring: 0
        property real fraction: 0
        property color ink: "black"
        property var gauge: root
        readonly property bool live: ring === 1 ? gauge.dual : true
        readonly property real sweep: gauge.hasValue ? fraction * gauge.sweepAngle : 0
        fillColor: "transparent"
        strokeWidth: live && sweep > 0.4 ? gauge.valueStroke : 0
        strokeColor: "transparent"
        capStyle: ShapePath.RoundCap
        strokeGradient: TrailGradient {
            ink: valueArc.ink
            head: valueArc.fraction
        }

        PathAngleArc {
            centerX: valueArc.gauge.cx
            centerY: valueArc.gauge.cy
            radiusX: valueArc.gauge.ringRadius(valueArc.ring)
            radiusY: valueArc.gauge.ringRadius(valueArc.ring)
            startAngle: valueArc.gauge.startAngle
            sweepAngle: Math.max(0.01, valueArc.sweep)
        }
    }

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        TrackArc { ring: 0 }
        TrackArc { ring: 1 }
        ZoneNotches { ring: 0 }
        ZoneNotches { ring: 1 }
        ValueArc { ring: 0; fraction: root.fraction; ink: root.arcColor }
        ValueArc { ring: 1; fraction: root.secondFraction; ink: root.secondArcColor }
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

    // Value head: a soft halo in the ring colour, then a solid disc with a
    // rim in the card colour so it reads on top of the track.
    component Knob: Item {
        id: knobItem
        property int ring: 0
        property real angle: root.startAngle
        property color ink: "black"
        readonly property point center: root.pointAt(angle, root.ringRadius(ring))
        width: root.stroke * 1.5
        height: width
        x: center.x - width / 2
        y: center.y - height / 2

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: Qt.alpha(knobItem.ink, root.paintTheme === Material.Dark ? 0.30 : 0.22)
        }
        Rectangle {
            anchors.centerIn: parent
            width: root.stroke * 1.08
            height: width
            radius: width / 2
            color: knobItem.ink
            border.width: Math.max(2, root.stroke * 0.16)
            border.color: root.cardColor
        }
    }

    Knob {
        visible: root.dual && root.hasValue
        ring: 1
        angle: root.secondValueAngle
        ink: root.secondArcColor
    }
    Knob {
        visible: root.hasValue
        ring: 0
        angle: root.valueAngle
        ink: root.arcColor
    }

    // One ring: the integer above its name. Names use the ring colour.
    // While recording the integer is hidden. With no measurement it is a dash.
    component ValueBlock: Column {
        id: block
        property string figure: ""
        property bool figureVisible: true
        property string caption: ""
        // Small unit after the integer, such as "%". Hidden with no measurement.
        property string suffix: ""
        property color ink: Theme.onSurfaceVariant(root.paintTheme)
        property real captionSize: root.captionPx
        width: root.blockWidth
        spacing: 0

        Item {
            visible: block.figureVisible
            width: block.width
            height: figureText.height

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 1

                Text {
                    id: figureText
                    text: block.figure
                    font.pixelSize: root.numberPx
                    font.weight: root.hasValue ? Font.Bold : Font.Light
                    font.letterSpacing: root.hasValue ? -0.04 * root.numberPx : 0
                    font.features: { "tnum": 1 }
                    color: root.hasValue ? Theme.onSurface(root.paintTheme) : Theme.outline(root.paintTheme)
                    Accessible.ignored: true
                }
                Text {
                    visible: root.hasValue && block.suffix.length > 0
                    anchors.baseline: figureText.baseline
                    text: block.suffix
                    font.pixelSize: Math.max(9, Math.round(root.numberPx * 0.5))
                    font.weight: Font.DemiBold
                    color: Theme.onSurfaceVariant(root.paintTheme)
                    Accessible.ignored: true
                }
            }
        }
        Text {
            width: block.width
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.NoWrap
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 8
            text: block.caption
            font.pixelSize: block.captionSize
            font.weight: Font.DemiBold
            color: block.ink
            Accessible.ignored: true
        }
    }

    // Inside the rings, low, near the open bottom edge. Both paces sit side
    // by side, with the unit under them. One pace is that block alone, with
    // the unit under it.
    Item {
        id: readout
        readonly property bool figures: !root.hasValue || root.showNumbers
        readonly property string paceFigure: root.hasValue ? root.animatedValue.toFixed(0) : "– –"
        readonly property string secondFigure: root.hasValue ? root.animatedSecond.toFixed(0) : "– –"
        width: root.dual ? root.blockWidth * 2 + root.pairGap : root.blockWidth
        height: dualColumn.visible ? dualColumn.implicitHeight : singleColumn.implicitHeight
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: root.plotInset / 2
        y: {
            // Sit on the open bottom of the semicircle. The diameter is the
            // lower edge of the arcs; a few pixels keep the unit above it.
            var ceiling = root.cy - root.holeRadius + 2
            var floor = root.cy - 8 - height
            return Math.max(ceiling, floor)
        }

        Column {
            id: dualColumn
            visible: root.dual
            width: parent.width
            spacing: 1

            Row {
                width: parent.width
                spacing: root.pairGap

                ValueBlock {
                    figure: readout.paceFigure
                    figureVisible: readout.figures
                    caption: root.metricLabel
                    ink: root.captionInk(root.hasValue && root.value > 0, root.arcColor)
                }
                ValueBlock {
                    figure: readout.secondFigure
                    figureVisible: readout.figures
                    caption: root.secondMetricLabel
                    ink: root.captionInk(root.hasValue && root.secondValue > 0, root.secondArcColor)
                }
            }

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: root.unit
                font.pixelSize: Math.min(AppScale.fs(11), root.captionPx + 1)
                color: Theme.onSurfaceVariant(root.paintTheme)
            }
        }

        Column {
            id: singleColumn
            visible: !root.dual
            width: parent.width
            spacing: 1

            ValueBlock {
                width: parent.width
                figure: readout.paceFigure
                figureVisible: readout.figures
                caption: root.metricLabel
                ink: root.captionInk(root.hasValue && root.value > 0, root.arcColor)
            }
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: root.unit
                font.pixelSize: Math.min(AppScale.fs(11), root.captionPx + 1)
                color: Theme.onSurfaceVariant(root.paintTheme)
            }
        }
    }

    Item {
        id: micSlot
        visible: root.micActive
        width: Math.min(root.holeRadius * 1.3, 120)
        height: micRow.implicitHeight
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: root.plotInset / 2
        // Open part of the semicircle, between the end labels, so it does
        // not cover the names inside the rings.
        y: root.cy + 6

        Row {
            id: micRow
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 6
            Accessible.role: Accessible.ProgressBar
            Accessible.name: qsTr("Microphone level")
            Accessible.description: qsTr("%1 percent").arg(Math.round(root.micClamped * 100))

            Text {
                anchors.verticalCenter: parent.verticalCenter
                font.family: Icons.familySolid
                font.pixelSize: AppScale.fs(12)
                text: Icons.faMicrophone
                color: root.micShown > 0.04
                       ? Theme.primary(root.paintTheme)
                       : Theme.onSurfaceVariant(root.paintTheme)
                Behavior on color { ColorAnimation { duration: 120 } }
            }

            Rectangle {
                id: micTrack
                anchors.verticalCenter: parent.verticalCenter
                width: Math.max(36, Math.min(72, root.holeRadius * 0.7))
                height: 8
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
    }

    Text {
        id: minLabel
        readonly property point tip: root.pointAt(root.startAngle, root.radius)
        x: tip.x - width / 2
        y: tip.y + root.stroke / 2 + 2
        text: root.minimum.toFixed(0)
        font.pixelSize: AppScale.fs(12)
        color: Theme.onSurfaceVariant(Material.theme)
    }

    Text {
        id: maxLabel
        readonly property point tip: root.pointAt(root.startAngle + root.sweepAngle, root.radius)
        x: tip.x - width / 2
        y: tip.y + root.stroke / 2 + 2
        text: root.maximum.toFixed(0)
        font.pixelSize: AppScale.fs(12)
        color: Theme.onSurfaceVariant(Material.theme)
    }
}
