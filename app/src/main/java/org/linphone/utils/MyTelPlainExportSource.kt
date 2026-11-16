package org.linphone.utils

import java.io.Closeable
import java.io.File
import java.io.IOException
import java.util.UUID
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.currentCoroutineContext
import kotlinx.coroutines.ensureActive
import kotlinx.coroutines.withContext

/** Borrow a plain original, or own one newly decrypted file for a single export. */
internal class MyTelPlainExportSource private constructor(
    val file: File,
    private val ownedCache: File? = null
) : Closeable {
    private var closed = false

    override fun close() = synchronized(activePaths) {
        if (!closed) {
            closed = true
            if (ownedCache != null) {
                try {
                    if (file.canonicalFile.parentFile != ownedCache || !hasOwnershipRecord(file, ownedCache)) {
                        throw IOException("Export temporary ownership changed")
                    }
                    if (file.exists() && !file.delete()) throw IOException("Export temporary could not be removed")
                    if (!ownershipRecord(file).delete()) throw IOException("Export ownership record could not be removed")
                } finally {
                    activePaths.remove(file.absolutePath)
                }
            }
        }
    }

    companion object {
        private val encryptedMagic = "bcEncryptedFs".toByteArray(Charsets.US_ASCII)
        private val ownedName = Regex("mytel-document-export-([a-f0-9]{32})\\.plain")
        private val activePaths = mutableSetOf<String>()

        private fun ownershipRecord(file: File) = File(file.path + ".owner")

        private fun ownershipToken(file: File): String =
            "mytel-document-export-v1:" + (ownedName.matchEntire(file.name)?.groupValues?.get(1) ?: "")

        private fun hasOwnershipRecord(file: File, cache: File): Boolean {
            if (!ownedName.matches(file.name)) return false
            val marker = ownershipRecord(file)
            return marker.canonicalFile.parentFile == cache && marker.isFile && marker.length() <= 128 &&
                marker.readText() == ownershipToken(file)
        }

        suspend fun <T> withPrepared(
            prepare: suspend () -> MyTelPlainExportSource,
            copy: suspend (File) -> T
        ): T = withContext(Dispatchers.IO) {
            var lease: MyTelPlainExportSource? = null
            var failure: Throwable? = null
            try {
                // A cancelled caller still owns a result produced by an SDK task
                // already in flight. Assign before returning to cancelled context.
                withContext(NonCancellable) { lease = prepare() }
                currentCoroutineContext().ensureActive()
                copy(requireNotNull(lease).file)
            } catch (error: Throwable) {
                failure = error
                throw error
            } finally {
                try {
                    lease?.close()
                } catch (cleanupError: Exception) {
                    if (failure != null) failure.addSuppressed(cleanupError) else throw cleanupError
                }
            }
        }

        private fun hasEncryptedHeader(file: File): Boolean = file.inputStream().use { input ->
            val header = ByteArray(encryptedMagic.size)
            var count = 0
            while (count < header.size) {
                val read = input.read(header, count, header.size - count)
                if (read <= 0) break
                count += read
            }
            count == header.size && header.contentEquals(encryptedMagic)
        }

        /** SDK callbacks must run on its core thread. No original-file fallback on failure. */
        fun prepare(
            source: File,
            plainCache: File,
            isEncrypted: () -> Boolean,
            exportPlainFile: () -> String
        ): MyTelPlainExportSource {
            val original = source.canonicalFile
            if (!original.isFile || !original.canRead()) throw IOException("Export source is not readable")
            val physicalCiphertext = hasEncryptedHeader(original)
            if (!isEncrypted()) {
                if (physicalCiphertext) throw IOException("Encrypted export requires an initialized VFS key")
                return MyTelPlainExportSource(original)
            }

            val cache = plainCache.canonicalFile
            val preexisting = cache.listFiles().orEmpty().map { it.canonicalPath }.toSet()
            val path = exportPlainFile()
            if (path.isBlank()) throw IOException("SDK could not decrypt the export source")
            val exported = File(path).canonicalFile
            if (exported == original || exported.parentFile != cache || exported.path in preexisting) {
                throw IOException("SDK export is not a newly owned plaintext cache file")
            }
            var adopted = false
            try {
                if (!exported.isFile || !exported.canRead() || hasEncryptedHeader(exported)) {
                    throw IOException("SDK did not return a readable plaintext file")
                }
                return synchronized(activePaths) {
                    val owned = File(cache, "mytel-document-export-${UUID.randomUUID().toString().replace("-", "")}.plain")
                    val marker = ownershipRecord(owned)
                    if (owned.exists() || !marker.createNewFile()) throw IOException("Could not claim export plaintext")
                    try {
                        marker.writeText(ownershipToken(owned))
                        if (!exported.renameTo(owned)) throw IOException("Could not own export plaintext")
                        activePaths.add(owned.absolutePath)
                        adopted = true
                        MyTelPlainExportSource(owned, cache)
                    } catch (error: Exception) {
                        marker.delete()
                        throw error
                    }
                }
            } finally {
                if (!adopted && exported.exists() && !exported.delete()) {
                    throw IOException("Failed export plaintext could not be removed")
                }
            }
        }

        fun isActive(file: File): Boolean = synchronized(activePaths) {
            file.canonicalPath.removeSuffix(".owner") in activePaths
        }

        /** Retry only our abandoned nonce-named files, including after process death. */
        fun clearAbandoned(plainCache: File): Int = synchronized(activePaths) {
            val cache = plainCache.canonicalFile
            var removed = 0
            for (marker in cache.listFiles().orEmpty()) {
                if (!marker.name.endsWith(".owner")) continue
                val file = File(cache, marker.name.removeSuffix(".owner"))
                if (file.canonicalFile.parentFile == cache && hasOwnershipRecord(file, cache) &&
                    file.canonicalPath !in activePaths
                ) {
                    if (file.exists()) {
                        if (!file.isFile || !file.delete()) throw IOException("Abandoned export plaintext could not be removed")
                        removed++
                    }
                    if (!marker.delete()) throw IOException("Abandoned export ownership record could not be removed")
                }
            }
            removed
        }
    }
}
