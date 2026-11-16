package org.linphone.utils

import org.junit.Assert.assertEquals
import org.junit.Test

class MyTelLegacyCompatibilityTest {
    @Test
    fun enabledNougatKeepAliveIsNotBlockedByMissingNotificationChannels() {
        assertEquals(2, MyTelLegacyCompatibility.notificationImportance(24, true, null, 2))
    }

    @Test
    fun enabledNougatCallCanUseItsLegacyHighPriority() {
        assertEquals(4, MyTelLegacyCompatibility.notificationImportance(25, true, null, 4))
    }

    @Test
    fun disabledLegacyNotificationsRemainDisabled() {
        assertEquals(0, MyTelLegacyCompatibility.notificationImportance(24, false, null, 4))
    }

    @Test
    fun legacyPlatformDoesNotDependOnAnUnavailableChannelSetting() {
        assertEquals(4, MyTelLegacyCompatibility.notificationImportance(25, true, 0, 4))
    }

    @Test
    fun missingOreoChannelRemainsDisabledInsteadOfBypassingUserSettings() {
        assertEquals(0, MyTelLegacyCompatibility.notificationImportance(26, true, null, 4))
    }

    @Test
    fun disabledModernChannelRemainsDisabled() {
        assertEquals(0, MyTelLegacyCompatibility.notificationImportance(28, true, 0, 4))
    }

    @Test
    fun modernChannelImportanceIsPreserved() {
        assertEquals(2, MyTelLegacyCompatibility.notificationImportance(26, true, 2, 4))
        assertEquals(4, MyTelLegacyCompatibility.notificationImportance(37, true, 4, 2))
    }

    @Test
    fun modernChannelPolicyRemainsIndependentOfLegacyFallback() {
        assertEquals(4, MyTelLegacyCompatibility.notificationImportance(37, false, 4, 2))
    }
}
