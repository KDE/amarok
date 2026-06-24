/****************************************************************************************
 * Copyright (c) 2025 Amarok Team <amarok@kde.org>                                     *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

/** MySQL-specific database upgrade operations.
 *  These cover historical schema migrations (charset/engine conversions,
 *  FULLTEXT indexes, MySQL-specific ALTER TABLE syntax) that only apply
 *  when upgrading an existing MySQL database. SQLite databases are always
 *  created fresh with the correct schema.
 *
 *  This file is compiled only when WITH_MYSQL=ON.
 */

#define DEBUG_PREFIX "DatabaseUpdaterMySql"

#include "DatabaseUpdater.h"
#include "core/support/Debug.h"
#include <core/storage/SqlStorage.h>

#include <QMultiMap>
#include <QStringList>

static const int TEXT_COLUMN_LENGTH = 255;

static void doUpgradeVersion4to5( SqlStorage *storage )
{
    // ALTER DATABASE/ALTER TABLE charset conversion + MODIFY columns on MySQL
    storage->query( QStringLiteral("ALTER DATABASE ") + storage->databaseName() + QStringLiteral(" DEFAULT CHARACTER SET utf8 DEFAULT COLLATE utf8_unicode_ci") );

    QStringList tables;
    tables << QStringLiteral("admin") << QStringLiteral("albums") << QStringLiteral("amazon") << QStringLiteral("artists") << QStringLiteral("bookmark_groups") << QStringLiteral("bookmarks");
    tables << QStringLiteral("composers") << QStringLiteral("devices") << QStringLiteral("directories") << QStringLiteral("genres") << QStringLiteral("images") << QStringLiteral("labels") << QStringLiteral("lyrics");
    tables << QStringLiteral("playlist_groups") << QStringLiteral("playlist_tracks") << QStringLiteral("playlists");
    tables << QStringLiteral("podcastchannels") << QStringLiteral("podcastepisodes");
    tables << QStringLiteral("statistics") << QStringLiteral("statistics_permanent") << QStringLiteral("statistics_tag");
    tables << QStringLiteral("tracks") << QStringLiteral("urls") << QStringLiteral("urls_labels") << QStringLiteral("years");

    for( const QString &table : tables )
        storage->query( QStringLiteral("ALTER TABLE ") + table + QStringLiteral(" DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci") );

    // varchar columns
    typedef QPair<QString, int> vcpair;
    QMultiMap<QString, vcpair> columns;
    columns.insert( QStringLiteral("admin"), vcpair( QStringLiteral("component"), 255 ) );
    columns.insert( QStringLiteral("albums"), vcpair( QStringLiteral("name"), TEXT_COLUMN_LENGTH ) );
    columns.insert( QStringLiteral("amazon"), vcpair( QStringLiteral("asin"), 20 ) );
    columns.insert( QStringLiteral("amazon"), vcpair( QStringLiteral("locale"), 2 ) );
    columns.insert( QStringLiteral("amazon"), vcpair( QStringLiteral("filename"), 33 ) );
    columns.insert( QStringLiteral("artists"), vcpair( QStringLiteral("name"), TEXT_COLUMN_LENGTH ) );
    columns.insert( QStringLiteral("bookmark_groups"), vcpair( QStringLiteral("name"), 255 ) );
    columns.insert( QStringLiteral("bookmark_groups"), vcpair( QStringLiteral("description"), 255 ) );
    columns.insert( QStringLiteral("bookmark_groups"), vcpair( QStringLiteral("custom"), 255 ) );
    columns.insert( QStringLiteral("bookmarks"), vcpair( QStringLiteral("name"), 255 ) );
    columns.insert( QStringLiteral("bookmarks"), vcpair( QStringLiteral("url"), 1024 ) );
    columns.insert( QStringLiteral("bookmarks"), vcpair( QStringLiteral("description"), 1024 ) );
    columns.insert( QStringLiteral("bookmarks"), vcpair( QStringLiteral("custom"), 255 ) );
    columns.insert( QStringLiteral("composers"), vcpair( QStringLiteral("name"), TEXT_COLUMN_LENGTH ) );
    columns.insert( QStringLiteral("devices"), vcpair( QStringLiteral("type"), 255 ) );
    columns.insert( QStringLiteral("devices"), vcpair( QStringLiteral("label"), 255 ) );
    columns.insert( QStringLiteral("devices"), vcpair( QStringLiteral("lastmountpoint"), 255 ) );
    columns.insert( QStringLiteral("devices"), vcpair( QStringLiteral("uuid"), 255 ) );
    columns.insert( QStringLiteral("devices"), vcpair( QStringLiteral("servername"), 255 ) );
    columns.insert( QStringLiteral("devices"), vcpair( QStringLiteral("sharename"), 255 ) );
    columns.insert( QStringLiteral("directories"), vcpair( QStringLiteral("dir"), 1024 ) );
    columns.insert( QStringLiteral("genres"), vcpair( QStringLiteral("name"), 255 ) );
    columns.insert( QStringLiteral("images"), vcpair( QStringLiteral("path"), 255 ) );
    columns.insert( QStringLiteral("labels"), vcpair( QStringLiteral("label"), TEXT_COLUMN_LENGTH ) );
    columns.insert( QStringLiteral("lyrics"), vcpair( QStringLiteral("url"), 1024 ) );
    columns.insert( QStringLiteral("playlist_groups"), vcpair( QStringLiteral("name"), 255 ) );
    columns.insert( QStringLiteral("playlist_groups"), vcpair( QStringLiteral("description"), 255 ) );
    columns.insert( QStringLiteral("playlist_tracks"), vcpair( QStringLiteral("url"), 1024 ) );
    columns.insert( QStringLiteral("playlist_tracks"), vcpair( QStringLiteral("title"), 255 ) );
    columns.insert( QStringLiteral("playlist_tracks"), vcpair( QStringLiteral("album"), 255 ) );
    columns.insert( QStringLiteral("playlist_tracks"), vcpair( QStringLiteral("artist"), 255 ) );
    columns.insert( QStringLiteral("playlist_tracks"), vcpair( QStringLiteral("uniqueid"), 128 ) );
    columns.insert( QStringLiteral("playlists"), vcpair( QStringLiteral("name"), 255 ) );
    columns.insert( QStringLiteral("playlists"), vcpair( QStringLiteral("description"), 255 ) );
    columns.insert( QStringLiteral("playlists"), vcpair( QStringLiteral("urlid"), 1024 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("copyright"), 255 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("directory"), 255 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("labels"), 255 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("subscribedate"), 255 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("guid"), 1024 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("mimetype"), 255 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("pubdate"), 255 ) );
    columns.insert( QStringLiteral("statistics_permanent"), vcpair( QStringLiteral("url"), 1024 ) );
    columns.insert( QStringLiteral("statistics_tag"), vcpair( QStringLiteral("name"), 255 ) );
    columns.insert( QStringLiteral("statistics_tag"), vcpair( QStringLiteral("artist"), 255 ) );
    columns.insert( QStringLiteral("tracks"), vcpair( QStringLiteral("title"), TEXT_COLUMN_LENGTH ) );
    columns.insert( QStringLiteral("urls"), vcpair( QStringLiteral("rpath"), 1024 ) );
    columns.insert( QStringLiteral("urls"), vcpair( QStringLiteral("uniqueid"), 128 ) );
    columns.insert( QStringLiteral("years"), vcpair( QStringLiteral("name"), TEXT_COLUMN_LENGTH ) );

    QMultiMap<QString, vcpair>::const_iterator i, iEnd;
    for( i = columns.constBegin(), iEnd = columns.constEnd(); i != iEnd; ++i )
    {
        storage->query( QStringLiteral("ALTER TABLE ") + i.key() + QStringLiteral(" MODIFY ") + i.value().first + QStringLiteral(" VARBINARY(") + QString::number( i.value().second ) + QLatin1Char(')') );
        storage->query( QStringLiteral("ALTER IGNORE TABLE ") + i.key() + QStringLiteral(" MODIFY ") + i.value().first +
            QStringLiteral(" VARCHAR(") + QString::number( i.value().second ) + QStringLiteral(") CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL") );
    }

    columns.clear();

    // text columns
    columns.insert( QStringLiteral("lyrics"), vcpair( QStringLiteral("lyrics"), 0 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("url"), 0 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("title"), 0 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("weblink"), 0 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("image"), 0 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("description"), 0 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("url"), 0 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("localurl"), 0 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("title"), 0 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("subtitle"), 0 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("description"), 0 ) );
    columns.insert( QStringLiteral("tracks"), vcpair( QStringLiteral("comment"), 0 ) );

    storage->query( QStringLiteral("DROP INDEX url_podchannel ON podcastchannels") );
    storage->query( QStringLiteral("DROP INDEX url_podepisode ON podcastepisodes") );
    storage->query( QStringLiteral("DROP INDEX localurl_podepisode ON podcastepisodes") );
    for( i = columns.constBegin(), iEnd = columns.constEnd(); i != iEnd; ++i )
    {
        storage->query( QStringLiteral("ALTER TABLE ") + i.key() + QStringLiteral(" MODIFY ") + i.value().first + QStringLiteral(" BLOB") );
        storage->query( QStringLiteral("ALTER IGNORE TABLE ") + i.key() + QStringLiteral(" MODIFY ") + i.value().first + QStringLiteral(" TEXT CHARACTER SET utf8 NOT NULL") );
    }
    storage->query( QStringLiteral("CREATE FULLTEXT INDEX url_podchannel ON podcastchannels( url )") );
    storage->query( QStringLiteral("CREATE FULLTEXT INDEX url_podepisode ON podcastepisodes( url )") );
    storage->query( QStringLiteral("CREATE FULLTEXT INDEX localurl_podepisode ON podcastepisodes( localurl )") );
}

