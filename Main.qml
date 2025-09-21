import QtQuick

Window {
    width: 1080
    height: 1920
    visible: true
    title: qsTr("Boost Example")

    Text {
        text: "Boost DateTime Example:\nCurrent time: " + boostTime
        font.family: "Helvetica"
        font.pointSize: 24
        color: "blue"
        anchors.centerIn: parent
    }
}