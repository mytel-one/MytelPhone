/*
 * Copyright (c) 2010-2024 Belledonne Communications SARL.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QXmlStreamReader>
#include <cstdio>

#ifdef MYTEL_LEGACY_TOAST_XML
#include "legacy_toast_xml_production.inc"
#else
#include "core/notifier/ToastXml.hpp"
#endif

namespace {
struct Node {
	QString path;
	QMap<QString, QString> attributes;
	QString text;
};
struct Parsed {
	QList<Node> nodes;
	bool valid = false;
};
int cases = 0;
int failures = 0;

Parsed parse(const QString &xml) {
	QXmlStreamReader reader(xml);
	Parsed result;
	QStringList path;
	QList<qsizetype> open;
	while (!reader.atEnd()) {
		reader.readNext();
		if (reader.isStartElement()) {
			path.append(reader.name().toString());
			Node node;
			node.path = path.join('/');
			for (const auto &attribute : reader.attributes())
				node.attributes.insert(attribute.name().toString(), attribute.value().toString());
			open.append(result.nodes.size());
			result.nodes.append(node);
		} else if (reader.isCharacters() && !open.isEmpty()) {
			result.nodes[open.last()].text += reader.text();
		} else if (reader.isEndElement()) {
			path.removeLast();
			open.removeLast();
		}
	}
	result.valid = !reader.hasError() && path.isEmpty();
	return result;
}

void record(const char *name, bool ok) {
	++cases;
	if (!ok) ++failures;
	std::printf("%s %s\n", ok ? "PASS" : "FAIL", name);
}

bool common(const Parsed &parsed, const QStringList &paths, const QString &icon) {
	QStringList actual;
	for (const auto &node : parsed.nodes)
		actual.append(node.path);
	return parsed.valid && actual == paths &&
	       parsed.nodes[2].attributes == QMap<QString, QString>{{"template", "ToastGeneric"}} &&
	       parsed.nodes[3].attributes ==
	           QMap<QString, QString>{{"src", "file:///" + icon}, {"placement", "appLogoOverride"}} &&
	       parsed.nodes.last().attributes == QMap<QString, QString>{{"silent", "true"}};
}

void callCase(const char *name,
              const QString &title,
              const QString &address,
              const QString &description,
              const QString &icon,
              const QList<MyTelToastXml::Action> &actions) {
	const auto parsed = parse(MyTelToastXml::call(title, address, description, icon, actions));
	QStringList paths{"toast",
	                  "toast/visual",
	                  "toast/visual/binding",
	                  "toast/visual/binding/image",
	                  "toast/visual/binding/text",
	                  "toast/visual/binding/text",
	                  "toast/visual/binding/text"};
	if (!actions.isEmpty()) {
		paths.append("toast/actions");
		for (qsizetype i = 0; i < actions.size(); ++i)
			paths.append("toast/actions/action");
	}
	paths.append("toast/audio");
	bool ok = common(parsed, paths, icon);
	if (ok) {
		ok = parsed.nodes[0].attributes == QMap<QString, QString>{{"scenario", "reminder"}} &&
		     parsed.nodes[4].text == title && parsed.nodes[5].text == address && parsed.nodes[6].text == description &&
		     parsed.nodes[4].attributes == QMap<QString, QString>{{"hint-style", "header"}} &&
		     parsed.nodes[5].attributes == QMap<QString, QString>{{"hint-style", "base"}} &&
		     parsed.nodes[6].attributes == QMap<QString, QString>{{"hint-style", "body"}};
		for (qsizetype i = 0; i < actions.size(); ++i) {
			const auto &action = actions[i];
			ok = ok && parsed.nodes[8 + i].attributes == QMap<QString, QString>{{"content", action.label},
			                                                                    {"arguments", action.argument},
			                                                                    {"imageUri", action.icon}};
		}
	}
	record(name, ok);
}

void messageCase(
    const char *name, const QString &title, const QString &sender, const QString &body, const QString &icon) {
	const auto parsed = parse(MyTelToastXml::message(title, sender, body, icon));
	const QStringList paths{"toast",
	                        "toast/visual",
	                        "toast/visual/binding",
	                        "toast/visual/binding/image",
	                        "toast/visual/binding/text",
	                        "toast/visual/binding/text",
	                        "toast/visual/binding/group",
	                        "toast/visual/binding/group/subgroup",
	                        "toast/visual/binding/group/subgroup/text",
	                        "toast/audio"};
	const bool ok = common(parsed, paths, icon) && parsed.nodes[0].attributes.isEmpty() &&
	                parsed.nodes[4].text == title && parsed.nodes[5].text == sender && parsed.nodes[8].text == body &&
	                parsed.nodes[8].attributes == QMap<QString, QString>{{"hint-style", "body"}};
	record(name, ok);
}
} // namespace

int main() {
	const QString icon = QStringLiteral("C:/MyTel/logo.png");
	const QString address = QStringLiteral("sip:202@192.0.2.10");
	const QList<MyTelToastXml::Action> actions{{"Accept", "accept", "C:/MyTel/accept.png"},
	                                           {"Decline", "decline", "C:/MyTel/decline.png"}};
	callCase("call-controls-original-hierarchy", "MyTel Demo", address, "Incoming call", icon, actions);
	callCase("caller-ampersand-persian-and-markup", QStringLiteral("فروش & پشتیبانی <واحد> \"VIP\""), address,
	         QStringLiteral("تماس ورودی"), icon, actions);
	callCase("caller-address-ampersand", "MyTel Demo", address + "?a=1&b=2", "Incoming call", icon, actions);
	callCase("call-icon-attribute-roundtrip", "MyTel Demo", address, "Incoming call", "C:/MyTel & Team/logo \"1\".png",
	         actions);
	auto specialActions = actions;
	specialActions[0].label = QStringLiteral("پاسخ & گفتگو \"سریع\"");
	specialActions[0].icon = "C:/MyTel & Team/accept.png";
	callCase("accept-decline-labels-and-icons", "MyTel Demo", address, "Incoming call", icon, specialActions);
	callCase("caller-literal-entity-and-placeholder", "&amp; %1 %2", address, "Incoming call", icon, actions);
	callCase("call-without-actions", "MyTel Demo", address, "Incoming call", icon, {});
	messageCase("message-persian-and-emoji", QStringLiteral("گروه مای‌تل"), QStringLiteral("فروش"),
	            QStringLiteral("سلام 😀 & <tag>"), icon);
	messageCase("message-literal-cdata-close", "MyTel Demo", "Sales", QStringLiteral("متن ]]> & ادامه"), icon);
	messageCase("room-title-cdata-close", QStringLiteral("گروه ]]> فروش"), "Sales", "Hello", icon);
	messageCase("sender-cdata-close", "MyTel Demo", QStringLiteral("فروش ]]> پشتیبانی"), "Hello", icon);
	messageCase("message-icon-attribute-roundtrip", "MyTel Demo", "", "Hello", "C:/MyTel & Team/logo \"1\".png");
	messageCase("message-literal-entity-and-placeholder", "MyTel Demo", "", "&amp; %1 %2 <b> &lt;", icon);
	messageCase("message-empty-fields", "", "", "", icon);
	std::printf("CASES %d FAILURES %d\n", cases, failures);
	return failures ? 1 : 0;
}
