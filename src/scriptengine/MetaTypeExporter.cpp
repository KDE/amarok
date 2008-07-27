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

#include <QtScript>

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

    QScriptValue MetaTypeExporter::TrackMeta_toScriptValue(QScriptEngine *engine, const TrackMeta &in)
    {
        QScriptValue obj = engine->newObject();
        obj.setProperty( "isValid", QScriptValue( engine, in.isValid ) );
        obj.setProperty( "sampleRate", QScriptValue( engine, in.sampleRate ) );
        obj.setProperty( "bitrate", QScriptValue( engine, in.bitrate ) );
        obj.setProperty( "score", QScriptValue( engine, in.score ) );
        obj.setProperty( "rating", QScriptValue( engine, in.rating ) );
        obj.setProperty( "inCollection", QScriptValue( engine, in.inCollection ) );
        obj.setProperty( "type", QScriptValue( engine, in.type ) );
        obj.setProperty( "length", QScriptValue( engine, in.length ) );
        obj.setProperty( "fileSize", QScriptValue( engine, in.fileSize ) );
        obj.setProperty( "trackNumber", QScriptValue( engine, in.trackNumber ) );
        obj.setProperty( "discNumber", QScriptValue( engine, in.discNumber ) );
        obj.setProperty( "playCount", QScriptValue( engine, in.playCount ) );
        obj.setProperty( "playable", QScriptValue( engine, in.playable ) );
        obj.setProperty( "album", QScriptValue( engine, in.album ) );
        obj.setProperty( "artist", QScriptValue( engine, in.artist ) );
        obj.setProperty( "composer", QScriptValue( engine, in.composer ) );
        obj.setProperty( "genre", QScriptValue( engine, in.genre ) );
        obj.setProperty( "year", QScriptValue( engine, in.year ) );
        obj.setProperty( "comment", QScriptValue( engine, in.comment ) );
        obj.setProperty( "path", QScriptValue( engine, in.path ) );
        return obj;
    }

    void MetaTypeExporter::TrackMeta_fromScriptValue(const QScriptValue &value, TrackMeta &out)
    {
        Q_UNUSED( value );
        Q_UNUSED( out );
    }

    void MetaTypeExporter::TrackMeta_Register()
    {
        qScriptRegisterMetaType<TrackMeta>( m_scriptEngine, TrackMeta_toScriptValue, TrackMeta_fromScriptValue );
    }
}

#include "MetaTypeExporter.moc"
