package org.linphone.utils

object MyTelPermissionRequestPolicy {
    enum class Action { NONE, REQUEST, SETTINGS }

    fun resolve(granted: Boolean, requestedBefore: Boolean, showRationale: Boolean): Action = when {
        granted -> Action.NONE
        !requestedBefore || showRationale -> Action.REQUEST
        else -> Action.SETTINGS
    }

    fun canResume(pendingCall: Long, activeCalls: Set<Long>, activityAlive: Boolean): Boolean =
        activityAlive && pendingCall != 0L && pendingCall in activeCalls
}
