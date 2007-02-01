/***************************************************************************
 * copyright            : (C) 2006 Ian Monroe <ian@monroe.nu>              *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef DAAPCONTENTFETCHER_H
#define DAAPCONTENTFETCHER_H

#include <q3http.h>
//Added by qt3to4:
#include <Q3CString>

class QDataStream;
class QFile;

namespace Daap {

/**
   Inspired by a daapsharp class of the same name. Basically it adds all the silly headers
   that DAAP needs
	@author Ian Monroe <ian@monroe.nu>
*/
class ContentFetcher : public Q3Http
{
    Q_OBJECT

    public:
        ContentFetcher( const QString & hostname, Q_UINT16 port, const QString& password, QObject * parent = 0, const char * name = 0 );
        ~ContentFetcher();

        void getDaap( const QString & command, QIODevice* musicFile = 0 );
        QDataStream& results();

    private slots:
        void checkForErrors( int state );

    signals:
        void httpError( const QString& );

    private:
        QString m_hostname;
        Q_UINT16 m_port;
        Q3CString m_authorize;
        bool m_selfDestruct;
        static int s_requestId; //! Apple needs this for some reason
};



}

#endif
