/*
*  Receiver for KIO
*  Copyright (C) 2002 Tim Jansen <tim@tjansen.de>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU Library General Public
* License along with this library; if not, write to the
* Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA.
*/

/**
 * TODO: 
 * - Two pause modes: one flushes the buffer (for live streaming), the 
 *                    other doesnt.
 */

#include "kioreceiver.h"

#include <string.h>
#include <kio/scheduler.h>


void KioReceiver::customEvent( QCustomEvent *e )
{
    if ( ( e->type() == CloseFileEventType ) ||
            ( e->type() == QuitEventType ) ||
            ( e->type() == DoneEventType ) )
    {
        if ( m_job )
        {
            m_lock.lock();
            m_job->suspend();
            KIO::Scheduler::cancelJob( m_job );
            m_job = 0;
            m_wait.wakeAll();
            m_lock.unlock();
        }
        if ( e->type() != CloseFileEventType )
            deleteLater();
        if ( e->type() == QuitEventType )
            KApplication::kApplication() ->exit_loop();
    }
    else if ( e->type() == OpenFileEventType )
    {
        if ( m_job )
            kdDebug() << "Warning: open without close" << endl;
        m_lock.lock();
        m_job = KIO::get( KURL( ( ( OpenFileEvent* ) e ) ->name() ),
                              false,
                              m_progressInfo );
        connect( m_job, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
                 SLOT( slotData( KIO::Job *, const QByteArray & ) ) );
        connect( m_job, SIGNAL( result( KIO::Job* ) ),
                 SLOT( KIO::Job * ) );
        connect( m_job, SIGNAL( canceled( KIO::Job * ) ),
                 SLOT( result( KIO::Job * ) ) );
        connect( m_job,
                 SIGNAL( totalSize( KIO::Job *, KIO::filesize_t ) ),
                 SLOT( slotTotalSize( KIO::Job *, KIO::filesize_t ) ) );
        connect( m_job, SIGNAL( infoMessage( KIO::Job *, const QString & ) ),
                 SLOT( slotInfoMsg( KIO::Job *, const QString & ) ) );
        m_state = ( m_job ) ? RUNNING : ERROR;
        m_jobNum++;
        m_buffer.clear();
        m_bufferOffset = 0;
        m_bufferLen = 0;
        m_curPosition = 0;
        m_fileSize = -1;
        m_lock.unlock();
    }
    else if ( e->type() == PauseEventType )
    {
        if ( m_job )
        {
            m_lock.lock();
            if ( m_state == RUNNING )
            {
                m_job->suspend();
                m_state = SUSPENDED;
            }
            else if ( m_state == SUSPENDED_PREREAD )
                m_state = SUSPENDED;
            m_lock.unlock();
        }
    }
    else if ( e->type() == UnPauseEventType )
    {
        if ( m_job && ( m_state == SUSPENDED ) )
        {
            m_lock.lock();
            if ( !isBufferTooLarge() )
            {
                m_job->resume();
                m_state = RUNNING;
            }
            else
                m_state = SUSPENDED_PREREAD;
            m_lock.unlock();
        }
    }
}

// call only when locked
bool KioReceiver::isBufferTooLarge()
{
    return ( m_bufferLen - m_bufferOffset ) > m_maxPreread;
}

void KioReceiver::slotData( KIO::Job *, const QByteArray &data )
{
    m_lock.lock();
    m_buffer.push_back( data );
    m_bufferLen += data.size();
    if ( isBufferTooLarge() )
    {
        m_job->suspend();
        m_state = SUSPENDED_PREREAD;
    }
    bool wake = ( m_bufferLen - m_bufferOffset ) >= minRead();
    m_lock.unlock();
    if ( wake )
        m_wait.wakeOne();
}

void KioReceiver::slotResult( KIO::Job *job )
{
    m_lock.lock();
    m_state = DONE;
    m_lock.unlock();
    m_wait.wakeOne();
}

void KioReceiver::slotCanceled( KIO::Job *job )
{
    m_lock.lock();
    m_state = ERROR;
    m_lock.unlock();
    m_wait.wakeOne();
}

void KioReceiver::slotInfoMsg( KIO::Job *, const QString & msg )
{
    // TODO: give to src to send a gobject signal?
}

