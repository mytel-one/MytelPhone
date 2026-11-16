package org.linphone.ui.assistant

import org.junit.Assert.assertEquals
import org.junit.Test

class SipLocalNetworkPolicyTest {
    @Test fun privateAndLinkLocalRoutesRequirePermission() {
        listOf("10.0.2.2:5098", "172.16.0.1", "172.31.255.255", "192.168.8.171:5060", "169.254.1.1", "SIPS:[fd12::1]:5061", "sip:[fe80::1%wlan0]:5060", "pbx.LOCAL.", "[::ffff:192.168.8.1]").forEach {
            assertEquals(it, SipLocalNetworkPolicy.Need.REQUIRED, SipLocalNetworkPolicy.route(it))
        }
    }
    @Test fun publicAndLoopbackNumericRoutesDoNotNeedPermission() {
        listOf("8.8.8.8", "172.15.1.1", "172.32.1.1", "192.169.1.1", "127.0.0.1:5098", "[::1]:5060", "[2001:4860:4860::8888]", "localhost:5060").forEach {
            assertEquals(it, SipLocalNetworkPolicy.Need.NONE, SipLocalNetworkPolicy.route(it))
        }
    }
    @Test fun hostnamesRemainOptionalWithoutResolvingOrPretendingPublic() {
        listOf("pbx", "pbx.example.com", "sip:user@office.internal:5060;transport=tcp").forEach {
            assertEquals(SipLocalNetworkPolicy.Need.OPTIONAL, SipLocalNetworkPolicy.route(it))
        }
    }
    @Test fun anyKnownLanRouteWinsAcrossAllThreeConfiguredRoutes() {
        assertEquals(SipLocalNetworkPolicy.Need.REQUIRED, SipLocalNetworkPolicy.routes(listOf("8.8.8.8", "pbx.example.com", "10.0.0.1")))
        assertEquals(SipLocalNetworkPolicy.Need.OPTIONAL, SipLocalNetworkPolicy.routes(listOf("8.8.8.8", "", "proxy.example.com")))
    }
    @Test fun invalidFieldsAndRoutesDoNotTriggerPermissionPrompts() {
        listOf("", "sip:", "https://pbx", "bad host", "[fe80::1]:+5060", "1.2.3.999", "pbx:0", "[bad::ip]:5060").forEach {
            assertEquals(it, SipLocalNetworkPolicy.Need.INVALID, SipLocalNetworkPolicy.route(it))
        }
    }
}
