import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import QtQuick.Controls.Basic as Control
import Linphone
import UtilsCpp
import SettingsCpp
import "qrc:/qt/qml/Linphone/view/Style/buttonStyle.js" as ButtonStyle
import "qrc:/qt/qml/Linphone/view/Control/Tool/Helper/utils.js" as Utils

AbstractMainPage {
    id: mainItem
    //: "Nouvel appel"
    noItemButtonText: qsTr("history_call_start_title")
    //: "Historique d'appel vide"
    emptyListText: qsTr("call_history_empty_title")
    newItemIconSource: AppIcons.newCall

    property var selectedRowHistoryGui
    signal listViewUpdated
    signal goToCallForwardSettings

    onVisibleChanged: if (!visible) {
        goToCallHistory()
    }

    // ── MyTel floating dial pad (owner 2026-11-13) ──
    // Prominent green FAB opens an instant dial popup — always visible
    // on the Calls page, no navigation needed.
    function mytelNormalizeDial(raw) {
        return Utils.normalizeSipDestination(raw)
    }

    readonly property var dialSearchBar: listStackView.currentItem?.objectName === "historyListItem"
                                            ? listStackView.currentItem.dialSearchBar : null

    function placeDialedCall(raw) {
        var destination = mytelNormalizeDial(raw === undefined ? dialSearchBar?.inputText : raw)
        if (!destination) {
            UtilsCpp.showInformationPopup(qsTr("information_popup_error_title"),
                                          qsTr("mytel_chat_invalid_destination"), false)
            return
        }
        numericPadPopupItem.close()
        UtilsCpp.createCall(destination)
    }

    function openDialPad() {
        goToCallHistory()
        Qt.callLater(function() {
            if (mainItem.dialSearchBar) {
                mainItem.dialSearchBar.clearText()
                numericPadPopupItem.open()
            }
        })
    }

    Button {
        id: mytelDialFab
        z: 1000
        width: 56
        height: 56
        radius: 28
        anchors.right: parent.right
        anchors.rightMargin: 24
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        style: ButtonStyle.phoneGreen
        icon.source: AppIcons.dialer
        icon.width: 26
        icon.height: 26
        Accessible.name: qsTr("open_dialer_acccessibility_label")
        onClicked: mainItem.openDialPad()
    }

    /**
     * Focus on the first pertinent element in the page (LINQT-2202)
     * @override
     */
    function forceActiveFocus(reason = undefined){
        listStackView.currentItem?.newCallButton?.forceActiveFocus(reason)
    }

    //Group call properties
    property ConferenceInfoGui confInfoGui
    property AccountProxy accounts: AccountProxy {
        id: accountProxy
    }
    property AccountGui account: accountProxy.defaultAccount
    property bool isRegistered: account ? account.core?.registrationState
                                          == LinphoneEnums.RegistrationState.Ok : false
    property int selectedParticipantsCount
    signal createCallFromSearchBarRequested
    // MyTel (owner 2026-11-13): Enter in the search bar / numeric-pad Call
    // button actually places a call to the typed number — the handler was
    // lost when the old NewCallForm was replaced
    onCreateCallFromSearchBarRequested: mainItem.placeDialedCall()
    signal createContactRequested(string name, string address)
    signal openNumPadRequest

    property alias numericPadPopup: numericPadPopupItem

    Connections {
        enabled: confInfoGui
        target: confInfoGui ? confInfoGui.core : null
        function onConferenceSchedulerStateChanged() {
            if (confInfoGui.core.schedulerState === LinphoneEnums.ConferenceSchedulerState.Ready) {
                listStackView.pop()
            }
        }
    }

    onSelectedRowHistoryGuiChanged: {
        if (selectedRowHistoryGui)
            rightPanelStackView.replace(contactDetailComp,
                                        Control.StackView.Immediate)
        else
            rightPanelStackView.replace(emptySelection,
                                        Control.StackView.Immediate)
    }
    rightPanelStackView.initialItem: emptySelection
    rightPanelStackView.width: Utils.getSizeWithScreenRatio(360)

    onNoItemButtonPressed: goToNewCall()

    showDefaultItem: listStackView.currentItem
                     && listStackView.currentItem.objectName == "historyListItem"
                     && listStackView.currentItem.listView.count === 0 || false

    function goToNewCall() {
        openDialPad()
    }
    function goToCallHistory() {
        if (listStackView.currentItem
                && listStackView.currentItem.objectName != "historyListItem")
            listStackView.replace(historyListItem)
    }

    Dialog {
        id: deleteHistoryPopup
        width: Utils.getSizeWithScreenRatio(637)
        //: Supprimer l\'historique d\'appels ?
        title: qsTr("history_dialog_delete_all_call_logs_title")
        //: "L'ensemble de votre historique d'appels sera définitivement supprimé."
        text: qsTr("history_dialog_delete_all_call_logs_message")
    }
    Dialog {
        id: deleteForUserPopup
        width: Utils.getSizeWithScreenRatio(637)
        //: Supprimer l'historique d\'appels ?
        title: qsTr("history_dialog_delete_call_logs_title")
        //: "L\'ensemble de votre historique d\'appels avec ce correspondant sera définitivement supprimé."
        text: qsTr("history_dialog_delete_call_logs_message")
    }

    leftPanelContent: Item {
        id: leftPanel
        Layout.fillWidth: true
        Layout.fillHeight: true

        Control.StackView {
            id: listStackView
            anchors.fill: parent
            anchors.leftMargin: Utils.getSizeWithScreenRatio(45)
            clip: true
            initialItem: historyListItem
            focus: true
            onActiveFocusChanged: if (activeFocus) {
                currentItem.forceActiveFocus()
            }
        }

        Item {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: Utils.getSizeWithScreenRatio(402)
            NumericPadPopup {
                id: numericPadPopupItem
                width: parent.width
                height: parent.height
                visible: false
                onLaunchCall: {
                    mainItem.createCallFromSearchBarRequested()
                }
            }
        }
    }

    Component {
        id: historyListItem
        FocusScope {
            objectName: "historyListItem"
            property alias listView: historyListView
            property alias dialSearchBar: searchBar
            property alias newCallButton: newCallButton

            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                RowLayout {
                    id: titleCallLayout
                    // direction: FlexboxLayout.Row
                    spacing: Utils.getSizeWithScreenRatio(16)
                    // alignItems: FlexboxLayout.AlignCenter
                    Layout.rightMargin: Utils.getSizeWithScreenRatio(39)
                    Layout.fillHeight: false
                    Text {
                        Layout.fillWidth: true
                        //: "Appels"
                        text: qsTr("call_history_call_list_title")
						maximumLineCount: 1
                        color: DefaultStyle.main2_700
                        font.pixelSize: Typography.h2.pixelSize
                        font.weight: Typography.h2.weight
                    }
                    PopupButton {
                        id: removeHistory
                        icon.width: Utils.getSizeWithScreenRatio(24)
                        icon.height: Utils.getSizeWithScreenRatio(24)
                        focus: true
                        popup.x: 0
                        KeyNavigation.right: newCallButton
                        KeyNavigation.down: listStackView
                        //: Call history options
                        popUpTitle: qsTr("call_history_list_options_accessible_name")
                        popup.contentItem: ColumnLayout {
                            IconLabelButton {
                                Layout.fillWidth: true
                                focus: visible
                                //: "Supprimer l'historique"
                                text: qsTr("menu_delete_history")
                                icon.source: AppIcons.trashCan
                                style: ButtonStyle.hoveredBackgroundRed
                                onClicked: {
                                    removeHistory.close()
                                    deleteHistoryPopup.open()
                                }
                            }
                        }
                        Connections {
                            target: deleteHistoryPopup
                            onAccepted: {
                                if (listStackView.currentItem.listView)
                                    listStackView.currentItem.listView.model.removeAllEntries()
                            }
                        }
                    }
                    Button {
                        id: newCallButton
                        focus: true
                        style: ButtonStyle.noBackground
                        icon.source: AppIcons.newCall
                        Layout.preferredWidth: Utils.getSizeWithScreenRatio(34)
                        Layout.preferredHeight: Utils.getSizeWithScreenRatio(34)
                        icon.width: Utils.getSizeWithScreenRatio(28)
                        icon.height: Utils.getSizeWithScreenRatio(28)
                        KeyNavigation.left: removeHistory
                        KeyNavigation.down: listStackView
                        //: Create new call
                        Accessible.name: qsTr("create_new_call_accessible_name")
                        onClicked: {
                            // Owner 2026-11-13: open the big numeric pad
                            // directly — the newCallItem page was redundant
                            console.debug("[CallPage]User: create new call — opening numeric pad")
                            mainItem.openDialPad()
                        }
                    }
                }
                SearchBar {
                    id: searchBar
                    // CRITICAL FIX (owner 2026-11-13): wire the numeric pad
                    // popup to the search bar so digits pressed on the pad
                    // actually land in this field — without this, the pad's
                    // digits go nowhere and Call sends an empty number
                    numericPadPopup: numericPadPopupItem
                    magnifierVisible: !numericPadPopupItem.visible
                    Layout.fillWidth: true
                    Layout.topMargin: Utils.getSizeWithScreenRatio(18)
                    Layout.rightMargin: Utils.getSizeWithScreenRatio(39)
                    //: "Rechercher un appel"
                    placeholderText: qsTr("call_search_in_history")
                    // Always visible so numeric-pad digits are seen by the user
                    visible: historyListView.count !== 0 || text.length !== 0 || numericPadPopupItem.visible
                    focus: true
                    KeyNavigation.up: newCallButton
                    KeyNavigation.down: historyListView
                    Binding {
                        target: mainItem
                        property: "showDefaultItem"
                        when: searchBar.text.length != 0
                        value: false
                    }
                    onValidated: (text) => mainItem.placeDialedCall(text)
                }
                Button {
                    visible: SettingsCpp.callForwardToAddress.length > 0 && !SettingsCpp.disableCallForward
                    Layout.fillWidth: true
                    Layout.preferredHeight: Utils.getSizeWithScreenRatio(40)
                    Layout.topMargin: Utils.getSizeWithScreenRatio(18)
                    Layout.rightMargin: Utils.getSizeWithScreenRatio(39)
                    radius: Utils.getSizeWithScreenRatio(25)
                    color: "transparent"
                    borderColor: DefaultStyle.warning_500_main
                    borderWidth: Utils.getSizeWithScreenRatio(2)
                    text: qsTr("call_forward_to_address_info") + (SettingsCpp.callForwardToAddress == 'voicemail' ? qsTr("call_forward_to_address_info_voicemail") : SettingsCpp.callForwardToAddress)
                    textColor: DefaultStyle.warning_500_main
                    textSize: Typography.p1.pixelSize
                    textWeight: Typography.p1.weight
                    icon.source: AppIcons.callForward
                    contentImageColor: DefaultStyle.warning_500_main
                    icon.width: Utils.getSizeWithScreenRatio(24)
                    icon.height: Utils.getSizeWithScreenRatio(24)

                    onClicked: {
                        goToCallForwardSettings()
                    }
                }
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Control.Control {
                        id: listLayout
                        anchors.fill: parent
                        anchors.rightMargin: Utils.getSizeWithScreenRatio(39)
                        padding: 0
                        background: Item {}
                        contentItem: ColumnLayout {
                            Text {
                                visible: historyListView.count === 0 && !historyListView.loading
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignHCenter
                                Layout.alignment: Qt.AlignHCenter
                                maximumLineCount: 2
                                Layout.topMargin: Utils.getSizeWithScreenRatio(137)
                                //: "Aucun résultat…"
                                text: searchBar.text.length != 0 ? qsTr("list_filter_no_result_found")
                                                                   //: "Aucun appel dans votre historique"
                                                                 : qsTr("history_list_empty_history")
                                font {
                                    pixelSize: Typography.h4.pixelSize
                                    weight: Typography.h4.weight
                                }
                            }
                            CallHistoryListView {
                                id: historyListView
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.topMargin: Utils.getSizeWithScreenRatio(38)
                                searchBar: searchBar
                                Control.ScrollBar.vertical: scrollbar

                                Connections {
                                    target: mainItem
                                    function onListViewUpdated() {
                                        historyListView.model.reload()
                                    }
                                }
                                Binding {
                                    target: mainItem
                                    property: "showDefaultItem"
                                    when: historyListView.loading
                                    value: false
                                }
                                onCurrentIndexChanged: {
                                    mainItem.selectedRowHistoryGui = model.getAt(
                                                currentIndex)
                                }
                                onCountChanged: {
                                    mainItem.selectedRowHistoryGui = model.getAt(
                                                currentIndex)
                                }
                            }
                        }
                    }
                    ScrollBar {
                        id: scrollbar
                        visible: historyListView.contentHeight > parent.height
                        active: visible
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                        anchors.rightMargin: Utils.getSizeWithScreenRatio(8)
                        policy: Control.ScrollBar.AsNeeded
                    }
                }

            }
        }
    }

    Component {
        id: newCallItem
        // MyTel (owner 2026-11-13): completely rewritten dial pad —
        // professional phone-dialer UI, no contact picker, no
        // CreationFormLayout dependencies, no topLayoutVisible bugs
        // Sizes use the PARENT dimensions, not getSizeWithScreenRatio
        // (which uses Screen.width on multi-monitor and overflows).
        FocusScope {
            objectName: "newCallItem"
            width: parent?.width
            height: parent?.height

            // Responsive sizing based on the actual available space
            readonly property real dialUnit: Math.min(width, height * 1.2) / 10

            function normalizeDial(raw) {
                var s = raw.trim()
                if (s.toLowerCase().startsWith("sip:")) return s
                var d = s.replace(/[\s\-\.\(\)]/g, "")
                d = d.replace(/[\u06F0-\u06F9]/g, c => String.fromCharCode(c.charCodeAt(0) - 0x06F0 + 0x30))
                d = d.replace(/[\u0660-\u0669]/g, c => String.fromCharCode(c.charCodeAt(0) - 0x0660 + 0x30))
                if (/^\+98/.test(d)) d = "0" + d.slice(3)
                else if (/^0098/.test(d)) d = "0" + d.slice(4)
                else if (/^098/.test(d)) d = "0" + d.slice(3)
                return d
            }

            function placeCall() {
                var dest = normalizeDial(numberDisplay.text)
                if (dest.length === 0) return
                console.log("[MyTel Dial] Calling [" + dest + "]")
                UtilsCpp.createCall(dest)
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: dialUnit * 0.1

                // ── Header: Back + Title ──
                RowLayout {
                    spacing: dialUnit * 0.12
                    Button {
                        icon.width: dialUnit * 0.3
                        icon.height: dialUnit * 0.3
                        Layout.preferredWidth: dialUnit * 0.38
                        Layout.preferredHeight: dialUnit * 0.38
                        style: ButtonStyle.noBackground
                        icon.source: AppIcons.leftArrow
                        onClicked: listStackView.pop()
                    }
                    Text {
                        text: qsTr("call_action_start_new_call")
                        color: DefaultStyle.main2_700
                        font.pixelSize: Typography.h2m.pixelSize
                        font.weight: Typography.h2m.weight
                    }
                    Item { Layout.fillWidth: true }
                }

                // ── Number Display ──
                Control.TextField {
                    id: numberDisplay
                    Layout.fillWidth: true
                    Layout.preferredHeight: dialUnit * 0.8
                    Layout.maximumWidth: dialUnit * 5.2
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: dialUnit * 0.2
                    font.pixelSize: Math.max(16, dialUnit * 0.35)
                    color: DefaultStyle.main2_900
                    horizontalAlignment: TextInput.AlignHCenter
                    verticalAlignment: TextInput.AlignVCenter
                    placeholderText: qsTr("mytel_chat_destination_hint")
                    background: Rectangle {
                        radius: height / 2
                        color: DefaultStyle.grey_100
                        border.width: numberDisplay.activeFocus ? 2 : 1
                        border.color: numberDisplay.activeFocus
                            ? DefaultStyle.main1_500_main
                            : DefaultStyle.grey_200
                    }
                    onAccepted: placeCall()
                    Keys.onReturnPressed: placeCall()
                }

                // ── Keypad 4×3 ──
                GridLayout {
                    columns: 3
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: dialUnit * 0.15
                    columnSpacing: dialUnit * 0.18
                    rowSpacing: dialUnit * 0.12

                    Repeater {
                        model: [
                            { label: "1", sub: "" },   { label: "2", sub: "ABC" },
                            { label: "3", sub: "DEF" }, { label: "4", sub: "GHI" },
                            { label: "5", sub: "JKL" }, { label: "6", sub: "MNO" },
                            { label: "7", sub: "PQRS" },{ label: "8", sub: "TUV" },
                            { label: "9", sub: "WXYZ" },{ label: "*", sub: "" },
                            { label: "0", sub: "+" },   { label: "#", sub: "" }
                        ]
                        delegate: Rectangle {
                            required property var modelData
                            width: dialUnit
                            height: dialUnit
                            radius: width / 2
                            color: dialMouse.containsPress ? DefaultStyle.grey_200 : DefaultStyle.grey_100
                            border.width: 1
                            border.color: DefaultStyle.grey_200

                            MouseArea {
                                id: dialMouse
                                anchors.fill: parent
                                onClicked: {
                                    numberDisplay.text += modelData.label
                                    numberDisplay.cursorPosition = numberDisplay.text.length
                                }
                            }

                            Column {
                                anchors.centerIn: parent
                                spacing: 0
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: modelData.label
                                    font.pixelSize: Math.max(14, dialUnit * 0.32)
                                    font.weight: Font.DemiBold
                                    color: DefaultStyle.main2_900
                                }
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: modelData.sub
                                    font.pixelSize: Math.max(6, dialUnit * 0.1)
                                    color: DefaultStyle.main2_400
                                    visible: modelData.sub.length > 0
                                }
                            }
                        }
                    }
                }

                // ── Bottom row: [space] [Call] [Backspace] ──
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: dialUnit * 0.15
                    spacing: dialUnit * 0.35

                    // Green Call button
                    Rectangle {
                        width: dialUnit
                        height: dialUnit
                        radius: width / 2
                        color: callMouse.containsPress ? "#16a34a" : "#22C55E"

                        MouseArea {
                            id: callMouse
                            anchors.fill: parent
                            onClicked: placeCall()
                        }

                        EffectImage {
                            anchors.centerIn: parent
                            width: dialUnit * 0.38
                            height: dialUnit * 0.38
                            imageSource: AppIcons.phone
                            colorizationColor: "#FFFFFF"
                        }
                    }

                    // Backspace button
                    Rectangle {
                        width: dialUnit * 0.7
                        height: dialUnit * 0.7
                        radius: width / 2
                        color: backspaceMouse.containsPress ? DefaultStyle.grey_200 : DefaultStyle.grey_100

                        MouseArea {
                            id: backspaceMouse
                            anchors.fill: parent
                            onClicked: {
                                if (numberDisplay.text.length > 0)
                                    numberDisplay.text = numberDisplay.text.slice(0, -1)
                            }
                            onPressAndHold: numberDisplay.text = ""
                        }

                        Text {
                            anchors.centerIn: parent
                            text: "⌫"
                            font.pixelSize: Math.max(12, dialUnit * 0.28)
                            color: DefaultStyle.main2_600
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }

    Component {
        id: groupCallItem
        GroupCreationFormLayout {
            objectName: "groupCallItem"
            //: "Appel de groupe"
            formTitle: qsTr("call_start_group_call_title")
            //: "Lancer"
            createGroupButtonText: qsTr("call_action_start_group_call")
            Control.StackView.onActivated: {
                addParticipantsLayout.forceActiveFocus()
            }
            onReturnRequested: {
                listStackView.pop()
                listStackView.currentItem?.forceActiveFocus()
            }
            onGroupCreationRequested: {
                if (groupName.text.length === 0) {
                    UtilsCpp.showInformationPopup(qsTr("information_popup_error_title"),
                                                    //: "Un nom doit être donné à l'appel de groupe
                                                    qsTr("group_call_error_must_have_name"), false)
                } else if (!mainItem.isRegistered) {
                    UtilsCpp.showInformationPopup(qsTr("information_popup_error_title"),
                                //: "Vous n'etes pas connecté"
                                qsTr("group_call_error_not_connected"), false)
                } else {
                    UtilsCpp.createGroupCall(groupName.text, addParticipantsLayout.selectedParticipants)
                }
            }
        }
    }

    Component {
        id: emptySelection
        Item {
            objectName: "emptySelection"
        }
    }
    Component {
        id: contactDetailComp
        FocusScope {
            CallHistoryLayout {
                id: contactDetail
                anchors.fill: parent
                anchors.topMargin: Utils.getSizeWithScreenRatio(45)
                anchors.bottomMargin: Utils.getSizeWithScreenRatio(45)
                visible: mainItem.selectedRowHistoryGui != undefined
                callHistoryGui: selectedRowHistoryGui

                property var contactObj: UtilsCpp.findFriendByAddress(specificAddress)
                contact: contactObj && contactObj.value || null
                specificAddress: callHistoryGui
                                 && callHistoryGui.core.remoteAddress || ""
                property bool isLocalFriend: contact ? contact.core.isAppFriend : false

                buttonContent: PopupButton {
                    id: detailOptions
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    popUpTitle: qsTr("call_history_options_accessible_name")
                    popup.x: width
                    popup.contentItem: FocusScope {
                        implicitHeight: detailsButtons.implicitHeight
                        implicitWidth: detailsButtons.implicitWidth
                        Keys.onPressed: event => {
                                            if (event.key == Qt.Key_Left
                                                || event.key == Qt.Key_Escape) {
                                                detailOptions.popup.close()
                                                event.accepted = true
                                            }
                                        }
                        ColumnLayout {
                            id: detailsButtons
                            anchors.fill: parent
                            IconLabelButton {
                                id: addContactButton
                                Layout.fillWidth: true
                                //: "Show contact"
                                text: contactDetail.isLocalFriend ? qsTr("menu_see_existing_contact") :
                                                              //: "Add to contacts"
                                                              qsTr("menu_add_address_to_contacts")
                                icon.source: contactDetail.isLocalFriend ? AppIcons.adressBook : AppIcons.plusCircle
                                icon.width: Utils.getSizeWithScreenRatio(32)
                                icon.height: Utils.getSizeWithScreenRatio(32)
                                onClicked: {
                                    detailOptions.close()
                                    if (contactDetail.isLocalFriend)
                                        mainWindow.displayContactPage(contactDetail.contactAddress)
                                    else
                                        mainItem.createContactRequested(contactDetail.contactName, contactDetail.contactAddress)
                                }
                                KeyNavigation.up: visibleChildren.length
                                                    != 0 ? detailOptions.getPreviousItem(
                                                                0) : null
                                KeyNavigation.down: visibleChildren.length
                                                    != 0 ? detailOptions.getNextItem(
                                                                0) : null
                            }
                            IconLabelButton {
                                id: copySIPAddressButton
                                Layout.fillWidth: true
                                //: "Copier l'adresse SIP"
                                text: qsTr("menu_copy_sip_address")
                                icon.source: AppIcons.copy
                                icon.width: Utils.getSizeWithScreenRatio(32)
                                icon.height: Utils.getSizeWithScreenRatio(32)
                                onClicked: {
                                    detailOptions.close()
                                    var success = UtilsCpp.copyToClipboard(
                                                mainItem.selectedRowHistoryGui
                                                && mainItem.selectedRowHistoryGui.core.remoteAddress)
                                    if (success)
                                        UtilsCpp.showInformationPopup(
                                                    //: Adresse copiée
                                                    qsTr("sip_address_copied_to_clipboard_toast"),
                                                    //: L'adresse a été copié dans le presse_papiers
                                                    qsTr("sip_address_copied_to_clipboard_message"),
                                                    true)
                                    else
                                        UtilsCpp.showInformationPopup(
                                                    qsTr("information_popup_error_title"),
                                                    //: "Erreur lors de la copie de l'adresse"
                                                    qsTr("sip_address_copy_to_clipboard_error"),
                                                    false)
                                }
                                KeyNavigation.up: visibleChildren.length
                                                    != 0 ? detailOptions.getPreviousItem(
                                                                1) : null
                                KeyNavigation.down: visibleChildren.length
                                                    != 0 ? detailOptions.getNextItem(
                                                                1) : null
                            }
                            // IconLabelButton {
                            // 	background: Item {}
                            // 	enabled: false
                            // 	contentItem: IconLabel {
                            // 		text: qsTr("Bloquer")
                            // 		iconSource: AppIcons.empty
                            // 	}
                            // 	onClicked: console.debug("[CallPage.qml] TODO : block user")
                            // }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: Utils.getSizeWithScreenRatio(2)
                                color: DefaultStyle.main2_400
                            }

                            IconLabelButton {
                                id: deleteHistoryButton
                                Layout.fillWidth: true
                                //: "Supprimer l'historique"
                                text: qsTr("menu_delete_history")
                                icon.source: AppIcons.trashCan
                                icon.width: Utils.getSizeWithScreenRatio(32)
                                icon.height: Utils.getSizeWithScreenRatio(32)
                                style: ButtonStyle.hoveredBackgroundRed
                                Connections {
                                    target: deleteForUserPopup
                                    function onAccepted() {
                                        detailListView.model.removeEntriesWithFilter(
                                                    detailListView.searchText)
                                        mainItem.listViewUpdated()
                                    }
                                }
                                onClicked: {
                                    detailOptions.close()
                                    deleteForUserPopup.open()
                                }
                                KeyNavigation.up: visibleChildren.length
                                                    != 0 ? detailOptions.getPreviousItem(
                                                                2) : null
                                KeyNavigation.down: visibleChildren.length
                                                    != 0 ? detailOptions.getNextItem(
                                                                2) : null
                            }
                        }
                    }
                }
                onConferenceChatDisplayRequested: {
                    rightPanelStackView.push(historyChatComp, {"callHistory": mainItem.selectedRowHistoryGui})
                }
                detailContent: Item {
                    Layout.preferredWidth: Utils.getSizeWithScreenRatio(360)
                    Layout.fillHeight: true
                    RoundedPane {
                        id: detailControl
                        visible: detailListView.count > 0
                        width: parent.width
                        height: Math.min(parent.height, detailListView.contentHeight) + topPadding + bottomPadding
                        background: Rectangle {
                            id: detailListBackground
                            anchors.fill: parent
                            color: DefaultStyle.grey_0
                            radius: Utils.getSizeWithScreenRatio(15)
                        }
                        contentItem: Control.ScrollView {
                            id: historyScrollView
                            Control.ScrollBar.vertical: ScrollBar {
                                id: historyScrollBar
                                visible: historyScrollView.contentHeight > historyScrollView.height
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                anchors.right: parent.right
                            }
                            CallHistoryListView {
                                id: detailListView
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.rightMargin: historyScrollBar.width + Utils.getSizeWithScreenRatio(8)
                                spacing: Utils.getSizeWithScreenRatio(14)
                                clip: true
                                searchText: mainItem.selectedRowHistoryGui ? mainItem.selectedRowHistoryGui.core.remoteAddress : ""
                                busyIndicatorSize: Utils.getSizeWithScreenRatio(40)

                                delegate: Item {
                                    visible: !detailListView.loading
                                    width: detailListView.width
                                    height: Utils.getSizeWithScreenRatio(56)
                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: Utils.getSizeWithScreenRatio(20)
                                        anchors.rightMargin: Utils.getSizeWithScreenRatio(20)
                                        anchors.verticalCenter: parent.verticalCenter
                                        ColumnLayout {
                                            Layout.alignment: Qt.AlignVCenter
                                            RowLayout {
                                                EffectImage {
                                                    id: statusIcon
                                                    imageSource: modelData.core.status
                                                                === LinphoneEnums.CallStatus.Declined
                                                                || modelData.core.status === LinphoneEnums.CallStatus.DeclinedElsewhere || modelData.core.status === LinphoneEnums.CallStatus.Aborted || modelData.core.status === LinphoneEnums.CallStatus.EarlyAborted ? AppIcons.arrowElbow : modelData.core.isOutgoing ? AppIcons.arrowUpRight : AppIcons.arrowDownLeft
                                                    colorizationColor: modelData.core.status === LinphoneEnums.CallStatus.Declined || modelData.core.status === LinphoneEnums.CallStatus.DeclinedElsewhere || modelData.core.status === LinphoneEnums.CallStatus.Aborted || modelData.core.status === LinphoneEnums.CallStatus.EarlyAborted || modelData.core.status === LinphoneEnums.CallStatus.Missed ? DefaultStyle.danger_500_main : modelData.core.isOutgoing ? DefaultStyle.info_500_main : DefaultStyle.success_500_main
                                                    Layout.preferredWidth: Utils.getSizeWithScreenRatio(16)
                                                    Layout.preferredHeight: Utils.getSizeWithScreenRatio(16)
                                                    transform: Rotation {
                                                        angle: modelData.core.isOutgoing
                                                            && (modelData.core.status === LinphoneEnums.CallStatus.Declined || modelData.core.status === LinphoneEnums.CallStatus.DeclinedElsewhere || modelData.core.status === LinphoneEnums.CallStatus.Aborted || modelData.core.status === LinphoneEnums.CallStatus.EarlyAborted) ? 180 : 0
                                                        origin {
                                                            x: statusIcon.width / 2
                                                            y: statusIcon.height / 2
                                                            }
                                                        }
                                                    }
                                                Text {
                                                    //: "Appel manqué"
                                                    text: modelData.core.status === LinphoneEnums.CallStatus.Missed ? qsTr("notification_missed_call_title")
                                                                                                                    : modelData.core.isOutgoing
                                                                                                                        //: "Appel sortant"
                                                                                                                        ? qsTr("call_outgoing")
                                                                                                                        //: "Appel entrant"
                                                                                                                        : qsTr("call_audio_incoming")
                                                    font {
                                                        pixelSize: Typography.p1.pixelSize
                                                        weight: Typography.p1.weight
                                                    }
                                                }
                                            }
                                            Text {
                                                text: UtilsCpp.formatDate(modelData.core.date)
                                                color: modelData.core.status === LinphoneEnums.CallStatus.Missed ? DefaultStyle.danger_500_main : DefaultStyle.main2_500_main
                                                font {
                                                    pixelSize: Utils.getSizeWithScreenRatio(12)
                                                    weight: Utils.getSizeWithScreenRatio(300)
                                                }
                                            }
                                        }
                                        Item {
                                            Layout.fillHeight: true
                                            Layout.fillWidth: true
                                        }
                                        Text {
                                            text: UtilsCpp.formatElapsedTime(modelData.core.duration,false)
                                            font {
                                                pixelSize: Utils.getSizeWithScreenRatio(12)
                                                weight: Utils.getSizeWithScreenRatio(300)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                Item {
                    Layout.fillHeight: true
                }
            }
        }
    }
    Component {
        id: historyChatComp
        SelectedChatView {
            backButtonVisible: true
            property var callHistory: null
            property var chatObj: callHistory ? UtilsCpp.getChatForCallLog(callHistory) : null
            chat: chatObj ? chatObj.value : null
            onBackButtonPressed: {
                rightPanelStackView.pop()
            }
        }
    }

    component IconLabel: RowLayout {
        id: iconLabel
        property string text
        property string iconSource
        property color colorizationColor: DefaultStyle.main2_500_main
        EffectImage {
            imageSource: iconLabel.iconSource
            Layout.preferredWidth: Utils.getSizeWithScreenRatio(24)
            Layout.preferredHeight: Utils.getSizeWithScreenRatio(24)
            fillMode: Image.PreserveAspectFit
            colorizationColor: iconLabel.colorizationColor
        }
        Text {
            text: iconLabel.text
            color: iconLabel.colorizationColor
            font {
                pixelSize: Typography.p1.pixelSize
                weight: Typography.p1.weight
            }
        }
    }
}
