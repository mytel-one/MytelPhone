#include <QCoreApplication>
#include <QGuiApplication>
#include <QInputMethod>
#include <QKeyEvent>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QtGui/private/qinputmethod_p.h>
#include <QtGui/qpa/qplatforminputcontext.h>
#include <cstdio>

// Private Qt APIs are used only to inject the test keyboard's direction.
// The exact production helper below uses public Qt APIs exclusively.
class PersianInputContext : public QPlatformInputContext {
public:
	bool isValid() const override {
		return true;
	}
	Qt::LayoutDirection inputDirection() const override {
		return Qt::RightToLeft;
	}
	QLocale locale() const override {
		return QLocale(QLocale::Persian);
	}
};

class Utils {
public:
	static void forceInputLeftToRight(QObject *input);
};
#include "dial_input_production.inc"

static qreal positionX(QObject *input, int position) {
	QRectF rectangle;
	if (!QMetaObject::invokeMethod(input, "positionToRectangle", Q_RETURN_ARG(QRectF, rectangle), Q_ARG(int, position)))
		qFatal("TextInput.positionToRectangle unavailable");
	return rectangle.x();
}

int main(int argc, char **argv) {
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QGuiApplication app(argc, argv);
	PersianInputContext persian;
	auto *inputMethod = QInputMethodPrivate::get(QGuiApplication::inputMethod());
	inputMethod->testContext = &persian;
	QQmlEngine engine;
	QQmlComponent component(&engine);
	component.setData(R"(
        import QtQuick
        import QtQuick.Controls.Basic as Control
        Item {
            width: 400; height: 80
            Control.TextField {
                objectName: "field"; width: parent.width; height: 50
                font.pixelSize: 24; horizontalAlignment: TextInput.AlignLeft
                inputMethodHints: Qt.ImhPreferLatin | Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
            }
        }
    )",
	                  QUrl());
	int failures = 0, cases = 0;
	for (const bool enabled : {true, false}) {
		for (const auto &text :
		     {QStringLiteral("*43"), QStringLiteral("*21*301#"), QStringLiteral("+98301"), QString::fromUtf8("*۴۳")}) {
			QScopedPointer<QObject> container(component.create());
			if (!container) {
				qCritical() << component.errors();
				return 2;
			}
			auto *input = container->findChild<QQuickItem *>("field");
			input->setEnabled(enabled);
			input->setProperty("text", text);
			const bool reproduced = positionX(input, 0) > positionX(input, 1);
			QMetaObject::invokeMethod(input, "select", Q_ARG(int, 1), Q_ARG(int, 3));
			const auto cursor = input->property("cursorPosition");
			const auto start = input->property("selectionStart"), end = input->property("selectionEnd");
			Utils::forceInputLeftToRight(input);
			const bool corrected = positionX(input, 0) < positionX(input, 1);
			const bool untouched = input->property("text").toString() == text &&
			                       input->property("cursorPosition") == cursor &&
			                       input->property("selectionStart") == start && input->property("selectionEnd") == end;
			container->setProperty("width", 480);
			const bool bound = input->width() == 480;
			input->setProperty("text", text + "2");
			const bool subsequent = positionX(input, 0) < positionX(input, 1);
			const bool passed = reproduced && corrected && untouched && bound && subsequent;
			std::printf("%s enabled=%d: RTL reproduced=%d corrected=%d raw/cursor/selection=%d widthBinding=%d "
			            "nextEdit=%d -> %s\n",
			            qPrintable(text), enabled, reproduced, corrected, untouched, bound, subsequent,
			            passed ? "PASS" : "FAIL");
			if (!passed) failures++;
			cases++;
		}
	}
	Utils::forceInputLeftToRight(nullptr);
	QObject ordinaryObject;
	Utils::forceInputLeftToRight(&ordinaryObject);
	inputMethod->testContext = nullptr;
	std::printf("Dial input direction: %d/%d PASS\n", cases - failures, cases);
	return failures ? 1 : 0;
}
