/*
 * Copyright (c) 2026 MyTel.
 * This file is part of Linphone, licensed under the GNU GPL version 3 or later.
 */

#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QImage>
#include <QMap>
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
#include <QTranslator>
#include <QXmlStreamReader>
#include <cstdio>

class PredialContext : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString themeMainColor READ themeMainColor CONSTANT)
	Q_PROPERTY(bool keyboardFocus READ keyboardFocus CONSTANT)
public:
	QStringList destinations;
	QVariantMap lastOptions;
	QString themeMainColor() const {
		return QStringLiteral("mytel");
	}
	bool keyboardFocus() const {
		return false;
	}
	Q_INVOKABLE void forceInputLeftToRight(QObject *) {
	}
	Q_INVOKABLE void createCall(const QString &address, const QVariantMap &options) {
		destinations << address;
		lastOptions = options;
	}
};

class PredialImages : public QQuickImageProvider {
public:
	PredialImages() : QQuickImageProvider(QQuickImageProvider::Image) {
	}
	QImage requestImage(const QString &id, QSize *size, const QSize &requested) override {
		QSvgRenderer renderer(QStringLiteral(MYTEL_SOURCE_ROOT "/Linphone/data/image/") + id.section('?', 0, 0));
		QImage image(requested.isEmpty() ? renderer.defaultSize() : requested, QImage::Format_ARGB32_Premultiplied);
		image.fill(Qt::transparent);
		QPainter painter(&image);
		renderer.render(&painter);
		if (size) *size = image.size();
		return image;
	}
};

// Read the actual production TS catalog: missing contexts must remain untranslated.
class CatalogTranslator : public QTranslator {
public:
	QMap<QPair<QString, QString>, QString> entries;
	explicit CatalogTranslator(const QString &path) {
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly)) return;
		QXmlStreamReader xml(&file);
		QString context, source;
		while (!xml.atEnd()) {
			xml.readNext();
			if (!xml.isStartElement()) continue;
			if (xml.name() == "name") context = xml.readElementText();
			else if (xml.name() == "source") source = xml.readElementText();
			else if (xml.name() == "translation") {
				const bool finished = xml.attributes().value("type") != "unfinished";
				const auto value = xml.readElementText(QXmlStreamReader::IncludeChildElements);
				if (finished && !value.isEmpty()) entries.insert({context, source}, value);
			}
		}
		if (xml.hasError()) entries.clear();
	}
	bool isEmpty() const override {
		return entries.isEmpty();
	}
	QString translate(const char *context, const char *source, const char *, int) const override {
		return entries.value({QString::fromUtf8(context), QString::fromUtf8(source)});
	}
};

static QString readFile(const QString &path) {
	QFile file(path);
	return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()).replace("\r\n", "\n") : QString();
}

static QString between(const QString &source, const QString &first, const QString &next) {
	const int start = source.indexOf(first), end = source.indexOf(next, start);
	return start >= 0 && end > start ? source.mid(start, end - start) : QString();
}

