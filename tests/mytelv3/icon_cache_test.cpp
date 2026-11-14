#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QStandardPaths>
#include <QSvgRenderer>
#include <QTemporaryDir>
#include <cstdio>

class Utils {
public:
	static QString getIconAsPng(const QString &, const QSize & = QSize(64, 64));
};
#include "icon_cache_production.inc"

int main(int argc, char **argv) {
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QGuiApplication app(argc, argv);
	QTemporaryDir dir;
	const QString source = dir.filePath("logo.svg");
	auto writeSvg = [&](const QByteArray &color) {
		QFile file(source);
		if (!file.open(QIODevice::WriteOnly)) return false;
		return file.write("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"32\" height=\"32\"><rect width=\"32\" "
		                  "height=\"32\" fill=\"" +
		                  color + "\"/></svg>") > 0;
	};
	if (!writeSvg("#ff9900")) return 2;
	const QString old = Utils::getIconAsPng(source);
	if (!writeSvg("#0088ff")) return 2;
	const QString updated = Utils::getIconAsPng(source);
	const QString large = Utils::getIconAsPng(source, QSize(128, 128));
	int failures = 0, checks = 0;
	auto check = [&](bool ok, const char *name) {
		std::printf("%s %s\n", ok ? "PASS" : "FAIL", name);
		++checks;
		failures += !ok;
	};
	check(old != updated, "changed-logo-does-not-reuse-old-orange-cache");
	check(QImage(updated).pixelColor(32, 32) == QColor("#0088ff"), "changed-logo-renders-current-artwork");
	check(large != updated && QImage(large).size() == QSize(128, 128), "requested-resolution-has-independent-cache");
	check(Utils::getIconAsPng(source) == updated, "unchanged-artwork-reuses-valid-cache");
	check(Utils::getIconAsPng(dir.filePath("missing.svg")).isEmpty(), "invalid-icon-does-not-return-fake-path");
	QSvgRenderer branded(QStringLiteral(MYTEL_SOURCE_ROOT "/Linphone/data/image/logo.svg"));
	QImage image(256, 256, QImage::Format_ARGB32_Premultiplied);
	image.fill(Qt::transparent);
	QPainter painter(&image);
	branded.render(&painter);
	painter.end();
	const QColor top = image.pixelColor(128, 12);
	check(branded.isValid() && top.blue() > top.red() + 60, "actual-branded-svg-renders-blue-MyTel-ring");
	std::printf("%d/%d icon rendering/cache checks passed.\n", checks - failures, checks);
	return failures ? 1 : 0;
}
