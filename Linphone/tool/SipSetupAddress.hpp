#ifndef SIP_SETUP_ADDRESS_HPP
#define SIP_SETUP_ADDRESS_HPP

#include <QRegularExpression>
#include <QString>
#include <linphone++/linphone.hh>
#include <optional>

namespace SipSetupAddress {
// Keep first-time setup address construction separate from stored account settings.
inline bool isSecure(const QString &server) {
	return server.trimmed().startsWith(QStringLiteral("sips:"), Qt::CaseInsensitive);
}

inline std::shared_ptr<linphone::Address>
parseServer(const QString &server, linphone::TransportType transport, bool automaticTransport, bool requireTls) {
	auto uri = server.trimmed();
	if (uri.isEmpty()) return nullptr;
	const bool secure = isSecure(uri);
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
	if (secure || requireTls) transport = linphone::TransportType::Tls;
	else if (automaticTransport) {
		const auto parameter = QString::fromStdString(address->getUriParam("transport")).toLower();
		if (parameter == QStringLiteral("tls")) transport = linphone::TransportType::Tls;
		else if (parameter == QStringLiteral("tcp")) transport = linphone::TransportType::Tcp;
		else if (parameter == QStringLiteral("udp")) transport = linphone::TransportType::Udp;
		else transport = address->getPort() == 5061 ? linphone::TransportType::Tls : linphone::TransportType::Udp;
	}
	if (secure) {
		// The SDK's setTransport() ignores SIPS, so replace a supplied UDP/TCP URI parameter explicitly.
		address->setUriParam("transport", "tls");
	} else if (address->setTransport(transport)) return nullptr;
	return address;
}

inline bool applyIdentity(const std::shared_ptr<linphone::Address> &identity,
                          const std::shared_ptr<linphone::Address> &server) {
	if (!identity || !server) return false;
	// The identity must contain the SDK's host/port, never a prefixed URI or host:port as its domain.
	if (identity->setDomain(server->getDomain()) || identity->setPort(server->getPort())) return false;
	identity->setSecure(server->getSecure());
	return true;
}

inline std::shared_ptr<linphone::Address>
parse(const QString &server, linphone::TransportType transport, const std::shared_ptr<linphone::Address> &identity) {
	auto address = parseServer(server, transport, false, false);
	if (!applyIdentity(identity, address)) return nullptr;
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
	// A secure registrar or outbound route makes the complete setup require TLS.
	const bool requireTls = isSecure(server) || isSecure(registrar) || isSecure(outbound);
	Addresses addresses;
	addresses.server = parseServer(server, transport, automaticTransport, requireTls);
	if (!addresses.server) return std::nullopt;
	if (!registrar.trimmed().isEmpty()) {
		addresses.registrar = parseServer(registrar, transport, automaticTransport, requireTls);
		if (!addresses.registrar) return std::nullopt;
	}
	if (!outbound.trimmed().isEmpty()) {
		addresses.outbound = parseServer(outbound, transport, automaticTransport, requireTls);
		if (!addresses.outbound) return std::nullopt;
	}
	// Do not modify even the temporary identity until every supplied route is valid.
	if (!applyIdentity(identity, addresses.server)) return std::nullopt;
	return addresses;
}
} // namespace SipSetupAddress

#endif
