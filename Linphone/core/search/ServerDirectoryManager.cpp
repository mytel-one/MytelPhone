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
#include <QUrl>

#include "model/core/CoreModel.hpp"
#include "tool/Utils.hpp"
#include <QTimer>

constexpr int ConnectTimeoutMs = 10000;
constexpr int ReadTimeoutMs = 10000;
constexpr qint64 MaxBodyBytes = 512 * 1024;
constexpr int MaxNameLength = 128;
constexpr int MaxNumberLength = 32;

namespace {
bool isDirectoryUrl(const QString &value) {
	const QUrl url(value, QUrl::StrictMode);
	return url.isValid() && url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0 &&
	       !url.host().isEmpty();
}

bool parseDirectory(const QString &body, QVector<ServerDirectoryManager::Entry> &entries) {
	if (body.toUtf8().size() > MaxBodyBytes) return false;
	QJsonParseError error;
	const auto document = QJsonDocument::fromJson(body.toUtf8(), &error);
	if (error.error != QJsonParseError::NoError || !document.isArray()) return false;
	const auto array = document.array();
	for (const auto &value : array) {
		if (!value.isObject()) continue;
		const auto object = value.toObject();
		if (!object.contains("name") || !object.contains("number")) continue;
		if (!object.value("name").isString() || !object.value("number").isString()) return false;
		const auto name = object.value("name").toString();
		const auto number = object.value("number").toString();
		if (name.size() > MaxNameLength || number.size() > MaxNumberLength) return false;
		if (!name.isEmpty() && !number.isEmpty()) entries.append({name, number});
	}
	// Only [] is a valid empty directory. Malformed entries must not erase
	// the last good snapshot or create a partially validated cached phonebook.
	return array.isEmpty() || !entries.isEmpty();
}
} // namespace

ServerDirectoryManager *ServerDirectoryManager::getInstance() {
	static ServerDirectoryManager instance;
	return &instance;
}

QString ServerDirectoryManager::directoryUrlKey(const QString &identity) {
	return identity + QStringLiteral("_directory_url");
}

QString ServerDirectoryManager::directoryCacheKey(const QString &identity, const QString &url) {
	return identity + QStringLiteral("_directory_cache_") + QString::number(qHash(url), 16);
}

ServerDirectoryManager::ServerDirectoryManager() : QObject(nullptr) {
}

void ServerDirectoryManager::setDirectoryUrl(const QString &identity, const QString &url) {
	QMutexLocker locker(&mMutex);
	const QString trimmed = isDirectoryUrl(url.trimmed()) ? url.trimmed() : QString();
	// Keep an explicit empty value: a rejected/removed URL must not fall back
	// to the previous source still present in the SDK config.
	mUrls[identity] = trimmed;
	// Persist through the core config like Android does
	auto core = CoreModel::getInstance()->getCore();
	if (core) {
		core->getConfig()->setString("app", directoryUrlKey(identity).toStdString(), trimmed.toStdString());
	}
}

QString ServerDirectoryManager::getDirectoryUrl(const QString &identity) const {
	QMutexLocker locker(&mMutex);
	if (mUrls.contains(identity)) return mUrls[identity];
	auto core = CoreModel::getInstance()->getCore();
	if (!core) return QString();
	auto config = core->getConfig();
	if (!config) return QString();
	const auto value = QString::fromStdString(config->getString("app", directoryUrlKey(identity).toStdString(), ""));
	return isDirectoryUrl(value) ? value : QString();
}

bool ServerDirectoryManager::refresh(const QString &identity) {
	QString url = getDirectoryUrl(identity);
	if (url.isEmpty()) {
		lWarning() << "[Server Directory] No valid HTTPS directory URL for" << identity;
		return false;
	}
	bool ok = false;
	QString body = fetchToBuffer(url, &ok);
	if (!ok) {
		lWarning() << "[Server Directory] Fetch failed - keeping previous cache";
		return false;
	}
	// A nested network event loop may process a change of the configured
	// source. Never publish that old response over the new source's cache.
	if (getDirectoryUrl(identity) != url) return false;
	QVector<Entry> entries;
	if (!parseDirectory(body, entries)) {
		lWarning() << "[Server Directory] Invalid directory body, keeping previous cache";
		return false;
	}
	saveCache(identity, url, body);
	lInfo() << "[Server Directory] Refreshed" << entries.size() << "entries for" << identity;
	emit directoryUpdated(identity, entries.size());
	return true;
}

QVector<ServerDirectoryManager::Entry> ServerDirectoryManager::getEntries(const QString &identity) const {
	// getDirectoryUrl takes mMutex itself. Taking the non-recursive lock here
	// deadlocked every MagicSearch query, even with no directory configured.
	QString url = getDirectoryUrl(identity);
	QVector<Entry> out;
	if (url.isEmpty()) return out;
	QString body = loadCache(identity, url);
	if (body.isEmpty()) return out;
	if (!parseDirectory(body, out)) out.clear();
	return out;
}

QVector<ServerDirectoryManager::Entry> ServerDirectoryManager::searchEntries(const QString &identity,
                                                                             const QString &filter) const {
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

QString ServerDirectoryManager::fetchToBuffer(const QString &url, bool *ok) {
	QNetworkAccessManager manager;
	QNetworkRequest request((QUrl(url)));
	request.setRawHeader("Accept", "application/json");
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
	QNetworkReply *reply = manager.get(request);
	reply->setReadBufferSize(64 * 1024);
	QEventLoop loop;
	QByteArray body;
	bool oversized = false;
	auto consume = [&]() {
		while (reply->bytesAvailable() > 0 && !oversized) {
			body.append(reply->read(qMin(qint64(8192), MaxBodyBytes - body.size() + 1)));
			if (body.size() > MaxBodyBytes) {
				oversized = true;
				reply->abort();
				loop.quit();
			}
		}
	};
	QObject::connect(reply, &QNetworkReply::readyRead, &loop, consume);
	QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	QObject::connect(reply, &QNetworkReply::errorOccurred, &loop, &QEventLoop::quit);
	QTimer timer;
	timer.setSingleShot(true);
	QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
	timer.start(ConnectTimeoutMs + ReadTimeoutMs);
	loop.exec();
	consume();
	if (oversized || !reply->isFinished() || reply->error() != QNetworkReply::NoError) {
		// errorString() can embed the full URL, including access tokens.
		lWarning() << "[Server Directory] Fetch failed, network error:" << int(reply->error())
		           << "oversized:" << oversized;
		if (!reply->isFinished()) reply->abort();
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
	reply->deleteLater();
	if (ok) *ok = true;
	return QString::fromUtf8(body);
}

void ServerDirectoryManager::saveCache(const QString &identity, const QString &url, const QString &body) {
	QMutexLocker locker(&mMutex);
	mCaches[identity + QStringLiteral("_") + QString::number(qHash(url), 16)] = body;
	// Persist through the core config (atomic, survives restarts)
	auto core = CoreModel::getInstance()->getCore();
	if (core) {
		core->getConfig()->setString("app", directoryCacheKey(identity, url).toStdString(), body.toStdString());
	}
}

QString ServerDirectoryManager::loadCache(const QString &identity, const QString &url) const {
	auto core = CoreModel::getInstance()->getCore();
	if (!core) return QString();
	auto config = core->getConfig();
	if (!config) return QString();
	return QString::fromStdString(config->getString("app", directoryCacheKey(identity, url).toStdString(), ""));
}
