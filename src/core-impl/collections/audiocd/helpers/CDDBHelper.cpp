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
#include "CDDBHelper.h"
#include "core/support/Debug.h"
#include "core-impl/collections/audiocd/AudioCdCollection.h"

CDDBHelper::CDDBHelper( CdIo_t *cdio, track_t i_first_track, track_t i_last_track, const QString& encodingPreferences )
          : MetaDataHelper( encodingPreferences )
          , m_conn( 0 )
          , m_cddb_disc( 0 )
          , m_cddbInfoAvaialable( false )
          , m_i_first_track( i_first_track )
          , m_cddb_matches( 0 )
{
    DEBUG_BLOCK

    m_conn =  cddb_new();
    if ( !m_conn )
    {
        error() << "Unable to initialize libcddb";
        return;
    }

    cddb_set_email_address( m_conn, "me@home" );
    cddb_set_server_name( m_conn, "freedb.freedb.org" );
    cddb_set_server_port( m_conn, 8880 );
    cddb_http_disable( m_conn );
    m_cddb_disc = cddb_disc_new();

    if ( !m_cddb_disc )
    {
        error() << "Unable to create CDDB disc structure";
        return;
    }
    for( track_t i = i_first_track; i < i_last_track; i++)
    {
        cddb_track_t *t = cddb_track_new();
        cddb_track_set_frame_offset( t, cdio_get_track_lba( cdio, i ) );
        cddb_disc_add_track( m_cddb_disc, t );
    }

    cddb_disc_set_length( m_cddb_disc, cdio_get_track_lba( cdio, CDIO_CDROM_LEADOUT_TRACK ) / CDIO_CD_FRAMES_PER_SEC );

    if ( !cddb_disc_calc_discid( m_cddb_disc ) )
    {
        error() << "libcddb calc discid failed.";
        return;
    }

    m_cddb_matches = cddb_query( m_conn, m_cddb_disc );
    if ( -1 == m_cddb_matches )
    {
        error() << cddb_error_str( cddb_errno( m_conn ) );
        return;
    }
    else
        debug() << m_cddb_matches << "matches were founds in CDDB database";

    if ( m_cddb_matches > 0 )
    {
        // loads first match
        cddb_read( m_conn, m_cddb_disc );
        m_cddbInfoAvaialable = true;
    }
}

CDDBHelper::~CDDBHelper()
{
    DEBUG_BLOCK

    if ( m_cddb_disc )
        cddb_disc_destroy( m_cddb_disc );
    if ( m_conn )
        cddb_destroy( m_conn );
    if ( isAvailable() )
        libcddb_shutdown();
}

bool
CDDBHelper::isAvailable() const
{
    return m_cddbInfoAvaialable;
}

EntityInfo
CDDBHelper::getDiscInfo() const
{
    DEBUG_BLOCK

    EntityInfo discInfo;

    discInfo.title = encode( cddb_disc_get_title( m_cddb_disc ) );
    discInfo.artist = encode( cddb_disc_get_artist( m_cddb_disc ) );
    discInfo.genre = encode( cddb_disc_get_genre( m_cddb_disc ) );
    discInfo.year = QString( cddb_disc_get_year( m_cddb_disc ) );

    return EntityInfo( discInfo );
}

QByteArray
CDDBHelper::getRawDiscTitle() const
{
    return QByteArray( cddb_disc_get_title( m_cddb_disc ) );
}

EntityInfo
CDDBHelper::getTrackInfo( track_t trackNum ) const
{
    DEBUG_BLOCK

    EntityInfo trackInfo;

    track_t cddbTrackNum = trackNum - m_i_first_track;
    cddb_track_t *track = cddb_disc_get_track( m_cddb_disc, cddbTrackNum );
    if ( track )
    {
        trackInfo.title = encode( cddb_track_get_title( track ) );
        trackInfo.artist = encode( cddb_track_get_artist( track ) );
    }
    return EntityInfo( trackInfo );
}