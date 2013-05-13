/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "IpodMetaEditCapability.h"

#include "core/collections/Collection.h"


using namespace IpodMeta;

EditCapability::EditCapability( const KSharedPtr<Track> &track )
    : Meta::TrackEditor()
    , m_track( track )
{
}

EditCapability::~EditCapability()
{
}

void
EditCapability::setAlbum( const QString &newAlbum )
{
    m_track->setAlbum( newAlbum );
}

void
EditCapability::setAlbumArtist( const QString &newAlbumArtist )
{
    m_track->setAlbumArtist( newAlbumArtist );
}

void
EditCapability::setArtist( const QString &newArtist )
{
    m_track->setArtist( newArtist );
}

void
EditCapability::setComposer( const QString &newComposer )
{
    m_track->setComposer( newComposer );
}

void
EditCapability::setGenre( const QString &newGenre )
{
    m_track->setGenre( newGenre );
}

void
EditCapability::setYear( int newYear )
{
    m_track->setYear( newYear );
}

void
EditCapability::setBpm( const qreal newBpm )
{
    m_track->setBpm( newBpm );
}

void
EditCapability::setTitle( const QString &newTitle )
{
    m_track->setTitle( newTitle );
}

void
EditCapability::setComment( const QString &newComment )
{
    m_track->setComment( newComment );
}

void
EditCapability::setTrackNumber( int newTrackNumber )
{
    m_track->setTrackNumber( newTrackNumber );
}

void
EditCapability::setDiscNumber( int newDiscNumber )
{
    m_track->setDiscNumber( newDiscNumber );
}

void
EditCapability::beginMetaDataUpdate()
{
    m_track->beginUpdate();
}

void
EditCapability::endMetaDataUpdate()
{
    m_track->endUpdate();
}

#include "IpodMetaEditCapability.moc"
