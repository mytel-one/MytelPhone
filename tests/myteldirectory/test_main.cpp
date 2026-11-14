#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QProcess>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <cstdio>

// Test the network helper against a local stream without weakening the
// production HTTPS-only refresh API or the production certificate checks.
#include "core/search/ServerDirectoryManager.hpp"
#include "model/core/CoreModel.hpp"

class ServerDirectoryManagerTestAccess {
public:
	static QString fetch(ServerDirectoryManager *manager, const QString &url, bool *ok) {
		return manager->fetchToBuffer(url, ok);
	}
};

static const QString Identity = QStringLiteral("sip:301@example.test");
static const QString DirectoryUrl = QStringLiteral("https://directory.example.test/team");

static void seed(const QString &url, const QString &json) {
	CoreModel::getInstance()->getCore()->getConfig()->setString(
	    "app", ServerDirectoryManager::directoryCacheKey(Identity, url).toStdString(), json.toStdString());
}

static int scenario(const QString &name) {
	auto manager = ServerDirectoryManager::getInstance();
	if (name == "cache-search") {
		manager->setDirectoryUrl(Identity, DirectoryUrl);
		seed(DirectoryUrl, R"([{"name":"Alice","number":"001"},{"name":"Bob","number":"302"}])");
		const auto entries = manager->getEntries(Identity);
		if (entries.size() != 2 || entries[0].number != "001") return 1;
		if (manager->searchEntries(Identity, "ALICE").size() != 1) return 1;
		manager->setDirectoryUrl(Identity, "https://directory.example.test/new-source");
		return manager->getEntries(Identity).isEmpty() ? 0 : 1;
	}
	if (name == "invalid-url") {
		manager->setDirectoryUrl(Identity, DirectoryUrl);
		seed(DirectoryUrl, R"([{"name":"Alice","number":"001"}])");
		manager->setDirectoryUrl(Identity, "http://directory.example.test/insecure");
		return manager->getDirectoryUrl(Identity).isEmpty() && manager->getEntries(Identity).isEmpty() ? 0 : 1;
	}
	if (name == "invalid-cache") {
		manager->setDirectoryUrl(Identity, DirectoryUrl);
		seed(DirectoryUrl, R"([{"name":"Alice","number":"001"},{"name":"Invalid","number":302}])");
		return manager->getEntries(Identity).isEmpty() ? 0 : 1;
	}
	if (name == "stream-limit") {
		QTcpServer server;
		if (!server.listen(QHostAddress::LocalHost)) return 2;
		QObject::connect(&server, &QTcpServer::newConnection, &server, [&]() {
			auto socket = server.nextPendingConnection();
			QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
				if (socket->property("sent").toBool()) return;
				socket->readAll();
				socket->setProperty("sent", true);
				socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n");
				socket->write(QByteArray(600000, 'x'));
				// Deliberately keep the connection open: the byte cap must
				// abort as bytes arrive, rather than waiting for completion.
			});
		});
		bool ok = true;
		QElapsedTimer elapsed;
		elapsed.start();
		const auto result = ServerDirectoryManagerTestAccess::fetch(
		    manager, QString("http://127.0.0.1:%1/stream").arg(server.serverPort()), &ok);
		return !ok && result.isEmpty() && elapsed.elapsed() < 2500 ? 0 : 1;
	}
	return 2;
}

int main(int argc, char **argv) {
	QCoreApplication app(argc, argv);
	if (app.arguments().size() == 2) return scenario(app.arguments()[1]);
	int failures = 0;
	for (const auto &name : {"cache-search", "invalid-url", "invalid-cache", "stream-limit"}) {
		QProcess child;
		child.start(QCoreApplication::applicationFilePath(), {QString::fromLatin1(name)});
		const bool finished = child.waitForFinished(3000);
		if (!finished) {
			child.kill();
			child.waitForFinished();
		}
		const bool pass = finished && child.exitStatus() == QProcess::NormalExit && child.exitCode() == 0;
		std::printf("%s: %s%s\n", name, pass ? "PASS" : "FAIL", finished ? "" : " (hung)");
		if (!pass) {
			failures++;
			std::fprintf(stderr, "%s", child.readAllStandardError().constData());
		}
	}
	return failures == 0 ? 0 : 1;
}
