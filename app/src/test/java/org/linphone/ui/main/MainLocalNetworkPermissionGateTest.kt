package org.linphone.ui.main

import org.junit.Assert.*
import org.junit.Test

class MainLocalNetworkPermissionGateTest {
    private fun account(id: Long = 1, routes: List<String> = listOf("sip:192.168.1.2"), enabled: Boolean = true, failed: Boolean = true, identity: String = "sip:201@example.org") =
        MainLocalNetworkPermissionGate.AccountState(id, identity, routes, enabled, failed)

    @Test fun onlyEnabledFailedKnownLanRoutesNeedTheAlert() {
        val values = listOf(account(), account(2, listOf("sip:8.8.8.8")), account(3, listOf("sip:cloud.example.org")), account(4, listOf("sip:127.0.0.1")), account(5, enabled = false), account(6, failed = false))
        assertEquals(listOf(1L), MainLocalNetworkPermissionGate.blockedAccounts(values, true, false).map { it.pointer })
        assertTrue(MainLocalNetworkPermissionGate.blockedAccounts(values, false, false).isEmpty())
        assertTrue(MainLocalNetworkPermissionGate.blockedAccounts(values, true, true).isEmpty())
        val signedPointer = account(-7)
        assertEquals(listOf(-7L), MainLocalNetworkPermissionGate.blockedAccounts(listOf(signedPointer), true, false).map { it.pointer })
        assertTrue(MainLocalNetworkPermissionGate.matchesForRefresh(signedPointer, signedPointer))
        assertNotNull(MainLocalNetworkPermissionGate().begin(setOf(-7L)))
    }

    @Test fun everyConfiguredRouteIsClassifiedAndFailuresNeverCreateARequest() {
        val gate = MainLocalNetworkPermissionGate()
        val value = account(routes = listOf("sips:cloud.example.org", "sip:8.8.8.8", "sip:[fd00::10]:5061"))
        repeat(3) { assertEquals(listOf(value), MainLocalNetworkPermissionGate.blockedAccounts(listOf(value), true, false)) }
        assertNull(gate.current()) // Only explicit begin(), never registration failure, starts a flight.
    }

    @Test fun explicitDoubleTapAndRotationCannotLaunchTwice() {
        val gate = MainLocalNetworkPermissionGate()
        val token = gate.begin(setOf(1))!!
        assertNull(gate.begin(setOf(2)))
        assertTrue(gate.claimLaunch(token)); assertFalse(gate.claimLaunch(token))
        assertTrue(gate.result(token, true))
        assertNull(gate.consume(token, false))
        assertEquals(token, gate.current())
        assertEquals(setOf(1L), gate.consume(token, true)!!.targets)
        assertNull(gate.consume(token, true))
    }

    @Test fun denialEndsFlightWithoutAnAutomaticRetry() {
        val gate = MainLocalNetworkPermissionGate()
        val token = gate.begin(setOf(1))!!
        gate.claimLaunch(token); gate.result(token, false)
        assertFalse(gate.consume(token, true)!!.granted)
        assertNull(gate.current()); assertFalse(gate.result(token, true))
        assertNotNull(gate.begin(setOf(1))) // A new explicit tap may retry.
    }

    @Test fun cancelLaunchFailureAndOldProcessResultsCannotReviveARequest() {
        val gate = MainLocalNetworkPermissionGate()
        val old = gate.begin(setOf(1))!!
        gate.claimLaunch(old); gate.cancel(old)
        assertFalse(gate.result(old, true))
        val next = gate.begin(setOf(2))!!
        assertFalse(gate.result(old, true)); assertFalse(gate.claimLaunch(old))
        assertEquals(next, gate.current())
        assertFalse(MainLocalNetworkPermissionGate().result(next, true))
    }

    @Test fun emptyTargetsAndUnlaunchedResultsAreRejected() {
        val gate = MainLocalNetworkPermissionGate()
        assertNull(gate.begin(emptySet()))
        val token = gate.begin(setOf(1))!!
        assertFalse(gate.result(token, true)); assertNull(gate.consume(token, true))
    }

    @Test fun grantRefreshRequiresTheSameConfiguredIdentityAndRoutes() {
        val old = account()
        assertTrue(MainLocalNetworkPermissionGate.matchesForRefresh(old, old))
        listOf(account(2), account(identity = "sip:202@example.org"), account(routes = listOf("sip:192.168.1.3")), account(enabled = false), account(failed = false), account(routes = listOf("sip:cloud.example.org"))).forEach {
            assertFalse(MainLocalNetworkPermissionGate.matchesForRefresh(old, it))
        }
    }
}
