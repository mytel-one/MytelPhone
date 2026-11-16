package org.linphone.utils

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertSame
import org.junit.Assert.assertTrue
import org.junit.Test

class MyTelCallNotificationPolicyTest {
    @Test
    fun incomingRingKeepsMaximumPriorityAndFullScreenPresentation() {
        val ringing = MyTelCallNotificationPolicy.forCall(incoming = true)
        assertEquals(2, ringing.priority)
        assertTrue(ringing.fullScreen)
    }

    @Test
    fun incomingToConnectedTransitionRemovesStickyFullScreenAndHighPriority() {
        val ringing = MyTelCallNotificationPolicy.forCall(incoming = true)
        val connected = MyTelCallNotificationPolicy.forCall(incoming = false)
        assertTrue(ringing.fullScreen)
        assertFalse(connected.fullScreen)
        assertTrue(connected.priority < 0)
    }

    @Test
    fun outgoingCallDoesNotCreateAnIncomingRingHeadsUp() {
        val outgoing = MyTelCallNotificationPolicy.forCall(incoming = false)
        assertEquals(-1, outgoing.priority)
        assertFalse(outgoing.fullScreen)
    }

    @Test
    fun speakerOrContactUpdateKeepsTapActionWithoutAFullScreenIntent() {
        val activeCall = Any()
        assertSame(activeCall, MyTelCallNotificationPolicy.contentIntentForUpdate(activeCall, null))
    }

    @Test
    fun normalTapActionTakesPrecedenceOverAnOldFullScreenTarget() {
        val activeCall = Any()
        val oldRingingTarget = Any()
        assertSame(activeCall, MyTelCallNotificationPolicy.contentIntentForUpdate(activeCall, oldRingingTarget))
    }

    @Test
    fun olderIncomingNotificationCanStillUseItsOnlyAvailableTapTarget() {
        val ringingTarget = Any()
        assertSame(ringingTarget, MyTelCallNotificationPolicy.contentIntentForUpdate(null, ringingTarget))
        assertNull(MyTelCallNotificationPolicy.contentIntentForUpdate<Any>(null, null))
    }
}