static void doUpgradeVersion5to6( SqlStorage *storage )
{
    // ALTER TABLE ENGINE=MyISAM + MODIFY VARCHAR columns
    QStringList tables;
    tables << QStringLiteral("admin") << QStringLiteral("albums") << QStringLiteral("amazon") << QStringLiteral("artists") << QStringLiteral("bookmark_groups") << QStringLiteral("bookmarks");
    tables << QStringLiteral("composers") << QStringLiteral("devices") << QStringLiteral("directories") << QStringLiteral("genres") << QStringLiteral("images") << QStringLiteral("labels") << QStringLiteral("lyrics");
    tables << QStringLiteral("playlist_groups") << QStringLiteral("playlist_tracks") << QStringLiteral("playlists");
    tables << QStringLiteral("podcastchannels") << QStringLiteral("podcastepisodes");
    tables << QStringLiteral("statistics") << QStringLiteral("statistics_permanent") << QStringLiteral("statistics_tag");
    tables << QStringLiteral("tracks") << QStringLiteral("urls") << QStringLiteral("urls_labels") << QStringLiteral("years");

    for( const QString &table : tables )
        storage->query( QStringLiteral("ALTER TABLE ") + table + QStringLiteral(" ENGINE = MyISAM") );

    typedef QPair<QString, int> vcpair;
    QMultiMap<QString, vcpair> columns;
    columns.insert( QStringLiteral("bookmarks"), vcpair( QStringLiteral("url"), 1000 ) );
    columns.insert( QStringLiteral("bookmarks"), vcpair( QStringLiteral("description"), 1000 ) );
    columns.insert( QStringLiteral("directories"), vcpair( QStringLiteral("dir"), 1000 ) );
    columns.insert( QStringLiteral("lyrics"), vcpair( QStringLiteral("url"), 324 ) );
    columns.insert( QStringLiteral("playlist_tracks"), vcpair( QStringLiteral("url"), 1000 ) );
    columns.insert( QStringLiteral("playlists"), vcpair( QStringLiteral("urlid"), 1000 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("guid"), 1000 ) );
    columns.insert( QStringLiteral("statistics_permanent"), vcpair( QStringLiteral("url"), 324 ) );
    columns.insert( QStringLiteral("urls"), vcpair( QStringLiteral("rpath"), 324 ) );
    columns.insert( QStringLiteral("devices"), vcpair( QStringLiteral("servername"), 80 ) );
    columns.insert( QStringLiteral("devices"), vcpair( QStringLiteral("sharename"), 240 ) );
    columns.insert( QStringLiteral("statistics_tag"), vcpair( QStringLiteral("name"), 108 ) );
    columns.insert( QStringLiteral("statistics_tag"), vcpair( QStringLiteral("artist"), 108 ) );
    columns.insert( QStringLiteral("statistics_tag"), vcpair( QStringLiteral("album"), 108 ) );

    QMultiMap<QString, vcpair>::const_iterator i, iEnd;

    for( i = columns.constBegin(), iEnd = columns.constEnd(); i != iEnd; ++i )
        storage->query( QStringLiteral("ALTER IGNORE TABLE ") + i.key() + QStringLiteral(" MODIFY ") + i.value().first + QStringLiteral(" VARCHAR(") + QString::number( i.value().second ) + QStringLiteral(") ") );
}

