#include "tool/SipSetupAddress.hpp"

#include <QCoreApplication>
#include <iostream>

static int failures = 0;
static int checks = 0;
static void check(bool condition, const char *label) {
	++checks;
	std::cout << (condition ? "PASS " : "FAIL ") << label << '\n';
	if (!condition) ++failures;
}

static void valid(const char *input,
                  linphone::TransportType selected,
                  const char *domain,
                  int port,
                  linphone::TransportType expected,
                  bool secure) {
	auto identity = linphone::Factory::get()->createAddress("sip:100@initial.invalid");
	auto server = SipSetupAddress::parse(QString::fromUtf8(input), selected, identity);
	check(bool(server), input);
	if (!server) return;
	check(server->getDomain() == domain, "server host is parsed by the SDK");
	check(server->getPort() == port, "server port is preserved");
	check(server->getTransport() == expected, "server transport follows selection or secure scheme");
	check(server->getSecure() == secure, "server secure scheme is preserved");
	check(identity->getDomain() == domain, "identity uses the parsed host");
	check(identity->getPort() == port, "identity uses the parsed port");
	check(identity->getSecure() == secure, "identity secure scheme is preserved");
	check(identity->getUsername() == "100", "identity username is unchanged");
}

static void routes(const char *server,
                   const char *registrar,
                   const char *outbound,
                   linphone::TransportType selected,
                   bool automatic,
                   linphone::TransportType expectedServer,
                   linphone::TransportType expectedRegistrar,
                   linphone::TransportType expectedOutbound) {
	auto identity = linphone::Factory::get()->createAddress("sip:100@initial.invalid");
	auto addresses = SipSetupAddress::prepare(QString::fromUtf8(server), QString::fromUtf8(registrar),
	                                          QString::fromUtf8(outbound), selected, automatic, identity);
	check(bool(addresses), "route set parses with the SDK");
	if (!addresses) return;
	check(addresses->server->getTransport() == expectedServer, "main transport follows setup policy");
	if (*registrar)
		check(addresses->registrar->getTransport() == expectedRegistrar,
		      "registrar follows its URI or explicit setup policy");
	if (*outbound)
		check(addresses->outbound->getTransport() == expectedOutbound,
		      "outbound follows its URI or explicit setup policy");
	check(addresses->server->getSecure() == QString::fromUtf8(server).startsWith("sips:"), "main scheme is preserved");
	if (*registrar)
		check(addresses->registrar->getSecure() == QString::fromUtf8(registrar).startsWith("sips:"),
		      "registrar scheme is preserved");
	if (*outbound)
		check(addresses->outbound->getSecure() == QString::fromUtf8(outbound).startsWith("sips:"),
		      "outbound scheme is preserved");
}

