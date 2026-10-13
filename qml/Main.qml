import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import Camera 1.0
ApplicationWindow {
    id: window; visible:true; width:1280; height:820; minimumWidth:960; minimumHeight:640; title:"Camera Studio"; color:"#0B0C0F"
    property string toastText: ""; property bool toastVisible: false
        function formatTime(v)
        {function two(x){return x<10?"0"+x:""+x}var h=Math.floor(v/3600), m=Math.floor((v%3600)/60), s=v%60;return two(h)+":"+two(m)+":"+two(s)}
            function toast(s)
            {toastText=s;toastVisible=true;toastTimer.restart()}
                Timer {id:toastTimer;interval:2200;onTriggered:toastVisible=false}
                Connections {target:camera;function onErrorMessageChanged(){if(camera.errorMessage.length)window.toast(camera.errorMessage)}}
                Rectangle {anchors.fill:parent;color:"#0B0C0F"}
                Rectangle {
                    id:view;anchors.left:parent.left;anchors.right:parent.right;anchors.top:parent.top;anchors.bottom:panel.top;anchors.margins:14;anchors.bottomMargin:12;radius:18;color:"#050506";clip:true
                    CameraPreview {id:preview;objectName:"preview";anchors.fill:parent}
                    Rectangle {anchors.left:parent.left;anchors.right:parent.right;anchors.top:parent.top;height:105;gradient:Gradient {GradientStop {position:0;color:"#B0000000"}GradientStop {position:1;color:"#00000000"}}}
                    Row {anchors.left:parent.left;anchors.top:parent.top;anchors.margins:22;spacing:10
                        Rectangle {width:96;height:34;radius:9;color:"#A018191D";Text {anchors.centerIn:parent;text:"CAMERA";color:"white";font.pixelSize:13;font.bold:true}}
                        Rectangle {width:132;height:34;radius:9;color:"#A018191D";Text {anchors.centerIn:parent;text:camera.cameraWidth+" × "+camera.cameraHeight;color:"#E6E6E6";font.pixelSize:13}}
                        Rectangle {width:94;height:34;radius:9;color:"#A018191D";Text {anchors.centerIn:parent;text:camera.fps.toFixed(1)+" FPS";color:"#E6E6E6";font.pixelSize:13}}
                    }
                    Rectangle {anchors.right:parent.right;anchors.top:parent.top;anchors.margins:22;width:camera.recording?160:88;height:36;radius:10;color:camera.recording?"#CCB51F2A":"#A018191D"
                        Row {anchors.centerIn:parent;spacing:9;Rectangle {width:9;height:9;radius:5;color:camera.recording?"white":camera.running?"#42D77D":"#777"}Text {text:camera.recording?"REC  "+window.formatTime(camera.recordSeconds):camera.running?"LIVE":"OFFLINE";color:"white";font.pixelSize:13;font.bold:true}}
                    }
                    Column {anchors.left:parent.left;anchors.bottom:parent.bottom;anchors.margins:24;spacing:3
                        Text {text:camera.cameraName.length?camera.cameraName:"No Camera";color:"white";font.pixelSize:15;font.bold:true}
                        Text {text:camera.cameraPixelFormat+"  •  "+camera.cameraFps+" FPS";color:"#AFC0C0C0";font.pixelSize:12}
                    }
                    Rectangle {anchors.fill:parent;radius:view.radius;color:"transparent";border.width:camera.recording?2:0;border.color:"#E74A54"}
                    Rectangle {id:flash;anchors.fill:parent;color:"white";opacity:0;z:100}
                    SequentialAnimation {id:flashAnim;NumberAnimation {target:flash;property:"opacity";to:.82;duration:65}NumberAnimation {target:flash;property:"opacity";to:0;duration:220}}
                }
                Rectangle {
                    id:panel;anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:150;color:"#111216"
                    Column {anchors.left:parent.left;anchors.leftMargin:32;anchors.verticalCenter:parent.verticalCenter;spacing:6
                        Text {text:camera.recording?"Recording":camera.running?"Camera Ready":"Camera Disconnected";color:"white";font.pixelSize:16;font.bold:true}
                        Text {text:camera.cameraWidth+" × "+camera.cameraHeight+"  •  "+camera.cameraPixelFormat+"  •  "+camera.cameraFps+" FPS";color:"#8D8F98";font.pixelSize:13}
                    }
                    Row {anchors.centerIn:parent;spacing:34
                        Item {width:76;height:76;Rectangle {anchors.fill:parent;radius:38;color:"transparent";border.width:3;border.color:"white";Rectangle {anchors.centerIn:parent;width:60;height:60;radius:30;color:"white";scale:photoMouse.pressed?.88:1;Behavior on scale {NumberAnimation {duration:90}}}MouseArea {id:photoMouse;anchors.fill:parent;cursorShape:Qt.PointingHandCursor;onClicked: {if(camera.takePhoto()){flashAnim.restart();window.toast("Photo Saved")}else window.toast(camera.errorMessage)}}}}
                        Item {width:76;height:76;Rectangle {anchors.fill:parent;radius:38;color:"transparent";border.width:3;border.color:"#70727A";Rectangle {anchors.centerIn:parent;width:camera.recording?28:58;height:camera.recording?28:58;radius:camera.recording?6:29;color:"#F04452"}MouseArea {anchors.fill:parent;cursorShape:Qt.PointingHandCursor;onClicked: {if(camera.recording){camera.stopRecording();window.toast("Video Saved")}else if(camera.startRecording())window.toast("Recording Started");else window.toast(camera.errorMessage)}}}}
                    }
                    Button {
                        id:rescanButton;anchors.right:parent.right;anchors.rightMargin:96;anchors.verticalCenter:parent.verticalCenter;width:128;height:44;text:"Rescan"
                        background:Rectangle {radius:14;color:rescanButton.down?"#2A2D35":rescanButton.hovered?"#242730":"#1A1C22";border.width:1;border.color:"#3A3E49"}
                        contentItem:Text {text:rescanButton.text;color:"white";font.pixelSize:14;font.bold:true;horizontalAlignment:Text.AlignHCenter;verticalAlignment:Text.AlignVCenter}
                        onClicked: {if(camera.rescanCamera())window.toast("Camera Connected");else window.toast(camera.errorMessage)}
                    }
                    ComboBox {
                        id:cameraSelector;anchors.right:rescanButton.left;anchors.rightMargin:10;anchors.verticalCenter:parent.verticalCenter;width:190;height:44
                        model:camera.cameraList;currentIndex:camera.currentCameraIndex;enabled:!camera.recording && camera.cameraList.length>1
                        background:Rectangle {radius:14;color:cameraSelector.enabled?(cameraSelector.down?"#2A2D35":"#1A1C22"):"#121318";border.width:1;border.color:cameraSelector.enabled?"#3A3E49":"#242730"}
                        contentItem:Text {leftPadding:14;rightPadding:34;text:cameraSelector.displayText;color:cameraSelector.enabled?"white":"#777B86";font.pixelSize:13;verticalAlignment:Text.AlignVCenter;elide:Text.ElideRight}
                        onActivated: {if(!camera.switchCamera(index))window.toast(camera.errorMessage)}
                    }
                }
                Rectangle {id:toastBox;anchors.horizontalCenter:parent.horizontalCenter;anchors.bottom:panel.top;anchors.bottomMargin:20;width:toastLabel.width+46;height:46;radius:14;color:"#E5222328";opacity:toastVisible?1:0;visible:opacity>0;z:200;Behavior on opacity {NumberAnimation {duration:180}}Text {id:toastLabel;anchors.centerIn:parent;text:toastText;color:"white";font.pixelSize:14}}
            }
