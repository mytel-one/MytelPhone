#ifndef SIP_SETUP_ADDRESS_HPP
#define SIP_SETUP_ADDRESS_HPP

#include <QRegularExpression>
#include <QString>
#include <linphone++/linphone.hh>
#include <optional>

namespace SipSetupAddress {
// Keep first-time setup address construction separate from stored account settings.
inline std::shared_ptr<linphone::Address>
parse(const QString &server, linphone::TransportType transport, const std::shared_ptr<linphone::Address> &identity) {
	auto uri = server.trimmed();
	if (uri.isEmpty() || !identity) return nullptr;
	const bool secure = uri.startsWith(QStringLiteral("sips:"), Qt::CaseInsensitive);
	if (secure || uri.startsWith(QStringLiteral("sip:"), Qt::CaseInsensitive)) {
		// Canonicalize only the scheme. Keep the SDK responsible for parsing the URI.
		uri = (secure ? QStringLiteral("sips:") : QStringLiteral("sip:")) + uri.mid(uri.indexOf(':') + 1);
	} else {
		// A bare host:port is supported; other well-known URI schemes are not SIP servers.
		static const QRegularExpression foreignScheme(QStringLiteral("^(?:https?|tel|sms|mailto|file|ftps?|wss?|urn):"),
		                                              QRegularExpression::CaseInsensitiveOption);
		if (uri.contains(QStringLiteral("://")) || foreignScheme.match(uri).hasMatch()) return nullptr;
		uri.prepend(QStringLiteral("sip:"));
	}
	auto address = linphone::Factory::get()->createAddress(uri.toStdString());
	if (!address || !address->isSip() || address->getDomain().empty() || address->getPort() < 0 ||
	    address->getPort() > 65535)
		return nullptr;
	if (secure) {
		// The SDK's setTransport() ignores SIPS, so replace a supplied UDP/TCP URI parameter explicitly.
		address->setUriParam("transport", "tls");
	} else if (address->setTransport(transport)) return nullptr;
	// The identity must contain the SDK's host/port, never a prefixed URI or host:port as its domain.
	if (identity->setDomain(address->getDomain()) || identity->setPort(address->getPort())) return nullptr;
	identity->setSecure(secure);
	return address;
}

struct Addresses {
	std::shared_ptr<linphone::Address> server;
	std::shared_ptr<linphone::Address> registrar;
	std::shared_ptr<linphone::Address> outbound;
};

inline std::optional<Addresses> prepare(const QString &server,
                                        const QString &registrar,
                                        const QString &outbound,
                                        linphone::TransportType transport,
                                        bool automaticTransport,
                                        const std::shared_ptr<linphone::Address> &identity) {
	Q_UNUSED(automaticTransport)
	Addresses addresses;
	addresses.server = parse(server, transport, identity);
	if (!addresses.server) return std::nullopt;
	auto factory = linphone::Factory::get();
	if (!registrar.isEmpty()) {
		addresses.registrar = factory->createAddress(registrar.toStdString());
		if (!addresses.registrar) return std::nullopt;
	}
	if (!outbound.isEmpty()) {
		addresses.outbound = factory->createAddress(outbound.toStdString());
		if (!addresses.outbound) return std::nullopt;
	}
	return addresses;
}
} // namespace SipSetupAddress

#endif
