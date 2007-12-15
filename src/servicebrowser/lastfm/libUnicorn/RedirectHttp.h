/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
 *      Jono Cole, Last.fm Ltd <jono@last.fm>                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef REDIRECTHTTP_H
#define REDIRECTHTTP_H

#include "UnicornDllExportMacro.h"

#include <QDebug>
#include <QHttp>
#include <QString>
#include <QHash>

class UNICORN_DLLEXPORT RedirectHttp : public QHttp
{
    Q_OBJECT

    public:

        RedirectHttp( QObject* parent = 0 );
        ~RedirectHttp();

        int get( const QString& path, QIODevice* to = 0 );
        int post( const QString& path, QIODevice* data = 0, QIODevice* to = 0 );
        int post( const QString& path, const QByteArray& data, QIODevice* to = 0 );
        int request( const QHttpRequestHeader& header, QIODevice* data = 0, QIODevice* to = 0 );
        int request( const QHttpRequestHeader& header, const QByteArray& data, QIODevice* to = 0 );

    private slots:
        void onHeaderReceived( const QHttpResponseHeader& resp );

        void onRequestFinished( int id, bool error );
        void onRequestStarted( int id );

    private:

        enum RequestMode
        {
            GET = 0,
            POST,
            POSTIO,
            REQUEST,
            REQUESTIO
        };

        QByteArray m_data;
        QIODevice* m_device;
        QIODevice* m_to;
        QHttpRequestHeader m_header;

        QHash<int,int> m_idTrans;
        int m_mode;
        int m_lastRequest;
};


#endif // RedirectHttp_H