static void doUpgradeVersion6to7( SqlStorage *storage )
{
    typedef QPair<QString, int> vcpair;
    QMultiMap<QString, vcpair> columns;
    columns.insert( QStringLiteral("directories"), vcpair( QStringLiteral("dir"), 1000 ) );
    columns.insert( QStringLiteral("urls"), vcpair( QStringLiteral("rpath"), 324 ) );
    columns.insert( QStringLiteral("statistics_permanent"), vcpair( QStringLiteral("url"), 324 ) );

    QMultiMap<QString, vcpair>::const_iterator i, iEnd;

    for( i = columns.constBegin(), iEnd = columns.constEnd(); i != iEnd; ++i )
    {
        storage->query( QStringLiteral("ALTER IGNORE TABLE ") + i.key() + QStringLiteral(" MODIFY ") + i.value().first +
            QStringLiteral(" VARCHAR(") + QString::number( i.value().second ) + QStringLiteral(") COLLATE utf8_bin NOT NULL") );
    }
}

static void doUpgradeVersion9to10( SqlStorage *storage )
{
    // MySQL charset/engine conversions + BLOB/TEXT + FULLTEXT

    // first block: ALTER DATABASE + ALTER TABLE charset + MODIFY columns
    storage->query( QStringLiteral("ALTER DATABASE ") + storage->databaseName() + QStringLiteral(" DEFAULT CHARACTER SET utf8 DEFAULT COLLATE utf8_bin") );

    QStringList tables;
    tables << QStringLiteral("admin") << QStringLiteral("albums") << QStringLiteral("amazon") << QStringLiteral("artists") << QStringLiteral("bookmark_groups") << QStringLiteral("bookmarks");
    tables << QStringLiteral("composers") << QStringLiteral("devices") << QStringLiteral("directories") << QStringLiteral("genres") << QStringLiteral("images") << QStringLiteral("labels") << QStringLiteral("lyrics");
    tables << QStringLiteral("playlist_groups") << QStringLiteral("playlist_tracks") << QStringLiteral("playlists");
    tables << QStringLiteral("podcastchannels") << QStringLiteral("podcastepisodes");
    tables << QStringLiteral("statistics") << QStringLiteral("statistics_permanent") << QStringLiteral("statistics_tag");
    tables << QStringLiteral("tracks") << QStringLiteral("urls") << QStringLiteral("urls_labels") << QStringLiteral("years");

    for( const QString &table : tables )
        storage->query( QStringLiteral("ALTER TABLE ") + table + QStringLiteral(" DEFAULT CHARACTER SET utf8 DEFAULT COLLATE utf8_bin COLLATE utf8_bin ENGINE = MyISAM") );

    // varchar columns
    typedef QPair<QString, int> vcpair;
    QMultiMap<QString, vcpair> columns;
    columns.insert( QStringLiteral("admin"), vcpair( QStringLiteral("component"), 255 ) );
    columns.insert( QStringLiteral("albums"), vcpair( QStringLiteral("name"), TEXT_COLUMN_LENGTH ) );
    columns.insert( QStringLiteral("amazon"), vcpair( QStringLiteral("asin"), 20 ) );
    columns.insert( QStringLiteral("amazon"), vcpair( QStringLiteral("locale"), 2 ) );
    columns.insert( QStringLiteral("amazon"), vcpair( QStringLiteral("filename"), 33 ) );
    columns.insert( QStringLiteral("artists"), vcpair( QStringLiteral("name"), TEXT_COLUMN_LENGTH ) );
    columns.insert( QStringLiteral("bookmark_groups"), vcpair( QStringLiteral("name"), 255 ) );
    columns.insert( QStringLiteral("bookmark_groups"), vcpair( QStringLiteral("description"), 255 ) );
    columns.insert( QStringLiteral("bookmark_groups"), vcpair( QStringLiteral("custom"), 255 ) );
    columns.insert( QStringLiteral("bookmarks"), vcpair( QStringLiteral("name"), 255 ) );
    columns.insert( QStringLiteral("bookmarks"), vcpair( QStringLiteral("url"), 1000 ) );
    columns.insert( QStringLiteral("bookmarks"), vcpair( QStringLiteral("description"), 1000 ) );
    columns.insert( QStringLiteral("bookmarks"), vcpair( QStringLiteral("custom"), 255 ) );
    columns.insert( QStringLiteral("composers"), vcpair( QStringLiteral("name"), TEXT_COLUMN_LENGTH ) );
    columns.insert( QStringLiteral("devices"), vcpair( QStringLiteral("type"), 255 ) );
    columns.insert( QStringLiteral("devices"), vcpair( QStringLiteral("label"), 255 ) );
    columns.insert( QStringLiteral("devices"), vcpair( QStringLiteral("lastmountpoint"), 255 ) );
    columns.insert( QStringLiteral("devices"), vcpair( QStringLiteral("uuid"), 255 ) );
    columns.insert( QStringLiteral("devices"), vcpair( QStringLiteral("servername"), 80 ) );
    columns.insert( QStringLiteral("devices"), vcpair( QStringLiteral("sharename"), 240 ) );
    columns.insert( QStringLiteral("directories"), vcpair( QStringLiteral("dir"), 1000 ) );
    columns.insert( QStringLiteral("genres"), vcpair( QStringLiteral("name"), TEXT_COLUMN_LENGTH ) );
    columns.insert( QStringLiteral("images"), vcpair( QStringLiteral("path"), 255 ) );
    columns.insert( QStringLiteral("labels"), vcpair( QStringLiteral("label"), TEXT_COLUMN_LENGTH ) );
    columns.insert( QStringLiteral("lyrics"), vcpair( QStringLiteral("url"), 324 ) );
    columns.insert( QStringLiteral("playlist_groups"), vcpair( QStringLiteral("name"), 255 ) );
    columns.insert( QStringLiteral("playlist_groups"), vcpair( QStringLiteral("description"), 255 ) );
    columns.insert( QStringLiteral("playlist_tracks"), vcpair( QStringLiteral("url"), 1000 ) );
    columns.insert( QStringLiteral("playlist_tracks"), vcpair( QStringLiteral("title"), 255 ) );
    columns.insert( QStringLiteral("playlist_tracks"), vcpair( QStringLiteral("album"), 255 ) );
    columns.insert( QStringLiteral("playlist_tracks"), vcpair( QStringLiteral("artist"), 255 ) );
    columns.insert( QStringLiteral("playlist_tracks"), vcpair( QStringLiteral("uniqueid"), 128 ) );
    columns.insert( QStringLiteral("playlists"), vcpair( QStringLiteral("name"), 255 ) );
    columns.insert( QStringLiteral("playlists"), vcpair( QStringLiteral("description"), 255 ) );
    columns.insert( QStringLiteral("playlists"), vcpair( QStringLiteral("urlid"), 1000 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("copyright"), 255 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("directory"), 255 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("labels"), 255 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("subscribedate"), 255 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("guid"), 1000 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("mimetype"), 255 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("pubdate"), 255 ) );
    columns.insert( QStringLiteral("statistics_permanent"), vcpair( QStringLiteral("url"), 324 ) );
    columns.insert( QStringLiteral("statistics_tag"), vcpair( QStringLiteral("name"), 108 ) );
    columns.insert( QStringLiteral("statistics_tag"), vcpair( QStringLiteral("artist"), 108 ) );
    columns.insert( QStringLiteral("statistics_tag"), vcpair( QStringLiteral("album"), 108 ) );
    columns.insert( QStringLiteral("tracks"), vcpair( QStringLiteral("title"), TEXT_COLUMN_LENGTH ) );
    columns.insert( QStringLiteral("urls"), vcpair( QStringLiteral("rpath"), 324 ) );
    columns.insert( QStringLiteral("urls"), vcpair( QStringLiteral("uniqueid"), 128 ) );
    columns.insert( QStringLiteral("years"), vcpair( QStringLiteral("name"), TEXT_COLUMN_LENGTH ) );

    QMultiMap<QString, vcpair>::const_iterator i, iEnd;

    for( i = columns.constBegin(), iEnd = columns.constEnd(); i != iEnd; ++i )
    {
        storage->query( QStringLiteral("ALTER TABLE ") + i.key() + QStringLiteral(" MODIFY ") + i.value().first + QStringLiteral(" VARBINARY(") + QString::number( i.value().second ) + QLatin1Char(')') );
        storage->query( QStringLiteral("ALTER IGNORE TABLE ") + i.key() + QStringLiteral(" MODIFY ") + i.value().first +
            QStringLiteral(" VARCHAR(") + QString::number( i.value().second ) + QStringLiteral(") CHARACTER SET utf8 COLLATE utf8_bin NOT NULL") );
    }

    // text fields
    columns.clear();
    columns.insert( QStringLiteral("lyrics"), vcpair( QStringLiteral("lyrics"), 0 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("url"), 0 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("title"), 0 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("weblink"), 0 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("image"), 0 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("description"), 0 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("url"), 0 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("localurl"), 0 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("title"), 0 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("subtitle"), 0 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("description"), 0 ) );
    columns.insert( QStringLiteral("tracks"), vcpair( QStringLiteral("comment"), 0 ) );

    iEnd = columns.constEnd();

    storage->query( QStringLiteral("DROP INDEX url_podchannel ON podcastchannels") );
    storage->query( QStringLiteral("DROP INDEX url_podepisode ON podcastepisodes") );
    storage->query( QStringLiteral("DROP INDEX localurl_podepisode ON podcastepisodes") );
    for( i = columns.constBegin(); i != iEnd; ++i )
    {
        storage->query( QStringLiteral("ALTER TABLE ") + i.key() + QStringLiteral(" MODIFY ") + i.value().first + QStringLiteral(" BLOB") );
        storage->query( QStringLiteral("ALTER IGNORE TABLE ") + i.key() + QStringLiteral(" MODIFY ") + i.value().first + QStringLiteral(" TEXT CHARACTER SET utf8 COLLATE utf8_bin NOT NULL") );
    }
    storage->query( QStringLiteral("CREATE FULLTEXT INDEX url_podchannel ON podcastchannels( url )") );
    storage->query( QStringLiteral("CREATE FULLTEXT INDEX url_podepisode ON podcastepisodes( url )") );
    storage->query( QStringLiteral("CREATE FULLTEXT INDEX localurl_podepisode ON podcastepisodes( localurl )") );
}

