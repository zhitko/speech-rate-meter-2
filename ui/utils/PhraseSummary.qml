pragma Singleton
import QtQuick

// Names for Settings → Measurement → Session summary. Home and History use the
// same labels after Stop.
QtObject {
    // Median. Matches AppSettings::summaryMethod.
    readonly property int defaultId: 2

    readonly property var choices: [
        { name: qsTr("Weighted mean"), id: 0 },
        { name: qsTr("Mean"), id: 1 },
        { name: qsTr("Median"), id: 2 },
        { name: qsTr("Weighted median"), id: 3 },
        { name: qsTr("Harmonic mean"), id: 4 },
        { name: qsTr("Geometric mean"), id: 5 },
        { name: qsTr("Trimmed mean"), id: 6 },
        { name: qsTr("Quadratic mean"), id: 7 }
    ]

    function title(method) {
        const id = method === undefined || method === null ? 0 : method
        for (let i = 0; i < choices.length; ++i) {
            if (choices[i].id === id)
                return choices[i].name
        }
        return choices[0].name
    }
}
