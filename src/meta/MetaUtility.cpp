/*
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "MetaUtility.h"

#include "meta.h"
#include "meta/Capability.h"
#include "meta/EditCapability.h"

#include <QChar>

QVariantMap
Meta::Field::mapFromTrack( const Meta::Track *track )
{
    //note: track does not support bpm, first_played yet
    QVariantMap map;
    if( !track )
        return map;

    if( track->name().isEmpty() )
        map.insert( Meta::Field::TITLE, QVariant( track->prettyName() ) );
    else
        map.insert( Meta::Field::TITLE, QVariant( track->name() ) );
    if( !track->artist()->name().isEmpty() )
    map.insert( Meta::Field::ARTIST, QVariant( track->artist()->name() ) );
    if( !track->album()->name().isEmpty() )
    map.insert( Meta::Field::ALBUM, QVariant( track->album()->name() ) );
    if( track->filesize() )
        map.insert( Meta::Field::FILESIZE, QVariant( track->filesize() ) );
    if( !track->genre()->name().isEmpty() )
    map.insert( Meta::Field::GENRE, QVariant( track->genre()->name() ) );
    if( !track->composer()->name().isEmpty() )
    map.insert( Meta::Field::COMPOSER, QVariant( track->composer()->name() ) );
    if( !track->year()->name().isEmpty() )
    map.insert( Meta::Field::YEAR, QVariant( track->year()->name() ) );
    if( !track->comment().isEmpty() )
        map.insert( Meta::Field::COMMENT, QVariant( track->comment() ) );
    if( track->trackNumber() )
        map.insert( Meta::Field::TRACKNUMBER, QVariant( track->trackNumber() ) );
    if( track->discNumber() )
        map.insert( Meta::Field::DISCNUMBER, QVariant( track->discNumber() ) );
    if( track->bitrate() )
        map.insert( Meta::Field::BITRATE, QVariant( track->bitrate() ) );
    if( track->length() )
        map.insert( Meta::Field::LENGTH, QVariant( track->length() ) );
    if( track->sampleRate() )
        map.insert( Meta::Field::SAMPLERATE, QVariant( track->sampleRate() ) );
    map.insert( Meta::Field::URL, QVariant( track->prettyUrl() ) );
    map.insert( Meta::Field::RATING, QVariant( track->rating() ) );
    map.insert( Meta::Field::SCORE, QVariant( track->score() ) );
    map.insert( Meta::Field::PLAYCOUNT, QVariant( track->playCount() ) );
    map.insert( Meta::Field::LAST_PLAYED, QVariant( track->lastPlayed() ) );
    return map;
}


void
Meta::Field::updateTrack( Meta::Track *track, const QVariantMap &metadata )
{
    if( !track || !track->hasCapabilityInterface( Meta::Capability::Editable ) )
        return;

    Meta::EditCapability *ec = track->as<Meta::EditCapability>();
    if( !ec || !ec->isEditable() )
        return;
    ec->beginMetaDataUpdate();
    QString title = metadata.contains( Meta::Field::TITLE ) ?
                            metadata.value( Meta::Field::TITLE ).toString() : QString();
    ec->setTitle( title );

    ec->endMetaDataUpdate();
}


QString
Meta::msToPrettyTime( int ms )
{
    return Meta::secToPrettyTime( ms / 1000 );
}

QString
Meta::secToPrettyTime( int seconds )
{
    int minutes = ( seconds / 60 ) % 60;
    int hours = seconds / 3600;
    QString s = QChar( ':' );
    s.append( ( seconds % 60 ) < 10 ? QString( "0%1" ).arg( seconds % 60 ) : QString::number( seconds % 60 ) ); //seconds

    if( hours )
    {
        s.prepend( minutes < 10 ? QString( "0%1" ).arg( minutes ) : QString::number( minutes ) );
        s.prepend( ':' );
    }
    else
    {
        s.prepend( QString::number( minutes ) );
        return s;
    }

    //don't zeroPad the last one, as it can be greater than 2 digits
    s.prepend( QString::number( hours ) );

    return s;
}