static void doUpgradeVersion13to14( SqlStorage *storage )
{
    // Restructure lyrics table
    storage->query( QStringLiteral("ALTER TABLE lyrics CHANGE url rpath VARCHAR(324) CHARACTER SET utf8 COLLATE utf8_bin NULL DEFAULT NULL") );
    storage->query( QStringLiteral("ALTER TABLE lyrics ADD COLUMN url INT NULL DEFAULT NULL FIRST") );
    storage->query( QStringLiteral("UPDATE lyrics l SET l.url = (SELECT u.id FROM urls u WHERE u.rpath = l.rpath LIMIT 1)") );
    storage->query( QStringLiteral("DELETE FROM lyrics WHERE url IS NULL") );
    storage->query( QStringLiteral("ALTER TABLE lyrics MODIFY url INT NOT NULL") );
    storage->query( QStringLiteral("CREATE TEMPORARY TABLE duplicate_lyrics_ids ( id INT NOT NULL ) "
        "ENGINE=MEMORY SELECT dupl.id FROM lyrics orig "
        "LEFT JOIN lyrics dupl ON dupl.url = orig.url AND dupl.id > orig.id") );
    storage->query( QStringLiteral("DELETE FROM lyrics WHERE id IN (SELECT id FROM duplicate_lyrics_ids)") );
    storage->query( QStringLiteral("ALTER TABLE lyrics DROP id, DROP rpath") );
    storage->query( QStringLiteral("ALTER TABLE lyrics ADD PRIMARY KEY(url)") );
}