int main(int argc, char **argv) {
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QGuiApplication app(argc, argv);
	QCoreApplication::setOrganizationName("MytelV310Tests");
	QCoreApplication::setApplicationName("PredialPopup");
	QTemporaryDir temporary;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temporary.path());
	const QString root = QStringLiteral(MYTEL_SOURCE_ROOT "/Linphone/view/");
	const QString output = argc > 1 ? QString::fromLocal8Bit(argv[1]) : temporary.path();
	QDir().mkpath(output);
	CatalogTranslator translator(QStringLiteral(MYTEL_SOURCE_ROOT "/Linphone/data/languages/en.ts"));
	app.installTranslator(&translator);
	std::printf("Catalog entries %lld; Call='%s'; Cancel='%s'\n", static_cast<long long>(translator.entries.size()),
	            qPrintable(QCoreApplication::translate("ContactPage", "dialog_call")),
	            qPrintable(QCoreApplication::translate("Dialog", "dialog_cancel")));
	for (const auto &font : {"Regular", "Medium", "SemiBold", "Bold"})
		QFontDatabase::addApplicationFont(QStringLiteral(MYTEL_SOURCE_ROOT "/Linphone/data/font/Noto_Sans/NotoSans-") +
		                                  font + ".ttf");
	PredialContext context;
	qmlRegisterSingletonInstance("SettingsCpp", 1, 0, "SettingsCpp", &context);
	qmlRegisterSingletonInstance("CustomControls", 1, 0, "FocusHelper", &context);
	for (const auto &name : {"Themes", "AppIcons", "DefaultStyle", "Typography"})
		qmlRegisterSingletonType(QUrl::fromLocalFile(root + "Style/" + name + ".qml"), "Linphone", 1, 0, name);
	for (const auto &name : {"Text", "EffectImage", "ToolTip"})
		qmlRegisterType(QUrl::fromLocalFile(root + "Control/Display/" + name + ".qml"), "Linphone", 1, 0, name);
	for (const auto &name : {"Button", "BigButton", "SmallButton", "ComboBox"})
		qmlRegisterType(QUrl::fromLocalFile(root + "Control/Button/" + name + ".qml"), "Linphone", 1, 0, name);
	qmlRegisterType(QUrl::fromLocalFile(root + "Control/Input/TextField.qml"), "Linphone", 1, 0, "TextField");
	qmlRegisterType(QUrl::fromLocalFile(root + "Control/Popup/Popup.qml"), "Linphone", 1, 0, "Popup");
	const QString production =
	    readFile(argc > 2 ? QString::fromLocal8Bit(argv[2]) : root + "Page/Window/AbstractWindow.qml");
	QString popup = between(production, "    Component {\n        id: preDialEditPopupComp",
	                        "    Component {\n        id: addressChooserPopupComp");
	const QString call =
	    between(production, "    function startCallWithContact(", "    function sendMessageToContact(");
	if (popup.isEmpty() || call.isEmpty()) return 2;
	// Observe the production popup without replacing any control, binding or handler.
	popup.replace("id: preDialEditPopup\n",
	              "id: preDialEditPopup\n            Component.onCompleted: mainWindow.observedPopup = this\n");
	QQmlEngine engine;
	engine.addImageProvider("internal", new PredialImages);
	engine.rootContext()->setContextProperty("UtilsCpp", &context);
	QStringList warnings;
	QObject::connect(&engine, &QQmlEngine::warnings, [&](const QList<QQmlError> &errors) {
		for (const auto &error : errors) {
			warnings << error.toString();
			std::fprintf(stderr, "QML: %s\n", qPrintable(error.toString()));
		}
	});
	QQmlComponent component(&engine);
	const QString source = QStringLiteral(R"(
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as Control
import Linphone
import 'qrc:/qt/qml/Linphone/view/Style/buttonStyle.js' as ButtonStyle
import 'qrc:/qt/qml/Linphone/view/Control/Tool/Helper/utils.js' as Utils
Control.ApplicationWindow {
    id: mainWindow
    visible: true; width: 800; height: 600
    property var observedPopup: null
    property var style: DefaultStyle
    function mytelNormalizeNumber(raw) { return Utils.normalizeSipDestination(raw) }
    function openContact(address, video) {
        startCallWithContact({core:{fullName:"Support & team", allAddresses:[address], defaultAddress:address, phoneNumbers:[]}}, video, contentItem)
    }
)") + popup + call + "\n}";
	// Keep the real source context: qsTr must resolve AbstractWindow, not a test name.
	component.setData(source.toUtf8(), QUrl::fromLocalFile(root + "Page/Window/AbstractWindow.qml"));
	QScopedPointer<QObject> object(component.create());
	if (!object) {
		for (const auto &error : component.errors())
			std::fprintf(stderr, "%s\n", qPrintable(error.toString()));
		return 2;
	}
	auto window = qobject_cast<QQuickWindow *>(object.data());
	auto style = object->property("style").value<QObject *>();
	if (!window || !style) return 2;
	int checks = 0, failures = 0;
	auto check = [&](bool ok, const char *name) {
		++checks;
		failures += !ok;
		std::printf("%s %s\n", name, ok ? "PASS" : "FAIL");
	};
	auto open = [&](const QString &address, bool video) {
		QMetaObject::invokeMethod(object.data(), "openContact", Q_ARG(QVariant, address), Q_ARG(QVariant, video));
		QTest::qWait(80);
		return object->property("observedPopup").value<QObject *>();
	};
	auto field = [](QObject *popup) {
		auto content = popup ? popup->property("contentItem").value<QObject *>() : nullptr;
		if (content)
			for (auto child : content->findChildren<QQuickItem *>())
				if (child->inherits("QQuickTextField")) return child;
		return static_cast<QQuickItem *>(nullptr);
	};
	auto buttons = [](QObject *popup) {
		QList<QQuickItem *> result;
		auto content = popup ? popup->property("contentItem").value<QObject *>() : nullptr;
		if (content)
			for (auto child : content->findChildren<QQuickItem *>())
				if (child->inherits("QQuickAbstractButton") && !child->property("text").toString().isEmpty())
					result << child;
		return result;
	};
	auto click = [&](QQuickItem *target) {
		if (!target) return;
		const auto p = target->mapToScene(QPointF(target->width() / 2, target->height() / 2));
		QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, p.toPoint());
		QTest::qWait(80);
	};
	for (const auto &theme : {QStringLiteral("light"), QStringLiteral("dark")}) {
		style->setProperty("themeMode", theme);
		const int before = context.destinations.size();
		auto popup = open("sip:39102@192.0.2.10;transport=tcp", true);
		auto input = field(popup);
		auto actions = buttons(popup);
		check(input && input->property("text").toString() == "39102", "single SIP destination prefilled at completion");
		check(actions.size() == 2 && actions[0]->property("text") == "Cancel" && actions[1]->property("text") == "Call",
		      "existing catalog contexts translate actions");
		bool translatedTitle = false;
		if (popup) {
			auto content = popup->property("contentItem").value<QObject *>();
			if (content)
				for (auto child : content->findChildren<QQuickItem *>())
					translatedTitle |= child->property("text").toString() == "Call Support & team";
		}
		check(translatedTitle, "title uses existing Call translation and literal contact name");
		check(actions.size() == 2 && actions[1]->isEnabled(), "prefilled Call enabled");
		QTest::qWait(100);
		check(window->grabWindow().save(output + "/predial-" + theme + ".png"), "native production controls rendered");
		if (actions.size() == 2) click(actions[0]);
		check(popup && !popup->property("visible").toBool() && context.destinations.size() == before,
		      "Cancel closes with no call");
		popup = open("sip:49202@192.0.2.11", false);
		input = field(popup);
		check(input && input->property("text").toString() == "49202", "second popup has fresh destination");
		if (input) input->forceActiveFocus();
		QTest::keyClick(window, Qt::Key_Escape);
		QTest::qWait(80);
		check(popup && !popup->property("visible").toBool() && context.destinations.size() == before,
		      "Escape closes with no call");
		popup = open("sip:39102@192.0.2.10;transport=tcp", false);
		actions = buttons(popup);
		if (actions.size() == 2) click(actions[1]);
		check(context.destinations.size() == before + 1 &&
		          context.destinations.last() == "39102@192.0.2.10;transport=tcp" &&
		          !context.lastOptions.value("localVideoEnabled").toBool(),
		      "default single SIP Call preserves destination and parameters");
		if (popup && popup->property("visible").toBool()) QMetaObject::invokeMethod(popup, "close");
		const int afterDefault = context.destinations.size();
		popup = open("sip:39102@192.0.2.10;transport=tcp", true);
		input = field(popup);
		actions = buttons(popup);
		if (input) input->setProperty("text", "39103");
		if (actions.size() == 2) click(actions[1]);
		check(context.destinations.size() == afterDefault + 1 &&
		          context.destinations.last() == "39103@192.0.2.10;transport=tcp" &&
		          context.lastOptions.value("localVideoEnabled").toBool(),
		      "edited extension preserves domain parameters and video flag");
		popup = open("sip:39102@192.0.2.10", false);
		input = field(popup);
		actions = buttons(popup);
		if (input) input->setProperty("text", "sip:other@198.51.100.7:5070;transport=tcp");
		if (actions.size() == 2) click(actions[1]);
		check(context.destinations.size() == afterDefault + 2 &&
		          context.destinations.last() == "sip:other@198.51.100.7:5070;transport=tcp" &&
		          !context.lastOptions.value("localVideoEnabled").toBool(),
		      "full edited SIP URI and audio flag preserved");
	}
	check(warnings.isEmpty(), "no QML warnings");
	std::printf("Checks %d; failures %d\n", checks, failures);
	return failures ? 1 : 0;
}

#include "predial_popup_test.moc"
