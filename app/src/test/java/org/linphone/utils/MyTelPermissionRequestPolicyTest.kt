package org.linphone.utils

import org.junit.Assert.assertEquals
import org.junit.Test

class MyTelPermissionRequestPolicyTest {
    @Test fun neverAskedPermissionRequestsEvenWithoutRationale() {
        assertEquals(MyTelPermissionRequestPolicy.Action.REQUEST, MyTelPermissionRequestPolicy.resolve(false, false, false))
    }
    @Test fun deniedOnceCanBeRequestedAgain() {
        assertEquals(MyTelPermissionRequestPolicy.Action.REQUEST, MyTelPermissionRequestPolicy.resolve(false, true, true))
    }
    @Test fun permanentlyDeniedPermissionUsesSettingsOnlyAfterARequest() {
        assertEquals(MyTelPermissionRequestPolicy.Action.SETTINGS, MyTelPermissionRequestPolicy.resolve(false, true, false))
    }
    @Test fun grantedPermissionDoesNotRequestOrOpenSettings() {
        assertEquals(MyTelPermissionRequestPolicy.Action.NONE, MyTelPermissionRequestPolicy.resolve(true, true, false))
    }
    @Test fun permissionResultResumesOnlyTheOriginalStillActiveCall() {
        assertEquals(true, MyTelPermissionRequestPolicy.canResume(42, setOf(42, 43), true, 42))
        assertEquals(false, MyTelPermissionRequestPolicy.canResume(42, setOf(43), true, 42))
        assertEquals(false, MyTelPermissionRequestPolicy.canResume(42, setOf(42), false, 42))
        assertEquals(false, MyTelPermissionRequestPolicy.canResume(0, setOf(0), true, 0))
    }
    @Test fun incomingSelectionChangeCannotApplyAnOlderPermissionIntent() {
        assertEquals(false, MyTelPermissionRequestPolicy.canResume(42, setOf(42, 43), true, 43))
        assertEquals(true, MyTelPermissionRequestPolicy.canResume(42, setOf(42, 43), true, 42))
    }
}
