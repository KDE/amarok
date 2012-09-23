/****************************************************************************************
 * Copyright (c) 2007-2008 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#include "core-impl/meta/stream/Stream.h"
#include "core-impl/meta/stream/Stream_p.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/meta/Meta.h"
#include "core-impl/meta/default/DefaultMetaTypes.h"
#include "core-impl/support/UrlStatisticsStore.h"

#include <Solid/Networking>

#include <QWeakPointer>
#include <QString>

using namespace MetaStream;

Track::Track( const KUrl &url )
    : Meta::Track()
    , d( new Track::Private( this ) )
{
    DEBUG_BLOCK

    d->url = url;
    d->artistPtr = Meta::ArtistPtr( new StreamArtist( d ) );
    d->albumPtr = Meta::AlbumPtr( new StreamAlbum( d ) );
    d->genrePtr = Meta::GenrePtr( new StreamGenre( d ) );
    d->composerPtr = Meta::ComposerPtr( new Meta::DefaultComposer() );
    d->yearPtr = Meta::YearPtr( new Meta::DefaultYear() );

    m_statsStore = new UrlStatisticsStore( this );
}

Track::~Track()
{
    delete d;
}

QString
Track::name() const
{
    if( d->title.isEmpty() )
        return i18n( "Stream (%1)", d->url.url() );
    return d->title;
}

KUrl
Track::playableUrl() const
{
    return d->url;
}

QString
Track::prettyUrl() const
{
    return playableUrl().url();
}

QString
Track::uidUrl() const
{
    return playableUrl().url();
}

bool
Track::isPlayable() const
{
    if( Solid::Networking::status() != Solid::Networking::Connected )
        return false;

    return true;
}

Meta::AlbumPtr
Track::album() const
{
    return d->albumPtr;
}

Meta::ArtistPtr
Track::artist() const
{
    return d->artistPtr;
}

Meta::GenrePtr
Track::genre() const
{
    return d->genrePtr;
}

Meta::ComposerPtr
Track::composer() const
{
    return d->composerPtr;
}

Meta::YearPtr
Track::year() const
{
    return d->yearPtr;
}

qreal
Track::bpm() const
{
    return -1.0;
}

QString
Track::comment() const
{
    return d->comment;
}

int
Track::trackNumber() const
{
    return d->trackNumber;
}

int
Track::discNumber() const
{
    return 0;
}

qint64
Track::length() const
{
    return d->length;
}

int
Track::filesize() const
{
    return 0;
}

int
Track::sampleRate() const
{
    return 0;
}

int
Track::bitrate() const
{
    return 0;
}

QString
Track::type() const
{
    return "stream";
}

Meta::StatisticsPtr
Track::statistics()
{
    return m_statsStore;
}

#include "Stream_p.moc"
