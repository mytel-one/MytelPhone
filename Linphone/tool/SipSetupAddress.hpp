#ifndef SIP_SETUP_ADDRESS_HPP
#define SIP_SETUP_ADDRESS_HPP

#include <QString>
#include <linphone++/linphone.hh>

namespace SipSetupAddress {
// Keep first-time setup address construction separate from stored account settings.
inline std::shared_ptr<linphone::Address>
parse(const QString &server, linphone::TransportType transport, const std::shared_ptr<linphone::Address> &identity) {
	auto address = linphone::Factory::get()->createAddress(QStringLiteral("sip:%1").arg(server).toStdString());
	if (!address) return nullptr;
	address->setTransport(transport);
	identity->setDomain(server.toStdString());
	return address;
}
} // namespace SipSetupAddress

#endif
