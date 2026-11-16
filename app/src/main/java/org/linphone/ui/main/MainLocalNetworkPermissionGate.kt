package org.linphone.ui.main

import java.util.UUID
import org.linphone.ui.assistant.SipLocalNetworkPolicy

/** Main-thread, explicit retry intent. Registration failures only compute an alert. */
class MainLocalNetworkPermissionGate {
    data class AccountState(val pointer: Long, val identity: String, val routes: List<String>, val enabled: Boolean, val failed: Boolean)
    data class Result(val granted: Boolean, val targets: Set<Long>)
    private data class Pending(val token: String, val targets: Set<Long>, var launched: Boolean = false, var granted: Boolean? = null)
    private var pending: Pending? = null

    companion object {
        fun blockedAccounts(values: List<AccountState>, permissionApplies: Boolean, granted: Boolean): List<AccountState> =
            if (!permissionApplies || granted) emptyList() else values.filter {
                it.pointer != 0L && it.enabled && it.failed && SipLocalNetworkPolicy.routes(it.routes) == SipLocalNetworkPolicy.Need.REQUIRED
            }.distinctBy { it.pointer }

        fun matchesForRefresh(before: AccountState, current: AccountState): Boolean =
            before.pointer != 0L && before.pointer == current.pointer && before.identity == current.identity &&
                before.routes == current.routes && current.enabled && current.failed &&
                SipLocalNetworkPolicy.routes(current.routes) == SipLocalNetworkPolicy.Need.REQUIRED
    }

    fun current(): String? = pending?.token
    fun begin(targets: Set<Long>): String? {
        if (pending != null || targets.isEmpty() || 0L in targets) return null
        return UUID.randomUUID().toString().also { pending = Pending(it, targets.toSet()) }
    }
    fun claimLaunch(token: String): Boolean {
        val value = pending?.takeIf { it.token == token && !it.launched } ?: return false
        value.launched = true
        return true
    }
    fun result(token: String, granted: Boolean): Boolean {
        val value = pending?.takeIf { it.token == token && it.launched && it.granted == null } ?: return false
        value.granted = granted
        return true
    }
    fun consume(token: String, active: Boolean): Result? {
        val value = pending?.takeIf { active && it.token == token } ?: return null
        val granted = value.granted ?: return null
        pending = null
        return Result(granted, value.targets)
    }
    fun cancel(token: String) { if (pending?.token == token) pending = null }
}
