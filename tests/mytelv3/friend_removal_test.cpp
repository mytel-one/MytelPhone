#include <QCoreApplication>
#include <QList>
#include <QProcess>
#include <QStringList>
#include <QVariant>
#include <cstdio>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

class FriendCore {
public:
	void removeAddress(int index);
	void removePhoneNumber(int index);
	void addressChanged() {
		addressNotifications++;
	}
	void phoneNumberChanged() {
		phoneNotifications++;
	}
	void setIsSaved(bool value) {
		saved = value;
	}
	QList<QVariant> mAddressList;
	QList<QVariant> mPhoneNumberList;
	QString mDefaultFullAddress;
	QString mDefaultAddress;
	bool saved = true;
	int addressNotifications = 0;
	int phoneNotifications = 0;
};
#include "remove_address_production.inc"
#include "remove_phone_production.inc"

static QList<QVariant> entries(const QStringList &values) {
	QList<QVariant> result;
	for (const auto &value : values)
		result.append(QVariantMap{{"address", value}});
	return result;
}

static int check(const QString &name) {
	FriendCore contact;
	const bool phone = name.startsWith("phone");
	const bool empty = name.endsWith("empty");
	const bool valid = name.contains("valid");
	const bool other = name.endsWith("other");
	const auto original = entries(empty ? QStringList{} : QStringList{"sip:301@pbx", "sip:302@pbx"});
	contact.mAddressList = original;
	contact.mPhoneNumberList = original;
	contact.mDefaultAddress = "sip:301@pbx";
	contact.mDefaultFullAddress = "sip:301@pbx";
	int index = name.endsWith("negative-one")   ? -1
	            : name.endsWith("negative-two") ? -2
	            : name.endsWith("at-size")      ? int(original.size())
	            : other                         ? 1
	                                            : 0;
	if (phone) contact.removePhoneNumber(index);
	else contact.removeAddress(index);
	const auto actual = phone ? contact.mPhoneNumberList : contact.mAddressList;
	auto expected = original;
	if (valid) expected.remove(index);
	const bool defaultsCleared = !phone && valid && !other;
	const QString expectedDefault = defaultsCleared ? QString() : QStringLiteral("sip:301@pbx");
	const bool passed = actual == expected && contact.saved == !valid &&
	                    contact.addressNotifications == (!phone && valid ? 1 : 0) &&
	                    contact.phoneNotifications == (phone && valid ? 1 : 0) &&
	                    contact.mDefaultAddress == expectedDefault && contact.mDefaultFullAddress == expectedDefault;
	return passed ? 0 : 1;
}

int main(int argc, char **argv) {
#ifdef Q_OS_WIN
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
	QCoreApplication app(argc, argv);
	if (app.arguments().size() > 1) return check(app.arguments().at(1));
	const QStringList cases = {
	    "address-negative-one", "address-at-size",    "address-empty", "address-valid-default", "address-valid-other",
	    "phone-negative-one",   "phone-negative-two", "phone-at-size", "phone-empty",           "phone-valid"};
	int failures = 0;
	for (const auto &name : cases) {
		QProcess child;
		child.start(app.applicationFilePath(), {name});
		const bool finished = child.waitForStarted(5000) && child.waitForFinished(5000);
		const bool passed = finished && child.exitStatus() == QProcess::NormalExit && child.exitCode() == 0;
		if (!finished) {
			child.kill();
			child.waitForFinished(1000);
		}
		std::printf("%s -> %s (exit %d)\n", qPrintable(name), passed ? "PASS" : "FAIL", child.exitCode());
		if (!passed) failures++;
	}
	std::printf("Contact removal bounds: %d/%d PASS\n", int(cases.size()) - failures, int(cases.size()));
	return failures == 0 ? 0 : 1;
}