int main(int argc, char **argv) {
	QCoreApplication app(argc, argv);
	valid("192.168.8.171:5060", linphone::TransportType::Udp, "192.168.8.171", 5060, linphone::TransportType::Udp,
	      false);
	valid(" pbx.example.invalid ", linphone::TransportType::Tcp, "pbx.example.invalid", 0, linphone::TransportType::Tcp,
	      false);
	valid("sip:pbx.example.invalid:5060", linphone::TransportType::Udp, "pbx.example.invalid", 5060,
	      linphone::TransportType::Udp, false);
	valid("sips:pbx.example.invalid:5061", linphone::TransportType::Udp, "pbx.example.invalid", 5061,
	      linphone::TransportType::Tls, true);
	valid(" SiPs:pbx.example.invalid:5060 ", linphone::TransportType::Tcp, "pbx.example.invalid", 5060,
	      linphone::TransportType::Tls, true);
	valid("[2001:db8::1]:5060", linphone::TransportType::Udp, "2001:db8::1", 5060, linphone::TransportType::Udp, false);
	valid("sips:[2001:db8::1]:5061", linphone::TransportType::Udp, "2001:db8::1", 5061, linphone::TransportType::Tls,
	      true);
	valid("sips:pbx.example.invalid:5061;transport=udp", linphone::TransportType::Udp, "pbx.example.invalid", 5061,
	      linphone::TransportType::Tls, true);
	valid("sips:pbx.example.invalid:5061;transport=tcp", linphone::TransportType::Tcp, "pbx.example.invalid", 5061,
	      linphone::TransportType::Tls, true);
	valid("sip:pbx.example.invalid:5061;transport=tcp", linphone::TransportType::Tcp, "pbx.example.invalid", 5061,
	      linphone::TransportType::Tcp, false);
	for (const char *input :
	     {"", "  ", "sip:", "sips:", "sip::5060", "http://pbx.example.invalid", "https://pbx.example.invalid",
	      "tel:100", "sip:user@", "sip:pbx.example.invalid:invalid"}) {
		auto identity = linphone::Factory::get()->createAddress("sip:100@initial.invalid");
		auto server = SipSetupAddress::parse(QString::fromUtf8(input), linphone::TransportType::Udp, identity);
		check(!server, input);
		check(identity->asStringUriOnly() == "sip:100@initial.invalid",
		      "invalid input leaves the cloned identity unchanged");
	}

	using T = linphone::TransportType;
	routes("pbx.example.invalid:5060", "sip:registrar.example.invalid;transport=tcp", "sip:proxy.example.invalid:5061",
	       T::Udp, true, T::Udp, T::Tcp, T::Tls);
	routes("pbx.example.invalid:5061", "sip:registrar.example.invalid:5060", "sip:proxy.example.invalid;transport=tcp",
	       T::Tls, true, T::Tls, T::Udp, T::Tcp);
	routes("pbx.example.invalid:5060", "sip:registrar.example.invalid;transport=tcp",
	       "sip:proxy.example.invalid;transport=udp", T::Tls, false, T::Tls, T::Tls, T::Tls);
	routes("pbx.example.invalid:5061", "sip:registrar.example.invalid;transport=tls", "sip:proxy.example.invalid:5061",
	       T::Udp, false, T::Udp, T::Udp, T::Udp);
	routes("pbx.example.invalid:5060", "sip:registrar.example.invalid;transport=udp",
	       "sip:proxy.example.invalid;transport=tls", T::Tcp, false, T::Tcp, T::Tcp, T::Tcp);
	routes("sips:pbx.example.invalid", "sip:registrar.example.invalid;transport=udp",
	       "sip:proxy.example.invalid;transport=tcp", T::Udp, true, T::Tls, T::Tls, T::Tls);
	routes("pbx.example.invalid:5060", "sips:registrar.example.invalid;transport=udp",
	       "sip:proxy.example.invalid;transport=tcp", T::Udp, false, T::Tls, T::Tls, T::Tls);
	routes("pbx.example.invalid:5060", "sip:registrar.example.invalid;transport=udp",
	       "sips:proxy.example.invalid;transport=tcp", T::Tcp, false, T::Tls, T::Tls, T::Tls);
	routes("sip:[2001:db8::1]:5061", "sip:[2001:db8::2]:5060;transport=tcp", "sip:[2001:db8::3]:5061", T::Tls, true,
	       T::Tls, T::Tcp, T::Tls);
	for (const char *invalidRoute : {"https://proxy.example.invalid", "tel:100", "sip:", "sips:"}) {
		auto identity = linphone::Factory::get()->createAddress("sip:100@initial.invalid");
		auto addresses = SipSetupAddress::prepare("pbx.example.invalid", invalidRoute, "", T::Udp, true, identity);
		check(!addresses, "invalid advanced URI is rejected before account creation");
		check(identity->asStringUriOnly() == "sip:100@initial.invalid", "invalid route leaves the identity unchanged");
	}
	std::cout << (checks - failures) << '/' << checks << " SDK-backed setup address checks passed\n";
	return failures ? 1 : 0;
}
