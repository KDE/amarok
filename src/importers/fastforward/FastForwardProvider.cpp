/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#include "FastForwardProvider.h"

#include "core/support/Debug.h"
#include "FastForwardTrack.h"
#include "MetaValues.h"

#include <KLocalizedString>

#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

namespace StatSyncing
{

FastForwardProvider::FastForwardProvider( const FastForwardSettings &settings )
    : m_settings( settings )
{
    QSqlDatabase db = QSqlDatabase::addDatabase( m_settings.dbDriver, m_settings.uid );
    db.setDatabaseName( m_settings.dbDriver == "SQLITE"
                        ? m_settings.dbPath : m_settings.dbName );

    if( m_settings.dbDriver != "SQLITE" )
    {
        db.setHostName( m_settings.dbHost );
        db.setUserName( m_settings.dbUser );
        db.setPassword( m_settings.dbPass );
        db.setPort    ( m_settings.dbPort );
    }

    connect( this, SIGNAL(artistsSearch()), SLOT(slotArtistsSearch()),
             Qt::BlockingQueuedConnection );
    connect( this, SIGNAL(artistTracksSearch(QString)),
             SLOT(slotArtistTracksSearch(QString)), Qt::BlockingQueuedConnection );
}

FastForwardProvider::~FastForwardProvider()
{
    QSqlDatabase::removeDatabase( m_settings.uid );
}

QString
FastForwardProvider::id() const
{
    return m_settings.uid;
}

QString
FastForwardProvider::prettyName() const
{
    return m_settings.dbName;
}

QString
FastForwardProvider::description() const
{
    return QString();
}

KIcon
FastForwardProvider::icon() const
{
    return KIcon( "app-amarok" );
}

qint64
FastForwardProvider::reliableTrackMetaData() const
{
    return Meta::valTitle | Meta::valArtist | Meta::valAlbum | Meta::valComposer
            | Meta::valYear | Meta::valTrackNr | Meta::valDiscNr;
}


qint64
FastForwardProvider::writableTrackStatsData() const
{
    //TODO: Write capabilities
    return 0;
}

Provider::Preference
FastForwardProvider::defaultPreference()
{
    return Ask; // ask on first appearance whether to synchronize by default
}

QSet<QString>
FastForwardProvider::artists()
{
    emit artistsSearch();

    QSet<QString> artistSet;
    artistSet.swap( m_artistsResult );

    return artistSet;
}

TrackList
FastForwardProvider::artistTracks( const QString &artistName )
{
    emit artistTracksSearch( artistName );

    TrackList artistTrackList;
    artistTrackList.swap( m_artistTracksResult );

    return artistTrackList;
}

void
FastForwardProvider::slotArtistsSearch()
{
    QSqlDatabase db = QSqlDatabase::database( m_settings.uid );
    if( !db.isOpen() )
    {
        warning() << __PRETTY_FUNCTION__ << "could not open database connection:"
                     << db.lastError().text();
        return;
    }

    QSqlQuery query( "SELECT name FROM artist", db );
    query.setForwardOnly( true ); // a hint for the database engine

    while( query.next() )
        m_artistsResult.insert( query.value( 0 ).toString() );
}

void
FastForwardProvider::slotArtistTracksSearch( const QString &artistName )
{
    QSqlDatabase db = QSqlDatabase::database( m_settings.uid );
    if( !db.isOpen() )
    {
        warning() << __PRETTY_FUNCTION__ << "could not open database connection:"
                     << db.lastError().text();
        return;
    }

    QSqlQuery query( db );
    query.setForwardOnly( true ); // a hint for the database engine

    query.prepare( "SELECT id FROM artist WHERE name = ?" );
    query.addBindValue( artistName );
    query.exec();

    if( !query.next() )
        return;

    query.prepare( "SELECT url FROM tags WHERE artist = ?" );
    query.addBindValue( query.value( 0 ).toInt() );

    while ( query.next() )
    {
        const QString url = query.value( 0 ).toString();
        m_artistTracksResult << TrackPtr( new FastForwardTrack( url, m_settings.uid ) );
    }
}

} // namespace StatSyncing
