package org.linphone.utils

import java.util.Locale
import java.util.TimeZone
import org.junit.Assert.assertEquals
import org.junit.Test

class MyTelElapsedTimeTest {
    @Test
    fun elapsedMillisecondsStartAtZeroAndRoundDownToWholeSeconds() {
        for ((milliseconds, expected) in listOf(-1L to "00:00", 0L to "00:00", 999L to "00:00", 12_345L to "00:12", 59_999L to "00:59", 60_000L to "01:00")) {
            assertEquals(expected, MyTelElapsedTime.formatMilliseconds(milliseconds, Locale.US))
        }
    }

    @Test
    fun elapsedDisplayKeepsHoursInsteadOfWrappingAtSixtyMinutes() {
        assertEquals("59:59", MyTelElapsedTime.formatMilliseconds(3_599_999L, Locale.US))
        assertEquals("1:00:00", MyTelElapsedTime.formatMilliseconds(3_600_000L, Locale.US))
        assertEquals("25:01:02", MyTelElapsedTime.formatMilliseconds(90_062_000L, Locale.US))
    }

    @Test
    fun changingTheDeviceTimeZoneNeverChangesTheElapsedDuration() {
        val previous = TimeZone.getDefault()
        try {
            for (zone in listOf("Asia/Tehran", "Asia/Kathmandu", "Pacific/Chatham", "America/St_Johns", "UTC")) {
                TimeZone.setDefault(TimeZone.getTimeZone(zone))
                assertEquals(zone, "00:00", MyTelElapsedTime.formatMilliseconds(0, Locale.US))
                assertEquals(zone, "00:28", MyTelElapsedTime.formatMilliseconds(28_201, Locale.US))
            }
        } finally {
            TimeZone.setDefault(previous)
        }
    }

    @Test
    fun elapsedDisplayUsesTheRequestedDigitLocale() {
        val locale = Locale.forLanguageTag("fa-IR")
        assertEquals(String.format(locale, "%02d:%02d", 1, 12), MyTelElapsedTime.formatMilliseconds(72_000, locale))
    }

    @Test
    fun veryLongDurationsDoNotOverflowOrBecomeNegative() {
        val seconds = Long.MAX_VALUE / 1000
        assertEquals(String.format(Locale.US, "%d:%02d:%02d", seconds / 3600, seconds / 60 % 60, seconds % 60), MyTelElapsedTime.formatMilliseconds(Long.MAX_VALUE, Locale.US))
    }
}
