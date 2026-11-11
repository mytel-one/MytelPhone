import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls.Basic
import Linphone
import UtilsCpp
import SettingsCpp
import 'qrc:/qt/qml/Linphone/view/Control/Tool/Helper/utils.js' as Utils

AbstractWindow {
	id: mainWindow
    // height: Utils.getSizeWithScreenRatio(982)
    title: applicationName
	// TODO : handle this bool when security mode is implemented
	property bool firstConnection: true
    property int initialWidth
    property int initialHeight
    Component.onCompleted: {
        initialWidth = width
        initialHeight = height
    }

	color: DefaultStyle.grey_0
    minimumWidth: Utils.getSizeWithScreenRatio(1020)
    minimumHeight: Utils.getSizeWithScreenRatio(700)

	signal callCreated()
	property var accountProxy

	// TODO : use this to make the border transparent
	flags: Qt.Window
	// menuBar: Rectangle {
	// 	width: parent.width
    // 	height: Utils.getSizeWithScreenRatio(40)
	// 	color: DefaultStyle.grey_100
	// }

	function openMainPage(connectionSucceed){
		if (!mainStackViewLoader.item || !mainStackViewLoader.item.currentItem || mainStackViewLoader.item.currentItem.objectName !== "mainPage") mainStackViewLoader.item.replace(mainPage, StackView.Immediate)
        //: "Connexion réussie"
        if (connectionSucceed) mainWindow.showInformationPopup(qsTr("information_popup_connexion_succeed_title"),
                                                               //: "Vous êtes connecté en mode %1"
                                                               qsTr("information_popup_connexion_succeed_message").arg(
                                                               //: interopérable
                                                               qsTr("interoperable")))
	}
	function goToCallHistory() {
		openMainPage()
		mainStackViewLoader.item.currentItem.goToCallHistory()
	}
	function goToNewCall() {
		openMainPage()
		mainStackViewLoader.item.currentItem.goToNewCall()
	}
	function displayContactPage(contactAddress) {
		openMainPage()
		mainStackViewLoader.item?.currentItem.displayContactPage(contactAddress)
	}
	function displayCreateContactPage(name, contactAddress) {
		openMainPage()
		mainStackViewLoader.item?.currentItem.createContact(name, contactAddress)
	}
	function displayChatPage(contactAddress) {
		openMainPage()
		mainStackViewLoader.item?.currentItem.displayChatPage(contactAddress)
	}
	function openChat(chat) {
		openMainPage()
		mainStackViewLoader.item?.currentItem.openChat(chat)
	}
	function transferCallSucceed() {
		openMainPage()
        //: "Appel transféré"
        mainWindow.showInformationPopup(qsTr("call_transfer_successful_toast_title"),
                                        //: "Votre correspondant a été transféré au contact sélectionné"
                                        qsTr("call_transfer_successful_toast_message"))
	}
	function initStackViewItem() {
        if(accountProxy && accountProxy.isInitialized) {
            if (accountProxy.haveAccount) openMainPage()
            // MyTel product decision: ALWAYS go directly to the third-party
            // SIP login form — no welcome carousel, no Linphone account page
            else mainStackViewLoader.item.replace(sipLoginPage, StackView.Immediate)
        }
    }
	
	function goToLogin() {
		// MyTel product decision: always the third-party SIP login form
		mainStackViewLoader.item.replace(sipLoginPage)
	}

	function scheduleMeeting(subject, addresses) {
		openMainPage()
		mainStackViewLoader.item.currentItem.scheduleMeeting(subject, addresses)
	}

	property bool authenticationPopupOpened: false
	// Set once the MyTel intro overlay has fully faded away; the splash
	// Loader then deactivates for the rest of the session
	property bool splashDismissed: false
	Component {
		id: authenticationPopupComp
		AuthenticationDialog{
			onOpened: mainWindow.authenticationPopupOpened = true
			onClosed: {
				mainWindow.authenticationPopupOpened = false
				destroy()
			}
		}
	}

	function reauthenticateAccount(identity, domain, callback){
		if (authenticationPopupOpened) return
		if (mainStackViewLoader.item?.currentItem.objectName === "loginPage" 
		|| mainStackViewLoader.item?.currentItem.objectName === "sipLoginPage")
			return
		console.log("Showing authentication dialog")
		var popup = authenticationPopupComp.createObject(mainWindow, {"identity": identity, "domain": domain, "callback":callback})	// Callback ownership is not passed
		popup.open()
		popup.announce()
	}

	Connections {
		target: SettingsCpp
		function onAssistantGoDirectlyToThirdPartySipAccountLoginChanged() {
            initStackViewItem()
		}
		function onIsSavedChanged(saved) {
            if (saved) UtilsCpp.showInformationPopup(qsTr("information_popup_success_title"),
                                                                   //: "Les changements ont été sauvegardés"
                                                                   qsTr("information_popup_changes_saved"), true, mainWindow)
        }
	}

	Connections {
		target: LoginPageCpp
		function onRegistrationStateChanged() {
			if (LoginPageCpp.registrationState === LinphoneEnums.RegistrationState.Ok) {
				openMainPage(true)
				proposeH264CodecsDownload()
			}
		}
	}

	Loader {
		id: accountProxyLoader
		active: AppCpp.coreStarted
		sourceComponent: AccountProxy {
            onInitializedChanged: if (isInitialized) {
				mainWindow.accountProxy = this
				mainWindow.initStackViewItem()
            }
		}
	}

	Loader {
		id: mainStackViewLoader
		active: AppCpp.coreStarted
		anchors.fill: parent
		sourceComponent: mainStackViewComp
	}

	// MyTel animated intro (owner requirement 2026-11-11; GPT-6.1-sol leader
	// decision #5): stays until the intro finished AND the core started, so a
	// fast core never cuts the animation and a slow core never restarts it.
	Loader {
		anchors.fill: parent
		active: !mainWindow.splashDismissed
		sourceComponent: splashScreen
	}

	Loader {
		anchors.fill: parent
		active: AppCpp.coreGlobalState === 4
		sourceComponent: ssoPage
	}

	Component {
		id: mainStackViewComp
		StackView {
			id: mainWindowStackView
			// H264 Cisco codec download
			PayloadTypeProxy {
				id: downloadableVideoPayloadTypeProxy
				filterType: PayloadTypeProxy.Video | PayloadTypeProxy.Downloadable
			}
			Repeater {
				id: codecDownloader
				model: null
				Item {
					Component.onCompleted: {
						if (modelData.core.mimeType == "H264")
							Utils.openCodecOnlineInstallerDialog(mainWindow, modelData.core)
					}
				}
			}
			function proposeH264CodecsDownload() {
				codecDownloader.model = downloadableVideoPayloadTypeProxy
			}
		}
	}

	// MyTel branded animated intro (owner requirement 2026-11-11; GPT-6.1-sol
	// leader design verdict 2026-11-11): emblem visible from the first frame,
	// luminous highlight travels once around the ring (0-500 ms), emblem
	// settles 0.96 -> 1.02 -> 1.0 with a <=12% halo (150-650 ms), wordmark
	// fades up (300-800 ms), 64x2 orange underline reveals (650-1000 ms),
	// crossfade out (1000-1200 ms). Transform/property animations only — no
	// per-frame Canvas repaints (leader decision #5).
	Component {
		id: splashScreen
		Item {
			id: mytelSplashRoot

			property bool introFinished: false
			property bool dismissRequested: false

			function requestDismiss() {
				if (mytelSplashRoot.dismissRequested) return
				mytelSplashRoot.dismissRequested = true
				dismissFade.start()
			}

			Rectangle {
				anchors.fill: parent
				color: "#001622"
			}

			Rectangle {
				id: halo
				anchors.centerIn: emblemContainer
				width: emblemContainer.width * 1.8
				height: width
				radius: width / 2
				// AARRGGBB: 12% orange (leader correction: valid 8-digit color)
				color: "#1FFFAB2E"
				opacity: 0
			}

			Item {
				id: emblemContainer
				anchors.centerIn: parent
				width: Utils.getSizeWithScreenRatio(160)
				height: width

				Image {
					id: emblem
					anchors.fill: parent
					source: "image://internal/mytel_logo.png"
					fillMode: Image.PreserveAspectFit
					asynchronous: false
					scale: 0.96
				}

				// Luminous highlight riding the ring once (rotating transform)
				Item {
					id: ringSweep
					anchors.centerIn: parent
					width: parent.width
					height: parent.height
					rotation: 0
					Rectangle {
						width: emblemContainer.width * 0.085
						height: width
						radius: width / 2
						anchors.horizontalCenter: parent.horizontalCenter
						y: -height / 2 + emblemContainer.width * 0.0425
						color: "#E8F1FC"
						opacity: 0.9
					}
				}
			}

			Text {
				id: wordmark
				anchors.top: emblemContainer.bottom
				anchors.topMargin: Utils.getSizeWithScreenRatio(20)
				anchors.horizontalCenter: parent.horizontalCenter
				// Leader correction: complete wordmark in soft white
				text: "MytellPhone"
				color: "#E8F1FC"
				font.pixelSize: Utils.getSizeWithScreenRatio(26)
				font.weight: Font.DemiBold
				opacity: 0
				transform: Translate {
					id: wordmarkTranslate
					y: Utils.getSizeWithScreenRatio(8)
				}
			}

			Rectangle {
				id: underline
				anchors.top: wordmark.bottom
				anchors.topMargin: Utils.getSizeWithScreenRatio(10)
				anchors.horizontalCenter: parent.horizontalCenter
				width: Utils.getSizeWithScreenRatio(64)
				height: Math.max(2, Utils.getSizeWithScreenRatio(2))
				color: "#FFAB2E"
				// Leader correction: Item has no scaleX — use a Scale transform
				transform: Scale {
					id: underlineScale
					origin.x: 0
					origin.y: underline.height / 2
					xScale: 0
				}
			}

			// Owner requirement: keyboard skip (Escape/Space) and click skip.
			// forceActiveFocus is required — plain focus:true loses to the
			// window's initial focus item and keys never arrive.
			Keys.onEscapePressed: mytelSplashRoot.requestDismiss()
			Keys.onSpacePressed: mytelSplashRoot.requestDismiss()

			MouseArea {
				anchors.fill: parent
				onClicked: mytelSplashRoot.requestDismiss()
			}

			Connections {
				target: AppCpp
				function onCoreStartedChanged() {
					if (AppCpp.coreStarted && mytelSplashRoot.introFinished) mytelSplashRoot.requestDismiss()
				}
			}

			// Leader correction: actionable navigation (calls window taking
			// focus, another window, dialogs) must interrupt the intro.
			// `active` is a Window property — watch it via the window, not
			// the Item (an Item-level onActiveChanged fails the whole QML
			// load).
			Connections {
				target: mainWindow
				function onActiveChanged() {
					if (!mainWindow.active) mytelSplashRoot.requestDismiss()
				}
			}

			// Reduced motion (leader correction): static brand only, quick
			// fade, no decorative stages. Driven by the real Windows
			// client-area animation preference through the C++ bridge
			// (SPI_GETCLIENTAREAANIMATION), with MYTELL_REDUCED_MOTION=1 as
			// a supplemental override.
			property bool reducedMotion: UtilsCpp.isReduceMotionEnabled()

			// Adaptive startup budget (leader correction): the ~2.2s launch
			// target covers startup through usable content, so decoration is
			// shortened when the app itself already consumed the budget.
			// 1 = full sequence, 0.5 = halved, 0 = static brand only.
			property real introScale: {
				if (reducedMotion) return 0
				var elapsed = AppCpp.startupElapsedMs()
				if (elapsed > 2500) return 0
				if (elapsed > 1500) return 0.5
				return 1
			}

			onReducedMotionChanged: if (reducedMotion) mytelSplashRoot.showStaticBrand()
			Component.onCompleted: {
				mytelSplashRoot.forceActiveFocus()
				if (introScale === 0) mytelSplashRoot.showStaticBrand()
			}

			function showStaticBrand() {
				introSequence.stop()
				wordmark.opacity = 1
				wordmarkTranslate.y = 0
				underlineScale.xScale = 1
				emblem.scale = 1
				introFinished = true
				// Always hold the static brand briefly so reduced motion
				// still shows the brand instead of skipping to the app
				staticHoldTimer.start()
			}

			Timer {
				id: staticHoldTimer
				interval: 600
				onTriggered: if (AppCpp.coreStarted) mytelSplashRoot.requestDismiss()
				else staticHoldTimer.restart()
			}

			ParallelAnimation {
				id: introSequence
				running: mytelSplashRoot.introScale > 0
				// 0-500 ms: highlight travels once around the ring
				NumberAnimation {
					target: ringSweep
					property: "rotation"
					from: 0
					to: 360
					duration: 500 * mytelSplashRoot.introScale
					easing.type: Easing.InOutQuad
				}
				// 150-650 ms: halo in + emblem settle
				SequentialAnimation {
					PauseAnimation { duration: 150 * mytelSplashRoot.introScale }
					ParallelAnimation {
						NumberAnimation {
							target: halo
							property: "opacity"
							to: 1
							duration: 250 * mytelSplashRoot.introScale
						}
						SequentialAnimation {
							NumberAnimation {
								target: emblem
								property: "scale"
								from: 0.96
								to: 1.02
								duration: 300 * mytelSplashRoot.introScale
								easing.type: Easing.OutQuad
							}
							NumberAnimation {
								target: emblem
								property: "scale"
								to: 1.0
								duration: 200 * mytelSplashRoot.introScale
								easing.type: Easing.InOutQuad
							}
						}
					}
				}
				// 300-800 ms: wordmark rises in (soft white, single text —
				// leader correction: complete wordmark in soft white)
				SequentialAnimation {
					PauseAnimation { duration: 300 * mytelSplashRoot.introScale }
					ParallelAnimation {
						NumberAnimation {
							target: wordmark
							property: "opacity"
							to: 1
							duration: 500 * mytelSplashRoot.introScale
							easing.type: Easing.OutQuad
						}
						NumberAnimation {
							target: wordmarkTranslate
							property: "y"
							to: 0
							duration: 500 * mytelSplashRoot.introScale
							easing.type: Easing.OutQuad
						}
					}
				}
				// 650-1000 ms: orange underline reveal
				SequentialAnimation {
					PauseAnimation { duration: 650 * mytelSplashRoot.introScale }
					NumberAnimation {
						target: underlineScale
						property: "xScale"
						to: 1
						duration: 350 * mytelSplashRoot.introScale
						easing.type: Easing.InOutQuad
					}
				}
				// 1000 ms: intro done; dismiss when the core is up
				SequentialAnimation {
					PauseAnimation { duration: 1000 * mytelSplashRoot.introScale }
					ScriptAction {
						script: {
							mytelSplashRoot.introFinished = true
							if (AppCpp.coreStarted) mytelSplashRoot.requestDismiss()
						}
					}
				}
			}

			NumberAnimation {
				id: dismissFade
				target: mytelSplashRoot
				property: "opacity"
				to: 0
				duration: 200
				easing.type: Easing.InOutQuad
				onFinished: mainWindow.splashDismissed = true
			}
		}
	}
	// MyTel: WelcomePage removed entirely — the app goes straight to the
	// third-party SIP login form (product decision). The component is kept
	// as an empty placeholder so any lingering reference compiles but shows
	// nothing if ever navigated to.
	Component {
		id: welcomePage
		Item {}
	}
	Component {
		id: ssoPage
		Rectangle {
			color: DefaultStyle.grey_0
			Image {
				id: logoImage
				anchors.centerIn: parent
				source: AppIcons.splashscreenLogo
                sourceSize.width: Utils.getSizeWithScreenRatio(395)
                sourceSize.height: Utils.getSizeWithScreenRatio(395)
                width: Utils.getSizeWithScreenRatio(395)
                height: Utils.getSizeWithScreenRatio(395)
			}
			ColumnLayout {
				anchors.top: logoImage.bottom
				anchors.topMargin: Utils.getSizeWithScreenRatio(24)
				anchors.horizontalCenter: parent.horizontalCenter
				Text {
					Layout.alignment: Qt.AlignHCenter
					//: "Trying to connect to single sign on on web page ..."
					text: qsTr("oidc_connection_waiting_message")
					font: Typography.h2
					horizontalAlignment: Text.AlignHCenter
				}
				Text {
					Layout.alignment: Qt.AlignHCenter
					text: UtilsCpp.formatDuration(AppCpp.remainingTimeBeforeOidcTimeout)
					font: Typography.h3m
					horizontalAlignment: Text.AlignHCenter
				}
				Button {
					Layout.alignment: Qt.AlignHCenter
					//: Cancel
					text: qsTr("cancel")
					onClicked: AppCpp.lForceOidcTimeout()
				}
			}
		}
	}
	Component {
		id: loginPage
		LoginPage {
			objectName: "loginPage"
			onGoBack: openMainPage()
			onUseSIPButtonClicked: mainStackViewLoader.item.push(sipLoginPage)
			onGoToRegister: mainStackViewLoader.item.replace(registerPage)
            showBackButton: false
            StackView.onActivated: if (mainWindow.accountProxy?.haveAccount) showBackButton = true
		}
	}
	Component {
		id: sipLoginPage
		SIPLoginPage {
			objectName: "sipLoginPage"
			onGoBack: {
				if(SettingsCpp.assistantGoDirectlyToThirdPartySipAccountLogin){
					openMainPage()
				}else
					mainStackViewLoader.item.pop()
			}
			onGoToRegister: mainStackViewLoader.item.replace(registerPage)
            showBackButton: false
            StackView.onActivated: if (!SettingsCpp.assistantGoDirectlyToThirdPartySipAccountLogin || mainWindow.accountProxy?.haveAccount) showBackButton = true
		}
	}
	Component {
		id: registerPage
		RegisterPage {
			onReturnToLogin: goToLogin()
            //: "Veuillez valider le captcha sur la page web"
            onBrowserValidationRequested: mainWindow.showLoadingPopup(qsTr("captcha_validation_loading_message"), true)
			Connections {
				target: RegisterPageCpp
				function onNewAccountCreationSucceed(withEmail, address, sipIdentityAddress) {
					mainStackViewLoader.item.push(checkingPage, {"registerWithEmail": withEmail, "address": address, "sipIdentityAddress": sipIdentityAddress})
				}
				function onRegisterNewAccountFailed(errorMessage) {
                    //: "Erreur lors de la création"
                    mainWindow.showInformationPopup(qsTr("assistant_register_error_title"), errorMessage, false)
					mainWindow.closeLoadingPopup()
				}
				function onTokenConversionSucceed(){ mainWindow.closeLoadingPopup()}
			}
		}
	}
	Component {
		id: checkingPage
		RegisterCheckingPage {
			id: registerCheckingPage
			onReturnToRegister: mainStackViewLoader.item.pop()
			onSendCode: (code) => {
				RegisterPageCpp.linkNewAccountUsingCode(code, registerWithEmail, sipIdentityAddress)
			}
			Connections {
				target: RegisterPageCpp
				function onLinkingNewAccountWithCodeSucceed() {
					goToLogin()
					//: "Compte créé"
					mainWindow.showInformationPopup(qsTr("assistant_register_success_title"),
													//: "Le compte a été créé. Vous pouvez maintenant vous connecter"
													qsTr("assistant_register_success_message"), true)
				}
				function onLinkingNewAccountWithCodeFailed(errorMessage) {
					registerCheckingPage.errorMessage = ""
					//: "Erreur dans le code de validation"
					if (errorMessage.length === 0) errorMessage = qsTr("assistant_register_error_code")
					registerCheckingPage.errorMessage = errorMessage
				}
			}
		}
	}
	Component {
		id: securityModePage
		SecurityModePage {
			id: securePage
			onModeSelected: (index) => {
				// TODO : connect to cpp part when ready
				var selectedMode = index == 0 ? "chiffrement" : "interoperable"
				console.debug("[SelectMode]User: User selected mode " + selectedMode)
				openMainPage()
			}
		}
	}
	Component {
		id: mainPage
		Item {
			// Wrapper so the connection chip floats above the page chrome
			// inside the page tree (a window-level item stopped painting in
			// some theme states)
			MainLayout {
				id: mainLayout
				anchors.fill: parent
				objectName: "mainPage"
				onAddAccountRequest: goToLogin()
				onAccountRemoved: {
					initStackViewItem()
				}
				Connections {
					target: mainWindow
					function onCallCreated(){ mainLayout.callCreated() }
				}
				// StackView.onActivated: connectionSecured(0) // TODO : connect to cpp part when ready
			}

			// Owner requirement 2026-11-11: unmistakable connection indicator
			// (reusable component; the GUI test loads the SAME file).
			MyTelConnectionChip {
				id: connectionChip
				x: parent.width - width - 24
				y: 46
				z: 1000
				visible: mainWindow.accountProxy !== null && mainWindow.accountProxy !== undefined
					&& mainWindow.accountProxy.haveAccount === true
				registered: {
					var account = mainWindow.accountProxy?.defaultAccount
					return account && account.core
						&& account.core.registrationState === LinphoneEnums.RegistrationState.Ok
				}
			}
		}
	}

}
