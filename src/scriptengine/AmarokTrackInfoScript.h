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

#ifndef AMAROK_TRACKINFO_SCRIPT_H
#define AMAROK_TRACKINFO_SCRIPT_H

#include <QObject>
#include <QtScript>

namespace AmarokScript
{

    class AmarokTrackInfoScript : public QObject
    {
        Q_OBJECT

        public:
            AmarokTrackInfoScript( QScriptEngine* ScriptEngine );
            ~AmarokTrackInfoScript();

            Q_PROPERTY( int SampleRate READ sampleRate );
            Q_PROPERTY( int Bitrate READ bitrate );
            Q_PROPERTY( int Rating WRITE setRating READ rating );
            Q_PROPERTY( double Score WRITE setScore READ score );
            Q_PROPERTY( bool InCollection READ inCollection );
            Q_PROPERTY( QString Type READ type );
            Q_PROPERTY( int Length READ length );
            Q_PROPERTY( int FileSize READ fileSize );
            Q_PROPERTY( int TrackNumber READ trackNumber );
            Q_PROPERTY( int DiscNumber READ discNumber );
            Q_PROPERTY( QString Comment READ comment );
            Q_PROPERTY( int PlayCount READ playCount );
            Q_PROPERTY( int Playable READ playable ); //-1 for trackerror
            Q_PROPERTY( QString Album READ album );
            Q_PROPERTY( QString Artist READ artist );
            Q_PROPERTY( QString Composer READ composer );
            Q_PROPERTY( QString Genre READ genre );
            Q_PROPERTY( QString Year READ year );
            Q_PROPERTY( QString Path READ playableUrl );
//TODO: good for now, but we'd prefer some more detail info for album, and maybe artist, composer...

        public slots:

        private:
            int sampleRate() const;
            int bitrate() const;
            int rating() const;
            void setRating( int Rating );
            double score() const;
            void setScore( double Score );
            int inCollection() const;
            QString type() const;
            int length() const;
            int fileSize() const;
            int trackNumber() const;
            int discNumber() const;
            QString comment() const;
            int playCount() const;
            int playable() const;
            QString album() const;
            QString artist() const;
            QString composer() const;
            QString genre() const;
            QString year() const;
            QString playableUrl() const;
    };
}

#endif
