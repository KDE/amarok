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

#include "playlist.h"
#include "playlistbrowser.h"
#include "playlistselection.h"
#include "playlistwindow.h"

#include "dynamicmode.h"

/////////////////////////////////////////////////////////////////////////////
///    CLASS DynamicMode
////////////////////////////////////////////////////////////////////////////

DynamicMode::DynamicMode( const QString &name )
    : m_title( name )
    , m_cycle( true )
    , m_mark( true )
    , m_upcoming( 20 )
    , m_previous( 5 )
    , m_appendType( RANDOM )
{
}

DynamicMode::~DynamicMode()
{
}

void
DynamicMode::edit()
{
    if( this == Playlist::instance()->dynamicMode() )
        Playlist::instance()->editActiveDynamicMode(); //so the changes get noticed
    else
        ConfigDynamic::editDynamicPlaylist( PlaylistWindow::self(), this );
}

QStringList DynamicMode::items() const { return m_items; }

QString DynamicMode::title() const { return m_title; }
bool  DynamicMode::cycleTracks() const { return m_cycle; }
bool  DynamicMode::markHistory() const { return m_mark; }
int   DynamicMode::upcomingCount() const { return m_upcoming; }
int   DynamicMode::previousCount() const { return m_previous; }
int   DynamicMode::appendType() const { return m_appendType; }

void  DynamicMode::setItems( const QStringList &list ) { m_items = list; }
void  DynamicMode::setCycleTracks( bool e )  { m_cycle = e; }
void  DynamicMode::setMarkHistory( bool e )  { m_mark = e; }
void  DynamicMode::setUpcomingCount( int c ) { m_upcoming = c; }
void  DynamicMode::setPreviousCount( int c ) { m_previous = c; }
void  DynamicMode::setAppendType( int type ) { m_appendType = type; }
void  DynamicMode::setTitle( const QString& title ) { m_title = title; }

void DynamicMode::setDynamicItems(const QPtrList<QListViewItem>& newList)
{
    QStringList strListEntries;
    QListViewItem* entry;
    QPtrListIterator<QListViewItem> it( newList );

    while( (entry = it.current()) != 0 )
    {
        ++it;
        strListEntries << entry->text(0);
    }

    setItems(strListEntries);
    PlaylistBrowser::instance()->saveDynamics();
}

