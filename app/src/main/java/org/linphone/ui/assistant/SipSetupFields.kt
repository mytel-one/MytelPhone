package org.linphone.ui.assistant

/** Presence checks only; SIP syntax is validated by the SDK at connection time. */
object SipSetupFields {
    fun canConnect(username: String?, server: String?): Boolean {
        return !username.isNullOrBlank() && !server.isNullOrBlank()
    }
}

