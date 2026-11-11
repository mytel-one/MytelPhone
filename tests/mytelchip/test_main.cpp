/*
 * MyTel connection-indicator GUI test (leader decision, 2026-11-11;
 * teardown correction same day).
 *
 * Loads the PRODUCTION indicator component (same file, same binding
 * formula — nothing is duplicated here) and drives reachability
 * true -> false -> true while registration is held Ok, asserting the
 * displayed label and color transitions Online -> Offline -> Online
 * through the real property-NOTIFY wiring.
 *
 * Focus-free (offscreen platform). The AppCpp/UtilsCpp context objects are
 * test seams exposing the same property/invokable surface the production
 * singletons provide; the connectivity formula itself lives only inside
 * MyTelConnectionChip.qml.
 *
 * Ownership (leader correction): the created QML root is owned by an RAII
 * holder declared AFTER the engine/context providers so it is destroyed
 * FIRST, including every early-return path. Any QML warning observed at
 * any point — including teardown — fails the test.
 */
#include <QColor>
#include <QEventLoop>
#include <QGuiApplication>
#include <QMessageLogContext>
#include <QObject>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QPointer>
#include <QScopedPointer>
#include <QtGlobal>
#include <QTimer>
#include <cstdio>
#include <functional>

static int g_qmlWarnings = 0;
static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message) {
    if (type == QtWarningMsg || type == QtCriticalMsg) {
        const QString cat = QString::fromUtf8(context.category);
        if (cat == QLatin1String("qml") || cat == QLatin1String("js") || message.contains(QLatin1String("QML"))) {
            g_qmlWarnings++;
            std::fprintf(stderr, "QML WARNING: %s\n", message.toUtf8().constData());
        }
    }
}

class AppContext : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool networkReachable READ networkReachable WRITE setNetworkReachable NOTIFY networkReachableChanged)
public:
    bool networkReachable() const { return mReachable; }
    void setNetworkReachable(bool reachable) {
        if (reachable != mReachable) {
            mReachable = reachable;
            emit networkReachableChanged(reachable);
        }
    }
signals:
    void networkReachableChanged(bool reachable);
private:
    bool mReachable = false;
};

class UtilsContext : public QObject {
    Q_OBJECT
public:
    // Reduced motion ON keeps rendering deterministic (no blink timers)
    Q_INVOKABLE bool isReduceMotionEnabled() const { return true; }
    Q_INVOKABLE bool isNetworkReachable() const { return true; }
};

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

// Destroys the owned root deterministically (RAII; declared last, so it is
// destroyed before the context providers and the engine on every path).
// dispose() is idempotent; the destructor calls it too.
class ChipRoot {
public:
    explicit ChipRoot(QObject *root) : mRoot(root) {}
    ~ChipRoot() { dispose(); }
    void dispose() {
        if (mRoot) {
            QPointer<QObject> guard(mRoot);
            mRoot->deleteLater();
            // Drain the deleteLater before the engine goes away
            QEventLoop loop;
            QTimer::singleShot(0, &loop, &QEventLoop::quit);
            loop.exec();
            if (guard) delete guard.data(); // only if deleteLater did not run
            mRoot = nullptr;
        }
    }
    QObject *get() const { return mRoot; }

private:
    QObject *mRoot = nullptr;
};

int main(int argc, char **argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);
    qInstallMessageHandler(&messageHandler);

    // Providers first (destroyed last)
    QQmlEngine engine;
    AppContext appCpp;
    UtilsContext utilsCpp;
    engine.rootContext()->setContextProperty("AppCpp", &appCpp);
    engine.rootContext()->setContextProperty("UtilsCpp", &utilsCpp);

    QQmlComponent component(&engine, QUrl::fromLocalFile(QStringLiteral(MYTEL_CHIP_QML)));
    if (component.status() == QQmlComponent::Error) {
        for (const QQmlError &e : component.errors()) {
            std::fprintf(stderr, "QML error: %s\n", e.toString().toUtf8().constData());
        }
        return 2;
    }
    QObject *created = component.create();
    if (!created) {
        std::fprintf(stderr, "component create failed\n");
        return 2;
    }
    ChipRoot chipRoot(created); // destroyed before providers/engine
    QObject *chip = chipRoot.get();
    QObject *label = chip->findChild<QObject *>("chipLabel");
    QObject *background = chip->findChild<QObject *>("chipBackground");
    if (!label || !background) {
        std::fprintf(stderr, "chip internals not found\n");
        return 2;
    }

    // Registration is held Ok for the whole test (leader decision)
    chip->setProperty("registered", true);

    int failures = 0;
    auto check = [&](const char *phase, bool connectedExpected, const QString &labelExpected, const QColor &colorExpected) {
        const bool connectedOk = waitFor([&]() { return chip->property("connected").toBool() == connectedExpected; }, 2000);
        const QString text = label->property("text").toString();
        const QColor bg = background->property("color").value<QColor>();
        const bool textOk = (text == labelExpected);
        const bool colorOk = (bg == colorExpected);
        std::printf("%-10s connected=%d label=%s color=%s -> %s\n", phase,
                    chip->property("connected").toBool(), text.toUtf8().constData(),
                    bg.name().toUtf8().constData(),
                    (connectedOk && textOk && colorOk) ? "PASS" : "FAIL");
        if (!(connectedOk && textOk && colorOk)) failures++;
    };

    app.processEvents();
    check("initial", false, QStringLiteral("Offline"), QColor("#EF4444")); // reachable=false at start
    appCpp.setNetworkReachable(true);
    check("online", true, QStringLiteral("Online"), QColor("#22C55E"));
    appCpp.setNetworkReachable(false);
    check("offline", false, QStringLiteral("Offline"), QColor("#EF4444"));
    appCpp.setNetworkReachable(true);
    check("re-online", true, QStringLiteral("Online"), QColor("#22C55E"));

    // Teardown happens here via ChipRoot before the providers/engine unwind;
    // any QML warning raised anywhere (including teardown) is a failure.
    chipRoot.dispose(); // idempotent; destructor is a no-op afterwards
    chip = nullptr;
    label = nullptr;
    background = nullptr;

    if (g_qmlWarnings > 0) {
        std::printf("QML WARNINGS OBSERVED: %d\n", g_qmlWarnings);
        failures++;
    }
    if (failures == 0) {
        std::printf("ALL PASS\n");
    } else {
        std::printf("FAILURES: %d\n", failures);
    }
    qInstallMessageHandler(nullptr);
    return failures == 0 ? 0 : 1;
}

#include "test_main.moc"
