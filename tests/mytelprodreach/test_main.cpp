/*
 * MyTel production-path reachability test (leader decision, 2026-11-11).
 *
 * Exercises the REAL production chain:
 *   core->setNetworkReachable() -> CoreModel::onNetworkReachable (production
 *   listener) -> CoreModel::networkReachableChanged -> the existing App
 *   connection installed by App::connectCoreModel() -> App::setNetworkReachable
 *   (GUI-thread delivery via the connection's invokeToCore) -> NOTIFY
 *   networkReachableChanged -> the Q_PROPERTY App exposes to QML.
 *
 * Asserts both false and true transitions observed through the property and
 * its notification signal. Runs offscreen. The owner's application data is
 * backed up before and restored after by the driving script.
 */
#include <QApplication>
#include <QEventLoop>
#include <QObject>
#include <QTimer>
#include <cstdio>
#include <functional>

#include "core/App.hpp"
#include "model/core/CoreModel.hpp"

static bool waitFor(const std::function<bool()> &predicate, int timeoutMs) {
    QEventLoop loop;
    QTimer poller;
    poller.setInterval(25);
    QObject::connect(&poller, &QTimer::timeout, &loop, [&]() {
        if (predicate()) loop.quit();
    });
    QTimer killer;
    killer.setSingleShot(true);
    killer.setInterval(timeoutMs);
    QObject::connect(&killer, &QTimer::timeout, &loop, &QEventLoop::quit);
    poller.start();
    killer.start();
    loop.exec();
    return predicate();
}

int main(int argc, char **argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    qputenv("QML_DISABLE_DISK_CACHE", "true");
    QApplication app(argc, argv);

    // Real application object with the production core bootstrap, but no
    // engine/UI: initCore() creates CoreModel + the connection object,
    // connectCoreModel() installs the exact production wiring under test.
    App *appObj = nullptr;
    try {
        appObj = new App(argc, argv);
    } catch (...) {
        std::fprintf(stderr, "App construction failed\n");
        return 2;
    }
    if (appObj->isSecondary()) {
        std::fprintf(stderr, "another instance is running; refusing to test\n");
        return 2;
    }
    appObj->initCore();
    appObj->connectCoreModel();

    auto coreModel = CoreModel::getInstance();
    if (!coreModel) {
        std::fprintf(stderr, "CoreModel not created\n");
        return 2;
    }
    coreModel->start();

    // Wait until the core is up and reachable (initial production seeding)
    if (!waitFor([&]() { return coreModel->getCore() && appObj->property("networkReachable").toBool(); }, 20000)) {
        std::fprintf(stderr, "core did not start/reach initial reachability\n");
        return 2;
    }

    // Track the production NOTIFY signal deliveries on the GUI thread
    int notifications = 0;
    bool lastNotifiedValue = true;
    QObject::connect(appObj, &App::networkReachableChanged, [&](bool reachable) {
        notifications++;
        lastNotifiedValue = reachable;
    });

    int failures = 0;
    auto drive = [&](bool target, const char *phase) {
        coreModel->getCore()->setNetworkReachable(target);
        const bool propOk = waitFor([&]() { return appObj->property("networkReachable").toBool() == target; }, 5000);
        const bool notified = waitFor([&]() { return lastNotifiedValue == target && notifications > 0; }, 5000);
        std::printf("%-8s property=%d notified=%d (count=%d) -> %s\n", phase,
                    appObj->property("networkReachable").toBool(), lastNotifiedValue, notifications,
                    (propOk && notified) ? "PASS" : "FAIL");
        if (!(propOk && notified)) failures++;
    };

    drive(false, "offline");
    drive(true, "online");
    drive(false, "offline2");

    if (failures == 0) {
        std::printf("ALL PASS\n");
    } else {
        std::printf("FAILURES: %d\n", failures);
    }
    return failures == 0 ? 0 : 1;
}
