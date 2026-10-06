import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material 6.8

SpinBox {
    id: root
    property int decimals: 2
    property real realValue: value / 100.0
    signal realValueEdited(real value)

    from: 0
    to: 100000
    stepSize: 1
    editable: true

    valueFromText: function(text) {
        var parsed = parseFloat(String(text).replace(",", "."))
        if (isNaN(parsed))
            return root.value
        return Math.round(parsed * 100)
    }
    textFromValue: function(value) {
        return (value / 100).toFixed(root.decimals)
    }
    onValueModified: realValueEdited(value / 100.0)
}
