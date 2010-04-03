/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "MemoryMatcher.h"

using namespace Meta;

MemoryMatcher::MemoryMatcher()
    : m_next( 0 )
{
}

MemoryMatcher::~MemoryMatcher()
{
    delete m_next;
}

bool
MemoryMatcher::isLast() const
{
    return !m_next;
}

MemoryMatcher*
MemoryMatcher::next() const
{
    return m_next;
}

void
MemoryMatcher::setNext( MemoryMatcher *next )
{
    delete m_next;
    m_next = next;
}

TrackMatcher::TrackMatcher( TrackPtr track )
    : MemoryMatcher()
    , m_track( track )
{}

TrackList TrackMatcher::match( Collections::MemoryCollection *memColl )
{
    if( !m_track || !memColl )
        return TrackList();
    TrackMap trackMap = memColl->trackMap();
    TrackList result;
    if ( trackMap.contains( m_track->uidUrl()  ) )
        result.append( trackMap.value( m_track->uidUrl() ) );
    return result; //checking for another matcher is not necessary
}

TrackList TrackMatcher::match( const TrackList &tracks )
{
    if( !m_track )
        return TrackList();
    TrackList result;
    QString url = m_track->uidUrl();
    foreach( TrackPtr track, tracks )
        if ( track->uidUrl() == url )
        {
            result.append( track );
            break;
        }
    return result; //checking for another matcher is not necessary
}



ArtistMatcher::ArtistMatcher( ArtistPtr artist )
    : MemoryMatcher()
    , m_artist( artist )
{}

TrackList ArtistMatcher::match( Collections::MemoryCollection *memColl )
{
    if( !m_artist || !memColl )
        return TrackList();
    ArtistMap artistMap = memColl->artistMap();
    if ( artistMap.contains( m_artist->name() ) )
    {
        ArtistPtr artist = artistMap.value( m_artist->name() );
        TrackList matchingTracks = artist->tracks();
        if ( isLast() )
            return matchingTracks;
        else
            return next()->match( matchingTracks );
    }
    else
        return TrackList();
}


TrackList ArtistMatcher::match( const TrackList &tracks )
{
    if( !m_artist )
        return TrackList();
    TrackList matchingTracks;
    QString name = m_artist->name();
    foreach( TrackPtr track, tracks )
        if ( track->artist()->name() == name )
            matchingTracks.append( track );
    if ( isLast() || matchingTracks.count() == 0)
        return matchingTracks;
    else
        return next()->match( matchingTracks );
}



AlbumMatcher::AlbumMatcher( AlbumPtr album )
    : MemoryMatcher()
    , m_album( album )
{}

TrackList AlbumMatcher::match( Collections::MemoryCollection *memColl )
{
    if( !m_album || !memColl )
        return TrackList();
    AlbumMap albumMap = memColl->albumMap();
    if ( albumMap.contains( m_album->name() ) )
    {
        AlbumPtr album = albumMap.value( m_album->name() );
        TrackList matchingTracks = album->tracks();
        if ( isLast() )
            return matchingTracks;
        else
            return next()->match( matchingTracks );
    }
    else
        return TrackList();
}

TrackList AlbumMatcher::match( const TrackList &tracks )
{
    if( !m_album )
        return TrackList();
    TrackList matchingTracks;
    QString name = m_album->name();
    foreach( TrackPtr track, tracks )
        if ( track->album()->name() == name )
            matchingTracks.append( track );
    if ( isLast() || matchingTracks.count() == 0)
        return matchingTracks;
    else
        return next()->match( matchingTracks );
}



GenreMatcher::GenreMatcher( GenrePtr genre )
    : MemoryMatcher()
    , m_genre( genre )
{}

TrackList GenreMatcher::match( Collections::MemoryCollection *memColl )
{
    if( !m_genre || !memColl )
        return TrackList();
    GenreMap genreMap = memColl->genreMap();
    if ( genreMap.contains( m_genre->name() ) )
    {
        GenrePtr genre = genreMap.value( m_genre->name() );
        TrackList matchingTracks = genre->tracks();
        if ( isLast() )
            return matchingTracks;
        else
            return next()->match( matchingTracks );
    }
    else
        return TrackList();
}

