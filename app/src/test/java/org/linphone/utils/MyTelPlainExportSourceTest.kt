package org.linphone.utils

import java.io.File
import java.io.IOException
import java.nio.file.Files
import java.util.concurrent.CancellationException
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotEquals
import org.junit.Assert.assertTrue
import org.junit.Assert.fail
import org.junit.Test

class MyTelPlainExportSourceTest {
    private fun fixture(test: (File, File, File) -> Unit) {
        val directory = Files.createTempDirectory("mytel-export-test").toFile()
        try {
            val cache = File(directory, "evfs").apply { mkdir() }
            // '?' is covered as a raw path by MyTelDocumentExportTest; Windows
            // host filesystems cannot create it, unlike the Android runtime.
            val original = File(directory, "جلسه#1-100%.mkv").apply { writeText("bcEncryptedFs-ciphertext") }
            test(directory, cache, original)
        } finally {
            directory.deleteRecursively()
        }
    }

    @Test fun plainSourcesAreBorrowedAndNeverDeleted() = fixture { directory, cache, _ ->
        val original = File(directory, "plain#100%.wav").apply { writeText("RIFF-plain") }
        var exported = false
        val source = MyTelPlainExportSource.prepare(original, cache, { false }, { exported = true; "" })
        assertEquals(original.canonicalFile, source.file)
        source.close()
        assertTrue(original.exists())
        assertFalse(exported)
    }

    @Test fun encryptedSourceUsesOwnedPlaintextAndPreservesOriginal() = fixture { _, cache, original ->
        val sdk = File(cache, original.name)
        val source = MyTelPlainExportSource.prepare(original, cache, { true }, { sdk.writeText("EBML-plaintext"); sdk.path })
        assertEquals("EBML-plaintext", source.file.readText())
        assertNotEquals(original.canonicalPath, source.file.canonicalPath)
        assertFalse(sdk.exists())
        source.close()
        assertFalse(source.file.exists())
        assertEquals("bcEncryptedFs-ciphertext", original.readText())
    }

    @Test fun closeIsIdempotent() = fixture { _, cache, original ->
        val sdk = File(cache, "sdk.mkv")
        val source = MyTelPlainExportSource.prepare(original, cache, { true }, { sdk.writeText("plain"); sdk.path })
        source.close(); source.close()
        assertTrue(original.exists())
    }

    @Test fun vfsDetectionFailureCannotExportPhysicalCiphertext() = fixture { _, cache, original ->
        try {
            MyTelPlainExportSource.prepare(original, cache, { false }, { "" })
            fail("Encrypted header must not be copied when SDK decryption is unavailable")
        } catch (_: IOException) { assertTrue(original.exists()) }
    }

    @Test fun emptySdkExportIsFailureWithoutOriginalFallback() = fixture { _, cache, original ->
        try {
            MyTelPlainExportSource.prepare(original, cache, { true }, { "" })
            fail("Expected decryption failure")
        } catch (_: IOException) { assertTrue(original.exists()) }
    }

    @Test fun sdkCannotClaimOriginalAsItsTemporaryExport() = fixture { _, cache, original ->
        try {
            MyTelPlainExportSource.prepare(original, cache, { true }, { original.path })
            fail("Expected ownership refusal")
        } catch (_: IOException) { assertTrue(original.exists()) }
    }

    @Test fun preexistingCacheFileIsNeverOwnedOrDeleted() = fixture { _, cache, original ->
        val borrowed = File(cache, "chat-viewer.mkv").apply { writeText("another viewer") }
        try {
            MyTelPlainExportSource.prepare(original, cache, { true }, { borrowed.path })
            fail("Expected pre-existing cache ownership refusal")
        } catch (_: IOException) { assertEquals("another viewer", borrowed.readText()) }
    }

    @Test fun sdkPathsOutsidePlainCacheAreNeverDeleted() = fixture { directory, cache, original ->
        val outside = File(directory, "unrelated.mkv").apply { writeText("private") }
        try {
            MyTelPlainExportSource.prepare(original, cache, { true }, { outside.path })
            fail("Expected outside cache refusal")
        } catch (_: IOException) { assertEquals("private", outside.readText()) }
    }

