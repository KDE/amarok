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
#ifndef CDDBHELPER_H
#define CDDBHELPER_H

#include "MetaDataHelper.h"

#include <cddb/cddb.h>
/**
 * A wrapper class for fetching cddb info
 */
class CDDBHelper: public MetaDataHelper
{
    public:
        CDDBHelper( CdIo_t *cdio, track_t i_first_track, track_t i_last_track, const QString& encodingPreferences );
        ~CDDBHelper();
        bool isAvailable() const;

        EntityInfo getDiscInfo() const;
        EntityInfo getTrackInfo( track_t trackNum ) const;
        QByteArray getRawDiscTitle() const;

    private:
        cddb_conn_t *m_conn;
        cddb_disc_t *m_cddb_disc;
        bool m_cddbInfoAvaialable;
        track_t m_i_first_track;
        int m_cddb_matches;
};

typedef KSharedPtr<CDDBHelper> CDDBHelperPtr;

#endif