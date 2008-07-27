/******************************************************************************
 * Copyright (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#include "MetaTypeExporter.h"

#include "App.h"
#include "EditCapability.h"

#include <QtScript>

TrackMeta::TrackMeta( Meta::TrackPtr track )
{
    setTrack( track );
}

TrackMeta::~TrackMeta()
{
}

int TrackMeta::sampleRate() const
{
    return m_track ? m_track->sampleRate() : 0;
}

int TrackMeta::bitrate() const
{
    return m_track ? m_track->bitrate() : 0;
}

double TrackMeta::score() const
{
    return m_track ? m_track->score() : 0.0;
}

int TrackMeta::rating() const
{
    return m_track ? m_track->rating() : 0;
}

bool TrackMeta::inCollection() const
{
    return m_track ? m_track->inCollection() : false;
}

QString TrackMeta::type() const
{
    return m_track ? m_track->type() : QString();
}

int TrackMeta::length() const
{
    return m_track ? m_track->length() : 0;
}

int TrackMeta::fileSize() const
{
    return m_track ? m_track->filesize() : 0;
}

int TrackMeta::trackNumber() const
{
    return m_track ? m_track->trackNumber() : 0;
}

int TrackMeta::discNumber() const
{
    return m_track ? m_track->discNumber() : 0;
}

int TrackMeta::playCount() const
{
    return m_track ? m_track->playCount() : 0;
}

bool TrackMeta::playable() const
{
    return m_track ? m_track->isPlayable() : false;
}

QString TrackMeta::album() const
{
    return m_track ? m_track->album()->prettyName() : QString();
}

QString TrackMeta::artist() const
{
    return m_track ? m_track->artist()->prettyName() : QString();
}

QString TrackMeta::composer() const
{
    return m_track ? m_track->composer()->prettyName() : QString();
}

QString TrackMeta::genre() const
{
    return m_track ? m_track->genre()->prettyName() : QString();
}

QString TrackMeta::year() const
{
    return m_track ? m_track->year()->prettyName() : QString();
}

QString TrackMeta::comment() const
{
    return m_track ? m_track->comment() : QString();
}

QString TrackMeta::path() const
{
    return m_track ? m_track->playableUrl().path() : QString();
}

bool TrackMeta::isValid() const
{
    if ( m_track ) return true;
    return false;
}
bool TrackMeta::isEditable() const
{
    if( ec )
        return ( ec->isEditable() );
    else
        return false;
}

QString TrackMeta::lyrics() const
{
    return m_track ? m_track->cachedLyrics() : QString();
}

void TrackMeta::setScore( double score )
{
    if ( m_track ) m_track->setScore( score );
}

void TrackMeta::setRating( int rating )
{
    if ( m_track ) m_track->setRating( rating );
}

void TrackMeta::setTrackNumber( int number )
{
    if ( isEditable() ) ec->setTrackNumber( number );
}

void TrackMeta::setDiscNumber( int number )
{
    if ( isEditable() ) ec->setDiscNumber( number );
}

void TrackMeta::setAlbum( QString album )
{
    if ( isEditable() ) ec->setAlbum( album );
}

void TrackMeta::setArtist( QString artist )
{
    if ( isEditable() ) ec->setArtist( artist );
}

void TrackMeta::setComposer( QString composer )
{
    if ( isEditable() ) ec->setComposer( composer );
}

void TrackMeta::setGenre( QString genre )
{
    if ( isEditable() ) ec->setGenre( genre );
}

void TrackMeta::setYear( QString year )
{
    if ( isEditable() ) ec->setYear( year );
}

void TrackMeta::setComment( QString comment )
{
    if ( isEditable() ) ec->setComment( comment );
}

void TrackMeta::setLyrics( QString lyrics )
{
    if ( m_track ) m_track->setCachedLyrics( lyrics );
}

void TrackMeta::setTrack( Meta::TrackPtr track )
{
    m_track = track;
    if ( track )
        ec = m_track->as<Meta::EditCapability>();
}

namespace AmarokScript
{

    MetaTypeExporter::MetaTypeExporter( QScriptEngine* ScriptEngine )
    : QObject( kapp )
    {
        m_scriptEngine = ScriptEngine;
    }

    MetaTypeExporter::~MetaTypeExporter()
    {
    }

    QScriptValue MetaTypeExporter::TrackMeta_toScriptValue( QScriptEngine *engine, TrackMeta* const &in )
    {
        return engine->newQObject( in );
    }

    void MetaTypeExporter::TrackMeta_fromScriptValue( const QScriptValue &value, TrackMeta* &out )
    {
        out = qobject_cast<TrackMeta*>( value.toQObject() );
    }

    void MetaTypeExporter::TrackMeta_Register()
    {
        qScriptRegisterMetaType<TrackMeta*>( m_scriptEngine, TrackMeta_toScriptValue, TrackMeta_fromScriptValue );
    }
}

#include "MetaTypeExporter.moc"
