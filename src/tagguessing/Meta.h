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

#ifndef TAGGUESSINGMETA_H
#define TAGGUESSINGMETA_H

#include <QString>

namespace TagGuessing
{
    static const QString ARTISTID       = "tg:ArtistID";
    static const QString RELEASEGROUPID = "tg:ReleaseGroupID";
    static const QString RELEASEID      = "tg:ReleaseID";
    static const QString RELEASELIST    = "tg:ReleaseList";
    static const QString TRACKCOUNT     = "tg:TrackCount";
    static const QString TRACKID        = "tg:TrackID";
    static const QString TRACKINFO      = "tg:TrackInfo";

    static const QString MUSICBRAINZ    = "tg:musicbrainz";
    static const QString MUSICDNS       = "tg:musicdns";

    static const QString SIMILARITY     = "tg:similarity";
    static const qreal   MINSIMILARITY  = 0.6;
}

#endif // TAGGUESSINGMETA_H
