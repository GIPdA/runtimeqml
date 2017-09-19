import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0
import QtQuick.Window 2.0

import "./../"

Window {
    width: 200
    height: 200

    Label {
        text: qsTr("Third3 page")
        anchors.centerIn: parent
    }

    Page4 {
        anchors.topMargin: 200
        anchors.fill: parent
    }
}
