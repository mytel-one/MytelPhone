#include "tool/LoginAuthTransaction.hpp"
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QHostAddress>
#include <QTemporaryDir>
#include <QThread>
#include <QUdpSocket>
#include <cstdio>
#include <functional>

struct TestLog {
	template <class T>
	TestLog arg(const T &) const {
		return *this;
	}
};
static TestLog log() {
	return {};
}
static QDebug operator<<(QDebug stream, const TestLog &) {
	return stream << "isolated account fixture";
}
template <class T>
static void mustBeInLinphoneThread(const T &) {
}
static QString getClassName() {
	return "AccountManagerFixture";
}
#define lInfo() qInfo()
struct Utils {
	static QString coreStringToAppString(const std::string &text) {
		return QString::fromStdString(text);
	}
};
class CoreModel {
public:
	static std::shared_ptr<linphone::Core> *active;
	static CoreModel *getInstance() {
		static CoreModel model;
		return &model;
	}
	std::shared_ptr<linphone::Core> getCore() {
		return *active;
	}
};
std::shared_ptr<linphone::Core> *CoreModel::active = nullptr;
class AccountModel : public QObject {
public:
	std::shared_ptr<linphone::Account> mMonitor;
	bool mToRemove = false;
	explicit AccountModel(std::shared_ptr<linphone::Account> account) : mMonitor(std::move(account)) {
	}
	void removeAccount(bool withData = true);
};
class AccountManager : public QObject {
public:
	std::shared_ptr<AccountModel> mAccountModel;
	LoginAuthTransaction mLoginAuth;
	~AccountManager();
	void cancelLogin();
};
#include "pending_manager_cancel.inc"
#include "pending_manager_destructor.inc"
#include "pending_model_remove.inc"

