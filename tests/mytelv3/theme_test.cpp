#include <QColor>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QSettings>
#include <QTemporaryDir>
#include <cmath>
#include <cstdio>

class AppContext : public QObject {
	Q_OBJECT
public:
	Q_INVOKABLE void setScreenRatio(double) {
	}
};

class SettingsContext : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString themeMainColor READ themeMainColor CONSTANT)
public:
	QString themeMainColor() const {
		return QStringLiteral("mytel");
	}
};

static double luminance(QColor color) {
	auto linear = [](double value) {
		return value <= 0.04045 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
	};
	return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) + 0.0722 * linear(color.blueF());
}

static double contrast(QColor foreground, QColor background) {
	const double a = luminance(foreground), b = luminance(background);
	return (std::max(a, b) + 0.05) / (std::min(a, b) + 0.05);
}

int main(int argc, char **argv) {
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QGuiApplication app(argc, argv);
	QCoreApplication::setOrganizationName("MytelV3Tests");
	QCoreApplication::setApplicationName("Palette");
	QTemporaryDir config;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, config.path());
	AppContext appContext;
	SettingsContext settings;
	qmlRegisterSingletonInstance("SettingsCpp", 1, 0, "SettingsCpp", &settings);
	const QString styleRoot = QStringLiteral(MYTEL_SOURCE_ROOT "/Linphone/view/Style/");
	qmlRegisterSingletonType(QUrl::fromLocalFile(styleRoot + "Themes.qml"), "Linphone", 1, 0, "Themes");
	const int styleId =
	    qmlRegisterSingletonType(QUrl::fromLocalFile(styleRoot + "DefaultStyle.qml"), "Linphone", 1, 0, "DefaultStyle");
	QQmlEngine engine;
	engine.rootContext()->setContextProperty("AppCpp", &appContext);
	QQmlComponent component(&engine);
	component.setData(R"(
        import QtQuick
        import Linphone
        import "qrc:/qt/qml/Linphone/view/Style/buttonStyle.js" as ButtonStyle
        QtObject {
            property color buttonSurface: ButtonStyle.numericPad.color.normal
            property color buttonText: ButtonStyle.numericPad.text.normal
            property color accentText: ButtonStyle.main.text.normal
        }
    )",
	                  QUrl());
	QScopedPointer<QObject> binding(component.create());
	if (!binding) {
		for (const auto &error : component.errors())
			std::fprintf(stderr, "%s\n", qPrintable(error.toString()));
		return 2;
	}
	auto style = engine.singletonInstance<QObject *>(styleId);
	if (!style) return 2;
	int failures = 0;
	auto check = [&](const char *mode) {
		style->setProperty("themeMode", mode);
		app.processEvents();
		const QColor surface = style->property("surface").value<QColor>();
		const QColor text(style->property("main2_600").toString());
		const bool reactive = binding->property("buttonSurface").value<QColor>() == surface &&
		                      binding->property("buttonText").value<QColor>() == text;
		const double ratio = contrast(text, surface);
		const bool whiteAccent = binding->property("accentText").value<QColor>() == QColor("#FFFFFF");
		const bool passed = reactive && whiteAccent && ratio >= 4.5;
		std::printf("%s theme: surface=%s text=%s contrast=%.2f liveBindings=%d -> %s\n", mode,
		            qPrintable(surface.name()), qPrintable(text.name()), ratio, reactive, passed ? "PASS" : "FAIL");
		if (!passed) failures++;
	};
	check("light");
	check("dark");
	check("light");
	check("dark");
	style->setProperty("themeMode", "system");
	return failures == 0 ? 0 : 1;
}

#include "theme_test.moc"
