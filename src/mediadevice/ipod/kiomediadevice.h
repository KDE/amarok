// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 Martin Aumueller <aumuell@reserv.at>
// See COPYING file for licensing information

#ifndef AMAROK_KIOMEDIADEVICE_H
#define AMAROK_KIOMEDIADEVICE_H

#include "mediabrowser.h"

#include <qmutex.h>
#include <qptrlist.h>

#include <kio/job.h>

class KioMediaDevice : public MediaDevice
{
    Q_OBJECT

    public:
                          KioMediaDevice();
        void              init( MediaBrowser* parent );
        virtual           ~KioMediaDevice();

    protected:
        bool              lockDevice(bool tryLock=false ) { if( tryLock ) { return m_mutex.tryLock(); } else { m_mutex.lock(); return true; } }
        void              unlockDevice() { m_mutex.unlock(); }

        virtual void      deleteFile( const KURL &url );
        QMutex m_mutex;

    protected slots:
        virtual void      fileDeleted( KIO::Job *job );
};

#endif
