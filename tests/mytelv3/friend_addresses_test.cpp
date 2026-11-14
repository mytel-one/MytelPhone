#include <QCoreApplication>
#include <QList>
#include <QStringList>
#include <QVariant>
#include <algorithm>
#include <cstdio>

// Compile the actual production method bodies. These seams replace only the
// unrelated application/SDK class members, not the deduplication or SIP parser.
class Utils {
public:
	static QString getUsername(const QString &address);
};
class FriendCore {
public:
	QList<QVariant> getAllAddresses() const;
	QList<QVariant> mAddressList;
	QList<QVariant> mPhoneNumberList;
};
#include "friend_production.inc"
#include "username_production.inc"

static QList<QVariant> entries(const QStringList &values) {
	QList<QVariant> result;
	for (const auto &value : values)
		result.append(QVariantMap{{"address", value}});
	return result;
}

static QStringList values(const QList<QVariant> &list) {
	QStringList result;
	for (const auto &entry : list)
		result.append(entry.toMap()["address"].toString());
	return result;
}

int main(int argc, char **argv) {
	QCoreApplication app(argc, argv);
	int failures = 0, count = 0;
	auto check = [&](const char *name, const QStringList &sip, const QStringList &phones, const QStringList &expected) {
		FriendCore contact;
		contact.mAddressList = entries(sip);
		contact.mPhoneNumberList = entries(phones);
		const auto actual = values(contact.getAllAddresses());
		const bool passed =
		    actual == expected && values(contact.mPhoneNumberList) == phones && values(contact.mAddressList) == sip;
		std::printf("%s: [%s] -> %s\n", name, qPrintable(actual.join(", ")), passed ? "PASS" : "FAIL");
		count++;
		if (!passed) failures++;
	};
	check("SIP takes precedence", {"sip:301@pbx.example"}, {"301", "302"}, {"302", "sip:301@pbx.example"});
	check("nonmatching phones retain order", {"sip:301@pbx.example"}, {"303", "302"},
	      {"303", "302", "sip:301@pbx.example"});
	check("multiple SIP matches", {"sip:301@pbx.example", "sip:302@pbx.example"}, {"301", "302", "303"},
	      {"303", "sip:301@pbx.example", "sip:302@pbx.example"});
	check("remove every matching duplicate", {"sip:301@pbx.example"}, {"301", "301"}, {"sip:301@pbx.example"});
	check("phone-only contact unchanged", {}, {"301", "302"}, {"301", "302"});
	check("SIP-only contact unchanged", {"sip:301@pbx.example"}, {}, {"sip:301@pbx.example"});
	check("empty contact", {}, {}, {});
	std::printf("Contact addresses: %d/%d PASS\n", count - failures, count);
	return failures == 0 ? 0 : 1;
}
