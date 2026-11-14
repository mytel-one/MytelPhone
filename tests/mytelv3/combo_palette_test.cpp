#include <QColor>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QSettings>
#include <QTemporaryDir>
#include <cmath>
#include <cstdio>

class ComboContext : public QObject {
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

static double luminance(QColor color) {
	auto linear = [](double v) { return v <= 0.04045 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); };
	return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) + 0.0722 * linear(color.blueF());
}
static double contrast(QColor a, QColor b) {
	const double x = luminance(a), y = luminance(b);
	return (std::max(x, y) + 0.05) / (std::min(x, y) + 0.05);
}

int main(int argc, char **argv) {
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QGuiApplication app(argc, argv);
	QCoreApplication::setOrganizationName("MytelV3Tests");
	QCoreApplication::setApplicationName("ComboPalette");
	QTemporaryDir config;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, config.path());
	ComboContext context;
	qmlRegisterSingletonInstance("SettingsCpp", 1, 0, "SettingsCpp", &context);
	qmlRegisterSingletonInstance("CustomControls", 1, 0, "FocusHelper", &context);
	const QString root = QStringLiteral(MYTEL_SOURCE_ROOT "/Linphone/view/");
	for (const auto &name : {"Themes", "AppIcons", "DefaultStyle", "Typography"}) {
		qmlRegisterSingletonType(QUrl::fromLocalFile(root + "Style/" + name + ".qml"), "Linphone", 1, 0, name);
	}
	for (const auto &name : {"Text", "EffectImage"}) {
		qmlRegisterType(QUrl::fromLocalFile(root + "Control/Display/" + name + ".qml"), "Linphone", 1, 0, name);
	}
	qmlRegisterType(QUrl::fromLocalFile(root + "Control/Button/ComboBox.qml"), "Linphone", 1, 0, "ComboBox");
	QQmlEngine engine;
	engine.rootContext()->setContextProperty("AppCpp", &context);
	QQmlComponent component(&engine);
	component.setData(R"(
        import QtQuick
        import QtQuick.Controls.Basic as Control
        import Linphone as App
        Control.ApplicationWindow {
            id: mainWindow; width: 480; height: 360; visible: true
            property alias combo: combo
            property var style: App.DefaultStyle
            property color popupSurface: combo.popup.background.children[0].color
            property color selectedSurface: combo.listView.highlightItem ? combo.listView.highlightItem.color : "transparent"
            function rowText(index) { return combo.listView.itemAtIndex(index).children[0].children[2].color }
            function hoverTint(index) { return combo.listView.itemAtIndex(index).children[1].children[0].color }
            function hoverOpacity(index) { return combo.listView.itemAtIndex(index).children[1].children[0].opacity }
            App.ComboBox {
                id: combo; width: 240; height: 48; textRole: "text"
                model: [{text: "UDP"}, {text: "TCP"}, {text: "TLS"}]
            }
            Component.onCompleted: combo.popup.open()
        }
    )",
	                  QUrl());
	QScopedPointer<QObject> window(component.create());
	if (!window) {
		for (const auto &error : component.errors())
			std::fprintf(stderr, "%s\n", qPrintable(error.toString()));
		return 2;
	}
	auto settle = [&] {
		QElapsedTimer timer;
		timer.start();
		while (timer.elapsed() < 100)
			app.processEvents();
	};
	auto style = window->property("style").value<QObject *>();
	auto combo = window->property("combo").value<QObject *>();
	if (!style || !combo) return 2;
	int failures = 0, checks = 0;
	for (const auto &mode : {"light", "dark", "light", "dark"}) {
		style->setProperty("themeMode", mode);
		settle();
		const QColor surface = window->property("popupSurface").value<QColor>();
		for (int row = 0; row < 3; ++row) {
			QVariant result;
			QMetaObject::invokeMethod(window.data(), "rowText", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, row));
			const QColor text = result.value<QColor>();
			QMetaObject::invokeMethod(window.data(), "hoverTint", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, row));
			const QColor tint = result.value<QColor>();
			QMetaObject::invokeMethod(window.data(), "hoverOpacity", Q_RETURN_ARG(QVariant, result),
			                          Q_ARG(QVariant, row));
			const double alpha = result.toDouble();
			const QColor hover = QColor::fromRgbF(surface.redF() * (1 - alpha) + tint.redF() * alpha,
			                                      surface.greenF() * (1 - alpha) + tint.greenF() * alpha,
			                                      surface.blueF() * (1 - alpha) + tint.blueF() * alpha);
			const QColor selected = window->property("selectedSurface").value<QColor>();
			const bool ok = surface == style->property("surface").value<QColor>() && contrast(text, surface) >= 4.5 &&
			                contrast(text, selected) >= 4.5 && contrast(text, hover) >= 4.5;
			std::printf("%s transport %d: surface=%s text=%s normal=%.2f selected=%.2f hover=%.2f %s\n", mode, row,
			            qPrintable(surface.name()), qPrintable(text.name()), contrast(text, surface),
			            contrast(text, selected), contrast(text, hover), ok ? "PASS" : "FAIL");
			++checks;
			failures += !ok;
		}
	}
	std::printf("%d/%d popup palette checks passed (actual production ComboBox/Text/EffectImage).\n", checks - failures,
	            checks);
	return failures ? 1 : 0;
}
#include "combo_palette_test.moc"
