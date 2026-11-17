package org.linphone.ui.main

import org.junit.Assert.assertEquals
import org.junit.Test

class MyTelCallRedirectionPermissionPolicyTest {
    private val phoneState = "android.permission.READ_PHONE_STATE"
    private val removedNumberPermission = "android.permission.READ_PHONE_NUMBERS"

    @Test fun grantedPhoneStateContinuesToTheOptionalRole() {
        assertEquals(MyTelCallRedirectionPermissionPolicy.Action.REQUEST_ROLE,
            MyTelCallRedirectionPermissionPolicy.onResult(mapOf(phoneState to true), true))
    }

    @Test fun deniedPhoneStateDisablesOnlyCallRedirection() {
        assertEquals(MyTelCallRedirectionPermissionPolicy.Action.DISABLE,
            MyTelCallRedirectionPermissionPolicy.onResult(mapOf(phoneState to false), true))
    }

    @Test fun cancelledEmptyResultCannotRequestTheRole() {
        assertEquals(MyTelCallRedirectionPermissionPolicy.Action.DISABLE,
            MyTelCallRedirectionPermissionPolicy.onResult(emptyMap(), true))
    }

    @Test fun unrelatedGrantCannotReplaceTheRequiredPhoneStateGrant() {
        assertEquals(MyTelCallRedirectionPermissionPolicy.Action.DISABLE,
            MyTelCallRedirectionPermissionPolicy.onResult(mapOf(removedNumberPermission to true), true))
    }

    @Test fun oldNumberPermissionDenialCannotBlockGrantedPhoneState() {
        assertEquals(MyTelCallRedirectionPermissionPolicy.Action.REQUEST_ROLE,
            MyTelCallRedirectionPermissionPolicy.onResult(
                mapOf(phoneState to true, removedNumberPermission to false), true))
    }

    @Test fun destroyedActivityIgnoresALateGrant() {
        assertEquals(MyTelCallRedirectionPermissionPolicy.Action.IGNORE,
            MyTelCallRedirectionPermissionPolicy.onResult(mapOf(phoneState to true), false))
    }

    @Test fun destroyedActivityIgnoresALateDenial() {
        assertEquals(MyTelCallRedirectionPermissionPolicy.Action.IGNORE,
            MyTelCallRedirectionPermissionPolicy.onResult(mapOf(phoneState to false), false))
    }

    @Test fun destroyedActivityIgnoresALateCancellation() {
        assertEquals(MyTelCallRedirectionPermissionPolicy.Action.IGNORE,
            MyTelCallRedirectionPermissionPolicy.onResult(emptyMap(), false))
    }
}