static int checks = 0, failures = 0;
static void check(bool condition, const char *message) {
	++checks;
	failures += !condition;
	std::printf("%s %s\n", condition ? "PASS" : "FAIL", message);
}
class Registrar {
public:
	QUdpSocket socket;
	bool drop = false;
	explicit Registrar() {
		check(socket.bind(QHostAddress(QHostAddress::LocalHost), quint16(0)),
		      "registrar binds only ephemeral loopback");
	}
	QString uri() const {
		return "sip:127.0.0.1:" + QString::number(socket.localPort());
	}
	void pump() {
		while (socket.hasPendingDatagrams()) {
			QByteArray packet;
			packet.resize(socket.pendingDatagramSize());
			QHostAddress sender;
			quint16 port;
			socket.readDatagram(packet.data(), packet.size(), &sender, &port);
			if (drop || !packet.startsWith("REGISTER ")) continue;
			const auto lines = packet.split('\n');
			auto header = [&](const QByteArray &key) {
				for (const auto &line : lines)
					if (line.toLower().startsWith(key.toLower() + ":")) return line.mid(key.size() + 1).trimmed();
				return QByteArray();
			};
			QByteArray to = header("To");
			if (!to.contains(";tag=")) to += ";tag=isolated-fixture";
			const auto response = "SIP/2.0 200 OK\r\nVia: " + header("Via") + "\r\nFrom: " + header("From") +
			                      "\r\nTo: " + to + "\r\nCall-ID: " + header("Call-ID") +
			                      "\r\nCSeq: " + header("CSeq") + "\r\nContact: " + header("Contact") +
			                      "\r\nExpires: " + (header("Expires") == "0" ? "0" : "3600") +
			                      "\r\nContent-Length: 0\r\n\r\n";
			socket.writeDatagram(response, sender, port);
		}
	}
	bool until(const std::shared_ptr<linphone::Core> &core, std::function<bool()> ready, int timeout = 3000) {
		QElapsedTimer timer;
		timer.start();
		while (timer.elapsed() < timeout) {
			pump();
			core->iterate();
			QGuiApplication::processEvents();
			if (ready()) return true;
			QThread::msleep(5);
		}
		return ready();
	}
};
static std::shared_ptr<linphone::AuthInfo> auth(const std::string &user, const char *password) {
	return linphone::Factory::get()->createAuthInfo(user, "", password, "", "", "example.invalid");
}
static std::shared_ptr<linphone::Account>
account(const std::shared_ptr<linphone::Core> &core, const std::string &user, const QString &server) {
	auto factory = linphone::Factory::get();
	auto params = core->createAccountParams();
	params->setIdentityAddress(factory->createAddress("sip:" + user + "@example.invalid"));
	auto serverAddress = factory->createAddress(server.toStdString());
	serverAddress->setTransport(linphone::TransportType::Udp);
	params->setServerAddress(serverAddress);
	params->enableRegister(true);
	auto created = core->createAccount(params);
	check(core->addAccount(created) == 0, "isolated account added");
	return created;
}
static bool hasHistory(const std::shared_ptr<linphone::Core> &core, const std::string &user) {
	for (const auto &entry : core->getCallLogs()) {
		const auto local = entry->getLocalAddress();
		if (local && local->getUsername() == user) return true;
	}
	return false;
}
int main(int argc, char **argv) {
	std::setvbuf(stdout, nullptr, _IONBF, 0);
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QGuiApplication application(argc, argv);
	QTemporaryDir directory(QDir::tempPath() + "/mytel-v310-pending-XXXXXX");
	directory.setAutoRemove(false);
	auto factory = linphone::Factory::get();
	factory->setDataDir(directory.path().toStdString());
	factory->setConfigDir(directory.path().toStdString());
	factory->setCacheDir(directory.path().toStdString());
	QFile seed(directory.path() + "/core.linphonerc");
	if (!directory.isValid() || !seed.open(QIODevice::WriteOnly)) return 2;
	seed.write("[sip]\nbind_address=127.0.0.1\n");
	seed.close();
	auto core = factory->createCore((directory.path() + "/core.linphonerc").toStdString(), "", nullptr);
	CoreModel::active = &core;
	core->setNetworkReachable(false);
	auto transports = factory->createTransports();
	transports->setUdpPort(-1);
	transports->setTcpPort(0);
	transports->setTlsPort(0);
	transports->setDtlsPort(0);
	core->setTransports(transports);
	check(core->start() == 0, "temporary Core starts with only loopback UDP");
	core->setNetworkReachable(true);
	Registrar registrar;
	{
		const std::string user = "restored-fixture";
		core->addAuthInfo(auth(user, "old-fixture-password"));
		LoginAuthTransaction transaction;
		transaction.begin(core, auth(user, "attempt-fixture-password"));
		auto pending = account(core, user, registrar.uri());
		check(registrar.until(core, [&] { return pending->getState() == linphone::RegistrationState::Ok; }),
		      "real loopback REGISTER reaches Ok");
		auto identity = factory->createAddress("sip:" + user + "@example.invalid");
		core->createCallLog(identity, factory->createAddress("sip:history@example.invalid"),
		                    linphone::Call::Dir::Outgoing, 10, 1700000000, 1700000001, linphone::Call::Status::Success,
		                    false, 4);
		check(!core->getCallLogs().empty(), "synthetic prior call history exists");
		transaction.rollback();
		AccountModel model(pending);
		model.removeAccount(false);
		check(registrar.until(core, [&] { return pending->getState() == linphone::RegistrationState::Cleared; }),
		      "pending removal reaches delayed Cleared");
		auto stored = core->getAuthInfoList();
		check(stored.size() == 1 && stored.front()->getPassword() == "old-fixture-password",
		      "delayed Cleared preserves restored credentials");
		check(!core->getCallLogs().empty(), "pending cleanup preserves prior call history");
		for (const auto &info : core->getAuthInfoList())
			core->removeAuthInfo(info);
	}
	{
		const std::string user = "independent-fixture";
		AccountManager manager;
		manager.mLoginAuth.begin(core, auth(user, "attempt-fixture-password"));
		auto pending = account(core, user, registrar.uri());
		manager.mAccountModel = std::make_shared<AccountModel>(pending);
		check(registrar.until(core, [&] { return pending->getState() == linphone::RegistrationState::Ok; }),
		      "second real loopback registration reaches Ok");
		core->addAuthInfo(auth(user, "independent-fixture-password"));
		manager.cancelLogin();
		check(registrar.until(core, [&] { return pending->getState() == linphone::RegistrationState::Cleared; }),
		      "cancel reaches delayed Cleared");
		auto stored = core->getAuthInfoList();
		check(stored.size() == 1 && stored.front()->getPassword() == "independent-fixture-password",
		      "delayed Cleared preserves independent replacement");
		for (const auto &info : core->getAuthInfoList())
			core->removeAuthInfo(info);
	}
	{
		const std::string user = "explicit-delete-fixture";
		core->addAuthInfo(auth(user, "fixture-password"));
		auto selected = account(core, user, registrar.uri());
		check(registrar.until(core, [&] { return selected->getState() == linphone::RegistrationState::Ok; }),
		      "explicit delete fixture registers");
		core->createCallLog(factory->createAddress("sip:" + user + "@example.invalid"),
		                    factory->createAddress("sip:history@example.invalid"), linphone::Call::Dir::Outgoing, 10,
		                    1700000000, 1700000001, linphone::Call::Status::Success, false, 4);
		check(hasHistory(core, user), "explicit delete fixture has synthetic history");
		AccountModel model(selected);
		model.removeAccount();
		check(registrar.until(core, [&] { return selected->getState() == linphone::RegistrationState::Cleared; }),
		      "explicit user deletion reaches Cleared");
		check(core->getAuthInfoList().empty(), "explicit user deletion retains existing auth deletion behavior");
		check(!hasHistory(core, user), "explicit user deletion retains history deletion behavior");
	}
	{
		registrar.drop = true;
		{
			AccountManager manager;
			manager.mLoginAuth.begin(core, auth("quit-fixture", "attempt-fixture-password"));
			manager.mAccountModel = std::make_shared<AccountModel>(account(core, "quit-fixture", registrar.uri()));
		}
		check(core->getAccountList().empty(), "manager destruction removes its unfinished account");
		check(core->getAuthInfoList().empty(), "manager destruction removes unfinished credentials");
		core->getConfig()->sync();
		check(core->getConfig()->getString("proxy_0", "reg_identity", "").empty(),
		      "manager destruction removes persisted pending account");
		registrar.drop = false;
		registrar.until(core, [&] { return core->getAccountList().empty(); }, 200);
	}
	for (const auto &pending : core->getAccountList())
		core->removeAccount(pending);
	core->setNetworkReachable(false);
	core->stop();
	CoreModel::active = nullptr;
	core = nullptr;
	std::printf("%d/%d actual pending-account lifecycle checks passed\n", checks - failures, checks);
	return failures ? 1 : 0;
}
