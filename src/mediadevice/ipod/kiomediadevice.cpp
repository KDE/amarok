// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#define DEBUG_PREFIX "KioMediaDevice"

#include <config.h>
#include <debug.h>
#include "kiomediadevice.h"

#include <kapplication.h>

#include <cstdlib>
#include <unistd.h>


KioMediaDevice::KioMediaDevice()
: MediaDevice()
{
    m_podcastItem = 0;
    m_staleItem = 0;
    m_orphanedItem = 0;
    m_invisibleItem = 0;
    m_playlistItem = 0;
}

void
KioMediaDevice::init( MediaBrowser* parent )
{
    MediaDevice::init( parent );
}

KioMediaDevice::~KioMediaDevice()
{
}

void
KioMediaDevice::fileDeleted( KIO::Job *job )  //SLOT, used in IpodMediaDevice
{
    if(job->error())
    {
        debug() << "file deletion failed: " << job->errorText() << endl;
    }
    m_waitForDeletion = false;
    m_parent->updateStats();
}

void
KioMediaDevice::deleteFile( const KURL &url ) // used in IpodMediaDevice
{
    debug() << "deleting " << url.prettyURL() << endl;
    m_waitForDeletion = true;
    KIO::Job *job = KIO::file_delete( url, false );
    connect( job, SIGNAL( result( KIO::Job * ) ),
            this,  SLOT( fileDeleted( KIO::Job * ) ) );
    do
    {
        kapp->processEvents( 100 );
        if( isCanceled() )
            break;
        usleep( 10000 );
    } while( m_waitForDeletion );

    if(!isTransferring())
        setProgress( progress() + 1 );
}

#include "kiomediadevice.moc"
