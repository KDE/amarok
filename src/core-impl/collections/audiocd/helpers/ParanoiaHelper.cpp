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

#include "ParanoiaHelper.h"

#include "core/support/Debug.h"
#include "WriteWavHelper.h"

#include <KTemporaryFile>

#include <QFile>
#include <QStringList>

ParanoiaHelper::ParanoiaHelper( const QString &device, Meta::TrackPtr track, const QString& name  )
              : m_device( device )
              , m_track( track )
              , m_name( name )
{
    connect( this, SIGNAL(done(ThreadWeaver::Job*)), this, SLOT(deleteLater()) );
}

cdrom_drive_t*
ParanoiaHelper::openDrive()
{
    DEBUG_BLOCK
    QByteArray devname = m_device.toAscii();
    cdrom_drive *device = cdda_identify( devname.constData(), 0, NULL );

    if (!device)
    {
        warning() << "Cdda identification failed";
        return 0;
    }

    if ( cdda_open( device ) )
    {
        warning() << "Cdda open failed for track";
        cdda_close(device);
        return 0;
    }
    // we do not want messages from paranoia to be printed
    cdda_verbose_set(device, CDDA_MESSAGE_FORGETIT, CDDA_MESSAGE_FORGETIT);
    return device;
}

static void paranoia_cb(long, paranoia_cb_mode_t)
{
}

void
ParanoiaHelper::run()
{
    DEBUG_BLOCK

    QStringList pathItems = m_track->playableUrl().path().split( '/', QString::KeepEmptyParts );
    bool ok = false;
    int trackNum = pathItems.at( 2 ).toInt( &ok );

    cdrom_drive_t* device = openDrive();
    if ( !device )
    {
        emit copyingDone( m_track, "", false );
        return;
    }

    long start = cdda_track_firstsector( device, trackNum );
    long  end = cdda_track_lastsector( device, trackNum );

    debug() << "Start" << start << "end" << end << "for track" << trackNum;
    cdrom_paranoia *paranoia = paranoia_init( device );
    if ( !paranoia )
    {
        error() << "Error in paranoia initialization";
        emit copyingDone( m_track, "", false );
        return;
    }
    paranoia_modeset( paranoia, PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP );
    paranoia_seek( paranoia, start, SEEK_SET );

    WriteWavHelper wav( m_name );
    wav.writeHeader( ( end - start + 1 ) * CDIO_CD_FRAMESIZE_RAW );
    for ( long curpos = start; curpos <= end; ++curpos )
    {
        short int* raw_data = paranoia_read( paranoia, paranoia_cb );
        wav.writeData( (char *)raw_data, CDIO_CD_FRAMESIZE_RAW );
    }

    paranoia_free( paranoia );
    cdda_close( device );
    emit copyingDone( m_track, m_name );
}
#include "ParanoiaHelper.moc"
