package org.linphone.ui.assistant

import java.util.Locale

/**
 * Chooses the transport for a new account. This does not validate a SIP URI:
 * the SDK remains authoritative for parsing, ports and account creation.
 */
object SipSetupTransport {
    fun transportParameterNames(uri: String): List<String> = uri.substringBefore('?')
        .split(';').drop(1).map { it.substringBefore('=') }
        .filter { it.equals("transport", ignoreCase = true) }.distinct()

    fun resolveRoute(route: String, selection: String, accountRoutes: List<String>): String {
        if ((accountRoutes + route).any { it.trim().startsWith("sips:", ignoreCase = true) }) {
            return "TLS"
        }
        return resolve(route, selection)
    }

    fun resolve(server: String, selection: String): String {
        val address = server.trim()
        // A secure URI must never be silently downgraded by a visible choice.
        if (address.startsWith("sips:", ignoreCase = true)) return "TLS"

        val explicit = selection.trim().uppercase(Locale.ROOT)
        if (explicit in setOf("UDP", "TCP", "TLS")) return explicit

        val parameters = address.substringBefore('?').split(';').drop(1)
        parameters.firstOrNull { it.startsWith("transport=", ignoreCase = true) }
            ?.substringAfter('=')
            ?.uppercase(Locale.ROOT)
            ?.takeIf { it in setOf("UDP", "TCP", "TLS") }
            ?.let { return it }

        val authority = address.substringBefore(';').substringBefore('?')
            .replaceFirst(Regex("(?i)^sip:"), "")
        val port = if (authority.startsWith("[")) {
            authority.substringAfter(']', "").takeIf { it.startsWith(":") }?.drop(1)
        } else if (authority.count { it == ':' } == 1) {
            authority.substringAfter(':')
        } else {
            null
        }
        return if (port == "5061") "TLS" else "UDP"
    }
}
