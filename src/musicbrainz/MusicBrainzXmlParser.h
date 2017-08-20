/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
 * Copyright (c) 2013 Alberto Villa <avilla@FreeBSD.org>                                *
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

#include <ThreadWeaver/Job>

#include <QDomDocument>
#include <QStringList>
#include <QVariantMap>

class MusicBrainzXmlParser : public ThreadWeaver::Job
{
    Q_OBJECT

    public:
        enum {
            TrackList,
            ReleaseGroup
        };

        explicit MusicBrainzXmlParser( const QString &doc );

        void run();

        int type();

        QMap<QString, QVariantMap> tracks;
        QMap<QString, QString> artists;
        QMap<QString, QVariantMap> releases;
        QMap<QString, QVariantMap> releaseGroups;

    private:
        void parseElement( const QDomElement &e );
        void parseChildren( const QDomElement &e );

        QStringList parseRecordingList( const QDomElement &e );
        QString parseRecording( const QDomElement &e );

        QStringList parseReleaseList( const QDomElement &e );
        QString parseRelease( const QDomElement &e );

        QVariantMap parseMediumList( const QDomElement &e );
        QVariantMap parseMedium( const QDomElement &e );
        QVariantMap parseTrackList( const QDomElement &e );
        QVariantMap parseTrack( const QDomElement &e );

        QString parseReleaseGroup( const QDomElement &e );

        QStringList parseArtist( const QDomElement &e );

        QDomDocument m_doc;

        int m_type;

        QVariantMap m_currentTrackInfo;
};

#endif // MUSICBRAINZXMLPARSER_H