KioReceiver::KioReceiver() :
        m_job( 0 ),
        m_jobNum( 0 ),
        m_progressInfo( false ),
        m_maxPreread( DEFAULT_PREREAD ),
        m_minRead( DEFAULT_MINREAD ),
        m_maxRead( DEFAULT_MAXREAD ),
        m_bufferOffset( 0 ),
        m_bufferLen( 0 )
{}

KioReceiver::~KioReceiver()
{
    if ( m_extraBuffer )
        delete m_extraBuffer;
}

bool KioReceiver::progressInfo()
{
    return m_progressInfo;
}
void KioReceiver::setProgressInfo( bool p )
{
    m_progressInfo = p;
}
int KioReceiver::maxPreread()
{
    return m_maxPreread;
}
void KioReceiver::setMaxPreread( int mp )
{
    m_lock.lock();
    m_maxPreread = mp;
    m_lock.unlock();
}
int KioReceiver::minRead()
{
    return m_minRead;
}
void KioReceiver::setMinRead( int mp )
{
    m_minRead = mp;
}
int KioReceiver::maxRead()
{
    return m_maxRead;
}
void KioReceiver::setMaxRead( int mp )
{
    m_lock.lock();
    m_maxRead = mp;
    m_lock.unlock();
}

// call only when locked
bool KioReceiver::isKioFinished()
{
    return ( m_state == DONE ) || ( m_state == ERROR );
}

// return true if ok, false when aborted or finished
bool KioReceiver::read( void *&ptr, int &size )
{
    m_lock.lock();
    if ( ( !m_job ) ||
            ( ( isKioFinished() ) && ( m_bufferOffset >= m_bufferLen ) ) )
    {
        m_lock.unlock();
        return false;
    }
    int jobNumAtBeginning = m_jobNum;

    cleanBuffers();

    // if size not sufficient, wait...
    while ( ( ( m_bufferLen - m_bufferOffset ) < minRead() ) &&
            !isKioFinished() )
    {
        m_wait.wait( &m_lock );
        m_lock.lock();
        if ( ( m_jobNum != jobNumAtBeginning )
                || !m_job )
        { // file got closed or changed while wait()
            m_lock.unlock();
            return false;
        }
    }

    // enough in the first buffer?
    // or only one buffer and state is DONE|ERROR?
    if ( ( ( m_buffer[ 0 ].size() - m_bufferOffset ) >= minRead() ) ||
            ( isKioFinished() && ( m_buffer.size() == 1 ) ) )
    {
        ptr = m_buffer[ 0 ].data() + m_bufferOffset;
        size = m_buffer[ 0 ].size() - m_bufferOffset;
        if ( size > maxRead() )
            size = maxRead();
        m_bufferOffset += size;
        m_curPosition += size;
        m_lock.unlock();
        return true;
    }

    // get data from several buffers
    int s = 0;
    int i = 0;
    while ( i < m_buffer.size() )
    {
        s += m_buffer[ i++ ].size();
        if ( s >= minRead() )
            break;
    }
    if ( s > maxRead() )
        s = maxRead();

    m_extraBuffer = new QByteArray( s );
    int off = 0;
    for ( int j = 0; j < i; j++ )
    {
        void *src = m_buffer[ j ].data();
        void *dst = m_extraBuffer->data() + off;
        int len = m_buffer[ j ].size();
        if ( j == 0 )
        {
            src = ( void* ) ( ( char* ) src + m_bufferOffset );
            len -= m_bufferOffset;
        }
        if ( ( off + len ) > s )
            len = s - off;
        memcpy( dst, src, len );
        off += len;
        m_bufferOffset += len;
    }

    ptr = m_extraBuffer->data();
    size = s;
    m_curPosition += size;
    m_lock.unlock();
    return true;
}

// call only when locked
bool KioReceiver::cleanBuffers()
{
    while ( ( m_buffer.size() > 0 ) &&
            ( m_buffer[ 0 ].size() <= m_bufferOffset ) )
    {
        m_bufferOffset -= m_buffer[ 0 ].size();
        m_bufferLen -= m_buffer[ 0 ].size();
        m_buffer.pop_front();
    }
    if ( m_extraBuffer )
        delete m_extraBuffer;
}

long long KioReceiver::currentPosition()
{
    if ( m_job )
        return m_curPosition;
    else
        return 0;
}

long long KioReceiver::fileSize()
{
    if ( m_job )
        return m_fileSize;
    else
        return -1;
}

void KioReceiver::slotTotalSize( KIO::Job *, KIO::filesize_t size )
{
    m_fileSize = size;
}


#include "kioreceiver.moc"


