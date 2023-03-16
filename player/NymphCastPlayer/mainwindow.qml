//import related modules
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

//window containing the application
ApplicationWindow {

    visible: true

    //title of the application
    title: qsTr("NymphCast Player")

    //menu containing two menu items
    menuBar: MenuBar {
        Menu {
            title: qsTr("File")
            MenuItem {
                text: qsTr("Exit")
                onTriggered: Qt.quit();
            }
        }
        
        Menu {
            title: qsTr("Cast")
            MenuItem {
                text: qsTr("File")
            }
            MenuItem {
                text: qsTr("URL")
            }
        }
    }

    //Content Area
    GridLayout {
        id: grid 
        columns: 1
        anchors.fill: parent

        RowLayout {
            //
            anchors.fill: parent
            spacing: 6
            Label {
                text: qsTr("Remote")
            }
            
            ComboBox {
                id: remotesComboBox
                model: remotesModel
            }
            
            Button {
                id: refreshRemotesButton
                icon.source: "icons/repeat.png"
            }
            
            Button {
                id: editRemotesButton
                text: "..."
            }
        }
        
        
        TabBar {
            id: bar
            TabButton {
                text: qsTr("Player")
            }
            TabButton {
                text: qsTr("Shares")
            }
            TabButton {
                text: qsTr("Apps")
            }
            TabButton {
                text: qsTr("Apps (GUI)")
            }
        }
        
        StackLayout {
            width: parent.width
            currentIndex: bar.currentIndex
            
            Item {
                id: playerTab
					
				ColumnLayout {
				RowLayout {
					Label {
						id: remotesLabel
						text: qsTr("No remote selected.")
					}
					
					ListView {
						model: tracksModel
						delegate: ItemDelegate {
							text: "Item " + index
						}
					}
                }
				
				RowLayout {
						CheckBox {
							id: singleCheckBox
							text: "Single"
							checkState: Qt.Checked
						}
						
						CheckBox {
							id: repeatCheckBox
							text: "Repeat"
						}
						
						Button {
							id: addButton
							text: "Add"
							icon.source: "icons/add.png"
						}
						
						Button {
							id: removeButton
							text: "Remove"
							icon.source: "icons/minus.png"
						}
					}
				}
				
				RowLayout {
					anchors.fill: parent
					
					ProgressBar {
						id: progressBar
						x: 93
						y: 234
						value: 0.5
					}
				
					Label {
						id: progressLabel
						text: "00:00/00:00"
					}
				}
                
                //a button in the middle of the content area
                /*Button {
                    text: qsTr("Hello World")
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.verticalCenterOffset: 53
                    anchors.horizontalCenterOffset: 50
                }*/
            }
            Item {
                id: sharesTab
            }
            Item {
                id: appsTab
            }
            Item {
                id: appsGuiTab
            }
        }
    }
    
}

/*##^##
Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
##^##*/