TrackList GenreMatcher::match( const TrackList &tracks )
{
    if( !m_genre )
        return TrackList();
    TrackList matchingTracks;
    QString name = m_genre->name();
    foreach( TrackPtr track, tracks )
        if ( track->genre()->name() == name )
            matchingTracks.append( track );
    if ( isLast() || matchingTracks.count() == 0)
        return matchingTracks;
    else
        return next()->match( matchingTracks );
}



ComposerMatcher::ComposerMatcher( ComposerPtr composer )
    : MemoryMatcher()
    , m_composer( composer )
{}

TrackList ComposerMatcher::match( Collections::MemoryCollection *memColl )
{
    if( !m_composer || !memColl )
        return TrackList();
    ComposerMap composerMap = memColl->composerMap();
    if ( composerMap.contains( m_composer->name() ) )
    {
        ComposerPtr composer = composerMap.value( m_composer->name() );
        TrackList matchingTracks = composer->tracks();
        if ( isLast() )
            return matchingTracks;
        else
            return next()->match( matchingTracks );
    }
    else
        return TrackList();
}

TrackList ComposerMatcher::match( const TrackList &tracks )
{
    if( !m_composer )
        return TrackList();
    TrackList matchingTracks;
    QString name = m_composer->name();
    foreach( TrackPtr track, tracks )
        if ( track->composer()->name() == name )
            matchingTracks.append( track );
    if ( isLast() || matchingTracks.count() == 0)
        return matchingTracks;
    else
        return next()->match( matchingTracks );
}



YearMatcher::YearMatcher( YearPtr year )
    : MemoryMatcher()
    , m_year( year )
{}

TrackList YearMatcher::match( Collections::MemoryCollection *memColl )
{
    if( !m_year || !memColl )
        return TrackList();
    YearMap yearMap = memColl->yearMap();
    if ( yearMap.contains( m_year->name() ) )
    {
        YearPtr year = yearMap.value( m_year->name() );
        TrackList matchingTracks = year->tracks();
        if ( isLast() )
            return matchingTracks;
        else
            return next()->match( matchingTracks );
    }
    else
        return TrackList();
}

TrackList YearMatcher::match( const TrackList &tracks )
{
    if( !m_year )
        return TrackList();
    TrackList matchingTracks;
    QString name = m_year->name();
    foreach( TrackPtr track, tracks )
        if ( track->year()->name() == name )
            matchingTracks.append( track );
    if ( isLast() || matchingTracks.count() == 0)
        return matchingTracks;
    else
        return next()->match( matchingTracks );
}

LabelMatcher::LabelMatcher( const Meta::LabelPtr &label )
    : MemoryMatcher()
    , m_label( label )
{
    //nothing to do
}

Meta::TrackList
LabelMatcher::match( const Meta::TrackList &tracks )
{
    if( !m_label )
        return Meta::TrackList();

    Meta::TrackList matchingTracks;
    QString name = m_label->name();
    //not really efficient...
    foreach( const Meta::TrackPtr &track, tracks )
    {
        foreach( const Meta::LabelPtr &label, track->labels() )
        {
            if( name == label->name() )
            {
                matchingTracks << track;
                break;
            }
        }
    }
    if( isLast() || matchingTracks.count() == 0 )
        return matchingTracks;
    else
        return next()->match( matchingTracks );
}

Meta::TrackList
LabelMatcher::match( Collections::MemoryCollection *memColl )
{
    if( !m_label )
        return Meta::TrackList();

    Meta::TrackList matchingTracks;

    if( memColl->labelMap().contains( m_label->name() ) )
    {
        //m_label might actually be a proxy label
        Meta::LabelPtr realLabel = memColl->labelMap().value( m_label->name() );
        matchingTracks = memColl->labelToTrackMap().value( realLabel );
    }
    if( isLast() || matchingTracks.count() == 0 )
        return matchingTracks;
    else
        return next()->match( matchingTracks );
}





