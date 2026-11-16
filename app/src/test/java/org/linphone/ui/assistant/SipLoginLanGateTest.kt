package org.linphone.ui.assistant

import org.junit.Assert.*
import org.junit.Test

class SipLoginLanGateTest {
    @Test fun requiredGrantResumesExactlyOnceAfterValidationAndLaunch() {
        val gate = SipLoginLanGate()
        val id = gate.begin(SipLocalNetworkPolicy.Need.REQUIRED)!!
        assertEquals(SipLoginLanGate.Action.WAIT, gate.action(id, true))
        gate.validated(id, true, false)
        assertEquals(SipLoginLanGate.Action.REQUEST, gate.action(id, true))
        assertTrue(gate.claimLaunch(id)); assertFalse(gate.claimLaunch(id))
        assertTrue(gate.result(id, true)); assertFalse(gate.result(id, true))
        assertEquals(SipLoginLanGate.Action.START, gate.action(id, true))
        assertEquals(SipLoginLanGate.Action.WAIT, gate.action(id, true))
    }
    @Test fun requiredDenialNeverStartsAnAttempt() {
        val gate = SipLoginLanGate(); val id = gate.begin(SipLocalNetworkPolicy.Need.REQUIRED)!!
        gate.validated(id, true, false); gate.claimLaunch(id); gate.result(id, false)
        assertEquals(SipLoginLanGate.Action.DENIED, gate.action(id, true))
        assertNull(gate.current())
    }
    @Test fun optionalDenialAllowsPublicPathAndDoesNotPromptAgain() {
        val gate = SipLoginLanGate(); val id = gate.begin(SipLocalNetworkPolicy.Need.OPTIONAL)!!
        gate.validated(id, true, false); gate.claimLaunch(id); gate.result(id, false)
        assertEquals(SipLoginLanGate.Action.START_WITH_HINT, gate.action(id, true))
        val next = gate.begin(SipLocalNetworkPolicy.Need.OPTIONAL)!!
        gate.validated(next, true, false)
        assertEquals(SipLoginLanGate.Action.START_WITH_HINT, gate.action(next, true))
    }
    @Test fun api24PreGrantedAndNumericPublicDoNotLaunchRequests() {
        listOf(Triple(SipLocalNetworkPolicy.Need.REQUIRED, false, false), Triple(SipLocalNetworkPolicy.Need.REQUIRED, true, true), Triple(SipLocalNetworkPolicy.Need.NONE, true, false)).forEach { (need, applies, granted) ->
            val gate = SipLoginLanGate(); val id = gate.begin(need)!!
            gate.validated(id, applies, granted)
            assertEquals(SipLoginLanGate.Action.START, gate.action(id, true))
        }
    }
    @Test fun doubleTapCancelAndDelayedValidationCannotRestartLogin() {
        val gate = SipLoginLanGate(); val old = gate.begin(SipLocalNetworkPolicy.Need.REQUIRED)!!
        assertNull(gate.begin(SipLocalNetworkPolicy.Need.REQUIRED))
        assertTrue(gate.cancel()); gate.validated(old, true, false)
        assertEquals(SipLoginLanGate.Action.WAIT, gate.action(old, true))
        val next = gate.begin(SipLocalNetworkPolicy.Need.REQUIRED)!!
        assertFalse(gate.result(old, true)); assertTrue(gate.isCurrent(next))
    }
    @Test fun rotationAndBackgroundDeferResultUntilMatchingActiveView() {
        val gate = SipLoginLanGate(); val id = gate.begin(SipLocalNetworkPolicy.Need.REQUIRED)!!
        gate.validated(id, true, false); gate.claimLaunch(id); gate.result(id, true)
        assertEquals(SipLoginLanGate.Action.WAIT, gate.action(id, false))
        assertEquals(id, gate.current())
        assertEquals(SipLoginLanGate.Action.START, gate.action(id, true))
    }
    @Test fun newProcessAndCancelledPermissionResultsAreIgnored() {
        val oldGate = SipLoginLanGate(); val old = oldGate.begin(SipLocalNetworkPolicy.Need.REQUIRED)!!
        oldGate.validated(old, true, false); oldGate.claimLaunch(old); oldGate.cancel()
        assertFalse(oldGate.result(old, true))
        val newGate = SipLoginLanGate(); val next = newGate.begin(SipLocalNetworkPolicy.Need.REQUIRED)!!
        assertFalse(newGate.result(old, true)); assertTrue(newGate.isCurrent(next))
    }
}
