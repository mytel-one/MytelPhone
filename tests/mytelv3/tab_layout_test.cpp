#include <QEventLoop>
#include <QFile>
#include <QGuiApplication>
#include <QJSValue>
#include <QJsonDocument>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QTimer>
#include <QVariant>
#include <iostream>

// Keep the real root control, height policy, availability count, content view,
// selection binding and click handler. Only unrelated branding/account badges
// are replaced by deterministic rows. No private Qt API is used.
static QString block(const QString &source, const QString &start) {
	const int begin = source.indexOf(start);
	const int open = source.indexOf('{', begin);
	if (begin < 0 || open < 0) qFatal("Missing production source seam");
	int depth = 0;
	for (int i = open; i < source.size(); ++i) {
		if (source[i] == '{') ++depth;
		if (source[i] == '}' && !--depth) return source.mid(begin, i - begin + 1);
	}
	qFatal("Unbalanced production source seam");
	return {};
}

static QString sourceLine(const QString &source, const QString &pattern, bool required = true) {
	const auto match = QRegularExpression(pattern, QRegularExpression::MultilineOption).match(source);
	if (!match.hasMatch() && required) qFatal("Missing production source line");
	return match.captured().trimmed();
}

static void settle() {
	QEventLoop loop;
	QTimer::singleShot(100, &loop, &QEventLoop::quit);
	loop.exec();
}

int main(int argc, char **argv) {
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QGuiApplication app(argc, argv);
	app.setQuitOnLastWindowClosed(false);
	const QString path = argc > 1
	                         ? QString::fromLocal8Bit(argv[1])
	                         : QStringLiteral(MYTEL_SOURCE_ROOT "/Linphone/view/Control/Container/VerticalTabBar.qml");
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) qFatal("Cannot open production navigation component");
	const auto source = QString::fromUtf8(file.readAll());
	const auto rootType = sourceLine(source, "^\\s*Control\\.(?:TabBar|Container)\\s*\\{");
	const auto height = source.contains("Binding on height")
	                        ? block(source, "Binding on height")
	                        : sourceLine(source, "^\\s*height:\\s*isAvailable[^\\r\\n]*");
	const auto checked = sourceLine(source, "^\\s*checked:[^\\r\\n]*", false);
	const auto exclusive = sourceLine(source, "^\\s*autoExclusive:[^\\r\\n]*", false);
	QString qml = R"QML(import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Controls.Basic as Control
Window {
    visible: true; width: 640; height: 480
    Control.StackView { id: stack; anchors.fill: parent; initialItem: mainPage }
    Component { id: mainPage
        Item { property alias nav: mainItem
            @ROOT@ id: mainItem; width: 120; height: parent.height
                property int visibleCount: 0
                property bool featureAvailable: false
                @COUNT@
                @LIST@
                Repeater {
                    model: [{label:"Calls"},{label:"Contacts"},{label:"Chats"},{label:"Meetings",visible:mainItem.featureAvailable}]
                    Control.TabButton { id: tabButton; width: mainItem.width; implicitHeight: 64
                        readonly property bool isAvailable: modelData?.visible ?? true
                        @HEIGHT@
                        @CHECKED@
                        @EXCLUSIVE@
                        @CLICK@
                        visible: modelData?.visible != undefined ? modelData.visible : true
                        text: modelData.label
                        onVisibleChanged: mainItem.updateVisibleCount()
                    }
                }
            }
        }
    }
    Component { id: accountPage; Rectangle { color: "white" } }
    function pushPage() { stack.push(accountPage, {}, Control.StackView.Immediate) }
    function popPage() { stack.pop(Control.StackView.Immediate) }
    function selectChats() { stack.get(0).nav.itemAt(2).clicked() }
    function showFeature() { stack.get(0).nav.featureAvailable = true }
    function hideFeature() { stack.get(0).nav.featureAvailable = false }
    function sample() {
        const nav = stack.get(0).nav; let rows = [];
        for (let i=0; i<nav.count; i++) {
            const row=nav.itemAt(i);
            rows.push({height:row.height,y:row.y,available:row.isAvailable,checked:row.checked});
        }
        return {count:nav.visibleCount,currentIndex:nav.currentIndex,rows:rows};
    }
})QML";
	qml.replace("@ROOT@", rootType);
	qml.replace("@COUNT@", block(source, "function updateVisibleCount()"));
	qml.replace("@LIST@", block(source, "contentItem: ListView"));
	qml.replace("@HEIGHT@", height);
	qml.replace("@CHECKED@", checked);
	qml.replace("@EXCLUSIVE@", exclusive);
	qml.replace("@CLICK@", block(source, "onClicked:"));
	QQmlEngine engine;
	QQmlComponent component(&engine);
	component.setData(qml.toUtf8(), QUrl());
	QObject *root = component.create();
	if (!root) {
		qCritical() << component.errors();
		return 2;
	}
	int checks = 0, failures = 0;
	auto check = [&](bool feature, bool hidden) {
		settle();
		QVariant result;
		QMetaObject::invokeMethod(root, "sample", Q_RETURN_ARG(QVariant, result));
		if (result.canConvert<QJSValue>()) result = result.value<QJSValue>().toVariant();
		const auto state = result.toMap();
		const auto rows = state.value("rows").toList();
		bool ok = rows.size() == 4 && state.value("count").toInt() == (feature ? 4 : 3) &&
		          state.value("currentIndex").toInt() == 2;
		double lastY = -1;
		int selected = 0;
		for (int i = 0; i < rows.size(); ++i) {
			const auto row = rows[i].toMap();
			const bool available = i != 3 || feature;
			ok &= row.value("height").toDouble() == (available ? 64 : 0);
			if (available) {
				const auto y = row.value("y").toDouble();
				ok &= y > lastY;
				lastY = y;
			}
			if (row.value("checked").toBool()) ++selected;
			ok &= row.value("checked").toBool() == (i == 2);
		}
		ok &= selected == 1;
		++checks;
		failures += !ok;
		std::cout << (ok ? "PASS " : "FAIL ") << "feature=" << feature << " ancestorHidden=" << hidden << ' '
		          << QJsonDocument::fromVariant(result).toJson(QJsonDocument::Compact).toStdString() << std::endl;
	};
	settle();
	for (bool feature : {false, true, false}) {
		QMetaObject::invokeMethod(root, feature ? "showFeature" : "hideFeature");
		// Feature availability intentionally rebuilds the Repeater model.
		// Choose Chats through the production click handler after that reset,
		// then verify selection persists across every Back navigation.
		settle();
		QMetaObject::invokeMethod(root, "selectChats");
		check(feature, false);
		for (int cycle = 0; cycle < 3; ++cycle) {
			QMetaObject::invokeMethod(root, "pushPage");
			check(feature, true);
			QMetaObject::invokeMethod(root, "popPage");
			check(feature, false);
		}
	}
	delete root;
	std::cout << checks - failures << '/' << checks << " navigation geometry/selection checks passed" << std::endl;
	return failures ? 1 : 0;
}
