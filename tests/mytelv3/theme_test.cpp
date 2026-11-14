#include <QColor>
#include <QFile>
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
	qmlRegisterSingletonType(QUrl::fromLocalFile(styleRoot + "AppIcons.qml"), "Linphone", 1, 0, "AppIcons");
	const int styleId =
	    qmlRegisterSingletonType(QUrl::fromLocalFile(styleRoot + "DefaultStyle.qml"), "Linphone", 1, 0, "DefaultStyle");
	QQmlEngine engine;
	QObject::connect(&engine, &QQmlEngine::warnings, [](const QList<QQmlError> &warnings) {
		for (const auto &warning : warnings)
			std::fprintf(stderr, "QML: %s\n", qPrintable(warning.toString()));
	});
	engine.rootContext()->setContextProperty("AppCpp", &appContext);
	QQmlComponent component(&engine);
	component.setData(R"(
        import QtQuick
        import QtQuick.Controls.Basic as Control
        import Linphone
        import "qrc:/qt/qml/Linphone/view/Style/buttonStyle.js" as ButtonStyle
        Control.Button {
            property color direct: DefaultStyle.surface
            property string debug: String(ButtonStyle.numericPad.color.normal) + "/" + String(ButtonStyle.numericPad.text.normal)
            property color buttonSurface: ButtonStyle.numericPad.color.normal
            property color buttonText: ButtonStyle.numericPad.text.normal
            property color accentText: ButtonStyle.main.text.normal
            property var mediaStyle: ButtonStyle.callControl
            property color mediaSurface: mediaStyle.color.normal
            property color mediaHovered: mediaStyle.color.hovered
            property color mediaText: mediaStyle.text.normal
            property color callBackground: DefaultStyle.callBackground
            property color participantSurface: DefaultStyle.callSurface
            background: Rectangle { color: parent.mediaSurface }
            contentItem: Text { text: "Microphone"; color: parent.mediaText }
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
	QFile windowSource(QStringLiteral(MYTEL_SOURCE_ROOT "/Linphone/view/Page/Window/AbstractWindow.qml"));
	if (!windowSource.open(QIODevice::ReadOnly)) return 2;
	QByteArray paletteBindings;
	for (const auto &line : windowSource.readAll().split('\n')) {
		if (line.trimmed().startsWith("palette.")) paletteBindings += line + '\n';
	}
	QQmlComponent inheritedComponent(&engine);
	inheritedComponent.setData(R"(
        import QtQuick
        import QtQuick.Controls.Basic as Control
        import Linphone
        Control.ApplicationWindow {
            width: 360; height: 240; visible: true
    )" + paletteBindings + R"(
            property color popupSurface: popup.background.color
            property color popupText: popup.contentItem.color
            property color fieldText: field.color
            property color fieldSurface: field.background.color
            Control.TextField { id: field; text: "301" }
            Control.Popup {
                id: popup; visible: true
                contentItem: Control.Label { text: "Settings" }
            }
        }
    )",
	                           QUrl());
	QScopedPointer<QObject> inherited(inheritedComponent.create());
	if (!inherited) {
		for (const auto &error : inheritedComponent.errors())
			std::fprintf(stderr, "%s\n", qPrintable(error.toString()));
		return 2;
	}
	int failures = 0;
	auto check = [&](const char *mode) {
		style->setProperty("themeMode", mode);
		app.processEvents();
		const QColor surface = style->property("surface").value<QColor>();
		const QColor text(style->property("main2_600").toString());
		std::printf(" debug=%s direct=%s bound surface=%s text=%s accent=%s\n",
		            qPrintable(binding->property("debug").toString()),
		            qPrintable(binding->property("direct").value<QColor>().name()),
		            qPrintable(binding->property("buttonSurface").value<QColor>().name()),
		            qPrintable(binding->property("buttonText").value<QColor>().name()),
		            qPrintable(binding->property("accentText").value<QColor>().name()));
		const bool reactive = binding->property("buttonSurface").value<QColor>() == surface &&
		                      binding->property("buttonText").value<QColor>() == text;
		const double ratio = contrast(text, surface);
		const bool whiteAccent = binding->property("accentText").value<QColor>() == QColor("#FFFFFF");
		const QColor mediaText = binding->property("mediaText").value<QColor>();
		const double mediaRatio = contrast(mediaText, binding->property("mediaSurface").value<QColor>());
		const double hoveredRatio = contrast(mediaText, binding->property("mediaHovered").value<QColor>());
		const double callRatio = contrast(mediaText, binding->property("callBackground").value<QColor>());
		const double participantRatio = contrast(mediaText, binding->property("participantSurface").value<QColor>());
		const bool readableMedia =
		    mediaRatio >= 4.5 && hoveredRatio >= 4.5 && callRatio >= 4.5 && participantRatio >= 4.5;
		const QColor popupSurface = inherited->property("popupSurface").value<QColor>();
		const QColor popupText = inherited->property("popupText").value<QColor>();
		const QColor fieldSurface = inherited->property("fieldSurface").value<QColor>();
		const QColor fieldText = inherited->property("fieldText").value<QColor>();
		const bool inheritedPalette =
		    popupSurface == surface && popupText == text && contrast(fieldText, fieldSurface) >= 4.5;
		std::printf("inherited Qt popup: surface=%s text=%s fieldContrast=%.2f -> %s\n",
		            qPrintable(popupSurface.name()), qPrintable(popupText.name()), contrast(fieldText, fieldSurface),
		            inheritedPalette ? "PASS" : "FAIL");
		const bool passed = reactive && whiteAccent && ratio >= 4.5 && readableMedia && inheritedPalette;
		std::printf("media contrast: controls=%.2f hovered=%.2f stage=%.2f participant=%.2f\n", mediaRatio,
		            hoveredRatio, callRatio, participantRatio);
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
