package org.linphone.ui.assistant

import org.junit.Assert.assertEquals
import org.junit.Test

class SipSetupTransportTest {
    @Test fun ordinaryPbxAddressUsesUdpWithoutExposingAdvancedOptions() {
        assertEquals("UDP", SipSetupTransport.resolve("192.168.8.171:5060", "Automatic"))
        assertEquals("UDP", SipSetupTransport.resolve("pbx.example.com", ""))
    }

    @Test fun secureSchemeAlwaysPreservesTls() {
        assertEquals("TLS", SipSetupTransport.resolve("  SiPs:pbx.example.com  ", "UDP"))
        assertEquals("TLS", SipSetupTransport.resolve("sips:pbx.example.com:5060", "Automatic"))
    }

    @Test fun secureDefaultPortIsInferredForHostAndIpv6() {
        assertEquals("TLS", SipSetupTransport.resolve("pbx.example.com:5061", "Automatic"))
        assertEquals("TLS", SipSetupTransport.resolve("sip:[2001:db8::1]:5061", "Automatic"))
    }

    @Test fun explicitProviderChoiceWinsOverPortConvention() {
        assertEquals("TCP", SipSetupTransport.resolve("pbx.example.com:5061", "tcp"))
        assertEquals("TLS", SipSetupTransport.resolve("pbx.example.com:5060", "TLS"))
        assertEquals("UDP", SipSetupTransport.resolve("pbx.example.com:5061", "UDP"))
    }

    @Test fun sipUriTransportParameterIsHonouredInAutomaticMode() {
        assertEquals("TCP", SipSetupTransport.resolve("sip:pbx.example.com;transport=tcp", "Automatic"))
        assertEquals("TLS", SipSetupTransport.resolve("pbx.example.com;transport=tls", "Automatic"))
        assertEquals("UDP", SipSetupTransport.resolve("pbx.example.com;transport=udp", "Automatic"))
    }

    @Test fun ipv6AddressDigitsAreNotMistakenForAPort() {
        assertEquals("UDP", SipSetupTransport.resolve("[2001:db8::5061]", "Automatic"))
        assertEquals("UDP", SipSetupTransport.resolve("2001:db8::5061", "Automatic"))
    }

    @Test fun unknownAutomaticLabelFallsBackToUdp() {
        assertEquals("UDP", SipSetupTransport.resolve("pbx.example.com", "Automatic (recommended)"))
        assertEquals("UDP", SipSetupTransport.resolve("", ""))
    }
}

