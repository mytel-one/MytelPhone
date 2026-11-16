package org.linphone.utils

import android.content.Context

object MyTelPermissionRequestHistory {
    private const val PREFERENCES = "mytel_runtime_permission_requests"

    fun wasRequested(context: Context, permission: String): Boolean =
        context.getSharedPreferences(PREFERENCES, Context.MODE_PRIVATE).getBoolean(permission, false)

    fun record(context: Context, permission: String) {
        context.getSharedPreferences(PREFERENCES, Context.MODE_PRIVATE).edit()
            .putBoolean(permission, true).apply()
    }
}
