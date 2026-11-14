#include <QColor>
#include <QGuiApplication>
#include <QImage>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <cmath>
#include <cstdio>

class SecondaryContext : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString themeMainColor READ themeMainColor CONSTANT)
	Q_PROPERTY(bool keyboardFocus READ keyboardFocus CONSTANT)
public:
	QString themeMainColor() const {
		return QStringLiteral("mytel");
	}
	bool keyboardFocus() const {
		return false;
	}
	Q_INVOKABLE void setScreenRatio(double) {
	}
};
static double luminance(QColor c) {
	auto linear = [](double v) { return v <= 0.04045 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); };
	return 0.2126 * linear(c.redF()) + 0.7152 * linear(c.greenF()) + 0.0722 * linear(c.blueF());
}
static double contrast(QColor a, QColor b) {
	const double x = luminance(a), y = luminance(b);
	return (std::max(x, y) + 0.05) / (std::min(x, y) + 0.05);
}
int main(int argc, char **argv) {
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QGuiApplication app(argc, argv);
	QCoreApplication::setOrganizationName("MytelV3Tests");
	QCoreApplication::setApplicationName("SecondaryContrast");
	QTemporaryDir config;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, config.path());
	SecondaryContext context;
	qmlRegisterSingletonInstance("SettingsCpp", 1, 0, "SettingsCpp", &context);
	qmlRegisterSingletonInstance("CustomControls", 1, 0, "FocusHelper", &context);
	const QString root = QStringLiteral(MYTEL_SOURCE_ROOT "/Linphone/view/");
	for (const auto &name : {"Themes", "AppIcons", "DefaultStyle", "Typography"})
		qmlRegisterSingletonType(QUrl::fromLocalFile(root + "Style/" + name + ".qml"), "Linphone", 1, 0, name);
	for (const auto &name : {"Text", "EffectImage", "ToolTip"})
		qmlRegisterType(QUrl::fromLocalFile(root + "Control/Display/" + name + ".qml"), "Linphone", 1, 0, name);
	qmlRegisterType(QUrl::fromLocalFile(root + "Control/Button/Button.qml"), "Linphone", 1, 0, "Button");
	QQmlEngine engine;
	engine.rootContext()->setContextProperty("AppCpp", &context);
	engine.rootContext()->setContextProperty(
	    "testIcon", QUrl::fromLocalFile(QStringLiteral(MYTEL_SOURCE_ROOT "/Linphone/data/image/phone.svg")));
	QQmlComponent component(&engine);
	component.setData(R"(
        import QtQuick
        import QtQuick.Controls.Basic as Control
        import Linphone as App
        import "qrc:/qt/qml/Linphone/view/Style/buttonStyle.js" as ButtonStyle
        Control.ApplicationWindow {
            width:360; height:160; visible:true
            property alias button: subject
            property var style: App.DefaultStyle
            property color renderedText: subject.contentItem.currentItem.children[0].children[1].color
            property color renderedIcon: subject.contentItem.currentItem.children[0].children[0].colorizationColor
            property color renderedBackground: subject.background.item.children[0].color
            App.Button {
                id:subject; x:20;y:30;width:240;height:60;text:"Back"
                style:ButtonStyle.secondary;icon.source:testIcon;icon.width:20;icon.height:20
            }
        }
    )",
	                  QUrl());
	QScopedPointer<QObject> object(component.create());
	if (!object) {
		for (const auto &e : component.errors())
			std::fprintf(stderr, "%s\n", qPrintable(e.toString()));
		return 2;
	}
	auto window = qobject_cast<QQuickWindow *>(object.data());
	auto button = object->property("button").value<QObject *>();
	auto style = object->property("style").value<QObject *>();
	if (!window || !button || !style) return 2;
	int failures = 0, checks = 0;
	for (const auto &mode : {"light", "dark", "light", "dark"}) {
		style->setProperty("themeMode", mode);
		QTest::mouseMove(window, QPoint(320, 120));
		QTest::qWait(120);
		for (const auto &state : {"normal", "hovered", "pressed"}) {
			if (QString(state) == "hovered") QTest::mouseMove(window, QPoint(140, 60));
			if (QString(state) == "pressed") QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(140, 60));
			QTest::qWait(120);
			const QColor text = object->property("renderedText").value<QColor>();
			const QColor icon = object->property("renderedIcon").value<QColor>();
			const QColor background = object->property("renderedBackground").value<QColor>();
			const QImage image = window->grabWindow();
			const bool hovered = button->property("hovered").toBool(), pressed = button->property("pressed").toBool();
			const bool stateOk = QString(state) == "normal"    ? !hovered && !pressed
			                     : QString(state) == "hovered" ? hovered && !pressed
			                                                   : pressed;
			const bool painted = !image.isNull() && image.pixelColor(40 * image.devicePixelRatio(),
			                                                         60 * image.devicePixelRatio()) == background;
			const bool ok =
			    stateOk && painted && contrast(text, background) >= 4.5 && contrast(icon, background) >= 4.5;
			std::printf(
			    "%s %s: text=%s icon=%s background=%s textContrast=%.4f iconContrast=%.4f state=%d painted=%d %s\n",
			    mode, state, qPrintable(text.name()), qPrintable(icon.name()), qPrintable(background.name()),
			    contrast(text, background), contrast(icon, background), stateOk, painted, ok ? "PASS" : "FAIL");
			checks++;
			failures += !ok;
			if (QString(state) == "pressed")
				QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(140, 60));
		}
	}
	std::printf("%d/%d actual production Button rendered contrast checks passed.\n", checks - failures, checks);
	return failures ? 1 : 0;
}
#include "secondary_contrast_test.moc"
