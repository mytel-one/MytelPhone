#include "tool/LoginAuthTransaction.hpp"
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QTemporaryDir>
#include <cstdio>

static int checks = 0, failures = 0;
static void check(bool condition, const char *message) {
	++checks;
	if (!condition) ++failures;
	std::printf("%s %s\n", condition ? "PASS" : "FAIL", message);
}

struct IsolatedCore {
	QTemporaryDir directory{QDir::tempPath() + "/mytel-v310-auth-XXXXXX"};
	std::shared_ptr<linphone::Core> core;
	IsolatedCore() {
		directory.setAutoRemove(false);
		auto factory = linphone::Factory::get();
		factory->setConfigDir(directory.path().toStdString());
		factory->setDataDir(directory.path().toStdString());
		factory->setCacheDir(directory.path().toStdString());
		QFile seed(directory.path() + "/core.linphonerc");
		seed.open(QIODevice::WriteOnly);
		seed.close();
		core = factory->createCore((directory.path() + "/core.linphonerc").toStdString(), "", nullptr);
		core->setNetworkReachable(false);
		auto transports = factory->createTransports();
		transports->setUdpPort(-1);
		transports->setTcpPort(-1);
		transports->setTlsPort(-1);
		transports->setDtlsPort(-1);
		core->setTransports(transports);
		check(core->getAccountList().empty(), "isolated Core has no account");
		check(core->start() == 0, "isolated Core starts with all SIP transports disabled");
		core->setNetworkReachable(false);
		check(!core->isNetworkReachable(), "isolated Core network remains unreachable");
	}
	~IsolatedCore() {
		std::puts("isolated Core stop begins");
		core->stop();
		std::puts("isolated Core stop returned");
		core = nullptr;
		std::puts("isolated Core released");
	}
	bool hasPersistedAuth() {
		core->getConfig()->sync();
		QFile file(directory.path() + "/core.linphonerc");
		file.open(QIODevice::ReadOnly);
		return file.readAll().contains("[auth_info_");
	}
};

static std::shared_ptr<linphone::AuthInfo> auth(const char *user, const char *password) {
	return linphone::Factory::get()->createAuthInfo(user, "fixture-id", password, "", "fixture-realm",
	                                                "example.invalid");
}

int main(int argc, char **argv) {
	std::setvbuf(stdout, nullptr, _IONBF, 0);
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QGuiApplication application(argc, argv);
	IsolatedCore fixture;
	// This Core contains only fake credentials created below; no production Core/profile is used.
	auto removeFixtureCredentials = [&]() {
		for (const auto &stored : fixture.core->getAuthInfoList())
			fixture.core->removeAuthInfo(stored);
	};
	{
		auto supplied = auth("new-fixture", "new-password");
		LoginAuthTransaction attempt;
		attempt.begin(fixture.core, supplied);
		check(fixture.core->getAuthInfoList().size() == 1, "setup stores one auth info");
		check(fixture.core->getAuthInfoList().front() != supplied, "real SDK stores its own clone");
		check(fixture.hasPersistedAuth(), "started isolated Core persists auth before cancellation");
		attempt.rollback();
		check(fixture.core->getAuthInfoList().empty(), "cancel removes only the Core-owned attempt clone");
		check(!fixture.hasPersistedAuth(), "cancel removes persisted auth sections");
		attempt.rollback();
		check(fixture.core->getAuthInfoList().empty(), "repeated cancellation is idempotent");
	}
	removeFixtureCredentials();
	{
		fixture.core->addAuthInfo(auth("existing-fixture", "keep-password"));
		auto existing = fixture.core->getAuthInfoList().front();
		LoginAuthTransaction attempt;
		attempt.begin(fixture.core, auth("another-fixture", "new-password"));
		attempt.rollback();
		auto current = fixture.core->getAuthInfoList();
		check(current.size() == 1 && current.front() == existing,
		      "unrelated existing Core auth object survives unchanged");
	}
	removeFixtureCredentials();
	{
		fixture.core->addAuthInfo(auth("shared-fixture", "old-password"));
		LoginAuthTransaction attempt;
		attempt.begin(fixture.core, auth("shared-fixture", "new-password"));
		check(fixture.core->getAuthInfoList().front()->getPassword() == "new-password",
		      "SDK replaces matching identity during setup");
		attempt.rollback();
		auto current = fixture.core->getAuthInfoList();
		check(current.size() == 1 && current.front()->getPassword() == "old-password",
		      "cancel restores displaced preexisting credentials");
	}
	removeFixtureCredentials();
	{
		fixture.core->addAuthInfo(auth("shared-fixture", "old-password"));
		LoginAuthTransaction attempt;
		attempt.begin(fixture.core, auth("shared-fixture", "attempt-password"));
		fixture.core->addAuthInfo(auth("shared-fixture", "later-independent-password"));
		attempt.rollback();
		auto current = fixture.core->getAuthInfoList();
		check(current.size() == 1 && current.front()->getPassword() == "later-independent-password",
		      "rollback preserves a later independent replacement");
	}
	removeFixtureCredentials();
	{
		LoginAuthTransaction attempt;
		attempt.begin(fixture.core, auth("accepted-fixture", "accepted-password"));
		attempt.commit();
		attempt.rollback();
		check(fixture.core->getAuthInfoList().size() == 1, "accepted setup keeps its credentials");
	}
	removeFixtureCredentials();
	{
		LoginAuthTransaction abandoned;
		abandoned.begin(fixture.core, auth("abandoned-fixture", "temporary-password"));
	}
	check(fixture.core->getAuthInfoList().empty(), "destroyed unfinished setup rolls back credentials");
	removeFixtureCredentials();
	{
		fixture.core->addAuthInfo(auth("shared-fixture", "old-password"));
		LoginAuthTransaction attempt;
		attempt.begin(fixture.core, auth("shared-fixture", "attempt-password"));
		fixture.core->removeAuthInfo(fixture.core->getAuthInfoList().front());
		attempt.rollback();
		check(fixture.core->getAuthInfoList().empty(), "rollback respects a later independent credential deletion");
	}
	std::printf("%d/%d actual isolated SDK auth checks passed\n", checks - failures, checks);
	return failures ? 1 : 0;
}
