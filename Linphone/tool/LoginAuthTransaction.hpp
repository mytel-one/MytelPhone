#ifndef LOGIN_AUTH_TRANSACTION_HPP
#define LOGIN_AUTH_TRANSACTION_HPP

#include <linphone++/linphone.hh>

// Authentication created by an unfinished account setup must be rolled back.
class LoginAuthTransaction {
public:
	void begin(const std::shared_ptr<linphone::Core> &core, const std::shared_ptr<linphone::AuthInfo> &auth) {
		mCore = core;
		mAuth = auth;
		core->addAuthInfo(auth);
	}
	void rollback() {
		if (mCore && mAuth) mCore->removeAuthInfo(mAuth);
		commit();
	}
	void commit() {
		mAuth = nullptr;
		mCore = nullptr;
	}

private:
	std::shared_ptr<linphone::Core> mCore;
	std::shared_ptr<linphone::AuthInfo> mAuth;
};

#endif
