import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import QtQuick.Controls.Basic as Control
import SettingsCpp
import UtilsCpp

AbstractSettingsMenu {
	id: mainItem
	layoutsPath: "qrc:/qt/qml/Linphone/view/Page/Layout/Settings"
    //: "Paramètres"
    titleText: qsTr("settings_title")
	families: [
        //: "Appels"
        {title: qsTr("Calls & audio"), description: qsTr("Microphone, speakers and call preferences"), layout: "CallSettingsLayout"},
		//: "Transfert d'appel"
        {title: qsTr("settings_call_forward"), description: qsTr("Route incoming calls to another number"), layout: "CallForwardSettingsLayout", visible: !SettingsCpp.disableCallForward},
        //: "Conversations"
        {title: qsTr("Chats"), description: qsTr("Messages and conversation preferences"), layout: "ChatSettingsLayout", visible: !SettingsCpp.disableChatFeature},
        //: "Contacts"
        {title: qsTr("settings_contacts_title"), description: qsTr("Address books and contact directories"), layout: "ContactsSettingsLayout"},
        //: "Réunions"
        {title: qsTr("settings_meetings_title"), description: qsTr("Video meeting preferences"), layout: "MeetingsSettingsLayout", visible: !SettingsCpp.disableMeetingsFeature},
        //: "Affichage"
        {title: qsTr("Appearance"), description: qsTr("Light, dark or the system theme"), layout: "DisplaySettingsLayout"},
        //: "Security"
        // {title: qsTr("settings_security_title"), layout: "SecuritySettingsLayout"},
        //: "Réseau"
        {title: qsTr("settings_network_title"), description: qsTr("Connection and network options"), layout: "NetworkSettingsLayout"},
        //: "Paramètres avancés"
        {title: qsTr("settings_advanced_title"), description: qsTr("Extra options for administrators"), layout: "AdvancedSettingsLayout"}
	]

	onGoBackRequested: mainItem.goBack()

	Component.onCompleted: {
		SettingsCpp.isSaved = true
	}
}
