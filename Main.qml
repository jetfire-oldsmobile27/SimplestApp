import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Window {
    id: root
    width: 1080
    height: 1800
    visible: true
    title: qsTr("Camera OpenCV Example")

    Component.onCompleted: {
        cameraController.errorOccured.connect(function(msg) {
            console.error("CameraController error: " + msg)
            // показать всплывающее уведомление
            errorText.text = msg
            errorOverlay.visible = true
        })
    }

    Rectangle {
        id: errorOverlay
        visible: false
        anchors.fill: parent
        color: "#88000000"
        Text { id: errorText; anchors.centerIn: parent; color: "white" }
        MouseArea { anchors.fill: parent; onClicked: errorOverlay.visible = false }
    }

    Rectangle {
        id: container
        anchors.fill: parent
        color: "black"

        // Видео (источник - data url от C++)
        Image {
            id: videoImage
            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop
            source: cameraController.frameData   // data:image/jpeg;base64,...
            cache: false
            asynchronous: true
            smooth: true
        }

        // Overlay: снизу — карусель камер + кнопки
        Rectangle {
            id: controls
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 120
            color: "#88000000"
            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                // Список камер (горизонтальный список - карусель)
                ListView {
                    id: camList
                    Layout.preferredWidth: parent.width * 0.65
                    Layout.alignment: Qt.AlignVCenter
                    model: cameraController.cameraIds
                    orientation: ListView.Horizontal
                    boundsBehavior: Flickable.StopAtBounds
                    spacing: 8
                    delegate: Rectangle {
                        width: 80; height: 80
                        radius: 8
                        color: ListView.isCurrentItem ? "#ff5555" : "#ffffff33"
                        border.color: "white"
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: "ID " + modelData
                            color: "white"
                            font.bold: true
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                camList.currentIndex = index
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.preferredWidth: parent.width * 0.35 - 16
                    spacing: 6
                    Button {
                        text: "Refresh"
                        onClicked: cameraController.refreshCameraList()
                    }
                    Button {
                        text: "Open"
                        onClicked: {
                            if (camList.currentIndex >= 0 && camList.count > 0) {
                                var id = parseInt(camList.model.get(camList.currentIndex))
                                cameraController.openCamera(id)
                            }
                        }
                    }
                    Button {
                        text: "Close"
                        onClicked: cameraController.closeCamera()
                    }
                }
            }
        }
    }
}
