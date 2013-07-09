/****************************************************************************************
 * Copyright (c) 2013 Tatjana Gornak <t.gornak@gmail.com>                               *
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
#ifndef CDTEXTHELPER_H
#define CDTEXTHELPER_H

#include <helpers/MetaDataHelper.h>

class EntityInfo;

/**
 * A wrapper class for fetching CD-Text info from disc
 */
class CDTEXTHelper: public MetaDataHelper
{
    public:
        CDTEXTHelper( CdIo_t *cdio, const QString& encodingPreferences );
        ~CDTEXTHelper();
        bool isAvailable() const;

        EntityInfo getDiscInfo() const ;
        EntityInfo getTrackInfo( track_t trackNum ) const;
        QByteArray getRawDiscTitle() const;

    private:
        cdtext_t *m_cdtext;
};

typedef KSharedPtr<CDTEXTHelper> CDTEXTHelperPtr;

#endif
