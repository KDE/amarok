/***************************************************************************
                      titleproxy.h  -  description
                         -------------------
begin                : Nov 20 14:35:18 CEST 2003
copyright            : (C) 2003 by Mark Kretschmann
email                :
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef TITLEPROXY_H
#define TITLEPROXY_H

#include <kextendedsocket.h>
#include <kurl.h>

#include <qcstring.h>
#include <qobject.h>

class QString;

/**
 * @brief: Metadata Streaming.
 */

class TitleProxy : public QObject
{
    Q_OBJECT

    public:
        TitleProxy( KURL url );
        ~TitleProxy();

        KURL proxyUrl();
        
        // ATTRIBUTES ------

    signals:
        void metaData( QString, QString );    
    
    public slots:

    private slots:
        void readRemote();
        void accept();

    private:
        void transmitData( QString data );
        QString extractStr( QString str, QString key );
        
        // ATTRIBUTES ------
        KURL m_urlRemote;
        bool m_initSuccess;
        int m_metaInt;
        int m_byteCount;
                        
        QByteArray m_bufIn;
        QByteArray m_bufOut;
        
        KExtendedSocket m_sockLocal;
        KExtendedSocket m_sockRemote;
        KExtendedSocket *m_pSockServer;
};
#endif
