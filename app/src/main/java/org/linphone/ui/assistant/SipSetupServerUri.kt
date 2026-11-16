package org.linphone.ui.assistant

import java.util.Locale

/** Normalizes setup input only; the SIP SDK remains responsible for address parsing. */
object SipSetupServerUri {
    fun normalize(value: String): String? {
        val input = value.trim()
        if (input.isEmpty() || input.any { it.isWhitespace() }) return null
        val scheme = when {
            input.startsWith("sips:", ignoreCase = true) -> "sips:"
            input.startsWith("sip:", ignoreCase = true) -> "sip:"
            else -> null
        }
        val address = if (scheme != null) input.substring(scheme.length) else input
        if (address.substringBefore(';').substringBefore('?').substringAfterLast('@').isEmpty()) return null
        if (scheme == null) {
            val prefix = input.substringBefore(':').lowercase(Locale.ROOT)
            if (prefix in setOf("http", "https", "tel", "sms", "mailto", "urn", "file", "ftp")) return null
            // A single colon in a bare authority can only introduce its numeric port.
            val authority = input.substringBefore(';').substringBefore('?')
            if (authority.count { it == ':' } == 1 && authority.substringAfter(':').any { !it.isDigit() }) return null
        }
        return "${scheme ?: "sip:"}$address"
    }
}
