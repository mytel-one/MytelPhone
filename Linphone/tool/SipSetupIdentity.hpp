#ifndef SIP_SETUP_IDENTITY_HPP
#define SIP_SETUP_IDENTITY_HPP

#include <QString>
#include <algorithm>
#include <functional>
#include <linphone++/linphone.hh>
#include <list>
#include <vector>

namespace SipSetupIdentity {
using Account = std::shared_ptr<linphone::Account>;
using Params = std::shared_ptr<linphone::AccountParams>;
using Setter = std::function<int(const Account &, const Params &)>;
inline int setAccountParams(const Account &account, const Params &params) {
	return account->setParams(params);
}
struct Entry {
	Account account;
	Params original;
	std::string canonicalIdentity;
};
struct Plan {
	std::vector<Entry> entries;
	size_t failed = 0;
	size_t rollbackFailed = 0;
};
struct Result {
	size_t migrated = 0;
	size_t failed = 0;
	size_t rollbackFailed = 0;
};

inline bool eligible(const std::shared_ptr<const linphone::Address> &identity,
                     const std::shared_ptr<const linphone::Address> &server) {
	if (!identity || !server || !identity->isSip() || !server->isSip() || identity->getSecure() ||
	    server->getSecure() || identity->getUsername().empty() || identity->getDomain().empty() ||
	    identity->getDomain() != server->getDomain() || identity->getPort() <= 0 || identity->getPort() > 65535 ||
	    identity->getPort() != server->getPort() || !server->getUsername().empty())
		return false;
	// Match only the old generated plain form. Full serialization also rejects
	// passwords, URI headers/parameters and outer name-address parameters.
	auto factory = linphone::Factory::get();
	auto plain = factory->createAddress("sip:placeholder@history.invalid");
	if (plain->setUsername(identity->getUsername()) || plain->setDomain(identity->getDomain()) ||
	    plain->setPort(identity->getPort()))
		return false;
	plain->setDisplayName(identity->getDisplayName());
	if (plain->asString() != identity->asString()) return false;
	auto registrar = factory->createAddress("sip:history.invalid");
	if (registrar->setDomain(server->getDomain()) || registrar->setPort(server->getPort())) return false;
	if (registrar->asString() == server->asString()) return true;
	if (registrar->setTransport(server->getTransport())) return false;
	return registrar->asString() == server->asString();
}

inline bool sameFolded(const std::string &one, const std::string &two) {
	return QString::fromStdString(one).compare(QString::fromStdString(two), Qt::CaseInsensitive) == 0;
}

inline Plan prepare(const std::shared_ptr<linphone::Core> &core, const Setter &setter = setAccountParams) {
	Plan plan;
	if (!core || core->getConfig()->isReadonly() || core->getGlobalState() == linphone::GlobalState::On) return plan;
	struct Snapshot {
		Account account;
		Params params;
	};
	std::vector<Snapshot> snapshot;
	for (const auto &account : core->getAccountList())
		if (account && account->getParams()) snapshot.push_back({account, account->getParams()->clone()});
	// Decide every collision from the original account set, before changing any
	// identity. Distinct accounts on different ports must never be merged.
	for (const auto &item : snapshot) {
		const auto identity = item.params->getIdentityAddress();
		if (item.account->getDependency() || !eligible(identity, item.params->getServerAddress())) continue;
		bool collision = false;
		for (const auto &other : snapshot) {
			auto candidate = other.params->getIdentityAddress();
			if (other.account != item.account && candidate &&
			    sameFolded(identity->getUsername(), candidate->getUsername()) &&
			    sameFolded(identity->getDomain(), candidate->getDomain())) {
				collision = true;
				break;
			}
		}
		if (collision) continue;
		auto canonical = identity->clone();
		auto updated = item.params->clone();
		if (canonical->setPort(0) || updated->setIdentityAddress(canonical)) {
			++plan.failed;
			continue;
		}
		if (setter(item.account, updated)) {
			++plan.failed;
			// The SDK assigns its params before validating them, so failure
			// does not imply that the previous params are still installed.
			if (setter(item.account, item.params->clone())) ++plan.rollbackFailed;
			continue;
		}
		plan.entries.push_back({item.account, item.params, canonical->asString()});
	}
	return plan;
}

inline Result commit(const std::shared_ptr<linphone::Core> &core, Plan &plan, const Setter &setter = setAccountParams) {
	Result result{0, plan.failed, plan.rollbackFailed};
	const auto accounts = core ? core->getAccountList() : std::list<Account>();
	for (const auto &entry : plan.entries) {
		const auto current = entry.account->getParams();
		// A concurrent provisioning/account replacement owns its new state.
		if (std::find(accounts.begin(), accounts.end(), entry.account) == accounts.end() || !current ||
		    !current->getIdentityAddress() || current->getIdentityAddress()->asString() != entry.canonicalIdentity ||
		    current->getIdkey() != entry.original->getIdkey()) {
			++result.failed;
			continue;
		}
		if (!core || core->getGlobalState() != linphone::GlobalState::On || setter(entry.account, current->clone())) {
			++result.failed;
			if (setter(entry.account, entry.original->clone())) ++result.rollbackFailed;
			continue;
		}
		++result.migrated;
	}
	// Pre-start setParams prepares loading but the SDK writer is not ready yet.
	// Reapplying the current clone after start uses its normal account writer.
	if (result.migrated) core->getConfig()->sync();
	plan.entries.clear();
	return result;
}
} // namespace SipSetupIdentity
#endif
