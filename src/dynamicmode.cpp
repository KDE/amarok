/***************************************************************************
 * copyright            : (C) 2005 Seb Ruiz <me@sebruiz.net>               *
 * copyright            : (C) 2006 Gábor Lehel <illissius@gmail.com>       *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "DynamicMode"

#include "collectiondb.h" // querybuilder, similar artists
#include "debug.h"
#include "enginecontroller.h" // HACK to get current artist for suggestion retrieval
#include "mountpointmanager.h" // device ids
#include "playlist.h"
#include "playlistbrowser.h"
#include "playlistbrowseritem.h"
#include "playlistselection.h"
#include "playlistwindow.h"

#include "dynamicmode.h"

#include <kapplication.h> // random func

#include <qregexp.h>

/////////////////////////////////////////////////////////////////////////////
///    CLASS DynamicMode
////////////////////////////////////////////////////////////////////////////

DynamicMode::DynamicMode( const QString &name )
    : m_title( name )
    , m_cycle( true )
    , m_upcoming( 20 )
    , m_previous( 5 )
    , m_appendType( RANDOM )
{
}

DynamicMode::~DynamicMode()
{}

void
DynamicMode::deleting()
{
    if( this == Playlist::instance()->dynamicMode() )
        Playlist::instance()->disableDynamicMode();
}

void
DynamicMode::edit()
{
    if( this == Playlist::instance()->dynamicMode() )
        Playlist::instance()->editActiveDynamicMode(); //so the changes get noticed
    else
        ConfigDynamic::editDynamicPlaylist( PlaylistWindow::self(), this );
}

QStringList DynamicMode::items()   const { return m_items; }

QString DynamicMode::title()       const { return m_title; }
bool DynamicMode::cycleTracks()   const { return m_cycle; }
int  DynamicMode::upcomingCount() const { return m_upcoming; }
int  DynamicMode::previousCount() const { return m_previous; }
int  DynamicMode::appendType()    const { return m_appendType; }

void DynamicMode::setItems( const QStringList &list ) { m_items = list;      }
void DynamicMode::setCycleTracks( bool e )            { m_cycle = e;         }
void DynamicMode::setUpcomingCount( int c )           { m_upcoming = c;      }
void DynamicMode::setPreviousCount( int c )           { m_previous = c;      }
void DynamicMode::setAppendType( int type )           { m_appendType = type; }
void DynamicMode::setTitle( const QString& title )    { m_title = title;     }

void DynamicMode::setDynamicItems( QPtrList<PlaylistBrowserEntry>& newList )
{
DEBUG_BLOCK

    QStringList strListEntries;
    PlaylistBrowserEntry* entry;
    QPtrListIterator<PlaylistBrowserEntry> it( newList );

    while( (entry = it.current()) != 0 )
    {
        ++it;
        strListEntries << entry->name();
    }

    setItems( strListEntries );
    PlaylistBrowser::instance()->saveDynamics();

    rebuildCachedItemSet();
}

void DynamicMode::rebuildCachedItemSet()
{
DEBUG_BLOCK

    m_cachedItemSet.clear();

    if( appendType() == RANDOM || appendType() == SUGGESTION )
    {
        QueryBuilder qb;
        qb.setOptions( QueryBuilder::optRandomize | QueryBuilder::optRemoveDuplicates );
        qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );

        if( appendType() == SUGGESTION )
        {
            // TODO some clever stuff here for spanning across artists
            QString artist = EngineController::instance()->bundle().artist();

            debug() << "seeding from: " << artist << endl;

            QStringList suggestions = CollectionDB::instance()->similarArtists( artist, 16 );
            // for this artist, choose 4 suggested artists at random, to get further suggestions from
            for( uint suggestCount = 0; suggestCount < 4; ++suggestCount )
            {
                if( suggestions.isEmpty() )
                    break;

                QString chosen = suggestions[ KApplication::random() % suggestions.count() ];

                debug() << "found similar artist: " << chosen << endl;

                QStringList newSuggestions = CollectionDB::instance()->similarArtists( chosen, 10 );
                QStringList newChosen;
                for( uint c = 0; c < 4; ++c ) // choose another 4 artists
                {
                    if( newSuggestions.isEmpty() )
                        break;

                    QString s = newSuggestions[ KApplication::random() % newSuggestions.count() ];

                    debug() << "found extended similar artist: " << s << endl;
                    newChosen += s;
                    newSuggestions.remove( s );
                }
                if ( !newChosen.isEmpty() )
                    qb.addMatches( QueryBuilder::tabArtist, newChosen );
                suggestions.remove( chosen );
            }
            qb.addMatches( QueryBuilder::tabArtist, suggestions );
        }

        qb.setLimit( 0, CACHE_SIZE );
        debug() << "Using SQL: " << qb.query() << endl;

        QStringList urls = qb.run();

        foreach( urls ) //we have to run setPath on all raw paths
        {
            KURL current;
            current.setPath( *it );
            m_cachedItemSet += current;
        }
    }

    else
    {
        PlaylistBrowser *pb = PlaylistBrowser::instance();
        QPtrList<PlaylistBrowserEntry> dynamicEntries = pb->dynamicEntries();

        PlaylistBrowserEntry* entry;
        QPtrListIterator<PlaylistBrowserEntry> it( dynamicEntries );

        const int itemsPerSource = CACHE_SIZE / dynamicEntries.count();
        debug() << "each source will return " << itemsPerSource << " entries" << endl;

        while( (entry = it.current()) != 0 )
        {
            ++it;

            if( entry->rtti() == PlaylistEntry::RTTI )
            {
                KURL::List t = tracksFromStaticPlaylist( static_cast<PlaylistEntry*>(entry), itemsPerSource );
                m_cachedItemSet += t;
            }

            else if( entry->rtti() == SmartPlaylist::RTTI )
            {
                KURL::List t = tracksFromSmartPlaylist( static_cast<SmartPlaylist*>(entry), itemsPerSource );
                m_cachedItemSet += t;
            }
        }
    }
}

KURL::List DynamicMode::tracksFromStaticPlaylist( PlaylistEntry *item, uint songCount )
{
DEBUG_BLOCK

    KURL::List trackList = item->tracksURL();
    KURL::List returnList;

    for( uint i=0; i < songCount; )
    {
        if( trackList.isEmpty() )
            break;

        KURL::List::Iterator urlIt = trackList.at( KApplication::random() % trackList.count() );
        if( (*urlIt).isValid() )
        {
            returnList << (*urlIt).path();
            ++i;
        }
        trackList.remove( urlIt );
    }

    debug() << "Returning " << returnList.count() << " tracks from " << item->text(0) << endl;

    return returnList;
}

KURL::List DynamicMode::tracksFromSmartPlaylist( SmartPlaylist *item, uint songCount )
{
DEBUG_BLOCK
    if( !item || !songCount )
        return KURL::List();

    bool useDirect = true;
    QString sql = item->query();

    // FIXME: All this SQL magic out of collectiondb is not a good thing
    // Many smart playlists require a special ordering in order to be effective (eg, last played).
    // We respect this, so if there is no order by statement, we add a random ordering and use the result
    // without further processing
    if ( sql.find( QString("ORDER BY"), false ) == -1 )
    {
        QRegExp limit( "(LIMIT.*)?;$" );
        sql.replace( limit, QString(" ORDER BY %1 LIMIT %2 OFFSET 0;").arg( CollectionDB::instance()->randomFunc() ).arg( songCount ) );
    }
    else
    {
        // we don't want stupid limits such as LIMIT 5 OFFSET 0 which would return the same results always
        uint first=0, limit=0;
        QRegExp limitSearch( "LIMIT.*(\\d+).*OFFSET.*(\\d+)" );
        int findLocation = sql.find( limitSearch, false );
        if( findLocation == -1 ) //not found, let's find out the higher limit the hard way
        {
            QString counting( sql );
            counting.replace( QRegExp( "SELECT.*FROM" ), "SELECT COUNT(*) FROM" );
            // Postgres' grouping rule doesn't like the following clause
            counting.replace( QRegExp( "ORDER BY.*" ), "" );
            QStringList countingResult = CollectionDB::instance()->query( counting );
            limit = countingResult[0].toInt();
        }
        else
        {   // There's a Limit, so we've got to respect it.
            limitSearch.search( sql );
            // capturedTexts() gives us the strings that were matched by each subexpression
            first = limitSearch.capturedTexts()[2].toInt();
            limit = limitSearch.capturedTexts()[1].toInt();
        }
        if ( limit <= songCount )
            // The list is even smaller than the number of songs we want :-(
            songCount = limit;
        else
            // Let's get a random limit, repecting the original one.
            first += KApplication::random() % (limit - songCount);

        if( findLocation == -1 ) // there is no limit
        {
            QRegExp limit( ";$" );
            sql.replace( limit, QString(" LIMIT %1 OFFSET %2;" ).arg( songCount*5 ).arg( first ) );
            useDirect = false;
        }
        else
            sql.replace( limitSearch, QString(" LIMIT %1 OFFSET %2;" ).arg( songCount ).arg( first ) );

    }

    // only return the fields that we need
    sql.replace( QRegExp( "SELECT.*FROM" ), "SELECT tags.url, tags.deviceid FROM" );
    QStringList queryResult = CollectionDB::instance()->query( sql );
    QStringList items;

    debug() << "Smart Playlist: adding urls from query: " << sql << endl;
    if ( !item->query().isEmpty() )
        //We have to filter all the un-needed results from query( sql )
        for( uint x=0; x < queryResult.count() ; x += 2 )
            items << MountPointManager::instance()->getAbsolutePath( queryResult[x+1].toInt(), queryResult[x] );
    else
        items = queryResult;


    KURL::List urls;
    foreach( items ) //we have to run setPath on all raw paths
    {
        KURL tmp;
        tmp.setPath( *it );
        urls << tmp;
    }
    KURL::List addMe;

    // we have to randomly select tracks from the returned query since we can't have
    // ORDER BY RAND() for some statements
    if( !useDirect )
    {
        for( uint i=0; i < songCount && urls.count(); i++ )
        {
            KURL::List::iterator newItem = urls.at( KApplication::random() % urls.count() );
            addMe << (*newItem);
            urls.remove( newItem );
        }
    }

    useDirect ?
            debug() << "Returning " << urls.count()  << " tracks from " << item->text(0) << endl:
            debug() << "Returning " << addMe.count() << " tracks from " << item->text(0) << endl;

    return useDirect ? urls : addMe;
}


KURL::List DynamicMode::retrieveTracks( const uint trackCount )
{
DEBUG_BLOCK
    KURL::List retrieval;

    if( m_cachedItemSet.count() <= trackCount )
        rebuildCachedItemSet();

    for( uint i=0; i < trackCount; i++ )
    {
        if( m_cachedItemSet.isEmpty() )
            break;
        const int pos = KApplication::random() % m_cachedItemSet.count();
        KURL::List::iterator newItem = m_cachedItemSet.at( pos );
        retrieval << (*newItem);
        m_cachedItemSet.remove( newItem );
    }

    return retrieval;
}
