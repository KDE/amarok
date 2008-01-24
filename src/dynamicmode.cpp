/***************************************************************************
 * copyright            : (C) 2005 Seb Ruiz <me@sebruiz.net>               *
 * copyright            : (C) 2006 Gábor Lehel <illissius@gmail.com>       *
 * copyright            : (C) 2006 Bonne Eggleston <b.eggleston@gmail.com  *
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
#include "statusbar.h"

#include "dynamicmode.h"

#include <kapplication.h> // random func

#include <qregexp.h>
#include <qvaluevector.h>

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
        strListEntries << entry->text(0);
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

            if( artist.isEmpty() )
            {
                PlaylistItem *currentItem = Playlist::instance()->currentItem();
                if( currentItem != 0 )
                    artist = currentItem->artist();
            }

            debug() << "seeding from: " << artist << endl;

            QStringList suggestions = CollectionDB::instance()->similarArtists( artist, 16 );
            // for this artist, choose 4 suggested artists at random, to get further suggestions from
            QStringList newChosen;
            for( uint suggestCount = 0; suggestCount < 4; ++suggestCount )
            {
                if( suggestions.isEmpty() )
                    break;

                QString chosen = suggestions[ KApplication::random() % suggestions.count() ];

                debug() << "found similar artist: " << chosen << endl;

                QStringList newSuggestions = CollectionDB::instance()->similarArtists( chosen, 10 );
                for( uint c = 0; c < 4; ++c ) // choose another 4 artists
                {
                    if( newSuggestions.isEmpty() )
                        break;

                    QString s = newSuggestions[ KApplication::random() % newSuggestions.count() ];

                    debug() << "found extended similar artist: " << s << endl;
                    newChosen += s;
                    newSuggestions.remove( s );
                }
                suggestions.remove( chosen );
            }
            if ( !newChosen.isEmpty() )
                suggestions += newChosen;
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
        if( !dynamicEntries.count() )
        {
            Amarok::StatusBar::instance()->longMessage( i18n( "This dynamic playlist has no sources set." ),
                                                        KDE::StatusBar::Sorry );
            return;
        }
        // Create an array of the sizes of each of the playlists
        QValueVector<int> trackCount(dynamicEntries.count()) ;
        int trackCountTotal = 0;

        for( uint i=0; i < dynamicEntries.count(); i++ ){
          trackCount[i] = 0;

          if ( QListViewItem *item = dynamicEntries.at( i ) ){
            if( item->rtti() == PlaylistEntry::RTTI )
              trackCount[i] = static_cast<PlaylistEntry *>(item)->tracksURL().count();
            else if( item->rtti() == SmartPlaylist::RTTI  )
              trackCount[i] = static_cast<SmartPlaylist *>(item)->length();

            trackCountTotal += trackCount[i];
          }
        }


        PlaylistBrowserEntry* entry;
        QPtrListIterator<PlaylistBrowserEntry> it( dynamicEntries );

        //const int itemsPerSource = CACHE_SIZE / dynamicEntries.count() != 0 ? CACHE_SIZE / dynamicEntries.count() : 1;

        int i = 0;
        while( (entry = it.current()) != 0 )
        {
            ++it;
            //trackCountTotal might be 0
            int itemsForThisSource = trackCountTotal ? CACHE_SIZE * trackCount[i] / trackCountTotal : 1;
            if (itemsForThisSource == 0)
              itemsForThisSource = 1; 
            debug() << "this source will return " << itemsForThisSource << " entries" << endl;

            if( entry->rtti() == PlaylistEntry::RTTI )
            {
                KURL::List t = tracksFromStaticPlaylist( static_cast<PlaylistEntry*>(entry), itemsForThisSource);
                m_cachedItemSet += t;
            }

            else if( entry->rtti() == SmartPlaylist::RTTI )
            {
                KURL::List t = tracksFromSmartPlaylist( static_cast<SmartPlaylist*>(entry), itemsForThisSource);
                m_cachedItemSet += t;
            }
            i++;
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
    const bool hasTimeOrder = item->isTimeOrdered();
    debug() << "The smart playlist: " << item->text(0) << ", time order? " << hasTimeOrder << endl;

    QString sql = item->query();

    // FIXME: All this SQL magic out of collectiondb is not a good thing

    // if there is no ordering, add random ordering
    if ( sql.find( QString("ORDER BY"), false ) == -1 )
    {
        QRegExp limit( "(LIMIT.*)?;$" );
        sql.replace( limit, QString(" ORDER BY %1 LIMIT %2 OFFSET 0;")
                            .arg( CollectionDB::instance()->randomFunc() )
                            .arg( songCount ) );
    }
    else
    {
        uint limit=0, offset=0;

        QRegExp limitSearch( "LIMIT.*(\\d+).*OFFSET.*(\\d+)" );
        int findLocation = limitSearch.search( sql, 0 );
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
        {   // There's a Limit, we have to respect it.
            // capturedTexts() gives us the strings that were matched by each subexpression
            offset = limitSearch.capturedTexts()[2].toInt();
            limit  = limitSearch.capturedTexts()[1].toInt();
        }

        // we must be ordering by some other arbitrary query.
        // we can scrap it, since it won't affect our result
        if( !hasTimeOrder )
        {
            // We can mess with the limits if the smart playlist is not orderd by a time criteria
            // Why? We can have a smart playlist which is ordered by name or by some other quality which
            // is meaningless in dynamic mode
            QRegExp orderLimit( "(ORDER BY.*)?;$" );

            sql.replace( orderLimit, QString(" ORDER BY %1 LIMIT %2 OFFSET 0;")
                                        .arg( CollectionDB::instance()->randomFunc() )
                                        .arg( songCount ) );
        }
        else // time ordered criteria, only mess with the limits
        {
            debug() << "time based criteria used!" << endl;
            if ( limit <= songCount ) // The list is even smaller than the number of songs we want :-(
                songCount = limit;
            else
                // Let's get a random limit, repecting the original one.
                offset += KApplication::random() % (limit - songCount);

            if( findLocation == -1 ) // there is no limit
            {
                QRegExp queryEnd( ";$" ); // find the end of the query an add a limit
                sql.replace( queryEnd, QString(" LIMIT %1 OFFSET %2;" ).arg( songCount*5 ).arg( offset ) );
                useDirect = false;
            }
            else // there is a limit, so find it and replace it
                sql.replace( limitSearch, QString(" LIMIT %1 OFFSET %2;" ).arg( songCount ).arg( offset ) );
        }
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

    // always rebuild with suggested mode since the artists will be changing
    if( m_cachedItemSet.count() <= trackCount || appendType() == SUGGESTION )
        rebuildCachedItemSet();

    for( uint i=0; i < trackCount; i++ )
    {
        if( m_cachedItemSet.isEmpty() )
            break;
        const int pos = KApplication::random() % m_cachedItemSet.count();
        KURL::List::iterator newItem = m_cachedItemSet.at( pos );
        if( QFile::exists( (*newItem).path() ) )
            retrieval << (*newItem);
        m_cachedItemSet.remove( newItem );
    }

    return retrieval;
}
