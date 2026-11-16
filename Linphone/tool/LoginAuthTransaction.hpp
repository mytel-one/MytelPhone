#ifndef LOGIN_AUTH_TRANSACTION_HPP
#define LOGIN_AUTH_TRANSACTION_HPP

#include <algorithm>
#include <linphone++/linphone.hh>
#include <vector>

// Authentication created by an unfinished account setup must be rolled back.
class LoginAuthTransaction {
public:
	LoginAuthTransaction() = default;
	LoginAuthTransaction(const LoginAuthTransaction &) = delete;
	LoginAuthTransaction &operator=(const LoginAuthTransaction &) = delete;
	~LoginAuthTransaction() {
		rollback();
	}

	// All operations, including destruction, run on the owning Core thread.
	void begin(const std::shared_ptr<linphone::Core> &core, const std::shared_ptr<linphone::AuthInfo> &auth) {
		rollback();
		if (!core || !auth) return;
		mCore = core;
		const auto before = core->getAuthInfoList();
		core->addAuthInfo(auth);
		const auto after = core->getAuthInfoList();
		// addAuthInfo() stores a clone. Never attempt to remove the caller's original object.
		for (const auto &stored : after) {
			if (std::find(before.begin(), before.end(), stored) == before.end() && sameIdentity(stored, auth)) {
				mAuth = stored;
				break;
			}
		}
		for (const auto &previous : before) {
			if (std::find(after.begin(), after.end(), previous) == after.end() && sameIdentity(previous, auth))
				mReplaced.push_back(previous);
		}
	}
	void rollback() {
		if (mCore && mAuth) {
			const auto current = mCore->getAuthInfoList();
			if (std::find(current.begin(), current.end(), mAuth) != current.end()) {
				mCore->removeAuthInfo(mAuth);
				for (const auto &previous : mReplaced) {
					const auto remaining = mCore->getAuthInfoList();
					const bool occupied = std::any_of(remaining.begin(), remaining.end(), [&](const auto &stored) {
						return sameIdentity(stored, previous);
					});
					if (!occupied) mCore->addAuthInfo(previous);
				}
			}
			// A later independent replacement/deletion has taken ownership; leave it untouched.
		}
		commit();
	}
	void commit() {
		mReplaced.clear();
		mAuth = nullptr;
		mCore = nullptr;
	}

private:
	static bool sameIdentity(const std::shared_ptr<linphone::AuthInfo> &left,
	                         const std::shared_ptr<linphone::AuthInfo> &right) {
		return left->isEqualButAlgorithms(right) && bool(left->getAccessToken()) == bool(right->getAccessToken());
	}
	std::shared_ptr<linphone::Core> mCore;
	std::shared_ptr<linphone::AuthInfo> mAuth;
	std::vector<std::shared_ptr<linphone::AuthInfo>> mReplaced;
};

#endif
