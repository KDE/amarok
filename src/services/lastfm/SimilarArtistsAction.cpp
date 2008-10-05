/***************************************************************************
 *   Copyright (c) 2008  Dan Meltzer <hydrogen@notyetimplemented.com       *
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

#include "SimilarArtistsAction.h"
#include "playlist/PlaylistModel.h"
#include "collection/CollectionManager.h"
#include "SvgHandler.h"

#include <KIcon>

SimilarArtistsAction::SimilarArtistsAction( QObject *parent )
    : GlobalCollectionArtistAction( i18n( "Play Similar Artists from Last.fm" ), parent )
{
    connect( this, SIGNAL( triggered( bool ) ), SLOT( slotTriggered() ) );

    setIcon( KIcon("view-services-lastfm-amarok") );
    setRenderer( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ) );
    setElementId( "lastfm" );
}

void SimilarArtistsAction::slotTriggered()
{
    const QString url = "lastfm://artist/" + artist()->prettyName() + "/similarartists";
    Meta::TrackPtr lastfmtrack = CollectionManager::instance()->trackForUrl( KUrl( url ) );
    The::playlistModel()->insertOptioned( lastfmtrack, Playlist::AppendAndPlay );
}

#include "SimilarArtistsAction.moc"
