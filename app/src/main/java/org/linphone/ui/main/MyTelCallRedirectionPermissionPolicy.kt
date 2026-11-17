package org.linphone.ui.main

/** Decides only the optional call-redirection permission continuation. */
object MyTelCallRedirectionPermissionPolicy {
    private const val PHONE_STATE_PERMISSION = "android.permission.READ_PHONE_STATE"

    enum class Action { REQUEST_ROLE, DISABLE, IGNORE }

    fun onResult(permissionResults: Map<String, Boolean>, activityAlive: Boolean): Action {
        if (!activityAlive) return Action.IGNORE

        // An empty/cancelled result is not a grant. Results from older requests
        // may contain extra permissions, which cannot grant or block this role.
        return if (permissionResults[PHONE_STATE_PERMISSION] == true) {
            Action.REQUEST_ROLE
        } else {
            Action.DISABLE
        }
    }
}
