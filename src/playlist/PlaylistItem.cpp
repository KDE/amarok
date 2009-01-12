/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>
 *                      : (C) 2008 Soren Harward <stharward@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#define DEBUG_PREFIX "Playlist::Item"

#include "PlaylistItem.h"
#include "meta/capabilities/SourceInfoCapability.h"

#include <KRandom>

Playlist::Item::Item( Meta::TrackPtr track )
        : m_track( track ), m_state( NewlyAdded )
{
    m_id = ( static_cast<quint64>( KRandom::random() ) << 32 ) | static_cast<quint64>( KRandom::random() );
}

Playlist::Item::~Item()
{ }


bool Playlist::Item::lessThanAlbum( Item * left, Item * right )
{
    if( !left->track() || !right->track() )
        return true;
    
    if( left->track()->album() &&  left->track()->album() )
        return left->track()->album()->prettyName() < right->track()->album()->prettyName();

    return true;
}

bool Playlist::Item::lessThanAlbumArtist( Item * left, Item * right )
{
    if( !left->track() || !right->track() )
        return true;
    
    if( left->track()->album() &&  left->track()->album() )
        if( left->track()->album()->albumArtist() &&  left->track()->album()->albumArtist() )
            return left->track()->album()->albumArtist()->prettyName() < right->track()->album()->albumArtist()->prettyName();
    
    return true;
}

bool Playlist::Item::lessThanArtist( Item * left, Item * right )
{
    if( !left->track() || !right->track() )
        return true;
    
    if( left->track()->artist() &&  left->track()->artist() )
        return left->track()->artist()->prettyName() < right->track()->artist()->prettyName();

    return true;
}

bool Playlist::Item::lessThanBitrate( Item * left, Item * right )
{
    if( !left->track() || !right->track() )
        return true;

    return left->track()->bitrate() < right->track()->bitrate();
}

bool Playlist::Item::lessThanBpm( Item * left, Item * right )
{
    //FIXME: ...
    return true;
}

bool Playlist::Item::lessThanComment( Item * left, Item * right )
{
    if( !left->track() || !right->track() )
        return true;

    return left->track()->comment() < right->track()->comment();
}

bool Playlist::Item::lessThanComposer( Item * left, Item * right )
{
    if( !left->track() || !right->track() )
        return true;
    
    if( left->track()->composer() &&  left->track()->composer() )
        return left->track()->composer()->prettyName() < right->track()->composer()->prettyName();

    return true;
}

bool Playlist::Item::lessThanDirectory( Item * left, Item * right )
{
    //FIXME: ...
    return true;
}

bool Playlist::Item::lessThanDiscNumber( Item * left, Item * right )
{
    if( !left->track() || !right->track() )
        return true;

    return left->track()->discNumber() < right->track()->discNumber();
}

bool Playlist::Item::lessThanFilename( Item * left, Item * right )
{
    //FIXME: ...
    return true;
}

bool Playlist::Item::lessThanFilesize( Item * left, Item * right )
{
    //FIXME: ...
    return true;
}

bool Playlist::Item::lessThanGenre( Item * left, Item * right )
{
    if( !left->track() || !right->track() )
        return true;
    
    if( left->track()->genre() &&  left->track()->genre() )
        return left->track()->genre()->prettyName() < right->track()->genre()->prettyName();

    return true;
}

bool Playlist::Item::lessThanLastPlayed( Item * left, Item * right )
{
    if( !left->track() || !right->track() )
        return true;

    return left->track()->lastPlayed() < right->track()->lastPlayed();
}

bool Playlist::Item::lessThanLength( Item * left, Item * right )
{
    if( !left->track() || !right->track() )
        return true;

    return left->track()->length() < right->track()->length();
}

bool Playlist::Item::lessThanMood( Item * left, Item * right )
{
    //FIXME: ...
    return true;
}

bool Playlist::Item::lessThanPlayCount( Item * left, Item * right )
{
    if( !left->track() || !right->track() )
        return true;

    return left->track()->playCount() < right->track()->playCount();
}

bool Playlist::Item::lessThanRating( Item * left, Item * right )
{
    if( !left->track() || !right->track() )
        return true;

    return left->track()->rating() < right->track()->rating();
}

bool Playlist::Item::lessThanSampleRate( Item * left, Item * right )
{
    if( !left->track() || !right->track() )
        return true;

    return left->track()->sampleRate() < right->track()->sampleRate();
}

bool Playlist::Item::lessThanScore( Item * left, Item * right )
{
    if( !left->track() || !right->track() )
        return true;

    return left->track()->score() < right->track()->score();
}

bool Playlist::Item::lessThanSource( Item * left, Item * right )
{
    if( !left->track() || !right->track() )
        return true;
    
    QString sourceLeft;
    Meta::SourceInfoCapability *sic = left->track()->as<Meta::SourceInfoCapability>();
    if ( sic )
    {
        sourceLeft = sic->sourceName();
        delete sic;
    }

    QString sourceRight;
    sic = left->track()->as<Meta::SourceInfoCapability>();
    if ( sic )
    {
        sourceRight = sic->sourceName();
        delete sic;
    }

    return sourceLeft < sourceRight;

}

bool Playlist::Item::lessThanTitle( Item * left, Item * right )
{
    if( !left->track() || !right->track() )
        return true;

    return left->track()->prettyName() < right->track()->prettyName();
}

bool Playlist::Item::lessThanTrackNumber( Item * left, Item * right )
{
    if( !left->track() || !right->track() )
        return true;

    return left->track()->trackNumber() < right->track()->trackNumber();
}

bool Playlist::Item::lessThanType( Item * left, Item * right )
{
    //FIXME: ...
    return true;
}

bool Playlist::Item::lessThanYear(Item * left, Item * right)
{
    if( !left->track() || !right->track() )
        return true;
    
    if( left->track()->year() &&  left->track()->year() )
        return left->track()->year()->prettyName() < right->track()->year()->prettyName();

    return true;
}