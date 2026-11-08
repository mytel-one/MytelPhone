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
#ifndef SERVER_DIRECTORY_MANAGER_H
#define SERVER_DIRECTORY_MANAGER_H

#include <QByteArray>
#include <QHash>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QVector>

/**
 * Server phonebook (owner requirement 2026-11-08, leader corrections
 * 2026-11-08): fetches a JSON directory over an explicitly configured HTTPS
 * URL per account and caches the last valid snapshot.
 *
 * The directory is used by the contact/search pickers (injected next to
 * MagicSearch results, which do not index custom friend lists on every
 * backend) and by the pre-dial editor for Iranian number normalization.
 *
 * HTTPS-only; snapshots cached per identity+URL; 512 KiB byte cap enforced
 * while streaming the body.
 */
class ServerDirectoryManager : public QObject {
	Q_OBJECT

public:
	struct Entry {
		QString name;
		QString number;
	};

	static ServerDirectoryManager *getInstance();

	// Config storage keys (mirrors the Android implementation keys)
	static QString directoryUrlKey (const QString &identity);
	static QString directoryCacheKey (const QString &identity, const QString &url);

	Q_INVOKABLE void setDirectoryUrl (const QString &identity, const QString &url);
	Q_INVOKABLE QString getDirectoryUrl (const QString &identity) const;

	// Synchronous fetch (IO thread) + parse. Blocking by design: called from
	// the search worker path. Returns false on failure (cache preserved).
	Q_INVOKABLE bool refresh (const QString &identity);

	QVector<Entry> getEntries (const QString &identity) const;
	QVector<Entry> searchEntries (const QString &identity, const QString &filter) const;

signals:
	void directoryUpdated (const QString &identity, int entryCount);

private:
	explicit ServerDirectoryManager ();

	QString fetchToBuffer (const QString &url, bool *ok);
	void saveCache (const QString &identity, const QString &url, const QString &body);
	QString loadCache (const QString &identity, const QString &url) const;

	QHash<QString, QString> mCaches; // identity+url -> body
	QHash<QString, QString> mUrls;   // identity -> url
	mutable QMutex mMutex;
};

#endif // SERVER_DIRECTORY_MANAGER_H
