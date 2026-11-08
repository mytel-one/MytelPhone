/*
 * Copyright (c) 2026 Belledonne Communications SARL.
 *
 * This file is part of linphone-desktop
 * (see https://www.linphone.org).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "ServerDirectoryManager.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "model/core/CoreModel.hpp"
#include "tool/Utils.hpp"
#include <QTimer>

constexpr int ConnectTimeoutMs = 10000;
constexpr int ReadTimeoutMs = 10000;
constexpr qint64 MaxBodyBytes = 512 * 1024;
constexpr int MaxNameLength = 128;
constexpr int MaxNumberLength = 32;

ServerDirectoryManager *ServerDirectoryManager::getInstance () {
	static ServerDirectoryManager instance;
	return &instance;
}

QString ServerDirectoryManager::directoryUrlKey (const QString &identity) {
	return identity + QStringLiteral("_directory_url");
}

QString ServerDirectoryManager::directoryCacheKey (const QString &identity, const QString &url) {
	return identity + QStringLiteral("_directory_cache_") + QString::number(qHash(url), 16);
}

ServerDirectoryManager::ServerDirectoryManager () : QObject(nullptr) {
}

void ServerDirectoryManager::setDirectoryUrl (const QString &identity, const QString &url) {
	QMutexLocker locker(&mMutex);
	QString trimmed = url.trimmed();
	if (trimmed.startsWith(QStringLiteral("https://"))) {
		mUrls[identity] = trimmed;
	} else if (trimmed.isEmpty()) {
		mUrls.remove(identity);
		mCaches.remove(identity); // URL removed -> cache gone too
	}
	// Persist through the core config like Android does
	auto core = CoreModel::getInstance()->getCore();
	if (core) {
		core->getConfig()->setString("app", directoryUrlKey(identity).toStdString(), trimmed.toStdString());
	}
}

QString ServerDirectoryManager::getDirectoryUrl (const QString &identity) const {
	QMutexLocker locker(&mMutex);
	if (mUrls.contains(identity)) return mUrls[identity];
	auto core = CoreModel::getInstance()->getCore();
	if (!core) return QString();
	auto config = core->getConfig();
	if (!config) return QString();
	return QString::fromStdString(config->getString("app", directoryUrlKey(identity).toStdString(), ""));
}

bool ServerDirectoryManager::refresh (const QString &identity) {
	QString url = getDirectoryUrl(identity);
	if (url.isEmpty() || !url.startsWith(QStringLiteral("https://"))) {
		lWarning() << "[Server Directory] No valid HTTPS directory URL for" << identity;
		return false;
	}
	bool ok = false;
	QString body = fetchToBuffer(url, &ok);
	if (!ok) {
		lWarning() << "[Server Directory] Fetch failed for" << url << "- keeping previous cache";
		return false;
	}
	// Parse and validate
	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(body.toUtf8(), &parseError);
	if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
		lWarning() << "[Server Directory] Invalid directory body, keeping previous cache";
		return false;
	}
	QJsonArray array = doc.array();
	QVector<Entry> entries;
	for (const auto &value : array) {
		if (!value.isObject()) continue;
		QJsonObject obj = value.toObject();
		if (!obj.contains("name") || !obj.contains("number")) continue;
		if (!obj.value("name").isString() || !obj.value("number").isString()) {
			lWarning() << "[Server Directory] Entry violates schema (name/number must be strings)";
			return false;
		}
		QString name = obj.value("name").toString();
		QString number = obj.value("number").toString();
		if (name.isEmpty() || number.isEmpty()) continue;
		if (name.length() > MaxNameLength || number.length() > MaxNumberLength) {
			lWarning() << "[Server Directory] Entry too long, invalid body";
			return false;
		}
		entries.append({name, number});
	}
	saveCache(identity, url, body);
	lInfo() << "[Server Directory] Refreshed" << entries.size() << "entries for" << identity;
	emit directoryUpdated(identity, entries.size());
	return true;
}

QVector<ServerDirectoryManager::Entry> ServerDirectoryManager::getEntries (const QString &identity) const {
	QMutexLocker locker(&mMutex);
	QString url = getDirectoryUrl(identity);
	QString cacheKey = identity + QStringLiteral("_") + QString::number(qHash(url), 16);
	Q_UNUSED(cacheKey)
	QVector<Entry> out;
	QString body = loadCache(identity, url);
	if (body.isEmpty()) return out;
	QJsonDocument doc = QJsonDocument::fromJson(body.toUtf8());
	if (!doc.isArray()) return out;
	for (const auto &value : doc.array()) {
		if (!value.isObject()) continue;
		QJsonObject obj = value.toObject();
		QString name = obj.value("name").toString();
		QString number = obj.value("number").toString();
		if (!name.isEmpty() && !number.isEmpty()) out.append({name, number});
	}
	return out;
}

QVector<ServerDirectoryManager::Entry> ServerDirectoryManager::searchEntries (
    const QString &identity, const QString &filter
) const {
	QVector<Entry> all = getEntries(identity);
	if (filter.isEmpty()) return all;
	QString lower = filter.toLower();
	QVector<Entry> out;
	for (const auto &entry : all) {
		if (entry.name.toLower().contains(lower) || entry.number.contains(filter)) {
			out.append(entry);
		}
	}
	return out;
}

QString ServerDirectoryManager::fetchToBuffer (const QString &url, bool *ok) {
	QNetworkAccessManager manager;
	QNetworkRequest request((QUrl(url)));
	request.setRawHeader("Accept", "application/json");
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
	                     QNetworkRequest::NoLessSafeRedirectPolicy);
	QNetworkReply *reply = manager.get(request);
	QEventLoop loop;
	QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	QObject::connect(reply, &QNetworkReply::errorOccurred, &loop, &QEventLoop::quit);
	QTimer timer;
	timer.setSingleShot(true);
	QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
	timer.start(ConnectTimeoutMs + ReadTimeoutMs);
	loop.exec();
	if (!reply->isFinished() || reply->error() != QNetworkReply::NoError) {
		lWarning() << "[Server Directory] Fetch error:" << reply->errorString();
		reply->deleteLater();
		if (ok) *ok = false;
		return QString();
	}
	int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (code != 200) {
		lWarning() << "[Server Directory] HTTP code" << code;
		reply->deleteLater();
		if (ok) *ok = false;
		return QString();
	}
	// Streaming byte cap: read in chunks, reject oversized bodies mid-read
	QByteArray body;
	while (!reply->atEnd()) {
		QByteArray chunk = reply->read(8192);
		body.append(chunk);
		if (body.size() > MaxBodyBytes) {
			lWarning() << "[Server Directory] Body exceeds" << MaxBodyBytes << "bytes while streaming";
			reply->deleteLater();
			if (ok) *ok = false;
			return QString();
		}
	}
	reply->deleteLater();
	if (ok) *ok = true;
	return QString::fromUtf8(body);
}

void ServerDirectoryManager::saveCache (const QString &identity, const QString &url, const QString &body) {
	QMutexLocker locker(&mMutex);
	mCaches[identity + QStringLiteral("_") + QString::number(qHash(url), 16)] = body;
	// Persist through the core config (atomic, survives restarts)
	auto core = CoreModel::getInstance()->getCore();
	if (core) {
		core->getConfig()->setString("app", directoryCacheKey(identity, url).toStdString(), body.toStdString());
	}
}

QString ServerDirectoryManager::loadCache (const QString &identity, const QString &url) const {
	auto core = CoreModel::getInstance()->getCore();
	if (!core) return QString();
	auto config = core->getConfig();
	if (!config) return QString();
	return QString::fromStdString(config->getString("app", directoryCacheKey(identity, url).toStdString(), ""));
}
