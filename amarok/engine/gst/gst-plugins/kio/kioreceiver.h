/*
*  Controller for KIO
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
 * Two pause modes: one flushes the buffer (for live streaming), the other
 * doesnt.
 */


#ifndef GST_KIORECEIVER_H
#define GST_KIORECEIVER_H

#include <kio/jobclasses.h>
#include <kapplication.h>
#include <kurl.h>
#include <kdebug.h>
#include <qmutex.h>
#include <qwaitcondition.h>
#include <qvaluelist.h>
#include <qevent.h>

using namespace KIO;

const int DEFAULT_PREREAD = 1024*1024;
const int DEFAULT_MAXREAD = 16384;
const int DEFAULT_MINREAD = 512;

const int QuitEventType = 3376360;
class QuitEvent : public QCustomEvent
{
    public:
        QuitEvent() :
                QCustomEvent( QuitEventType )
        {}
}
;
const int DoneEventType = 3376361;
class DoneEvent : public QCustomEvent
{
    public:
        DoneEvent() :
                QCustomEvent( DoneEventType )
        {}
}
;
const int CloseFileEventType = 3376362;
class CloseFileEvent : public QCustomEvent
{
    public:
        CloseFileEvent() :
                QCustomEvent( CloseFileEventType )
        {}
}
;
const int OpenFileEventType = 3376363;
class OpenFileEvent : public QCustomEvent
{
        QString m_filename;
    public:
        OpenFileEvent( const char *name ) :
                QCustomEvent( OpenFileEventType ),
                m_filename( name )
        {}
        QString name() { return m_filename; }
};
const int PauseEventType = 3376364;
class PauseEvent : public QCustomEvent
{
    public:
        PauseEvent() :
                QCustomEvent( PauseEventType )
        {}
}
;
const int UnPauseEventType = 3376365;
class UnPauseEvent : public QCustomEvent
{
    public:
        UnPauseEvent() :
                QCustomEvent( UnPauseEventType )
        {}
}
;


class KioReceiver : public QObject
{
        Q_OBJECT
    protected:
        TransferJob *m_job;
        int m_jobNum;
        bool m_progressInfo;
        int m_maxPreread;
        int m_minRead, m_maxRead;

        QMutex m_lock;              // protect the following values
        QWaitCondition m_wait;
        enum {  // state, only valid if m_job is set
            RUNNING,            // running normally
            SUSPENDED_PREREAD,  // suspended because buffer too large
            SUSPENDED,          // suspended to pause
            ERROR,
            DONE
    } m_state;
        QValueList<QByteArray> m_buffer; // list of buffers
        int m_bufferOffset;         // unused bytes at the beginning of the buffer list
        int m_bufferLen;            // buffer len in bytes
        QByteArray *m_extraBuffer;  // extra buffer to merge buffers

        int m_curPosition;          // position in the file
        int m_fileSize;             // file size or -1

        void customEvent( QCustomEvent *e );

        bool isBufferTooLarge();
        bool isKioFinished();
        bool cleanBuffers();

    private slots:
        void slotData( KIO::Job *, const QByteArray &data );
        void slotResult( KIO::Job *job );
        void slotCanceled( KIO::Job *job );
        void slotInfoMsg( KIO::Job *, const QString & msg );
        void slotTotalSize( KIO::Job *, KIO::filesize_t size );

    public:
        KioReceiver();
        ~KioReceiver();
        bool progressInfo();
        void setProgressInfo( bool p );
        int maxPreread();
        void setMaxPreread( int mp );
        int maxRead();
        void setMaxRead( int mp );
        int minRead();
        void setMinRead( int mp );

        long long currentPosition();
        long long fileSize();

        bool read( void *&ptr, int &size );
};


#endif  //GST_KIORECEIVER_H

