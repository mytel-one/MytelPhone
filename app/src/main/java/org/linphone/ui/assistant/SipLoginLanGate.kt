package org.linphone.ui.assistant

import java.util.UUID

/** In-memory Connect intent. Only the nonsecret request token may cross a view recreation. */
class SipLoginLanGate(private val token: () -> String = { UUID.randomUUID().toString() }) {
    enum class Action { WAIT, REQUEST, START, START_WITH_HINT, DENIED }
    private enum class Phase { VALIDATING, REQUEST, LAUNCHED, READY, DENIED }
    private data class Pending(val id: String, val need: SipLocalNetworkPolicy.Need, var phase: Phase, var hint: Boolean = false)
    private var pending: Pending? = null
    private var optionalAsked = false

    fun begin(need: SipLocalNetworkPolicy.Need): String? {
        if (pending != null || need == SipLocalNetworkPolicy.Need.INVALID) return null
        return token().also { pending = Pending(it, need, Phase.VALIDATING) }
    }

    fun current(): String? = pending?.id
    fun isCurrent(id: String): Boolean = pending?.id == id

    fun validated(id: String, permissionApplies: Boolean, granted: Boolean) {
        val value = pending?.takeIf { it.id == id && it.phase == Phase.VALIDATING } ?: return
        value.hint = permissionApplies && !granted && value.need == SipLocalNetworkPolicy.Need.OPTIONAL && optionalAsked
        value.phase = if (!permissionApplies || granted || value.need == SipLocalNetworkPolicy.Need.NONE ||
            (value.need == SipLocalNetworkPolicy.Need.OPTIONAL && optionalAsked)
        ) Phase.READY else Phase.REQUEST
    }

    fun claimLaunch(id: String): Boolean {
        val value = pending?.takeIf { it.id == id && it.phase == Phase.REQUEST } ?: return false
        value.phase = Phase.LAUNCHED
        if (value.need == SipLocalNetworkPolicy.Need.OPTIONAL) optionalAsked = true
        return true
    }

    fun result(id: String, granted: Boolean): Boolean {
        val value = pending?.takeIf { it.id == id && it.phase == Phase.LAUNCHED } ?: return false
        value.hint = !granted && value.need == SipLocalNetworkPolicy.Need.OPTIONAL
        value.phase = if (!granted && value.need == SipLocalNetworkPolicy.Need.REQUIRED) Phase.DENIED else Phase.READY
        return true
    }

    fun action(id: String, activeContext: Boolean): Action {
        val value = pending?.takeIf { it.id == id } ?: return Action.WAIT
        if (!activeContext) return Action.WAIT
        return when (value.phase) {
            Phase.REQUEST -> Action.REQUEST
            Phase.READY -> { pending = null; if (value.hint) Action.START_WITH_HINT else Action.START }
            Phase.DENIED -> { pending = null; Action.DENIED }
            else -> Action.WAIT
        }
    }

    fun cancel(): Boolean = (pending != null).also { pending = null }
}
