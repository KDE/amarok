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

#ifndef MUSICBRAINZMETA_H
#define MUSICBRAINZMETA_H

#include <QString>

namespace MusicBrainz
{
    static const QString ARTISTID       = QStringLiteral("mb:ArtistID");
    static const QString RELEASEGROUPID = QStringLiteral("mb:ReleaseGroupID");
    static const QString RELEASEID      = QStringLiteral("mb:ReleaseID");
    static const QString RELEASELIST    = QStringLiteral("mb:ReleaseList");
    static const QString TRACKCOUNT     = QStringLiteral("mb:TrackCount");
    static const QString TRACKID        = QStringLiteral("mb:TrackID");
    static const QString TRACKINFO      = QStringLiteral("mb:TrackInfo");

    static const QString MUSICBRAINZ    = QStringLiteral("mb:musicbrainz");
    static const QString MUSICDNS       = QStringLiteral("mb:musicdns");

    static const QString SIMILARITY     = QStringLiteral("mb:similarity");
    static const qreal   MINSIMILARITY  = 0.6;
}

#endif // MUSICBRAINZMETA_H
