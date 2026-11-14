#include <QDebug>
#include <QEventLoop>
#include <QGuiApplication>
#include <QPointer>
#include <QQuickWindow>
#include <QTimer>
#include <QVariantMap>
#include <functional>
#include <iostream>
#include <memory>

namespace LinphoneEnums {
enum class CallState { IncomingReceived, End, StreamsRunning };
}
struct Settings {
	bool dnd = false, background = false;
	bool dndEnabled() const {
		return dnd;
	}
	bool getKeepCallViewInBackgroundActive() const {
		return background;
	}
};
struct Model {
	LinphoneEnums::CallState state = LinphoneEnums::CallState::IncomingReceived;
	LinphoneEnums::CallState getState() const {
		return state;
	}
};
class CallGui : public QObject {
public:
	explicit CallGui(std::shared_ptr<Model>) : QObject(QCoreApplication::instance()) {
	}
};
struct CallList {
	bool haveCall = true;
	bool getHaveCall() const {
		return haveCall;
	}
};
struct App {
	static App *current;
	static App *getInstance() {
		return current;
	}
	static void postCoreAsync(std::function<void()> task) {
		QTimer::singleShot(0, std::move(task));
	}
	Settings settings;
	bool locked = false;
	int windowsCreated = 0, smartShows = 0;
	QQuickWindow *mCallsWindow = nullptr;
	CallList callList;
	CallList *mCallList = &callList;
	Settings *getSettings() {
		return &settings;
	}
	bool getSessionLocked() const {
		return locked;
	}
	QQuickWindow *getOrCreateCallsWindow(QVariant) {
		if (!mCallsWindow) {
			mCallsWindow = new QQuickWindow;
			++windowsCreated;
		}
		return mCallsWindow;
	}
	void closeCallsWindow();
};
App *App::current = nullptr;
struct Utils {
	static void openCallsWindow(CallGui *call);
	static void showCallsWindow(QQuickWindow *window);
	static void smartShowWindow(QQuickWindow *window) {
		++App::getInstance()->smartShows;
		window->show();
	}
};
#define lInfo qInfo
static void mustBeInMainThread(QString);
#include "incoming_close_production.inc"
#include "incoming_macro_production.inc"
#include "incoming_open_production.inc"
#include "incoming_show_production.inc"
namespace AbstractNotificationBackend {
enum NotificationType { ReceivedCall };
}
struct Notifier {
	int toasts = 0;
	bool toastWorks = false;
	QString getClassName() const {
		return QStringLiteral("Notifier-test");
	}
	QString log() const {
		return QStringLiteral("%1");
	}
	bool createNotification(AbstractNotificationBackend::NotificationType, QVariantMap) {
		++toasts;
		return toastWorks;
	}
	void route(std::shared_ptr<Model> model, bool notificationsAllowed) {
		// Original account-level early return is source-checked below; SDK
		// account construction/voicemail transfer are outside this GUI seam.
		if (!notificationsAllowed) return;
		QString displayName = QStringLiteral("Lab"), remoteAddrString = QStringLiteral("sip:301@lab");
#include "incoming_callback_production.inc"
	}
};

static void drain() {
	QEventLoop loop;
	QTimer::singleShot(30, &loop, &QEventLoop::quit);
	loop.exec();
	QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}
static void mustBeInMainThread(QString) {
}

int main(int argc, char **argv) {
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QGuiApplication application(argc, argv);
	application.setQuitOnLastWindowClosed(false);
	int failures = 0, total = 0;
	auto scenario = [&](const char *name, bool dnd, bool allowed, bool locked, bool ended, bool background,
	                    bool toastWorks) {
		App app;
		App::current = &app;
		app.settings.dnd = dnd;
		app.settings.background = background;
		app.locked = locked;
		Notifier notifier;
		notifier.toastWorks = toastWorks;
		auto model = std::make_shared<Model>();
		notifier.route(model, allowed);
		if (ended) model->state = LinphoneEnums::CallState::End;
		drain();
		const bool expected = !dnd && allowed && !locked && !ended;
		bool ok = app.windowsCreated == int(expected) && notifier.toasts == int(allowed && !dnd);
		ok &= app.smartShows == int(expected && !background);
		if (expected && app.mCallsWindow) {
			ok &= app.mCallsWindow != nullptr;
			if (background) ok &= app.mCallsWindow->visibility() == QWindow::Minimized;
			QPointer<QQuickWindow> window = app.mCallsWindow;
			app.closeCallsWindow();
			ok &= app.mCallsWindow != nullptr; // active call must remain reachable
			app.callList.haveCall = false;
			app.closeCallsWindow();
			drain();
			ok &= app.mCallsWindow == nullptr && window.isNull();
		}
		// The production callback has no answer/accept operation. Its exact
		// body is compiled, and it can only route the existing GUI window.
		++total;
		failures += !ok;
		std::cout << (ok ? "PASS " : "FAIL ") << name << " windows=" << app.windowsCreated
		          << " toasts=" << notifier.toasts << " foregroundShows=" << app.smartShows << std::endl;
	};
	scenario("native-toast-failed-manual-controls", false, true, false, false, false, false);
	scenario("native-toast-success-manual-controls", false, true, false, false, false, true);
	scenario("global-DND", true, true, false, false, false, false);
	scenario("account-notifications-disabled", false, false, false, false, false, false);
	scenario("session-locked", false, true, true, false, false, false);
	scenario("call-ended-before-GUI-queue", false, true, false, true, false, false);
	scenario("background-preference", false, true, false, false, true, false);
	std::cout << total - failures << '/' << total << " incoming routing/window lifecycle checks passed" << std::endl;
	return failures ? 1 : 0;
}
