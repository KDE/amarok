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

#include <qhttp.h>

class QDataStream;

namespace Daap {

/**
   Inspired by a daapsharp class of the same name. Basically it interfaces with the 
	@author Ian Monroe <ian@monroe.nu>
*/
class ContentFetcher : public QHttp
{
    Q_OBJECT

    public:
        ContentFetcher( const QString & hostname, Q_UINT16 port = 80, QObject * parent = 0, const char * name = 0 );
        ~ContentFetcher();

        void getDaap( const QString & path );

    private:
        QString m_hostname;
        static int s_requestId; //! Apple needs this for some reason
};



}

#endif
