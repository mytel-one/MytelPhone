package org.linphone.utils

import java.io.InputStream
import java.io.OutputStream

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
