/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#ifndef MUSICBRAINZXMLPARSER_H
#define MUSICBRAINZXMLPARSER_H

#include "MusicBrainzMetaClasses.h"
#include <QDomDocument>

#include <threadweaver/Job.h>

class MusicBrainzXmlParser : public ThreadWeaver::Job
{
    public:
        MusicBrainzXmlParser(QString &doc );
        void run();

        int type();

        MusicBrainzTrackList tracks;
        MusicBrainzArtistList artists;
        MusicBrainzReleaseList releases;

        enum
        {
            Release=0,
            TrackList=1,
            Track=2
        };

    private:
        void parseElement( const QDomElement &e );
        void parseChildren( const QDomElement &e );

        void parseReleaseList( const QDomElement &e );
        void parseRelease( const QDomElement &e );

        int parseReleaseEventList( const QDomElement &e );
        int parseReleaseEvent( const QDomElement &e );

        QStringList parseTrackList( const QDomElement &e );
        QString parseTrack( const QDomElement &e );

        QString parseArtist( const QDomElement &e );

        QDomDocument m_doc;

        QStringList m_curReleaseList;
        QList< int > m_curOffsetList;

        int m_type;
};

#endif // MUSICBRAINZXMLPARSER_H
