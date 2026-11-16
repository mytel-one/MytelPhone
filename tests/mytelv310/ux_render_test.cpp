#include <QColor>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickImageProvider>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QSvgRenderer>
#include <QTemporaryDir>
#include <QTest>
#include <cmath>
#include <cstdio>

class UxContext : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString themeMainColor READ themeMainColor CONSTANT)
	Q_PROPERTY(bool keyboardFocus READ keyboardFocus CONSTANT)
	Q_PROPERTY(bool otherFocus READ keyboardFocus CONSTANT)
	Q_PROPERTY(QString assistantThirdPartySipAccountDomain READ domain CONSTANT)
	Q_PROPERTY(QString assistantThirdPartySipAccountTransport READ transport CONSTANT)
	Q_PROPERTY(int registrationState MEMBER state NOTIFY registrationChanged)
	Q_PROPERTY(QString errorMessage MEMBER error NOTIFY registrationChanged)
public:
	int state = 0;
	int submissions = 0;
	int lastTransport = -1;
	QString lastServer;
	QString error;
	QString themeMainColor() const {
		return QStringLiteral("mytel");
	}
	bool keyboardFocus() const {
		return false;
	}
	QString domain() const {
		return QString();
	}
	QString transport() const {
		return QStringLiteral("tls");
	}
	Q_INVOKABLE void setScreenRatio(double) {
	}
	Q_INVOKABLE void cancelLogin() {
		state = 0;
		emit registrationChanged();
	}
	Q_INVOKABLE void login(const QString &,
	                       const QString &,
	                       const QString &,
	                       const QString &server,
	                       int transportType,
	                       const QString &,
	                       const QString &,
	                       const QString &) {
		submissions++;
		lastTransport = transportType;
		lastServer = server;
		state = 1;
		emit registrationChanged();
	}
