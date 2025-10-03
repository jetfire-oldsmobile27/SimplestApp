import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Window {
    id: root
    visible: true
    title: qsTr("Camera OpenCV Example")

    property bool cameraBusy: false


    Component.onCompleted: {
    cameraController.errorOccured.connect(function (msg) {
    console.log(">>> ERROR RECEIVED IN QML:", msg);
    if (errorDialog.visible) errorDialog.close();
    errorText.text = msg;
    errorDialog.open();
    root.cameraBusy = false;
    });


}

    
    Rectangle {
        anchors.fill: parent
        color: "black"

        BusyIndicator {
    anchors.centerIn: parent
    running: root.cameraBusy
    visible: root.cameraBusy
    width: 60; height: 60
}

        Image {
            id: videoImage
            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop
            source: cameraController.frameData
            cache: false
            asynchronous: true
            smooth: true
        }


        Column {
            id: controls
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.margins: 10
            spacing: 8


            ListView {
                id: camList
                width: Math.min(root.width * 0.9, 400)
                height: 60
                model: cameraController.cameraIds
                orientation: ListView.Horizontal
                spacing: 6
                clip: true
                delegate: Rectangle {
                    width: 70
                    height: 50
                    radius: 6
                    color: ListView.isCurrentItem ? "#ff5555" : "#44ffffff"
                    border.color: "white"
                    Text {
                        anchors.centerIn: parent
                        text: "ID " + modelData
                        color: "white"
                        font.pixelSize: 14
                        font.bold: true
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: camList.currentIndex = index
                    }
                }
            }


            Row {
                spacing: 6
                Button {
    text: "Refresh"
    enabled: !root.cameraBusy
    onClicked: {
        
        cameraController.refreshCameraList();
    }
}
                Button {
    text: "Open"
    enabled: !root.cameraBusy && camList.currentIndex >= 0
    onClicked: {
        root.cameraBusy = true;
        var id = parseInt(cameraController.cameraIds[camList.currentIndex]);
        cameraController.openCamera(id);
    }
}
                Button {
    text: "Close"
    enabled: !root.cameraBusy && cameraController.cameraIds.length > 0
    onClicked: {
        root.cameraBusy = true;
        cameraController.closeCamera();
    }
}
            }

            Row {
                spacing: 6
                Button { text: "Logs"; onClicked: logsDialog.open() }
                Button { text: "Clear"; onClicked: cameraController.clearLogs() }
            }
        }
    }


    Dialog {
    id: errorDialog
    modal: true
    title: "Error"
    standardButtons: Dialog.Ok
    contentWidth: Math.min(Screen.width * 0.9, 600)


    Text {
        id: errorText
        text: ""
        wrapMode: Text.WrapAnywhere
        width: parent.width - 20
        color: "white"
        font.pixelSize: 14
    }
}


    Dialog {
        id: logsDialog
        modal: true
        title: "Logs"
        standardButtons: Dialog.Ok
        contentWidth: Math.min(Screen.width * 0.95, 800)
        contentHeight: Math.min(Screen.height * 0.7, 900)

        ScrollView {
            anchors.fill: parent
            TextArea {
                readOnly: true
                text: cameraController.logs.join("\n")
                wrapMode: TextEdit.Wrap
                background: Rectangle { color: "#111111" }
                color: "white"
                font.pixelSize: 12
                selectByMouse: true
            }
        }
    }
}