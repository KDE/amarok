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
#include "CDTEXTHelper.h"
#include "core-impl/collections/audiocd/AudioCdCollection.h"

CDTEXTHelper::CDTEXTHelper( CdIo_t *cdio, const QString& encodingPreferences )
          : MetaDataHelper( encodingPreferences )
{
    m_cdtext = cdio_get_cdtext( cdio );
}

CDTEXTHelper::~CDTEXTHelper()
{
}

bool
CDTEXTHelper::isAvailable() const
{
    return 0 != m_cdtext ;
}

EntityInfo
CDTEXTHelper::getDiscInfo() const
{
    return getTrackInfo( ( track_t ) 0 );
}

EntityInfo
CDTEXTHelper::getTrackInfo( track_t trackNum ) const
{
    EntityInfo trackInfo;

    trackInfo.title = encode( cdtext_get_const( m_cdtext, CDTEXT_FIELD_TITLE, trackNum ) );
    trackInfo.artist = encode( cdtext_get_const( m_cdtext, CDTEXT_FIELD_PERFORMER, trackNum ) );
    trackInfo.genre = encode( cdtext_get_const( m_cdtext, CDTEXT_FIELD_GENRE, trackNum ) );

    return EntityInfo( trackInfo );
}

QByteArray
CDTEXTHelper::getRawDiscTitle() const
{
    return QByteArray( cdtext_get_const( m_cdtext, CDTEXT_FIELD_TITLE, ( track_t ) 0 ) );

}
