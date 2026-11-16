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
	std::cout << (checks - failures) << '/' << checks << " SDK-backed setup address checks passed\n";
	return failures ? 1 : 0;
}