static void doUpgradeVersion14to15( SqlStorage *storage )
{
    // Fix NULL/NOT NULL constraints on columns that were incorrectly set
    typedef QPair<QString, int> vcpair;
    QMultiMap<QString, vcpair> columns;

    columns.insert( QStringLiteral("admin"), vcpair( QStringLiteral("component"), 255 ) );
    columns.insert( QStringLiteral("devices"), vcpair( QStringLiteral("type"), 255 ) );
    columns.insert( QStringLiteral("devices"), vcpair( QStringLiteral("label"), 255 ) );
    columns.insert( QStringLiteral("devices"), vcpair( QStringLiteral("lastmountpoint"), 255 ) );
    columns.insert( QStringLiteral("devices"), vcpair( QStringLiteral("uuid"), 255 ) );
    columns.insert( QStringLiteral("devices"), vcpair( QStringLiteral("servername"), 80 ) );
    columns.insert( QStringLiteral("devices"), vcpair( QStringLiteral("sharename"), 240 ) );
    columns.insert( QStringLiteral("labels"), vcpair( QStringLiteral("label"), TEXT_COLUMN_LENGTH ) );
    columns.insert( QStringLiteral("lyrics"), vcpair( QStringLiteral("lyrics"), 0 ) );
    columns.insert( QStringLiteral("playlists"), vcpair( QStringLiteral("name"), 255 ) );
    columns.insert( QStringLiteral("playlists"), vcpair( QStringLiteral("description"), 255 ) );
    columns.insert( QStringLiteral("playlists"), vcpair( QStringLiteral("urlid"), 1000 ) );
    columns.insert( QStringLiteral("playlist_groups"), vcpair( QStringLiteral("name"), 255 ) );
    columns.insert( QStringLiteral("playlist_groups"), vcpair( QStringLiteral("description"), 255 ) );
    columns.insert( QStringLiteral("playlist_tracks"), vcpair( QStringLiteral("url"), 1000 ) );
    columns.insert( QStringLiteral("playlist_tracks"), vcpair( QStringLiteral("title"), 255 ) );
    columns.insert( QStringLiteral("playlist_tracks"), vcpair( QStringLiteral("album"), 255 ) );
    columns.insert( QStringLiteral("playlist_tracks"), vcpair( QStringLiteral("artist"), 255 ) );
    columns.insert( QStringLiteral("playlist_tracks"), vcpair( QStringLiteral("uniqueid"), 128 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("url"), 0 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("title"), 0 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("weblink"), 0 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("image"), 0 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("description"), 0 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("copyright"), 255 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("directory"), 255 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("labels"), 255 ) );
    columns.insert( QStringLiteral("podcastchannels"), vcpair( QStringLiteral("subscribedate"), 255 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("url"), 0 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("localurl"), 0 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("guid"), 1000 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("title"), 0 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("subtitle"), 0 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("description"), 0 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("mimetype"), 255 ) );
    columns.insert( QStringLiteral("podcastepisodes"), vcpair( QStringLiteral("pubdate"), 255 ) );
    columns.insert( QStringLiteral("statistics_tag"), vcpair( QStringLiteral("name"), 108 ) );
    columns.insert( QStringLiteral("statistics_tag"), vcpair( QStringLiteral("artist"), 108 ) );
    columns.insert( QStringLiteral("statistics_tag"), vcpair( QStringLiteral("album"), 108 ) );
    columns.insert( QStringLiteral("tracks"), vcpair( QStringLiteral("title"), TEXT_COLUMN_LENGTH ) );
    columns.insert( QStringLiteral("tracks"), vcpair( QStringLiteral("comment"), 0 ) );
    columns.insert( QStringLiteral("urls"), vcpair( QStringLiteral("uniqueid"), 128 ) );

    QMultiMapIterator<QString, vcpair> it( columns );
    while( it.hasNext() )
    {
        it.next();
        QString table = it.key();
        QString column = it.value().first;
        int length = it.value().second;

        QString query;
        if( length > 0 )
            query = QStringLiteral( "ALTER TABLE `%1` CHANGE `%2` `%2` VARCHAR(%3) CHARACTER SET utf8 "
                    "COLLATE utf8_bin NULL DEFAULT NULL" ).arg( table, column ).arg( length );
        else
            query = QStringLiteral( "ALTER TABLE `%1` CHANGE `%2` `%2` TEXT CHARACTER SET utf8 "
                    "COLLATE utf8_bin" ).arg( table, column );
        storage->query( query );
    }
}

