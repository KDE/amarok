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

#include <QDomDocument>
#include <QStringList>
#include <QVariantMap>

#include <threadweaver/Job.h>

class MusicBrainzXmlParser : public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        enum
        {
            Release     = 1,
            TrackList   = 2,
            Track       = 3
        };

        MusicBrainzXmlParser(QString &doc );
        void run();

        int type();

        QVariantMap grabTrackByLength( const quint64 length );

        QMap< QString, QVariantMap > tracks;
        QMap< QString, QString > artists;
        QMap< QString, QVariantMap > releases;

    private:
        void parseElement( const QDomElement &e );
        void parseChildren( const QDomElement &e );

        QStringList parseReleaseList( const QDomElement &e );
        QString parseRelease( const QDomElement &e );

        int parseReleaseEventList( const QDomElement &e );
        int parseReleaseEvent( const QDomElement &e );

        QStringList parseTrackList( const QDomElement &e );
        QString parseTrack( const QDomElement &e );

        QString parseArtist( const QDomElement &e );

        QDomDocument m_doc;

        int m_type;

        QVariantMap currentTrackOffsets;
};

#endif // MUSICBRAINZXMLPARSER_H
