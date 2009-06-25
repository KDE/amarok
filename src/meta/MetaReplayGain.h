/****************************************************************************************
 * Copyright (c) 2009 Alex Merry <alex.merry@kdemail.net>                               *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_METAREPLAYGAIN_H
#define AMAROK_METAREPLAYGAIN_H

#include "amarok_export.h"

#include <QMap>

// Taglib
#include <fileref.h>

/* This file exists (rather than putting the method in MetaUtility.h and
 * MetaUtility.cpp) because we need to share the implementation between
 * amaroklib and amarokcollectionscanner (which doesn't link to amaroklib).
 */
namespace Meta
{
    enum ReplayGainTag
    {
        ReplayGain_Track_Gain,
        ReplayGain_Track_Peak,
        ReplayGain_Album_Gain,
        ReplayGain_Album_Peak
    };

    typedef QMap<ReplayGainTag, qreal> ReplayGainTagMap;

    /**
     * Reads the replay gain tags from a taglib file.
     */
    AMAROK_EXPORT ReplayGainTagMap readReplayGainTags( TagLib::FileRef fileref );
}

#endif // AMAROK_METAREPLAYGAIN_H
