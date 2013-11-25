/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef AMAROK_METAVALUES_H
#define AMAROK_METAVALUES_H

#include <QHash>
#include <QVariant>

/* This file exists because we need to share the implementation between
 * amaroklib and amarokcollectionscanner (which doesn't link to amaroklib).
 */
namespace Meta
{
    /** This type can be used when a number of fields need to
     *  be given to some functions.
     */
    typedef QHash<qint64, QVariant> FieldHash;

    // the following constants are used at a number of places,
    // Most importantly the QueryMaker
    // it's also used when reading and writing taglib tags

    // if something is added here: also updsate MetaConstants.cpp

    //track metadata
    static const qint64 valUrl          = 1LL << 0;
    static const qint64 valTitle        = 1LL << 1;
    static const qint64 valArtist       = 1LL << 2;
    static const qint64 valAlbum        = 1LL << 3;
    static const qint64 valGenre        = 1LL << 4;
    static const qint64 valComposer     = 1LL << 5;
    static const qint64 valYear         = 1LL << 6;
    static const qint64 valComment      = 1LL << 7;
    static const qint64 valTrackNr      = 1LL << 8;
    static const qint64 valDiscNr       = 1LL << 9;
    static const qint64 valBpm          = 1LL << 10;
    //track data
    static const qint64 valLength       = 1LL << 11;
    static const qint64 valBitrate      = 1LL << 12;
    static const qint64 valSamplerate   = 1LL << 13;
    static const qint64 valFilesize     = 1LL << 14;
    static const qint64 valFormat       = 1LL << 15; // the file type a numeric value
    static const qint64 valCreateDate   = 1LL << 16;
    //statistics
    static const qint64 valScore        = 1LL << 17; // value 0 to 100
    static const qint64 valRating       = 1LL << 18; // value 0 to 10 (inclusive)
    static const qint64 valFirstPlayed  = 1LL << 19;
    static const qint64 valLastPlayed   = 1LL << 20;
    static const qint64 valPlaycount    = 1LL << 21;
    static const qint64 valUniqueId     = 1LL << 22;
    //replay gain
    static const qint64 valTrackGain    = 1LL << 23;
    static const qint64 valTrackGainPeak= 1LL << 24;
    static const qint64 valAlbumGain    = 1LL << 25;
    static const qint64 valAlbumGainPeak= 1LL << 26;

    static const qint64 valAlbumArtist  = 1LL << 27;
    static const qint64 valLabel        = 1LL << 28;
    static const qint64 valModified     = 1LL << 29;

    // currently only used for reading and writing tags. Not supported for queryMaker
    // TODO: support for queryMaker
    static const qint64 valCompilation  = 1LL << 40;
    static const qint64 valHasCover     = (1LL << 40) + 1;
    static const qint64 valImage        = (1LL << 40) + 2;
    static const qint64 valLyrics       = (1LL << 40) + 3;

    // start for custom numbers
    static const qint64 valCustom       = 1LL << 60;
}

#endif
