#include <QEventLoop>
#include <QFile>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>
#include <cstdio>

class GeometryUtils : public QObject {
	Q_OBJECT
public:
	Q_INVOKABLE double getSizeWithScreenRatio(double value) {
		return value;
	}
};

static QByteArray sourceBlock(const QString &file, const QByteArray &marker) {
	QFile input(file);
	if (!input.open(QIODevice::ReadOnly)) return {};
	const QByteArray source = input.readAll();
	const auto markerAt = source.indexOf(marker);
	if (markerAt < 0) return {};
	auto start = markerAt;
	if (marker.startsWith("header:")) start = source.indexOf("Control.Control", markerAt);
	auto end = source.indexOf('{', start) + 1;
	int depth = 1;
	while (depth && end < source.size()) {
		if (source.at(end) == '{') depth++;
		if (source.at(end) == '}') depth--;
		end++;
	}
	return source.mid(start, end - start);
}

int main(int argc, char **argv) {
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QGuiApplication app(argc, argv);
	const QString root = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(MYTEL_SOURCE_ROOT);
	qmlRegisterModule("Linphone", 1, 0);
	qmlRegisterType(QUrl::fromLocalFile(root + "/Linphone/view/Control/Display/Flickable.qml"), "TestComponents", 1, 0,
	                "Flickable");
	GeometryUtils utils;
	QQmlEngine engine;
	int warnings = 0;
	QObject::connect(&engine, &QQmlEngine::warnings, [&](const QList<QQmlError> &errors) {
		for (const auto &error : errors) {
			warnings++;
			std::fprintf(stderr, "QML layout warning: %s\n", qPrintable(error.toString()));
		}
	});
	engine.rootContext()->setContextProperty("Utils", &utils);
	const QVariantMap font{{"pixelSize", 14}, {"weight", 400}};
	engine.rootContext()->setContextProperty("Typography", QVariantMap{{"p1", font}, {"p3", font}});
	engine.rootContext()->setContextProperty("DefaultStyle",
	                                         QVariantMap{{"main2_400", "#6c7a87"}, {"main2_700", "#364860"}});
	const QByteArray prefix = R"(
        import QtQuick
        import QtQuick as Quick
        import QtQuick.Controls.Basic as Control
        import QtQuick.Layouts
        import TestComponents
    )";
	const QByteArray header =
	    sourceBlock(root + "/Linphone/view/Control/Display/Chat/ChatMessagesListView.qml", "header: Control.Control {");
	const QByteArray editor =
	    sourceBlock(root + "/Linphone/view/Control/Input/Chat/ChatDroppableTextArea.qml", "Flickable {");
	if (header.isEmpty() || editor.isEmpty()) return 2;
	auto run = [&](const char *name, const QByteArray &scene) {
		const int before = warnings;
		QQmlComponent component(&engine);
		component.setData(prefix + scene, QUrl());
		QScopedPointer<QObject> object(component.create());
		if (!object) {
			for (const auto &error : component.errors())
				std::fprintf(stderr, "%s\n", qPrintable(error.toString()));
			return false;
		}
		QQuickWindow window;
		window.setGeometry(0, 0, 460, 240);
		auto item = qobject_cast<QQuickItem *>(object.data());
		item->setParentItem(window.contentItem());
		window.show();
		QEventLoop loop;
		QTimer::singleShot(150, &loop, &QEventLoop::quit);
		loop.exec();
		item->setParentItem(nullptr);
		const bool passed = warnings == before;
		std::printf("%s: %d QML warnings -> %s\n", name, warnings - before, passed ? "PASS" : "FAIL");
		return passed;
	};
	const bool headerOk = run("production composing header", R"(
        Item {
            id: mainItem
            width: 460; height: 240
            property var chat: ({core: {composingName: "302", composingAddress: "sip:302@pbx"}})
            property color backgroundColor: "#ffffff"
            component Avatar: Quick.Item { property var _address }
        )" + header + "}");
	const bool editorOk = run("production composer Flickable", R"(
        Control.Control {
            id: mainItem
            width: 460; height: 160; padding: 16
            property string text: ""
            property var textArea
            property bool textAreaHeightSetByUser: false
            signal sendMessage()
            signal focusTextArea()
            component TextArea: Control.TextArea {}
            Item { id: recordButton }
            Item { id: sendMessageButton }
            QtObject { id: sendingControl; property real topPadding: 12; property real bottomPadding: 12 }
            RowLayout { anchors.fill: parent
        )" + editor + "}}");
	return headerOk && editorOk ? 0 : 1;
}

#include "chat_layout_test.moc"
