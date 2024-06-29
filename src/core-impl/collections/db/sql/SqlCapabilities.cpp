/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#define DEBUG_PREFIX "SqlCapabilities"

#include "SqlCapabilities.h"

#include "MetaValues.h"
#include "SqlMeta.h"
#include "core/support/Debug.h"

#include <QFile>
#include <QList>

namespace Capabilities {

OrganiseCapabilityImpl::OrganiseCapabilityImpl( Meta::SqlTrack *track )
    : Capabilities::OrganiseCapability()
    , m_track( track )
{}

OrganiseCapabilityImpl::~OrganiseCapabilityImpl()
{}


void
OrganiseCapabilityImpl::deleteTrack()
{
    if( QFile::remove( m_track->playableUrl().path() ) )
        m_track->remove();
}


TimecodeWriteCapabilityImpl::TimecodeWriteCapabilityImpl( Meta::SqlTrack *track )
    : Capabilities::TimecodeWriteCapability()
    , m_track( track )
{}

TimecodeWriteCapabilityImpl::~TimecodeWriteCapabilityImpl()
{}


TimecodeLoadCapabilityImpl::TimecodeLoadCapabilityImpl( Meta::SqlTrack *track )
    : Capabilities::TimecodeLoadCapability()
    , m_track( track )
{}

TimecodeLoadCapabilityImpl::~TimecodeLoadCapabilityImpl()
{}

bool
TimecodeLoadCapabilityImpl::hasTimecodes()
{
    return ( loadTimecodes().size() > 0 );
}

QList<AmarokSharedPointer<AmarokUrl> >
TimecodeLoadCapabilityImpl::loadTimecodes()
{
    QList<AmarokSharedPointer<AmarokUrl> > list = PlayUrlRunner::bookmarksFromUrl( m_track->playableUrl() );
    return list;
}


FindInSourceCapabilityImpl::FindInSourceCapabilityImpl( Meta::SqlTrack *track )
    : Capabilities::FindInSourceCapability()
    , m_track( track )
{}

FindInSourceCapabilityImpl::~FindInSourceCapabilityImpl()
{}

void
FindInSourceCapabilityImpl::findInSource( QFlags<TargetTag> tag )
{
    DEBUG_BLOCK

    QStringList filters;
    Meta::AlbumPtr album       = m_track->album();
    Meta::ArtistPtr artist     = m_track->artist();
    Meta::ComposerPtr composer = m_track->composer();
    Meta::GenrePtr genre       = m_track->genre();
    Meta::YearPtr year         = m_track->year();
    QString name;

    if( tag.testFlag(Artist) && !(name = artist ? artist->prettyName() : QString()).isEmpty() )
        filters << QStringLiteral( "%1:\"%2\"" ).arg( Meta::shortI18nForField( Meta::valArtist ), name );
    if( tag.testFlag(Album) && !(name = album ? album->prettyName() : QString()).isEmpty() )
        filters << QStringLiteral( "%1:\"%2\"" ).arg( Meta::shortI18nForField( Meta::valAlbum ), name );
    if( tag.testFlag(Composer) && !(name = composer ? composer->prettyName() : QString()).isEmpty() )
        filters << QStringLiteral( "%1:\"%2\"" ).arg( Meta::shortI18nForField( Meta::valComposer ), name );
    if( tag.testFlag(Genre) && !(name = genre ? genre->prettyName() : QString()).isEmpty() )
        filters << QStringLiteral( "%1:\"%2\"" ).arg( Meta::shortI18nForField( Meta::valGenre ), name );
    if( tag.testFlag(Track) && !(name = m_track ? m_track->prettyName() : QString()).isEmpty() )
        filters << QStringLiteral( "%1:\"%2\"" ).arg( Meta::shortI18nForField( Meta::valTitle ), name );
    if( tag.testFlag(Year) && !(name = year ? year->name() : QString()).isEmpty() )
        filters << QStringLiteral( "%1:%2" ).arg( Meta::shortI18nForField( Meta::valYear ), name );

    if( !filters.isEmpty() )
    {
        AmarokUrl url;
        url.setCommand( QStringLiteral("navigate") );
        url.setPath( QStringLiteral("collections") );
        url.setArg( QStringLiteral("filter"), filters.join( QLatin1String(" AND ") ) );

        debug() << "running url: " << url.url();
        url.run();
    }
}

} //namespace Capabilities


