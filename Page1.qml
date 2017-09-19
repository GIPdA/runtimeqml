import QtQuick 2.7

import "./pages/"

Page1Form {
    button1.onClicked: {
        console.log("Button Pressed. Entered text: " + textField1.text);

        window.show()
    }
    Page4 {
        anchors.topMargin: 200
        anchors.fill: parent
    }

    Page3 {
        id: window
    }
}
