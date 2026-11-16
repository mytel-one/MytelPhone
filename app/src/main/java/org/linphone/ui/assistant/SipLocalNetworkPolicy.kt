package org.linphone.ui.assistant

import java.net.URI
import java.util.Locale

/** Address classification only. URI parsing never resolves DNS or opens a socket. */
object SipLocalNetworkPolicy {
    enum class Need { NONE, OPTIONAL, REQUIRED, INVALID }

    fun routes(values: List<String>): Need {
        val needs = values.filter { it.isNotBlank() }.map(::route)
        return when {
            Need.INVALID in needs || needs.isEmpty() -> Need.INVALID
            Need.REQUIRED in needs -> Need.REQUIRED
            Need.OPTIONAL in needs -> Need.OPTIONAL
            else -> Need.NONE
        }
    }

    fun route(value: String): Need {
        val uri = SipSetupServerUri.normalize(value) ?: return Need.INVALID
        val authority = uri.substringAfter(':').substringBefore(';').substringBefore('?').substringAfterLast('@')
        val host = if (authority.startsWith('[')) {
            val end = authority.indexOf(']')
            if (end < 0 || !validPort(authority.substring(end + 1))) return Need.INVALID
            authority.substring(1, end)
        } else if (authority.count { it == ':' } == 1) {
            if (!validPort(":" + authority.substringAfter(':'))) return Need.INVALID
            authority.substringBefore(':')
        } else authority
        if (host.isBlank()) return Need.INVALID
        if (':' in host) {
            if (runCatching { URI("http://[$host]").host }.getOrNull() == null) return Need.INVALID
            val words = ipv6Words(host.substringBefore('%')) ?: return Need.INVALID
            if (words.take(5).all { it == 0 } && words[5] == 65535) {
                return ipv4Need(listOf(words[6] shr 8, words[6] and 255, words[7] shr 8, words[7] and 255))
            }
            return if ((words[0] and 0xfe00) == 0xfc00 || (words[0] and 0xffc0) == 0xfe80) Need.REQUIRED else Need.NONE
        }
        if (host.all { it.isDigit() || it == '.' }) {
            return ipv4(host)?.let(::ipv4Need) ?: Need.INVALID
        }
        if (!host.matches(Regex("[A-Za-z0-9_.-]+"))) return Need.INVALID
        val name = host.trimEnd('.').lowercase(Locale.ROOT)
        return when {
            name == "localhost" || name.endsWith(".localhost") -> Need.NONE
            name.endsWith(".local") -> Need.REQUIRED
            else -> Need.OPTIONAL
        }
    }

    private fun validPort(suffix: String): Boolean = suffix.isEmpty() ||
        (suffix.startsWith(':') && suffix.substring(1).isNotEmpty() &&
            suffix.substring(1).all { it.isDigit() } && suffix.substring(1).toIntOrNull() in 1..65535)

    private fun ipv4(host: String): List<Int>? {
        val parts = host.split('.')
        if (parts.size != 4 || parts.any { it.isEmpty() || it.length > 3 || it.any { c -> !c.isDigit() } }) return null
        return parts.map { it.toInt() }.takeIf { it.all { number -> number in 0..255 } }
    }

    private fun ipv4Need(parts: List<Int>): Need = if (
        parts[0] == 10 || (parts[0] == 172 && parts[1] in 16..31) ||
        (parts[0] == 192 && parts[1] == 168) || (parts[0] == 169 && parts[1] == 254)
    ) Need.REQUIRED else Need.NONE

    private fun ipv6Words(input: String): List<Int>? {
        var host = input
        if ('.' in host) {
            val tail = ipv4(host.substringAfterLast(':')) ?: return null
            host = host.substringBeforeLast(':') + ":" + ((tail[0] shl 8) + tail[1]).toString(16) + ":" + ((tail[2] shl 8) + tail[3]).toString(16)
        }
        fun words(value: String) = value.split(':').filter { it.isNotEmpty() }.map { it.toIntOrNull(16) ?: -1 }
        val result = if ("::" in host) {
            val left = words(host.substringBefore("::"))
            val right = words(host.substringAfter("::"))
            if (left.size + right.size >= 8) return null
            left + List(8 - left.size - right.size) { 0 } + right
        } else words(host)
        return result.takeIf { it.size == 8 && it.all { word -> word in 0..65535 } }
    }
}
