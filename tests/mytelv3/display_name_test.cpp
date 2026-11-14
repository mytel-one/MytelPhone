#include <QGuiApplication>
#include <QStandardPaths>
#include <QWindow>
#include <cstdio>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#define APPLICATION_NAME "MyTel"

int main(int argc, char **argv) {
	QGuiApplication app(argc, argv);
	QCoreApplication::setApplicationName("mytellphone");
	QCoreApplication::setOrganizationDomain("mytellphone");
	const QString profile = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
#include "display_name_production.inc"
	QWindow window;
	window.create(); // Hidden native window: exercise the platform's default title.
	const bool display = QGuiApplication::applicationDisplayName() == "MyTel";
	const bool identity = QCoreApplication::applicationName() == "mytellphone" &&
	                      QCoreApplication::organizationDomain() == "mytellphone" &&
	                      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) == profile;
	bool nativeTitle = true;
#ifdef Q_OS_WIN
	wchar_t title[256] = {};
	GetWindowTextW(reinterpret_cast<HWND>(window.winId()), title, 256);
	nativeTitle = QString::fromWCharArray(title) == "MyTel";
	std::printf("Native hidden default-title window: %s\n", qPrintable(QString::fromWCharArray(title)));
#endif
	std::printf("%s MyTel display name; %s preserved application/profile identity; %s native default title\n",
	            display ? "PASS" : "FAIL", identity ? "PASS" : "FAIL", nativeTitle ? "PASS" : "FAIL");
	return display && identity && nativeTitle ? 0 : 1;
}