static void doUpgradeVersion15to16( SqlStorage *storage )
{
    // Convert to utf8mb4 charset
    storage->query( QStringLiteral("ALTER DATABASE ") + storage->databaseName() + QStringLiteral(" DEFAULT CHARACTER SET utf8mb4 DEFAULT COLLATE utf8mb4_bin") );

    // Recreate devices_rshare with prefix indexes due to key length limits
    storage->query( QStringLiteral("DROP INDEX devices_rshare ON devices") );
    storage->query( QStringLiteral("CREATE INDEX devices_rshare ON devices( servername(60), sharename(180) );") );

    QStringList tables;
    tables << QStringLiteral("admin") << QStringLiteral("albums") << QStringLiteral("amazon") << QStringLiteral("artists") << QStringLiteral("bookmark_groups") << QStringLiteral("bookmarks");
    tables << QStringLiteral("composers") << QStringLiteral("devices") << QStringLiteral("directories") << QStringLiteral("genres") << QStringLiteral("images") << QStringLiteral("labels") << QStringLiteral("lyrics");
    tables << QStringLiteral("playlist_groups") << QStringLiteral("playlist_tracks") << QStringLiteral("playlists");
    tables << QStringLiteral("podcastchannels") << QStringLiteral("podcastepisodes");
    tables << QStringLiteral("statistics") << QStringLiteral("statistics_permanent") << QStringLiteral("statistics_tag");
    tables << QStringLiteral("tracks") << QStringLiteral("urls") << QStringLiteral("urls_labels") << QStringLiteral("years");
    tables << QStringLiteral("jamendo_albums") << QStringLiteral("jamendo_artists") << QStringLiteral("jamendo_genre") << QStringLiteral("jamendo_tracks");
    tables << QStringLiteral("magnatune_albums") << QStringLiteral("magnatune_artists") << QStringLiteral("magnatune_genre") << QStringLiteral("magnatune_moods") << QStringLiteral("magnatune_tracks");
    tables << QStringLiteral("opmldirectory_albums") << QStringLiteral("opmldirectory_artists") << QStringLiteral("opmldirectory_genre") << QStringLiteral("opmldirectory_tracks");

    for( const QString &table : tables )
    {
        storage->query( QStringLiteral("ALTER TABLE ") + table + QStringLiteral(" DEFAULT CHARACTER SET utf8mb4 DEFAULT COLLATE utf8mb4_bin COLLATE utf8mb4_bin ENGINE = MyISAM") );
        storage->query( QStringLiteral("ALTER TABLE ") + table + QStringLiteral(" CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_bin") );
    }

    storage->query( QStringLiteral("ALTER TABLE directories MODIFY changedate BIGINT") );
    storage->query( QStringLiteral("ALTER TABLE tracks MODIFY createdate BIGINT") );
    storage->query( QStringLiteral("ALTER TABLE tracks MODIFY modifydate BIGINT") );
}

