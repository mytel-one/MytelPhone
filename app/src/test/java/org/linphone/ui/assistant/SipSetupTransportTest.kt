package org.linphone.ui.assistant

import org.junit.Assert.assertEquals
import org.junit.Test

class SipSetupTransportTest {
    @Test fun serverUriNormalizesOnlySupportedSchemesWithoutChangingInputs() {
        val input = "  SiPs:[2001:db8::1]:5061;transport=udp  "
        assertEquals("sips:[2001:db8::1]:5061;transport=udp", SipSetupServerUri.normalize(input))
        assertEquals("  SiPs:[2001:db8::1]:5061;transport=udp  ", input)
        assertEquals("sip:pbx.example.com:5060", SipSetupServerUri.normalize("pbx.example.com:5060"))
        assertEquals("sip:pbx.example.com", SipSetupServerUri.normalize("SIP:pbx.example.com"))
        assertEquals("sip:[2001:db8::1]:5060", SipSetupServerUri.normalize("[2001:db8::1]:5060"))
    }

    @Test fun serverUriRejectsEmptyHostsAndNonSipSchemesBeforeSdkParsing() {
        listOf("", "  ", "sip:", "SIPS:;transport=tls", "https://pbx.example.com", "tel:123", "mailto:user@example.com", "pbx .example.com").forEach {
            assertEquals(null, SipSetupServerUri.normalize(it))
        }
    }

    @Test fun requiredFieldsRejectMissingAndWhitespaceOnlyInput() {
        assertEquals(false, SipSetupFields.canConnect(null, "pbx.example.com"))
        assertEquals(false, SipSetupFields.canConnect("101", null))
        assertEquals(false, SipSetupFields.canConnect("  ", "pbx.example.com"))
        assertEquals(false, SipSetupFields.canConnect("101", " \t\n"))
    }

    @Test fun requiredFieldsAllowTrimmedUsernamesAndServersWithoutChangingInputs() {
        val username = " 101 "
        val server = " 192.0.2.10:5060 "
        assertEquals(true, SipSetupFields.canConnect(username, server))
        assertEquals(" 101 ", username)
        assertEquals(" 192.0.2.10:5060 ", server)
    }

    @Test fun unicodeUsernamesAreNotStrippedByPresenceValidation() {
        assertEquals(true, SipSetupFields.canConnect("۱۲۳", "pbx.example.com"))
    }

    @Test fun ordinaryPbxAddressUsesUdpWithoutExposingAdvancedOptions() {
        assertEquals("UDP", SipSetupTransport.resolve("192.0.2.10:5060", "Automatic"))
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

