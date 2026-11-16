/*
 * Copyright (c) 2010-2024 Belledonne Communications SARL.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QList>
#include <QString>
#include <QXmlStreamWriter>

// Keep the native toast schema independent of COM and escape at the XML boundary.
namespace MyTelToastXml {
struct Action {
	QString label;
	QString argument;
	QString icon;
};

inline void beginVisual(QXmlStreamWriter &writer, const QString &icon) {
	writer.writeStartElement("visual");
	writer.writeStartElement("binding");
	writer.writeAttribute("template", "ToastGeneric");
	writer.writeEmptyElement("image");
	writer.writeAttribute("src", QStringLiteral("file:///") + icon);
	writer.writeAttribute("placement", "appLogoOverride");
}

inline void text(QXmlStreamWriter &writer, const QString &value, const QString &style = {}) {
	writer.writeStartElement("text");
	if (!style.isEmpty()) writer.writeAttribute("hint-style", style);
	writer.writeCharacters(value);
	writer.writeEndElement();
}

inline void endVisual(QXmlStreamWriter &writer) {
	writer.writeEndElement(); // binding
	writer.writeEndElement(); // visual
}

inline void silentAudio(QXmlStreamWriter &writer) {
	writer.writeEmptyElement("audio");
	writer.writeAttribute("silent", "true");
}

inline QString message(const QString &title, const QString &sender, const QString &body, const QString &icon) {
	QString xml;
	QXmlStreamWriter writer(&xml);
	writer.writeStartElement("toast");
	beginVisual(writer, icon);
	text(writer, title);
	text(writer, sender);
	writer.writeStartElement("group");
	writer.writeStartElement("subgroup");
	text(writer, body, "body");
	writer.writeEndElement(); // subgroup
	writer.writeEndElement(); // group
	endVisual(writer);
	silentAudio(writer);
	writer.writeEndElement(); // toast
	return xml;
}

inline QString call(const QString &title,
                    const QString &address,
                    const QString &description,
                    const QString &icon,
                    const QList<Action> &actions) {
	QString xml;
	QXmlStreamWriter writer(&xml);
	writer.writeStartElement("toast");
	writer.writeAttribute("scenario", "reminder");
	beginVisual(writer, icon);
	text(writer, title, "header");
	text(writer, address, "base");
	text(writer, description, "body");
	endVisual(writer);
	if (!actions.isEmpty()) {
		writer.writeStartElement("actions");
		for (const auto &action : actions) {
			writer.writeEmptyElement("action");
			writer.writeAttribute("content", action.label);
			writer.writeAttribute("arguments", action.argument);
			writer.writeAttribute("imageUri", action.icon);
		}
		writer.writeEndElement(); // actions
	}
	silentAudio(writer);
	writer.writeEndElement(); // toast
	return xml;
}
} // namespace MyTelToastXml
