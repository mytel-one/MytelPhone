/**
* Qml template used for welcome and login/register pages
**/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as Control

import Linphone
import ConstantsCpp
import 'qrc:/qt/qml/Linphone/view/Control/Tool/Helper/utils.js' as Utils
import 'qrc:/qt/qml/Linphone/view/Style/buttonStyle.js' as ButtonStyle

Rectangle {
	id: mainItem
	property alias titleContent : titleLayout.children
	property alias centerContent : centerLayout.children
	color: DefaultStyle.background

	component AboutLine: RowLayout {
		id: line
        spacing: Utils.getSizeWithScreenRatio(20)
		property var imageSource
		property string title
		property string text
		property bool enableMouseArea: false
		signal contentClicked()
		EffectImage {
            Layout.preferredWidth: Utils.getSizeWithScreenRatio(32)
            Layout.preferredHeight: Utils.getSizeWithScreenRatio(32)
			imageSource: parent.imageSource
			colorizationColor: DefaultStyle.main1_500_main
		}
		ColumnLayout {
			spacing: 0
			Text {
				Layout.fillWidth: true
				text: line.title
				color: DefaultStyle.main2_600
				font {
                    pixelSize: Typography.b2.pixelSize
                    weight: Typography.b2.weight
				}
				horizontalAlignment: Layout.AlignLeft
			}
			Text {
				id: content
				Layout.fillWidth: true
				text: line.text
				color: DefaultStyle.main2_500_main
                font.pixelSize: Utils.getSizeWithScreenRatio(14)
				horizontalAlignment: Layout.AlignLeft
				Keys.onPressed: (event)=> {
					if (event.key == Qt.Key_Space || event.key == Qt.Key_Enter || event.key == Qt.Key_Return) {
						line.contentClicked(undefined)
						event.accepted = true;
					}
				}
				MouseArea {
					id: privateMouseArea
					enabled: line.enableMouseArea
					anchors.fill: parent
					hoverEnabled: true
					cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
					onClicked: line.contentClicked()
				}
			}
		}
		// Item {Layout.fillWidth: true}
	}

    Dialog {
        id: aboutPopup
        parent: Control.Overlay.overlay
        width: Math.min(760, parent ? parent.width - 40 : 760)
        title: qsTr("About MyTel")
        content: Control.ScrollView {
            Layout.preferredWidth: aboutPopup.width - aboutPopup.leftPadding - aboutPopup.rightPadding
            Layout.preferredHeight: Math.max(160, Math.min(500, aboutPopup.parent ? aboutPopup.parent.height - 240 : 500))
            contentWidth: availableWidth
            contentHeight: aboutContent.implicitHeight
            clip: true
            MyTelAboutContent {
                id: aboutContent
                width: parent.width
            }
        }
        buttons: [MediumButton {
            text: qsTr("Close")
            style: ButtonStyle.main
            onClicked: aboutPopup.close()
        }]
    }

	RowLayout {
		anchors.fill: parent
		spacing: 0

		// MyTel brand pane (design approved by technical lead 2026-11-06):
		// navy pane with logo + wordmark on wide windows.
		Rectangle {
			id: brandPane
			visible: mainItem.width >= 760
			Layout.preferredWidth: Math.round(mainItem.width * 0.4)
			Layout.fillHeight: true
			color: "#001622"
			ColumnLayout {
				anchors.centerIn: parent
				spacing: 12
				Image {
					Layout.alignment: Qt.AlignHCenter
					source: "image://internal/mytel_logo.png"
					fillMode: Image.PreserveAspectFit
					Layout.preferredWidth: 96
					Layout.preferredHeight: 96
				}
				Text {
					Layout.alignment: Qt.AlignHCenter
					text: applicationName
					color: "#E8F1FC"
					font.pixelSize: 32
					font.weight: Font.Bold
				}
				Rectangle {
					Layout.alignment: Qt.AlignHCenter
					Layout.preferredWidth: 64
					Layout.preferredHeight: 4
					color: "#FFAB2E"
					radius: 2
				}
				Text {
					Layout.alignment: Qt.AlignHCenter
					text: "Calls and messages for your team"
					color: "#FFAB2E"
					font.pixelSize: 14
				}
			}
		}

		ColumnLayout {
			Layout.fillWidth: true
			Layout.fillHeight: true
			spacing: 0

			// Compact brand header for narrow windows
			RowLayout {
				visible: mainItem.width < 760
				Layout.fillWidth: true
				Layout.topMargin: 20
				spacing: 12
				Image {
					Layout.alignment: Qt.AlignHCenter
					Layout.leftMargin: 24
					source: "image://internal/mytel_logo.png"
					fillMode: Image.PreserveAspectFit
					Layout.preferredWidth: 64
					Layout.preferredHeight: 64
				}
				Text {
					text: applicationName
					color: DefaultStyle.main2_900
					font.pixelSize: 24
					font.weight: Font.Bold
				}
				Item { Layout.fillWidth: true }
			}

			RowLayout {
				Layout.fillWidth: true
				Layout.topMargin: Math.max(Utils.getSizeWithScreenRatio(5), Utils.getSizeWithScreenRatio(25 - ((25/(DefaultStyle.defaultHeight - mainWindow.minimumHeight))*(DefaultStyle.defaultHeight-mainWindow.height))))
				Layout.rightMargin: Math.max(Utils.getSizeWithScreenRatio(5), Utils.getSizeWithScreenRatio(42 - ((42/(DefaultStyle.defaultWidth - mainWindow.minimumWidth))*(DefaultStyle.defaultWidth-mainWindow.width))))
				spacing: 0
				Item {
					Layout.fillWidth: true
				}
				BigButton {
					id: aboutButton
					Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
					icon.source: AppIcons.info
					text: qsTr("help_about_title").arg(applicationName)
					textSize: Typography.p1.pixelSize
					textWeight: Typography.p1.weight
					textColor: DefaultStyle.main2_500_main
					onClicked: aboutPopup.open()
					style: ButtonStyle.noBackground
				}
			}

			RowLayout {
				id: titleLayout
				Layout.fillWidth: true
				Layout.topMargin: Math.max(Utils.getSizeWithScreenRatio(10), Utils.getSizeWithScreenRatio(40 - ((40/(DefaultStyle.defaultHeight - mainWindow.minimumHeight))*(DefaultStyle.defaultHeight-mainWindow.height))))
				spacing: 0
			}
			Item {
				id: centerLayout
				Layout.fillHeight: true
				Layout.fillWidth: true
				Layout.topMargin: Math.max(Utils.getSizeWithScreenRatio(15), Utils.getSizeWithScreenRatio(70 - ((70/(DefaultStyle.defaultHeight - mainWindow.minimumHeight))*(DefaultStyle.defaultHeight-mainWindow.height))))
				Layout.alignment: Qt.AlignBottom
			}
		}
	}

}

