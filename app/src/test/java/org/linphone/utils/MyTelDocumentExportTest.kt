package org.linphone.utils

import java.io.ByteArrayInputStream
import java.io.ByteArrayOutputStream
import java.io.IOException
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class MyTelDocumentExportTest {
    @Test
    fun pendingSourceCanBeRestoredAfterActivityRecreation() {
        val state = MyTelDocumentExport()
        assertTrue(state.begin("/private/recording.wav"))
        val restored = MyTelDocumentExport(state.pendingPath)
        assertEquals("/private/recording.wav", restored.takePendingPath())
        assertNull(restored.takePendingPath())
    }

    @Test
    fun emptyPathsDoNotStartAnExport() {
        val state = MyTelDocumentExport()
        assertFalse(state.begin("  "))
        assertNull(state.pendingPath)
    }

    @Test
    fun secondRequestCannotReplaceTheFileWhilePickerIsOpen() {
        val state = MyTelDocumentExport()
        assertTrue(state.begin("/private/first.wav"))
        assertFalse(state.begin("/private/second.wav"))
        assertEquals("/private/first.wav", state.takePendingPath())
    }

    @Test
    fun cancellationClearsOnlyThePendingRequestAndAllowsRetry() {
        val state = MyTelDocumentExport("/private/cancelled.wav")
        state.takePendingPath()
        assertTrue(state.begin("/private/retry.wav"))
        assertEquals("/private/retry.wav", state.pendingPath)
    }

    @Test
    fun documentTitleDoesNotContainDirectoryTraversalOrControls() {
        assertEquals("recording.wav", MyTelDocumentExport.safeFileName("../private/recording.wav"))
        assertEquals("file.txt", MyTelDocumentExport.safeFileName("C:\\private\\file\u0000.txt"))
        assertEquals("Mytel-export", MyTelDocumentExport.safeFileName(".."))
    }

    @Test
    fun unicodeDocumentNamesArePreserved() {
        assertEquals("جلسه.wav", MyTelDocumentExport.safeFileName("جلسه.wav"))
    }

    @Test
    fun successfulCopyIncludesAllBytesAndClosesBothStreams() {
        val bytes = byteArrayOf(0, 1, 2, -1, 42)
        var inputClosed = false
        var outputClosed = false
        val input = object : ByteArrayInputStream(bytes) {
            override fun close() { inputClosed = true; super.close() }
        }
        val output = object : ByteArrayOutputStream() {
            override fun close() { outputClosed = true; super.close() }
        }
        assertTrue(MyTelDocumentExport.copy({ input }, { output }))
        assertArrayEquals(bytes, output.toByteArray())
        assertTrue(inputClosed)
        assertTrue(outputClosed)
    }

    @Test
    fun missingDestinationDoesNotReportSuccessOrOpenTheSource() {
        var sourceOpened = false
        assertFalse(MyTelDocumentExport.copy({ sourceOpened = true; ByteArrayInputStream(byteArrayOf(1)) }, { null }))
        assertFalse(sourceOpened)
    }

    @Test
    fun missingSourceClosesTheOpenedDestinationAndReportsFailure() {
        var closed = false
        val output = object : ByteArrayOutputStream() {
            override fun close() { closed = true; super.close() }
        }
        assertFalse(MyTelDocumentExport.copy({ null }, { output }))
        assertTrue(closed)
    }

    @Test(expected = IOException::class)
    fun destinationCloseFailureCannotBecomeASuccessResult() {
        val output = object : ByteArrayOutputStream() {
            override fun close() { throw IOException("Storage disconnected during close") }
        }
        MyTelDocumentExport.copy({ ByteArrayInputStream(byteArrayOf(1)) }, { output })
    }

    @Test
    fun readFailureStillClosesBothStreams() {
        var inputClosed = false
        var outputClosed = false
        val input = object : ByteArrayInputStream(byteArrayOf(1)) {
            override fun read(b: ByteArray, off: Int, len: Int): Int { throw IOException("Read failed") }
            override fun close() { inputClosed = true; super.close() }
        }
        val output = object : ByteArrayOutputStream() {
            override fun close() { outputClosed = true; super.close() }
        }
        try {
            MyTelDocumentExport.copy({ input }, { output })
            throw AssertionError("Expected the copy failure")
        } catch (_: IOException) {
            assertTrue(inputClosed)
            assertTrue(outputClosed)
        }
    }

    @Test
    fun validEmptyDocumentCanBeExported() {
        assertTrue(MyTelDocumentExport.copy({ ByteArrayInputStream(byteArrayOf()) }, { ByteArrayOutputStream() }))
    }

    @Test
    fun providerRuntimeFailureClosesTheOpenedDestination() {
        var closed = false
        val output = object : ByteArrayOutputStream() {
            override fun close() { closed = true; super.close() }
        }
        try {
            MyTelDocumentExport.copy({ throw IllegalStateException("Provider disconnected") }, { output })
            throw AssertionError("Expected the provider failure")
        } catch (_: IllegalStateException) {
            assertTrue(closed)
        }
    }
}
