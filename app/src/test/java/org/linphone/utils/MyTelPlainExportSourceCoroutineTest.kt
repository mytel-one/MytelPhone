package org.linphone.utils

import java.io.File
import java.nio.file.Files
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.awaitCancellation
import kotlinx.coroutines.cancelAndJoin
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class MyTelPlainExportSourceCoroutineTest {
    private fun fixture(test: (File, File) -> Unit) {
        val directory = Files.createTempDirectory("mytel-export-cancel-test").toFile()
        try {
            val cache = File(directory, "evfs").apply { mkdir() }
            val original = File(directory, "original.mkv").apply { writeText("bcEncryptedFs-cipher") }
            test(original, cache)
        } finally {
            directory.deleteRecursively()
        }
    }

    @Test fun cancelledCallerStillReleasesPlaintextProducedByInFlightSdkPreparation() = fixture { original, cache ->
        runBlocking {
            val started = CompletableDeferred<Unit>()
            val finishSdk = CompletableDeferred<Unit>()
            var owned: File? = null
            var copied = false
            val job = launch {
                MyTelPlainExportSource.withPrepared({
                    started.complete(Unit)
                    finishSdk.await()
                    val sdk = File(cache, "sdk.mkv")
                    MyTelPlainExportSource.prepare(original, cache, { true }, { sdk.writeText("plain"); sdk.path })
                        .also { owned = it.file }
                }) { copied = true }
            }
            started.await()
            job.cancel()
            finishSdk.complete(Unit)
            job.join()
            assertFalse(copied)
            assertFalse(requireNotNull(owned).exists())
            assertTrue(original.exists())
        }
    }

    @Test fun cancellationWhileProviderCopiesReleasesOwnedPlaintext() = fixture { original, cache ->
        runBlocking {
            val copying = CompletableDeferred<Unit>()
            var owned: File? = null
            val job = launch {
                MyTelPlainExportSource.withPrepared({
                    val sdk = File(cache, "sdk.mkv")
                    MyTelPlainExportSource.prepare(original, cache, { true }, { sdk.writeText("plain"); sdk.path })
                }) {
                    owned = it
                    copying.complete(Unit)
                    awaitCancellation()
                }
            }
            copying.await()
            job.cancelAndJoin()
            assertFalse(requireNotNull(owned).exists())
            assertTrue(original.exists())
        }
    }

    @Test fun successfulCopyReturnsOnlyAfterOwnedPlaintextIsRemoved() = fixture { original, cache ->
        runBlocking {
            var owned: File? = null
            val result = MyTelPlainExportSource.withPrepared({
                val sdk = File(cache, "sdk.mkv")
                MyTelPlainExportSource.prepare(original, cache, { true }, { sdk.writeText("decoded"); sdk.path })
            }) { owned = it; it.readText() }
            assertEquals("decoded", result)
            assertFalse(requireNotNull(owned).exists())
            assertTrue(original.exists())
        }
    }
}