signals:
	void registrationChanged();
};
class UxEnums : public QObject {
	Q_OBJECT
public:
	enum class RegistrationState { None = 0, Progress = 1, Ok = 2, Cleared = 3, Failed = 4, Refreshing = 5 };
	Q_ENUM(RegistrationState)
	enum class TransportType { Udp = 0, Tcp = 1, Tls = 2 };
	Q_ENUM(TransportType)
};
class LocalImages : public QQuickImageProvider {
public:
	LocalImages() : QQuickImageProvider(QQuickImageProvider::Image) {
	}
	QImage requestImage(const QString &id, QSize *size, const QSize &requested) override {
		const QString file = QStringLiteral(MYTEL_SOURCE_ROOT "/Linphone/data/image/") + id.section('?', 0, 0);
		QImage image;
		if (file.endsWith(".svg")) {
			QSvgRenderer svg(file);
			const QSize svgSize = !requested.isEmpty() ? requested : svg.defaultSize();
			image = QImage(svgSize, QImage::Format_ARGB32_Premultiplied);
			image.fill(Qt::transparent);
			QPainter painter(&image);
			svg.render(&painter);
		} else image.load(file);
		if (size) *size = image.size();
		return !requested.isEmpty() ? image.scaled(requested, Qt::KeepAspectRatio, Qt::SmoothTransformation) : image;
	}
};
static double contrast(QColor a, QColor b) {
	auto luminance = [](QColor c) {
		auto linear = [](double v) { return v <= 0.04045 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); };
		return .2126 * linear(c.redF()) + .7152 * linear(c.greenF()) + .0722 * linear(c.blueF());
	};
	const double x = luminance(a), y = luminance(b);
	return (std::max(x, y) + .05) / (std::min(x, y) + .05);
}
int main(int argc, char **argv) {
	qputenv("QT_QPA_PLATFORM", "offscreen");
	qputenv("QT_COMMAND_LINE_PARSER_NO_GUI_MESSAGE_BOXES", "1");
	QGuiApplication app(argc, argv);
	QCoreApplication::setOrganizationName("MytelV310Tests");
	QCoreApplication::setApplicationName("UxRender");
	QTemporaryDir config;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, config.path());
	const QString output = argc > 1 ? QString::fromLocal8Bit(argv[1]) : config.path();
	QDir().mkpath(output);
	const QString root = QStringLiteral(MYTEL_SOURCE_ROOT "/Linphone/view/");
	for (const auto &font : {"Regular", "Medium", "SemiBold", "Bold"})
		QFontDatabase::addApplicationFont(QStringLiteral(MYTEL_SOURCE_ROOT "/Linphone/data/font/Noto_Sans/NotoSans-") +
		                                  font + ".ttf");
	UxContext context;
	qmlRegisterSingletonInstance("SettingsCpp", 1, 0, "SettingsCpp", &context);
	qmlRegisterSingletonInstance("CustomControls", 1, 0, "FocusHelper", &context);
	qmlRegisterUncreatableType<UxEnums>("Linphone", 1, 0, "LinphoneEnums", "Enums only");
	for (const auto &name : {"Themes", "AppIcons", "DefaultStyle", "Typography"})
		qmlRegisterSingletonType(QUrl::fromLocalFile(root + "Style/" + name + ".qml"), "Linphone", 1, 0, name);
	for (const auto &name : {"Text", "EffectImage", "ToolTip"})
		qmlRegisterType(QUrl::fromLocalFile(root + "Control/Display/" + name + ".qml"), "Linphone", 1, 0, name);
	for (const auto &name : {"Button", "BigButton", "SmallButton", "ComboBox"})
		qmlRegisterType(QUrl::fromLocalFile(root + "Control/Button/" + name + ".qml"), "Linphone", 1, 0, name);
	qmlRegisterType(QUrl::fromLocalFile(root + "Control/Input/TextField.qml"), "Linphone", 1, 0, "TextField");
	qmlRegisterType(QUrl::fromLocalFile(root + "Page/Form/Login/SIPLoginPage.qml"), "Linphone", 1, 0, "SIPLoginPage");
	qmlRegisterType(QUrl::fromLocalFile(root + "Control/Display/Settings/SettingsMenuItem.qml"), "Linphone", 1, 0,
	                "SettingsMenuItem");
	QQmlEngine engine;
	engine.addImageProvider("internal", new LocalImages);
	engine.rootContext()->setContextProperty("AppCpp", &context);
	engine.rootContext()->setContextProperty("LoginPageCpp", &context);
	engine.rootContext()->setContextProperty("applicationName", QStringLiteral("MyTel"));
	QStringList warnings;
	QObject::connect(&engine, &QQmlEngine::warnings, [&](const QList<QQmlError> &errors) {
		for (const auto &error : errors) {
			warnings << error.toString();
			std::fprintf(stderr, "QML: %s\n", qPrintable(error.toString()));
		}
	});
	QQmlComponent component(&engine);
	component.setData(R"(
        import QtQuick
        import QtQuick.Controls.Basic as Control
        import Linphone as App
        Control.ApplicationWindow {
            id: mainWindow
            visible:true; width:1020; height:700
            property var style: App.DefaultStyle
            property alias subject: subject
            App.SIPLoginPage { id:subject; objectName:"subject"; anchors.fill:parent }
        }
    )",
	                  QUrl());
	QScopedPointer<QObject> object(component.create());
	if (!object) {
		for (const auto &error : component.errors())
			std::fprintf(stderr, "%s\n", qPrintable(error.toString()));
		return 2;
	}
	auto window = qobject_cast<QQuickWindow *>(object.data());
	auto subject = object->property("subject").value<QObject *>();
	auto style = object->property("style").value<QObject *>();
	if (!window || !subject || !style) return 2;
	int checks = 0, failures = 0;
	auto check = [&](bool ok, const QString &name) {
		++checks;
		failures += !ok;
		std::printf("%s %s\n", qPrintable(name), ok ? "PASS" : "FAIL");
	};
	auto item = [&](const char *name) { return subject->findChild<QQuickItem *>(QString::fromLatin1(name)); };
	auto click = [&](QQuickItem *target) {
		if (!target) return;
		const QPointF p = target->mapToScene(QPointF(target->width() / 2, target->height() / 2));
		QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, p.toPoint());
		QTest::qWait(120);
	};
	auto capture = [&](const QString &name) {
		QTest::qWait(120);
		const QImage image = window->grabWindow();
		check(!image.isNull() && image.save(output + "/" + name + ".png"), name + " rendered");
	};
	for (const auto &size : {QSize(1020, 700), QSize(1512, 1007)}) {
		window->resize(size);
		for (const auto &mode : {"light", "dark"}) {
			context.cancelLogin();
			context.error.clear();
			style->setProperty("themeMode", mode);
			subject->setProperty("advanced", false);
			subject->setProperty("validationError", "");
			subject->setProperty("validationField", "");
			item("sipUsernameInput")->setProperty("text", "");
			item("sipPasswordInput")->setProperty("text", "");
			item("sipServerInput")->setProperty("text", "");
			item("sipTransportInput")->setProperty("currentIndex", 0);
			auto scroll = item("sipLoginScroll");
			scroll->setProperty("contentY", 0);
			QTest::qWait(200);
			const QString prefix = QString("%1-%2x%3").arg(mode).arg(size.width()).arg(size.height());
			auto connect = item("sipConnectButton");
			const QPointF buttonBottom = connect->mapToScene(QPointF(0, connect->height()));
			check(buttonBottom.y() < size.height() - 12 && buttonBottom.y() > 0, prefix + " primary action fits");
			check(!item("sipAdvancedOptions")->isVisible(), prefix + " optional fields collapsed");
			check(item("sipTransportInput")->property("currentValue").toString() == "Automatic",
			      prefix + " new account defaults to Automatic");
			const QColor text = item("sipUsernameInput")->property("placeholderTextColor").value<QColor>();
			const QColor background = item("sipUsernameInput")->property("backgroundColor").value<QColor>();
			check(contrast(text, background) >= 4.5, prefix + " input placeholder contrast");
			capture(prefix + "-setup");
			item("sipUsernameInput")->forceActiveFocus();
			QTest::keyClick(window, Qt::Key_Return);
			check(item("sipPasswordInput")->hasActiveFocus(), prefix + " keyboard reaches password");
			QTest::keyClick(window, Qt::Key_Return);
			check(item("sipServerInput")->hasActiveFocus(), prefix + " keyboard reaches server");
			click(connect);
			check(subject->property("validationField").toString() == "extension", prefix + " inline error shown");
			check(item("sipUsernameInput")->hasActiveFocus(), prefix + " missing field focused");
			capture(prefix + "-validation");
			subject->setProperty("validationField", "");
			subject->setProperty("validationError", "");
			item("sipUsernameInput")->setProperty("text", "301");
			item("sipPasswordInput")->setProperty("text", "demo-password");
			item("sipServerInput")->setProperty("text", " pbx.example.invalid:5060 ");
			click(connect);
			check(context.lastTransport == 0 && context.lastServer == "pbx.example.invalid:5060",
			      prefix + " ordinary PBX5060 passes UDP to login backend");
			check(!item("sipUsernameInput")->isEnabled() && item("sipCancelButton")->isVisible(),
			      prefix + " busy state");
			capture(prefix + "-connecting");
			click(item("sipCancelButton"));
			check(item("sipUsernameInput")->isEnabled(), prefix + " cancellation restores inputs");
			click(item("sipConnectionOptionsButton"));
			check(item("sipAdvancedOptions")->isVisible(), prefix + " options open by actual button");
			QQuickItem *selectedTransportText = nullptr;
			for (auto child : item("sipTransportInput")->findChildren<QQuickItem *>()) {
				if (child->property("text").toString() == "Automatic (recommended)" &&
				    child->property("lineCount").isValid())
					selectedTransportText = child;
			}
			check(selectedTransportText && selectedTransportText->property("lineCount").toInt() == 1 &&
			          !selectedTransportText->property("truncated").toBool(),
			      prefix + " recommended transport label fits one readable line");
			const bool overflow = scroll->property("contentHeight").toDouble() > scroll->height();
			check(size.height() < 800 ? overflow : !overflow, prefix + " optional fields use available height");
			capture(prefix + "-advanced");
			scroll->setProperty("contentY",
			                    std::max(0.0, scroll->property("contentHeight").toDouble() - scroll->height()));
			QTest::qWait(120);
			auto proxy = item("sipProxyInput");
			const QPointF proxyBottom = proxy->mapToScene(QPointF(0, proxy->height()));
			check(proxyBottom.y() > 0 && proxyBottom.y() < size.height(), prefix + " last optional input reachable");
			capture(prefix + "-advanced-bottom");
			item("sipTransportInput")->setProperty("currentIndex", 1);
			item("sipServerInput")->setProperty("text", "sips:pbx.example.invalid:5060");
			QMetaObject::invokeMethod(subject, "connectAccount");
			check(context.lastTransport == 2, prefix + " secure URI cannot downgrade through explicit UDP");
			context.cancelLogin();
			item("sipTransportInput")->setProperty("currentIndex", 2);
			item("sipServerInput")->setProperty("text", "pbx.example.invalid:5061");
			QMetaObject::invokeMethod(subject, "connectAccount");
			check(context.lastTransport == 1, prefix + " explicit TCP survives secure-port inference");
			context.state = 2;
			emit context.registrationChanged();
			QTest::qWait(80);
			check(!subject->property("connecting").toBool() && !item("sipCancelButton")->isVisible(),
			      prefix + " SDK success state ends connecting controls");
			context.cancelLogin();
		}
	}
	check(warnings.isEmpty(), "production setup warning-free");
	QQmlComponent menuComponent(&engine);
	menuComponent.setData(R"(
        import QtQuick
        import QtQuick.Layouts
        import QtQuick.Controls.Basic as Control
        import Linphone as App
        Control.ApplicationWindow {
            visible:true; width:420;height:700;color:App.DefaultStyle.background
            ColumnLayout {
                anchors.fill:parent;anchors.margins:20;spacing:5
                Repeater {
                    model:[
                        {title:"Calls & audio",description:"Microphone, speakers and call preferences"},
                        {title:"Chats",description:"Messages and conversation preferences"},
                        {title:"Appearance",description:"Light, dark or the system theme"},
                        {title:"Advanced",description:"Extra options for administrators"}
                    ]
                    Item {
                        Layout.fillWidth:true;Layout.preferredHeight:76
                        App.SettingsMenuItem {
                            titleText:modelData.title;descriptionText:modelData.description
                            isSelected:index===0
                        }
                    }
                }
                Item {Layout.fillHeight:true}
            }
        }
    )",
	                      QUrl());
	QScopedPointer<QObject> menuObject(menuComponent.create());
	auto menuWindow = qobject_cast<QQuickWindow *>(menuObject.data());
	if (!menuWindow) {
		for (const auto &error : menuComponent.errors())
			std::fprintf(stderr, "%s\n", qPrintable(error.toString()));
		return 2;
	}
	for (const auto &mode : {"light", "dark"}) {
		style->setProperty("themeMode", mode);
		QTest::qWait(150);
		const QImage image = menuWindow->grabWindow();
		check(!image.isNull() && image.save(output + "/" + mode + "-settings-menu.png"),
		      QString(mode) + " production settings menu rendered");
	}
	check(warnings.isEmpty(), "production settings menu warning-free");
	std::printf("%d/%d production UX checks passed.\n", checks - failures, checks);
	return failures ? 1 : 0;
}
#include "ux_render_test.moc"
