package org.linphone.utils

internal data class MyTelImageOrientation(
    val rotationDegrees: Int = 0,
    val mirrorHorizontally: Boolean = false
)

/** Platform decisions shared by the Android 7 image and notification fallbacks. */
internal object MyTelLegacyCompatibility {
    fun notificationImportance(
        sdkInt: Int,
        appNotificationsEnabled: Boolean,
        channelImportance: Int?,
        legacyImportance: Int
    ): Int {
        // Android 7 has no notification channels. A null channel there does not
        // mean that the user disabled the call or keep-alive notification.
        if (sdkInt < 26) return if (appNotificationsEnabled) legacyImportance else 0
        return channelImportance ?: 0
    }

    fun bitmapOrientation(exifOrientation: Int): MyTelImageOrientation = when (exifOrientation) {
        2 -> MyTelImageOrientation(0, true)
        3 -> MyTelImageOrientation(180)
        4 -> MyTelImageOrientation(180, true)
        5 -> MyTelImageOrientation(90, true)
        6 -> MyTelImageOrientation(90)
        7 -> MyTelImageOrientation(-90, true)
        8 -> MyTelImageOrientation(-90)
        else -> MyTelImageOrientation()
    }
}
