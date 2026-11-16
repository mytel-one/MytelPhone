package org.linphone.utils

import java.io.InputStream
import java.io.OutputStream
import java.net.URI
import java.net.URISyntaxException

/** One document picker request at a time; the source survives activity recreation. */
internal class MyTelDocumentExport(restoredPath: String? = null) {
    var pendingPath: String? = restoredPath?.takeIf { it.isNotBlank() }
        private set

    fun begin(path: String): Boolean {
        if (path.isBlank() || pendingPath != null) return false
        pendingPath = path
        return true
    }

    fun takePendingPath(): String? {
        val path = pendingPath
        pendingPath = null
        return path
    }

    companion object {
        /** Resolve only a local source, preserving the distinction between paths and URIs. */
        fun sourceFilePath(value: String): String? {
            if (value.startsWith('/')) return value
            if (!value.startsWith("file:", ignoreCase = true)) return null
            val uri = try {
                URI(value)
            } catch (_: URISyntaxException) {
                return null
            }
            if (uri.isOpaque || !uri.rawAuthority.isNullOrEmpty() ||
                uri.rawQuery != null || uri.rawFragment != null
            ) return null
            return uri.path?.takeIf { it.startsWith('/') }
        }

        fun exportMimeType(mappedMime: String?): String {
            val mime = mappedMime?.trim()
            val concreteMime = Regex(
                "(application|audio|font|image|message|model|multipart|text|video)/[A-Za-z0-9!#$&^_.+\\-]+",
                RegexOption.IGNORE_CASE
            )
            return mime?.takeIf { concreteMime.matches(it) } ?: "application/octet-stream"
        }

        fun safeFileName(name: String): String {
            val leaf = name.replace('\\', '/').substringAfterLast('/')
                .filterNot { Character.isISOControl(it) }
            return leaf.takeUnless { it.isBlank() || it == "." || it == ".." } ?: "Mytel-export"
        }

        /** A provider returning no stream is a failure; close failures propagate too. */
        fun copy(source: () -> InputStream?, destination: () -> OutputStream?): Boolean {
            return destination()?.use { output ->
                source()?.use { input ->
                    input.copyTo(output)
                    output.flush()
                    true
                } ?: false
            } ?: false
        }
    }
}
