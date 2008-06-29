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

namespace Amarok
{

    class AmarokTrackInfoScript : public QObject
    {
        Q_OBJECT

        public:
            AmarokTrackInfoScript( QScriptEngine* ScriptEngine );
            ~AmarokTrackInfoScript();

            Q_PROPERTY( int SampleRate READ getSampleRate );
            Q_PROPERTY( int Bitrate READ getBitrate );
            Q_PROPERTY( int Rating WRITE setRating READ getRating );
            Q_PROPERTY( double Score WRITE setScore READ getScore );
            Q_PROPERTY( bool InCollection READ getInCollection );
            Q_PROPERTY( QString Type READ getType );
            Q_PROPERTY( int Length READ getLength );
            Q_PROPERTY( int FileSize READ getFileSize );
            Q_PROPERTY( int TrackNumber READ getTrackNumber );
            Q_PROPERTY( int DiscNumber READ getDiscNumber );
            Q_PROPERTY( QString Comment READ getComment );
            Q_PROPERTY( int PlayCount READ getPlayCount );
            Q_PROPERTY( bool Playable READ getPlayable );
            //todo: good for now, but we'd prefer some more detail info for album, and maybe artist, composer...
            Q_PROPERTY( QString Album READ getAlbum );
            Q_PROPERTY( QString Artist READ getArtist );
            Q_PROPERTY( QString Composer READ getComposer );
            Q_PROPERTY( QString Genre READ getGenre );
            Q_PROPERTY( QString Year READ getYear );

/* todo: implement trackinfo
            Q_PROPERTY( KUrl playableUrl READ playableUrl );
            Q_PROPERTY( QString prettyUrl READ prettyUrl );
            Q_PROPERTY( QString url READ url );
            Q_PROPERTY( uint lastPlayed READ lastPlayed );
            Q_PROPERTY( uint firstPlayed READ firstPlayed );
            Q_PROPERTY( Collection collection READ collection );
            Q_PROPERTY( QString lyrics READ cachedLyrics WRITE setCachedLyrics );
*/
        public slots:

        private:
            int getSampleRate();
            int getBitrate();
            int getRating();
            void setRating( int Rating );
            double getScore();
            void setScore( double Score );
            int getInCollection();
            QString getType();
            int getLength();
            int getFileSize();
            int getTrackNumber();
            int getDiscNumber();
            QString getComment();
            int getPlayCount();
            int getPlayable();
            QString getAlbum();
            QString getArtist();
            QString getComposer();
            QString getGenre();
            QString getYear();
    };
}

#endif