    @Test fun failedDecryptionReturningCiphertextIsRejectedAndOwnedTemporaryRemoved() = fixture { _, cache, original ->
        val bad = File(cache, "failed.mkv")
        try {
            MyTelPlainExportSource.prepare(original, cache, { true }, { bad.writeText("bcEncryptedFs-still-cipher"); bad.path })
            fail("Expected ciphertext rejection")
        } catch (_: IOException) { assertFalse(bad.exists()); assertTrue(original.exists()) }
    }

    @Test fun copyFailureStillReleasesOwnedPlaintext() = fixture { _, cache, original ->
        val sdk = File(cache, "sdk.mkv")
        var plain: File? = null
        try {
            MyTelPlainExportSource.prepare(original, cache, { true }, { sdk.writeText("plain"); sdk.path }).use {
                plain = it.file
                throw IOException("Provider disconnected")
            }
        } catch (_: IOException) { assertFalse(plain!!.exists()); assertTrue(original.exists()) }
    }

    @Test fun cancellationAfterPreparationStillReleasesPlaintext() = fixture { _, cache, original ->
        val sdk = File(cache, "sdk.mkv")
        var plain: File? = null
        try {
            MyTelPlainExportSource.prepare(original, cache, { true }, { sdk.writeText("plain"); sdk.path }).use {
                plain = it.file
                throw CancellationException("Activity abandoned the export")
            }
        } catch (_: CancellationException) { assertFalse(plain!!.exists()); assertTrue(original.exists()) }
    }

    @Test fun missingSdkOutputIsFailureWithoutAnyFallbackCopy() = fixture { _, cache, original ->
        try {
            MyTelPlainExportSource.prepare(original, cache, { true }, { File(cache, "absent.mkv").path })
            fail("Expected absent plaintext failure")
        } catch (_: IOException) { assertTrue(original.exists()) }
    }

    @Test fun validEmptyPlaintextIsStillAValidDecryptedExport() = fixture { _, cache, original ->
        val sdk = File(cache, "empty.mkv")
        val source = MyTelPlainExportSource.prepare(original, cache, { true }, { sdk.writeBytes(byteArrayOf()); sdk.path })
        assertEquals(0L, source.file.length())
        source.close()
        assertTrue(original.exists())
    }

    @Test fun abandonedOwnedFilesAreRemovedButActiveAndOtherCachesArePreserved() = fixture { _, cache, original ->
        val sdk = File(cache, "sdk.mkv")
        val active = MyTelPlainExportSource.prepare(original, cache, { true }, { sdk.writeText("active"); sdk.path })
        val abandoned = File(cache, "mytel-document-export-" + "a".repeat(32) + ".plain").apply { writeText("orphan") }
        File(cache, abandoned.name + ".owner").writeText("mytel-document-export-v1:" + "a".repeat(32))
        val unrelated = File(cache, "chat-plaintext.mkv").apply { writeText("other viewer") }
        assertEquals(1, MyTelPlainExportSource.clearAbandoned(cache))
        assertTrue(active.file.exists())
        assertFalse(abandoned.exists())
        assertTrue(unrelated.exists())
        assertTrue(MyTelPlainExportSource.isActive(active.file))
        active.close()
        assertFalse(MyTelPlainExportSource.isActive(active.file))
    }

    @Test fun fileWithOnlySimilarPrefixIsNotClaimedAsAbandoned() = fixture { _, cache, _ ->
        val other = File(cache, "mytel-document-export-not-a-uuid.plain").apply { writeText("unrelated") }
        assertEquals(0, MyTelPlainExportSource.clearAbandoned(cache))
        assertTrue(other.exists())
    }

    @Test fun nonceNamedFileWithoutOurOwnershipRecordIsNeverDeleted() = fixture { _, cache, _ ->
        val original = File(cache, "mytel-document-export-" + "b".repeat(32) + ".plain").apply { writeText("unrelated original") }
        assertEquals(0, MyTelPlainExportSource.clearAbandoned(cache))
        assertTrue(original.exists())
    }
}