static void
mySqlUpgrade( SqlStorage *storage, int fromVersion )
{
    switch( fromVersion )
    {
        case 4:
            debug() << "Running MySQL-specific upgrade 4->5";
            doUpgradeVersion4to5( storage );
            Q_FALLTHROUGH();
        case 5:
            debug() << "Running MySQL-specific upgrade 5->6";
            doUpgradeVersion5to6( storage );
            Q_FALLTHROUGH();
        case 6:
            debug() << "Running MySQL-specific upgrade 6->7";
            doUpgradeVersion6to7( storage );
            Q_FALLTHROUGH();
        case 7:
        case 8:
        case 9:
            debug() << "Running MySQL-specific upgrade 9->10";
            doUpgradeVersion9to10( storage );
            Q_FALLTHROUGH();
        case 10:
        case 11:
        case 12:
        case 13:
            debug() << "Running MySQL-specific upgrade 13->14";
            doUpgradeVersion13to14( storage );
            Q_FALLTHROUGH();
        case 14:
            debug() << "Running MySQL-specific upgrade 14->15";
            doUpgradeVersion14to15( storage );
            Q_FALLTHROUGH();
        case 15:
            debug() << "Running MySQL-specific upgrade 15->16";
            doUpgradeVersion15to16( storage );
            break;
    }
}

// Static initializer: registers this function with DatabaseUpdater at library load time
struct MySqlUpgradeRegistrar
{
    MySqlUpgradeRegistrar()
    {
        DatabaseUpdater::setMySqlUpgradeFunc( mySqlUpgrade );
    }
} s_mySqlUpgradeRegistrar;
