import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material 6.8
import QtQuick.Shapes
import "../utils"

Item {
    id: root
    property real value: 0
    property real secondValue: 0
    // Both paces: an inner arc and a second number. The outer arc is `value`
    // and the top number (speech rate, Rs). The inner arc is `secondValue`
    // and the number under it (articulation, Ra).
    property bool showSecond: false
    property string metricLabel: qsTr("Speech rate")
    property string secondMetricLabel: qsTr("Articulation")
    property real minimum: 70
    property real maximum: 210
    // Fillers arc under the pace arcs. 0…100 percent of filler share.
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
    readonly property real innerStroke: Math.max(8, stroke * 0.68)
    readonly property real innerValueStroke: Math.max(3, innerStroke * 0.46)
    readonly property real dualGap: Math.max(8, stroke * 0.55)
    readonly property real innerRadius: dual
                                        ? Math.max(16, radius - (stroke + innerStroke) / 2 - dualGap)
                                        : radius
    readonly property real fillerRadius: Theme.fillerRadiusFor(radius)
    readonly property real fillerStroke: Theme.fillerStrokeFor(radius)
    readonly property real fillerValueStroke: Math.max(3, fillerStroke * 0.5)
    readonly property real cx: plotInset + plotSpan / 2
    readonly property real cy: Theme.paceCy(radius)
    readonly property real fillerCy: Theme.fillerCy(radius)
    readonly property real blockHeight: Theme.gaugeBlockHeight(radius)
    readonly property real labelBand: 22

    readonly property real gapAngle: stroke / Math.max(1, radius) * 180 / Math.PI + 2
    readonly property real segmentSweep: (sweepAngle - 2 * gapAngle) / 3
    readonly property real innerGapAngle: innerStroke / Math.max(1, innerRadius) * 180 / Math.PI + 2
    readonly property real innerSegmentSweep: (sweepAngle - 2 * innerGapAngle) / 3

    function segmentStart(index) {
        return startAngle + index * (segmentSweep + gapAngle)
    }

    function innerSegmentStart(index) {
        return startAngle + index * (innerSegmentSweep + innerGapAngle)
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

    // One zone of the track the pace arc runs along. The value itself is a
    // thinner stroke in the chart colour, drawn on top.
    component ZoneTrack: ShapePath {
        id: track
        property int zone: 0
        property bool inner: false
        property var gauge: root
        readonly property bool live: !inner || gauge.dual
        readonly property real from: inner ? gauge.innerSegmentStart(zone) : gauge.segmentStart(zone)
        readonly property real sweep: inner ? gauge.innerSegmentSweep : gauge.segmentSweep
        fillColor: "transparent"
        strokeWidth: live ? (inner ? gauge.innerStroke : gauge.stroke) : 0
        strokeColor: Theme.zoneTrackColor(zone, gauge.paintTheme)
        capStyle: ShapePath.RoundCap

        PathAngleArc {
            centerX: track.gauge.cx
            centerY: track.gauge.cy
            radiusX: track.inner ? track.gauge.innerRadius : track.gauge.radius
            radiusY: track.inner ? track.gauge.innerRadius : track.gauge.radius
            startAngle: track.from
            sweepAngle: Math.max(0.01, track.sweep)
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
        strokeWidth: live && sweep > 0.4 ? (inner ? gauge.innerValueStroke : gauge.valueStroke) : 0
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

        ZoneTrack { zone: 0 }
        ZoneTrack { zone: 1 }
        ZoneTrack { zone: 2 }
        ZoneTrack { zone: 0; inner: true }
        ZoneTrack { zone: 1; inner: true }
        ZoneTrack { zone: 2; inner: true }
        ValueArc { }
        ValueArc { inner: true }

        ShapePath {
            id: fillerTrack
            fillColor: "transparent"
            strokeWidth: root.fillerStroke
            strokeColor: root.paintOnCard(root.fillerColor, 0.34)
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: root.cx
                centerY: root.fillerCy
                radiusX: root.fillerRadius
                radiusY: root.fillerRadius
                startAngle: root.startAngle
                sweepAngle: root.sweepAngle
            }
        }

        ShapePath {
            id: fillerValueArc
            fillColor: "transparent"
            strokeWidth: root.hasValue && root.fillerFraction > 0.004 ? root.fillerValueStroke : 0
            strokeColor: root.fillerColor
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: root.cx
                centerY: root.fillerCy
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
            visible: !root.dual && tickRadius > 8
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
        width: root.innerStroke * 1.35
        height: width
        radius: width / 2
        x: center.x - width / 2
        y: center.y - height / 2
        color: root.cardColor
        border.width: Math.max(2, root.innerStroke * 0.28)
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
        readonly property point center: root.pointAt(root.fillerAngle, root.fillerRadius, root.fillerCy)
        visible: root.hasValue
        width: root.fillerStroke * 1.45
        height: width
        radius: width / 2
        x: center.x - width / 2
        y: center.y - height / 2
        color: root.cardColor
        border.width: Math.max(2, root.fillerStroke * 0.28)
        border.color: root.fillerColor
    }

    Column {
        id: readout
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: root.plotInset / 2
        y: root.cy - root.radius * (root.dual ? 0.46 : 0.42) - height / 2
        spacing: root.dual ? 0 : 2

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: !root.hasValue || root.showNumbers
            text: root.hasValue ? root.animatedValue.toFixed(0) : "– –"
            font.pixelSize: root.dual
                             ? Math.max(16, Math.min(28, root.innerRadius * 0.34))
                             : Math.max(22, root.radius * 0.28)
            font.weight: root.hasValue ? Font.DemiBold : Font.Light
            font.features: { "tnum": 1 }
            color: root.hasValue ? Theme.onSurface(Material.theme) : Theme.outline(Material.theme)
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.dual
            width: Math.max(48, root.innerRadius * 1.4)
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            text: root.metricLabel
            font.pixelSize: AppScale.fs(11)
            font.weight: Font.DemiBold
            color: root.hasValue && root.value > 0 ? root.arcColor : Theme.onSurfaceVariant(Material.theme)
            Accessible.ignored: true
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.dual && (!root.hasValue || root.showNumbers)
            topPadding: 2
            text: root.hasValue ? root.animatedSecond.toFixed(0) : "– –"
            font.pixelSize: Math.max(16, Math.min(28, root.innerRadius * 0.34))
            font.weight: root.hasValue ? Font.DemiBold : Font.Light
            font.features: { "tnum": 1 }
            color: root.hasValue ? Theme.onSurface(Material.theme) : Theme.outline(Material.theme)
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.dual
            width: Math.max(48, root.innerRadius * 1.4)
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            text: root.secondMetricLabel
            font.pixelSize: AppScale.fs(11)
            font.weight: Font.DemiBold
            color: root.hasValue && root.secondValue > 0
                   ? root.secondArcColor
                   : Theme.onSurfaceVariant(Material.theme)
            Accessible.ignored: true
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.unit
            font.pixelSize: AppScale.fs(root.dual ? 12 : 13)
            color: Theme.onSurfaceVariant(Material.theme)
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

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: !root.dual
            text: root.metricLabel
            font.pixelSize: AppScale.fs(12)
            font.weight: Font.DemiBold
            color: root.hasValue && root.value > 0 ? root.arcColor : Theme.onSurface(Material.theme)
            Accessible.ignored: true
        }
    }

    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: root.plotInset / 2
        y: root.fillerCy - root.fillerRadius * 0.62 - height / 2
        spacing: 0

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: !root.hasValue || root.showNumbers
            text: root.hasValue ? root.animatedFiller.toFixed(0) : "– –"
            font.pixelSize: Math.max(16, Math.min(26, root.fillerRadius * 0.42))
            font.weight: root.hasValue ? Font.DemiBold : Font.Light
            font.features: { "tnum": 1 }
            color: root.hasValue ? Theme.onSurface(Material.theme) : Theme.outline(Material.theme)
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTranslate("HomePage", "Fillers")
            font.pixelSize: AppScale.fs(11)
            font.weight: Font.DemiBold
            color: root.fillerColor
        }
    }

    Text {
        readonly property point tip: root.pointAt(root.startAngle, root.radius)
        x: tip.x - width / 2
        y: tip.y + root.stroke / 2 + 2
        text: root.minimum.toFixed(0)
        font.pixelSize: AppScale.fs(12)
        color: Theme.onSurfaceVariant(Material.theme)
    }

    Text {
        readonly property point tip: root.pointAt(root.startAngle + root.sweepAngle, root.radius)
        x: tip.x - width / 2
        y: tip.y + root.stroke / 2 + 2
        text: root.maximum.toFixed(0)
        font.pixelSize: AppScale.fs(12)
        color: Theme.onSurfaceVariant(Material.theme)
    }

    Text {
        readonly property point tip: root.pointAt(root.startAngle, root.fillerRadius, root.fillerCy)
        x: tip.x - width / 2
        y: tip.y + root.fillerStroke / 2 + 1
        text: "0"
        font.pixelSize: AppScale.fs(11)
        color: Theme.onSurfaceVariant(Material.theme)
    }

    Text {
        readonly property point tip: root.pointAt(root.startAngle + root.sweepAngle, root.fillerRadius, root.fillerCy)
        x: tip.x - width / 2
        y: tip.y + root.fillerStroke / 2 + 1
        text: "100"
        font.pixelSize: AppScale.fs(11)
        color: Theme.onSurfaceVariant(Material.theme)
    }
}
