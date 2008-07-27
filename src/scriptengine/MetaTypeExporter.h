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

#ifndef METATYPE_EXPORTER_H
#define METATYPE_EXPORTER_H

#include <QObject>
#include <QtScript>

namespace AmarokScript
{

    struct TrackMeta
    {
        int sampleRate;
        int bitrate;
        double score;
        int rating;
        bool inCollection;
        QString type;
        int length;
        int fileSize;
        int trackNumber;
        int discNumber;
        int playCount;
        int playable;
        QString album;
        QString artist;
        QString composer;
        QString genre;
        QString year;
        QString comment;
        QString path;
        bool isValid;
    };

    class MetaTypeExporter : public QObject
    {
        Q_OBJECT

        public:
            MetaTypeExporter( QScriptEngine* ScriptEngine );
            ~MetaTypeExporter();
        private:
            QScriptEngine*        m_scriptEngine;
            static QScriptValue   TrackMeta_toScriptValue(QScriptEngine *engine, const TrackMeta &in);
            static void           TrackMeta_fromScriptValue(const QScriptValue &value, TrackMeta &out);
        public:
            void                  TrackMeta_Register();
    };
}

Q_DECLARE_METATYPE( AmarokScript::TrackMeta )

#endif
