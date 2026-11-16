package org.linphone.ui.assistant

import org.junit.Assert.assertEquals
import org.junit.Test

class SipSetupRoutePolicyTest {
    @Test fun mixedCaseTransportKeysCanBeCanonicalizedWithoutRemovingOtherParameters() {
        val uri = "sip:pbx.example.invalid;lr;TrAnSpOrT=TCP;transport=udp?Subject=ignored"
        assertEquals(listOf("TrAnSpOrT", "transport"), SipSetupTransport.transportParameterNames(uri))
        assertEquals("TCP", SipSetupTransport.resolveRoute(uri, "Automatic", listOf(uri)))
    }

    @Test fun automaticUsesEachRoutesOwnTransportAndPort() {
        val routes = listOf("pbx.example.invalid:5061", "sip:proxy.example.invalid;transport=tcp", "route.example.invalid")
        assertEquals("TLS", SipSetupTransport.resolveRoute(routes[0], "Automatic", routes))
        assertEquals("TCP", SipSetupTransport.resolveRoute(routes[1], "Automatic", routes))
        assertEquals("UDP", SipSetupTransport.resolveRoute(routes[2], "Automatic", routes))
    }

    @Test fun explicitChoiceOverridesNonSecureRouteParameters() {
        val routes = listOf("pbx.example.invalid:5061", "sip:proxy.example.invalid;transport=udp")
        routes.forEach { assertEquals("TCP", SipSetupTransport.resolveRoute(it, "TCP", routes)) }
    }

    @Test fun AnySecureRouteRequiresTlsThroughoutThisNewAccount() {
        listOf(
            listOf("SIPS:pbx.example.invalid;transport=udp", "sip:proxy.example.invalid"),
            listOf("pbx.example.invalid", "sips:[2001:db8::1]:5061;transport=udp")
        ).forEach { routes ->
            routes.forEach { assertEquals("TLS", SipSetupTransport.resolveRoute(it, "UDP", routes)) }
        }
    }
}
