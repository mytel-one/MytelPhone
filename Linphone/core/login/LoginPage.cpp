/*
 * Copyright (c) 2010-2024 Belledonne Communications SARL.
 *
 * This file is part of linphone-desktop
 * (see https://www.linphone.org).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "LoginPage.hpp"
#include <QTimer>

#include "core/App.hpp"

#include "model/account/AccountManager.hpp"

DEFINE_ABSTRACT_OBJECT(LoginPage)

LoginPage::LoginPage(QObject *parent) : QObject(parent) {
	mustBeInMainThread(getClassName());
	mLoginTimeout.setSingleShot(true);
	mLoginTimeout.setInterval(20000);
	connect(&mLoginTimeout, &QTimer::timeout, this, [this]() {
		if (cancelLogin()) setErrorMessage(tr("Connection timed out. Check the server address and try again."));
	});
}

LoginPage::~LoginPage() {
	mustBeInMainThread("~" + getClassName());
	mLoginTimeout.stop();
	if (mAttemptState) mAttemptState->cancel();
	auto manager = mPendingLogin;
	App::postModelAsync([manager]() {
		if (manager) {
			manager->cancelLogin();
			manager->deleteLater();
		}
	});
}

linphone::RegistrationState LoginPage::getRegistrationState() const {
	// View thread
	return mRegistrationState;
}

void LoginPage::setRegistrationState(linphone::RegistrationState status) {
	// Should be view thread only because of object updates.
	mustBeInMainThread(log().arg(Q_FUNC_INFO));
	if (mRegistrationState != status) {
		mRegistrationState = status;
		emit registrationStateChanged();
	}
}

QString LoginPage::getErrorMessage() const {
	return mErrorMessage;
}

void LoginPage::setErrorMessage(const QString &error) {
	if (mErrorMessage != error) {
		mErrorMessage = error;
		emit errorMessageChanged(error);
	}
}

void LoginPage::login(const QString &username,
                      const QString &password,
                      QString displayName,
                      QString domain,
                      LinphoneEnums::TransportType transportType,
                      QString registrarUri,
                      QString outboundProxyAddress,
                      QString connectionId,
                      bool automaticTransport) {
	if (mRegistrationState == linphone::RegistrationState::Progress ||
	    mRegistrationState == linphone::RegistrationState::Refreshing)
		return;
	setErrorMessage("");
	mBadIds = false;
	emit reasonChanged();
	const quint64 attempt = ++mLoginAttempt;
	setRegistrationState(linphone::RegistrationState::Progress);
	mLoginTimeout.start();
	mAttemptState = std::make_shared<LoginAttempt>();
	const auto token = mAttemptState;
	const QPointer<LoginPage> page(this);
	App::postModelAsync([page, token, attempt, username, password, displayName, domain, transportType, registrarUri,
	                     outboundProxyAddress, connectionId, automaticTransport]() {
		if (!page || token->isCancelled()) return;
		QPointer<AccountManager> manager = new AccountManager();
		QObject::connect(page.data(), &QObject::destroyed, manager.data(), [manager, token]() {
			if (token->cancel() && manager) manager->cancelLogin();
			if (manager) manager->deleteLater();
		});
		QObject::connect(
		    manager.data(), &AccountManager::registrationStateChanged, manager.data(),
		    [manager](linphone::RegistrationState state) {
			    if (state == linphone::RegistrationState::Failed || state == linphone::RegistrationState::Ok ||
			        state == linphone::RegistrationState::Cleared || state == linphone::RegistrationState::None)
				    if (manager) manager->deleteLater();
		    });
		if (!page || token->isCancelled()) {
			manager->deleteLater();
			return;
		}
		QMetaObject::invokeMethod(
		    page.data(),
		    [page, manager, attempt]() {
			    if (!page) return;
			    if (attempt == page->mLoginAttempt) page->mPendingLogin = manager;
			    else
				    App::postModelAsync([manager]() {
					    if (manager) {
						    manager->cancelLogin();
						    manager->deleteLater();
					    }
				    });
		    },
		    Qt::QueuedConnection);
		QObject::connect(
		    manager.data(), &AccountManager::registrationStateChanged, page.data(),
		    [page, manager, attempt](linphone::RegistrationState state, linphone::Reason reason, QString message) {
			    if (!page || attempt != page->mLoginAttempt) return;
			    page->setRegistrationState(state);
			    page->mBadIds = reason == linphone::Reason::Forbidden;
			    emit page->reasonChanged();
			    if (state == linphone::RegistrationState::Failed || state == linphone::RegistrationState::Ok ||
			        state == linphone::RegistrationState::Cleared || state == linphone::RegistrationState::None) {
				    page->mLoginTimeout.stop();
				    page->mPendingLogin = nullptr;
				    page->mAttemptState.reset();
				    if (state == linphone::RegistrationState::Failed)
					    page->setErrorMessage(message.isEmpty() ? tr("default_account_connection_state_error_toast")
					                                            : message);
			    }
		    });
		QString error;
		if (!manager->login(username, password, displayName, domain, LinphoneEnums::toLinphone(transportType), &error,
		                    registrarUri, outboundProxyAddress, connectionId, token, automaticTransport)) {
			manager->cancelLogin();
			manager->deleteLater();
			if (page)
				QMetaObject::invokeMethod(
				    page.data(),
				    [page, attempt, error]() {
					    if (!page || attempt != page->mLoginAttempt) return;
					    page->mLoginTimeout.stop();
					    page->mPendingLogin = nullptr;
					    page->mAttemptState.reset();
					    page->setRegistrationState(linphone::RegistrationState::None);
					    page->setErrorMessage(error);
				    },
				    Qt::QueuedConnection);
		} else if (token->isCancelled()) {
			manager->cancelLogin();
			manager->deleteLater();
		}
	});
}

bool LoginPage::cancelLogin() {
	mustBeInMainThread(log().arg(Q_FUNC_INFO));
	mLoginTimeout.stop();
	// If SDK acceptance won the race, preserve that success and let its
	// already-queued notification finish the transition to the main page.
	if (mAttemptState && !mAttemptState->cancel()) return false;
	mAttemptState.reset();
	++mLoginAttempt;
	auto manager = mPendingLogin;
	mPendingLogin = nullptr;
	setRegistrationState(linphone::RegistrationState::None);
	setErrorMessage("");
	App::postModelAsync([manager]() {
		if (manager) {
			manager->cancelLogin();
			manager->deleteLater();
		}
	});
	return true;
}
