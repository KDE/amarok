/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#include "BookmarkMetaActions.h"
#include "AmarokUrlHandler.h"
#include "SvgHandler.h"


#include <KIcon>
#include <KLocale>

BookmarkAlbumAction::BookmarkAlbumAction( QObject *parent )
 : GlobalCollectionAlbumAction( i18n( "Bookmark this Album" ), parent )
{
    connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );
    setIcon( KIcon("bookmark") );
    setRenderer( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ) );
    setElementId( "lastfm" );
}

void BookmarkAlbumAction::slotTriggered()
{
    The::amarokUrlHandler()->bookmarkAlbum( album() );
}




BookmarkArtistAction::BookmarkArtistAction( QObject * parent )
    : GlobalCollectionArtistAction( i18n( "Bookmark this Artist" ), parent )
{
    connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );
    setIcon( KIcon("bookmark") );
    setRenderer( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ) );
    setElementId( "lastfm" );
}

void BookmarkArtistAction::slotTriggered()
{
    The::amarokUrlHandler()->bookmarkArtist( artist() );
}

#include "BookmarkMetaActions.moc"


