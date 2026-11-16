#include "tool/LoginAuthTransaction.hpp"
#include "tool/SipSetupAddress.hpp"
#include "tool/SipSetupIdentity.hpp"
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
#include <linphone/api/c-chat-message.h>

static int checks = 0, failures = 0;
static void check(bool pass, const char *label) {
	++checks;
	failures += !pass;
	std::printf("%s %s\n", pass ? "PASS" : "FAIL", label);
}
struct Registrar {
	QUdpSocket socket;
	Registrar() {
		check(socket.bind(QHostAddress(QHostAddress::LocalHost), quint16(0)), "ephemeral loopback registrar");
	}
	std::string uri() const {
		return "sip:127.0.0.1:" + std::to_string(socket.localPort());
	}
	void pump() {
		while (socket.hasPendingDatagrams()) {
			QByteArray packet;
			packet.resize(socket.pendingDatagramSize());
			QHostAddress sender;
			quint16 port;
			socket.readDatagram(packet.data(), packet.size(), &sender, &port);
			if (!packet.startsWith("REGISTER ") && !packet.startsWith("MESSAGE ")) continue;
			auto header = [&](const QByteArray &key) {
				for (auto line : packet.split('\n'))
					if (line.toLower().startsWith(key.toLower() + ":")) return line.mid(key.size() + 1).trimmed();
				return QByteArray();
			};
			auto to = header("To");
			if (!to.contains(";tag=")) to += ";tag=isolated-chat";
			const auto status =
			    packet.startsWith("REGISTER ") && header("From").contains("sip:202@") ? "403 Forbidden" : "200 OK";
			auto response = "SIP/2.0 " + QByteArray(status) + "\r\nVia: " + header("Via") +
			                "\r\nFrom: " + header("From") + "\r\nTo: " + to + "\r\nCall-ID: " + header("Call-ID") +
			                "\r\nCSeq: " + header("CSeq") + "\r\nContact: " + header("Contact") +
			                "\r\nExpires: 3600\r\nContent-Length: 0\r\n\r\n";
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
static auto
account(const std::shared_ptr<linphone::Core> &core, const char *user, Registrar &registrar, bool portFree = false) {
	auto params = core->createAccountParams();
	auto identity = linphone::Factory::get()->createAddress(
	    std::string("sip:") + user + "@127.0.0.1:" + std::to_string(registrar.socket.localPort()));
	if (portFree) {
		check(bool(SipSetupAddress::parse(QString::fromStdString(registrar.uri()), linphone::TransportType::Udp,
		                                  identity)),
		      "production new setup parses isolated registrar");
		check(identity->getPort() == 0, "production new setup creates port-free AOR");
	}
	params->setIdentityAddress(identity);
	params->setServerAddress(linphone::Factory::get()->createAddress(registrar.uri()));
	params->setRoutesAddresses({linphone::Factory::get()->createAddress(registrar.uri())});
	params->setIdkey(std::string("isolated-stable-") + user);
	params->setContactParameters("x-mytel-probe=stable");
	params->enableRegister(true);
	auto value = core->createAccount(params);
	check(core->addAccount(value) == 0, "isolated account accepted");
	return value;
}
static std::shared_ptr<linphone::Core>
openCore(const QString &path, bool normalizeBeforeStart = false, size_t expectedMigrations = 1) {
	auto factory = linphone::Factory::get();
	factory->setDataDir(path.toStdString());
	factory->setConfigDir(path.toStdString());
	factory->setCacheDir(path.toStdString());
	factory->setTopResourcesDir(std::string(MYTEL_PROBE_SDK) + "/share");
	// This test exercises storage and SIP MESSAGE only; optional video/codec
	// plugins have a separate native acceptance gate and must not initialize GPU
	// or capture devices during repeated offscreen Core restarts.
	QDir().mkpath(path + "/no-media-plugins");
	factory->setMspluginsDir((path + "/no-media-plugins").toStdString());
	auto core = factory->createCore((path + "/core.linphonerc").toStdString(), "", nullptr);
	core->setNetworkReachable(false);
	SipSetupIdentity::Plan plan;
	if (normalizeBeforeStart) {
		auto selected = core->getDefaultAccount();
		check(selected != nullptr, "migration sees same persisted default account before start");
		auto original = selected->getParams()->clone();
		const auto oldServer = original->getServerAddress()->asStringUriOnly();
		const auto oldRoute = original->getRoutesAddresses().front()->asStringUriOnly();
		const auto oldContact = original->getContactParameters();
		const auto oldKey = original->getIdkey();
		const auto oldAuthCount = core->getAuthInfoList().size();
		plan = SipSetupIdentity::prepare(core);
		check(plan.entries.size() == expectedMigrations && plan.failed == 0,
		      "production migration prepares exactly the expected eligible persisted accounts");
		check(selected->getParams()->getIdentityAddress()->getPort() == 0,
		      "production migration normalizes only identity port before Core.start");
		check(core->getDefaultAccount() == selected && core->getAccountList().front() == selected,
		      "account object and default selection preserved by migration");
		const auto actual = selected->getParams();
		check(actual->getIdkey() == oldKey && actual->getServerAddress()->asStringUriOnly() == oldServer &&
		          actual->getRoutesAddresses().front()->asStringUriOnly() == oldRoute &&
		          actual->getContactParameters() == oldContact &&
		          actual->registerEnabled() == original->registerEnabled(),
		      "idkey registrar route transport contact and register settings preserved");
		check(core->getAuthInfoList().size() == oldAuthCount && oldAuthCount == 1 &&
		          core->getAuthInfoList().front()->getUsername() == "201",
		      "migration preserves existing authentication slot");
	}
	auto transport = factory->createTransports();
	transport->setUdpPort(-1);
	transport->setTcpPort(0);
	transport->setTlsPort(0);
	transport->setDtlsPort(0);
	core->setTransports(transport);
	check(core->start() == 0, "isolated Core starts");
	if (normalizeBeforeStart) {
		auto result = SipSetupIdentity::commit(core, plan);
		check(result.migrated == expectedMigrations && result.failed == 0,
		      "production poststart SDK setter commits exactly the prepared canonical identities");
		check(core->getConfig()->getString("proxy_0", "reg_identity", "") == "sip:201@127.0.0.1",
		      "SDK account writer persists canonical identity after start");
	}
	return core;
}
static void scenario(bool failedPeer, bool portFree = false, bool migrate = false) {
	std::printf("SCENARIO %s %s %s\n",
	            failedPeer ? "failed-local202-equals-prior-remote202" : "baseline-no-pending-account",
	            portFree ? "port-free-AOR" : "registrar-port-in-AOR", migrate ? "prestart-migration" : "no-migration");
	QTemporaryDir temporary(QDir::tempPath() + "/mytel-chat-persistence-XXXXXX");
	temporary.setAutoRemove(false);
	check(temporary.isValid(), "new disposable profile exists");
	std::printf("ISOLATED_PROFILE %s\n", temporary.path().toUtf8().constData());
	QFile seed(temporary.path() + "/core.linphonerc");
	seed.open(QIODevice::WriteOnly);
	seed.write("[sip]\nbind_address=127.0.0.1\n[misc]\nprefer_basic_chat_room=1\nhide_empty_chat_rooms=0\n");
	seed.close();
	Registrar registrar;
	auto core = openCore(temporary.path());
	core->setNetworkReachable(true);
	core->addAuthInfo(
	    linphone::Factory::get()->createAuthInfo("201", "", "isolated-auth-fixture-only", "", "", "127.0.0.1"));
	auto existing = account(core, "201", registrar, portFree);
	core->setDefaultAccount(existing);
	check(registrar.until(core, [&] { return existing->getState() == linphone::RegistrationState::Ok; }),
	      "local201 registers");
	auto local = existing->getParams()->getIdentityAddress();
	auto peer =
	    linphone::Factory::get()->createAddress("sip:202@127.0.0.1:" + std::to_string(registrar.socket.localPort()));
	auto room = core->getChatRoom(peer, local);
	check(room != nullptr, "basic local201 remote202 chat exists");
	if (!room) {
		core->stop();
		return;
	}
	auto one = room->createMessageFromUtf8(u8"سلام؛ پیام آزمایشی ذخیره تاریخچه.");
	auto two = room->createMessageFromUtf8(u8"پاسخ آزمایشی ذخیره تاریخچه.");
	one->setToBeStored(true);
	two->setToBeStored(true);
	one->send();
	check(registrar.until(core, [&] { return one->getState() == linphone::ChatMessage::State::Delivered; }),
	      "first synthetic MESSAGE delivered through own ephemeral registrar");
	two->send();
	check(registrar.until(core, [&] { return two->getState() == linphone::ChatMessage::State::Delivered; }),
	      "second synthetic MESSAGE delivered through own ephemeral registrar");
	check(room->getHistorySize() == 2, "two synthetic Persian messages stored before cleanup");
	core->createCallLog(local->clone(), peer, linphone::Call::Dir::Outgoing, 10, 1700000000, 1700000001,
	                    linphone::Call::Status::Success, false, 4);
	check(core->getCallLogs().size() == 1, "synthetic prior call history exists");
	if (failedPeer) {
		LoginAuthTransaction auth;
		auth.begin(core,
		           linphone::Factory::get()->createAuthInfo("202", "", "fictional-probe-only", "", "", "127.0.0.1"));
		auto pending = account(core, "202", registrar, portFree);
		check(registrar.until(core, [&] { return pending->getState() == linphone::RegistrationState::Failed; }),
		      "pending local202 receives actual local403");
		core->removeAccount(pending);
		auth.rollback();
		registrar.until(core, [] { return false; }, 300);
		check(room->getHistorySize() == 2, "failed local202 cleanup preserves live201 remote202 history");
		check(core->getAccountList().size() == 1 && core->getDefaultAccount() == existing,
		      "existing201 remains sole default account");
	}
	core->getConfig()->sync();
	std::printf("LIVE_ROOMS %zu HISTORY %d\n", core->getChatRooms().size(), room->getHistorySize());
	one.reset();
	two.reset();
	room.reset();
	local.reset();
	peer.reset();
	existing.reset();
	core->stop();
	core.reset();
	QDir().mkpath(temporary.path() + "/different-working-directory");
	QDir::setCurrent(temporary.path() + "/different-working-directory");
	auto reopened = openCore(temporary.path(), migrate);
	std::printf("REOPEN_ACCOUNTS %zu\n", reopened->getAccountList().size());
	for (auto a : reopened->getAccountList())
		std::printf("REOPEN_IDENTITY %s\n", a->getParams()->getIdentityAddress()->asStringUriOnly().c_str());
	auto rooms = reopened->getChatRooms();
	std::printf("REOPEN_ROOMS %zu\n", rooms.size());
	check(rooms.size() == 1, "restart with changed CWD restores one chat room");
	if (!rooms.empty()) check(rooms.front()->getHistorySize() == 2, "restart restores both Persian messages");
	if (portFree || migrate)
		check(reopened->getDefaultAccount()->getChatRooms().size() == 1 &&
		          reopened->getDefaultAccount()->getChatRooms().front()->getHistorySize() == 2,
		      "account-scoped chat query restores both messages under untouched filters");
	check(reopened->getCallLogs().size() == 1, "restart preserves global call history");
	check(reopened->getDefaultAccount()->getCallLogs().size() == 1, "restart preserves account-scoped call history");
	check(reopened->getDefaultAccount()->getParams()->getIdkey() == "isolated-stable-201",
	      "restart preserves stable idkey");
	check(reopened->getAuthInfoList().size() == 1 &&
	          reopened->getAuthInfoList().front()->getPassword() == "isolated-auth-fixture-only",
	      "restart preserves exact201 authentication");
	auto config = reopened->getConfig();
	check(config->getInt("misc", "hide_chat_rooms_from_removed_proxies", 1) == 1 &&
	          config->getInt("chat", "hide_chat_rooms_with_media", 1) == 1,
	      "all original account and media chat filters remain enabled");
	rooms.clear();
	reopened->getConfig()->sync();
	reopened->stop();
	reopened.reset();
	if (migrate) {
		auto third = openCore(temporary.path(), true, 0);
		auto thirdIdentity = third->getDefaultAccount()->getParams()->getIdentityAddress();
		std::printf("THIRD_IDENTITY %s PORT_VALUE %d\n", thirdIdentity->asStringUriOnly().c_str(),
		            thirdIdentity->getPort());
		check(thirdIdentity->asStringUriOnly() == "sip:201@127.0.0.1",
		      "third start keeps canonical identity without a second migration");
		check(third->getChatRooms().size() == 1 && third->getChatRooms().front()->getHistorySize() == 2,
		      "third start preserves both messages");
		check(third->getDefaultAccount()->getChatRooms().size() == 1 &&
		          third->getDefaultAccount()->getChatRooms().front()->getHistorySize() == 2,
		      "third start restores account-scoped messages under default filters");
		check(third->getDefaultAccount()->getCallLogs().size() == 1, "third start preserves account call history");
		third->stop();
		third.reset();
	}
}
static void guardCases() {
	struct Case {
		const char *identity;
		const char *server;
		bool expected;
	};
	const Case cases[] = {
	    {"sip:201@pbx.invalid:5098", "sip:pbx.invalid:5098", true},
	    {"\"MyTel Team\" <sip:201@pbx.invalid:5098>", "sip:pbx.invalid:5098;transport=tcp", true},
	    {"sip:201@[2001:db8::1]:5098", "sip:[2001:db8::1]:5098", true},
	    {"sip:201@PBX.invalid:5098", "sip:pbx.invalid:5098", false},
	    {"sip:201@pbx.invalid", "sip:pbx.invalid:5098", false},
	    {"sip:201@pbx.invalid:5098", "sip:pbx.invalid", false},
	    {"sip:201@pbx.invalid:5098", "sip:pbx.invalid:5099", false},
	    {"sip:201@aor.invalid:5098", "sip:pbx.invalid:5098", false},
	    {"sips:201@pbx.invalid:5061", "sip:pbx.invalid:5061", false},
	    {"sips:201@[2001:db8::1]:5061", "sips:[2001:db8::1]:5061", false},
	    {"sip:201@pbx.invalid:0", "sip:pbx.invalid:5098", false},
	    {"sip:201@pbx.invalid:5061", "sips:pbx.invalid:5061", false},
	    {"sip:201@pbx.invalid:5098;transport=tcp", "sip:pbx.invalid:5098", false},
	    {"sip:201@pbx.invalid:5098;user=phone", "sip:pbx.invalid:5098", false},
	    {"sip:201@pbx.invalid:5098?Subject=fixture", "sip:pbx.invalid:5098", false},
	    {"<sip:201@pbx.invalid:5098>;tag=fixture", "sip:pbx.invalid:5098", false},
	    {"sip:201:fixture-only@pbx.invalid:5098", "sip:pbx.invalid:5098", false},
	    {"sip:201@pbx.invalid:5098", "sip:registrar-user@pbx.invalid:5098", false},
	    {"sip:201@pbx.invalid:5098", "sip:pbx.invalid:5098;lr", false},
	    {"sip:201@pbx.invalid:5098", "sip:pbx.invalid:5098?Subject=fixture", false},
	};
	for (const auto &item : cases) {
		auto factory = linphone::Factory::get();
		check(SipSetupIdentity::eligible(factory->createAddress(item.identity), factory->createAddress(item.server)) ==
		          item.expected,
		      "strict legacy identity/registrar guard matches expected policy");
	}
	for (bool alreadyCanonical : {false, true}) {
		QTemporaryDir directory(QDir::tempPath() + "/mytel-chat-collision-XXXXXX");
		directory.setAutoRemove(false);
		auto factory = linphone::Factory::get();
		factory->setDataDir(directory.path().toStdString());
		factory->setConfigDir(directory.path().toStdString());
		factory->setCacheDir(directory.path().toStdString());
		auto core = factory->createCore((directory.path() + "/core.linphonerc").toStdString(), "", nullptr);
		Registrar first, second;
		auto one = account(core, "201", first);
		auto two = account(core, "201", second);
		if (alreadyCanonical) {
			auto params = two->getParams()->clone();
			auto id = params->getIdentityAddress()->clone();
			id->setPort(0);
			params->setIdentityAddress(id);
			two->setParams(params);
		}
		const auto original = one->getParams()->getIdentityAddress()->asString();
		auto plan = SipSetupIdentity::prepare(core);
		check(plan.entries.empty() && one->getParams()->getIdentityAddress()->asString() == original,
		      "snapshot collision rejects both different-port or already-canonical duplicate identities");
		check(!first.socket.hasPendingDatagrams() && !second.socket.hasPendingDatagrams(),
		      "guard stage sends no registration traffic");
	}
	{
		QTemporaryDir directory(QDir::tempPath() + "/mytel-chat-rollback-XXXXXX");
		directory.setAutoRemove(false);
		auto factory = linphone::Factory::get();
		factory->setDataDir(directory.path().toStdString());
		factory->setConfigDir(directory.path().toStdString());
		factory->setCacheDir(directory.path().toStdString());
		auto core = factory->createCore((directory.path() + "/core.linphonerc").toStdString(), "", nullptr);
		Registrar registrar;
		auto selected = account(core, "201", registrar);
		const auto original = selected->getParams()->getIdentityAddress()->asString();
		int setterCalls = 0;
		auto plan = SipSetupIdentity::prepare(core, [&](const auto &a, const auto &p) {
			const auto status = a->setParams(p);
			return ++setterCalls == 1 ? -1 : status;
		});
		check(plan.entries.empty() && plan.failed == 1 && setterCalls == 2 &&
		          selected->getParams()->getIdentityAddress()->asString() == original,
		      "failed SDK setter restores original parameters even after mutating them");
	}
	for (int failureMode : {0, 1, 2}) {
		QTemporaryDir directory(QDir::tempPath() + "/mytel-chat-commit-XXXXXX");
		directory.setAutoRemove(false);
		auto factory = linphone::Factory::get();
		factory->setDataDir(directory.path().toStdString());
		factory->setConfigDir(directory.path().toStdString());
		factory->setCacheDir(directory.path().toStdString());
		auto core = factory->createCore((directory.path() + "/core.linphonerc").toStdString(), "", nullptr);
		core->setNetworkReachable(false);
		auto transports = factory->createTransports();
		transports->setUdpPort(0);
		transports->setTcpPort(0);
		transports->setTlsPort(0);
		transports->setDtlsPort(0);
		core->setTransports(transports);
		Registrar registrar;
		auto selected = account(core, "201", registrar);
		core->setDefaultAccount(selected);
		const auto original = selected->getParams()->getIdentityAddress()->asString();
		auto plan = SipSetupIdentity::prepare(core);
		check(plan.entries.size() == 1, "failure test prepares its owned legacy account");
		if (failureMode == 1) {
			auto params = selected->getParams()->clone();
			auto identity = params->getIdentityAddress()->clone();
			identity->setUsername("external201");
			params->setIdentityAddress(identity);
			selected->setParams(params);
		}
		if (failureMode == 2) {
			core->removeAccount(selected);
			selected = account(core, "201", registrar);
			core->setDefaultAccount(selected);
		}
		check(core->start() == 0, "commit failure test starts a disconnected Core");
		const auto before = selected->getParams()->getIdentityAddress()->asString();
		int setterCalls = 0;
		auto result = SipSetupIdentity::commit(core, plan, [&](const auto &a, const auto &p) {
			const auto status = a->setParams(p);
			return ++setterCalls == 1 ? -1 : status;
		});
		check(result.migrated == 0 && result.failed == 1,
		      "failed or no-longer-owned plan never claims migration success");
		if (failureMode == 0)
			check(setterCalls == 2 && selected->getParams()->getIdentityAddress()->asString() == original,
			      "poststart setter failure restores original identity via SDK");
		else
			check(setterCalls == 0 && selected->getParams()->getIdentityAddress()->asString() == before,
			      "changed or replacement account keeps independent parameters");
		check(plan.entries.empty(), "completed migration plan cannot be replayed");
		check(!registrar.socket.hasPendingDatagrams(), "migration and rollback send no registration traffic");
		core->stop();
	}
}
int main(int argc, char **argv) {
	std::setvbuf(stdout, nullptr, _IONBF, 0);
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QGuiApplication application(argc, argv);
	check(linphone::Factory::get()->isDatabaseStorageAvailable(), "installed SDK database storage available");
	scenario(false, true);
	scenario(true, true);
	scenario(false, false, true);
	scenario(true, false, true);
	guardCases();
	std::printf("RESULT %d checks %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
