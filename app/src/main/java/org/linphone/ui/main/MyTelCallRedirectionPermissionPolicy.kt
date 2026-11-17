package org.linphone.ui.main

/** Decides only the optional call-redirection permission continuation. */
object MyTelCallRedirectionPermissionPolicy {
    enum class Action { REQUEST_ROLE, DISABLE, IGNORE }

    fun onResult(permissionResults: Map<String, Boolean>, activityAlive: Boolean): Action {
        // Extracted legacy aggregation so the regression is exercised first.
        return if (permissionResults.values.all { it }) Action.REQUEST_ROLE else Action.DISABLE
    }
}
