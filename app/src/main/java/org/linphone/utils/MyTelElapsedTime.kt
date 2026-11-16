package org.linphone.utils

import java.util.Locale

/** A duration has no calendar date or time zone. SDK media durations use milliseconds. */
object MyTelElapsedTime {
    fun formatMilliseconds(milliseconds: Long, locale: Locale = Locale.getDefault()): String {
        val seconds = milliseconds.coerceAtLeast(0) / 1000
        return if (seconds < 3600) {
            String.format(locale, "%02d:%02d", seconds / 60, seconds % 60)
        } else {
            String.format(locale, "%d:%02d:%02d", seconds / 3600, seconds / 60 % 60, seconds % 60)
        }
    }
}
