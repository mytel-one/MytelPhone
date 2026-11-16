package org.linphone.utils

internal data class MyTelCallNotificationAlert(val priority: Int, val fullScreen: Boolean)

internal object MyTelCallNotificationPolicy {
    // Stable NotificationCompat values, kept independent of Android for policy tests.
    private const val PRIORITY_MAX = 2
    private const val PRIORITY_LOW = -1

    fun forCall(incoming: Boolean): MyTelCallNotificationAlert =
        MyTelCallNotificationAlert(if (incoming) PRIORITY_MAX else PRIORITY_LOW, incoming)

    fun <T> contentIntentForUpdate(contentIntent: T?, fullScreenIntent: T?): T? =
        contentIntent ?: fullScreenIntent
}
