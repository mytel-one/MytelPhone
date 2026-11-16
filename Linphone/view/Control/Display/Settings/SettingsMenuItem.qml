import QtQuick
import QtQuick.Controls.Basic as Control
import QtQuick.Layouts
import QtQuick.Effects
import Linphone
import CustomControls 1.0
import 'qrc:/qt/qml/Linphone/view/Control/Tool/Helper/utils.js' as Utils

Item {
	id: mainItem
    height: visible ? Utils.getSizeWithScreenRatio(descriptionText.length > 0 ? 76 : 52) : 0
	anchors.right: parent.right
	anchors.left: parent.left
	property bool keyboardOtherFocus: FocusHelper.keyboardFocus || FocusHelper.otherFocus

	property string titleText
    property string descriptionText: ""
	property bool isSelected: false
	
	signal selected()

	//: %1 settings
	Accessible.name: qsTr("setting_tab_accessible_name").arg(titleText)
	Accessible.role: Accessible.ListItem
    Accessible.description: descriptionText
	
	Keys.onPressed: (event)=>{
		if(event.key == Qt.Key_Space || event.key == Qt.Key_Return || event.key == Qt.Key_Enter){
			mainItem.selected()
		}
	}
	MouseArea {
		id: mouseArea
		hoverEnabled: true
		cursorShape: Qt.PointingHandCursor
		anchors.fill: parent
		Rectangle {
			id: background
			anchors.fill: parent
			color: mainItem.isSelected ? DefaultStyle.main2_200 : parent.containsMouse ? DefaultStyle.main2_100 : "transparent"
            radius: mainItem.height / 2
			bottomRightRadius: 0
			topRightRadius: 0
			visible: parent.containsMouse || mainItem.isSelected || mainItem.keyboardOtherFocus
			border.color: DefaultStyle.main2_900
			border.width: mainItem.keyboardOtherFocus ? Utils.getSizeWithScreenRatio(3) : 0
		}
		onClicked: {
			mainItem.selected()
		}
	}
	ColumnLayout {
		anchors.margins: 20
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.verticalCenter: parent.verticalCenter
        spacing: 3
        Text {
            Layout.fillWidth: true
            text: mainItem.titleText
            color: DefaultStyle.main2_800
            font.pixelSize: 16
            font.weight: mainItem.isSelected ? Font.DemiBold : Font.Medium
            elide: Text.ElideRight
        }
        Text {
            visible: text.length > 0
            Layout.fillWidth: true
            text: mainItem.descriptionText
            color: DefaultStyle.main2_600
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }
	}
	
}
