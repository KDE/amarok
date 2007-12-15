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

#include "RedirectHttp.h"
#include "Settings.h"

#include <QUrl>


RedirectHttp::RedirectHttp( QObject* parent ) :
    QHttp( parent ),
    m_device( 0 ),
    m_to( 0 ),
    m_lastRequest( 0 )
{
    connect( this, SIGNAL( responseHeaderReceived( const QHttpResponseHeader& ) ), SLOT( onHeaderReceived( const QHttpResponseHeader& ) ) );
    connect( this, SIGNAL( requestFinished( int , bool ) ), SLOT( onRequestFinished( int , bool ) ) );
    connect( this, SIGNAL( requestStarted( int ) ), SLOT( onRequestStarted( int ) ) );
}


RedirectHttp::~RedirectHttp()
{
}


int
RedirectHttp::get( const QString& path, QIODevice* to )
{
    m_mode = GET;
    m_data = QByteArray();
    m_to = to;

    m_lastRequest = QHttp::get( path, to );
    return m_lastRequest;
}


int
RedirectHttp::post( const QString& path, QIODevice* data, QIODevice* to )
{
    m_mode = POSTIO;
    m_data = QByteArray();
    m_device = data;
    m_to = to;

    m_lastRequest = QHttp::post( path, data );
    return m_lastRequest;
}


int
RedirectHttp::post( const QString& path, const QByteArray& data, QIODevice* to )
{
    m_mode = POST;
    m_data = data;
    m_to = to;

    m_lastRequest = QHttp::post( path, data );
    return m_lastRequest;
}


int
RedirectHttp::request( const QHttpRequestHeader& header, QIODevice* data, QIODevice* to )
{
    m_mode = REQUESTIO;
    m_data = QByteArray();
    m_device = data;
    m_header = header;
    m_to = to;

    m_lastRequest = QHttp::request( header, data, to );
    return m_lastRequest;
}


int
RedirectHttp::request( const QHttpRequestHeader& header, const QByteArray& data, QIODevice* to )
{
    m_mode = REQUEST;
    m_data = data;
    m_header = header;
    m_to = to;

    m_lastRequest = QHttp::request( header, data, to );
    return m_lastRequest;
}


void
RedirectHttp::onHeaderReceived( const QHttpResponseHeader& resp )
{
    switch( resp.statusCode() )
    {
        case 301:   //Moved Permanently
        case 302:
        case 307:   //Temporary Redirect
        {
            QString redirectUrl = resp.value( "location" );
            qDebug() << "Http request returned redirect (301, 302 or 307): " << redirectUrl;

            blockSignals( true );

            abort();
            close();

            QUrl url( redirectUrl );
            if ( !url.isValid() )
                return;

            setHost( url.host(), url.port() > 0 ? url.port() : 80 );
            int oldId = m_lastRequest;
            int id;

            switch( m_mode )
            {
                case GET:
                    id = get( url.path(), m_to );
                    break;

                case POST:
                    id = post( url.path(), m_data, m_to );
                    break;

                case POSTIO:
                    id = post( url.path(), m_device, m_to );
                    break;

                case REQUEST:
                    m_header.setRequest( "GET", url.path() );
                    m_header.setValue( "Host", url.host() );

                    id = request( m_header, m_data, m_to );
                    break;

                case REQUESTIO:
                    m_header.setRequest( "GET", url.path() );
                    m_header.setValue( "Host", url.host() );

                    id = request( m_header, m_device, m_to );
                    break;
            }

            m_idTrans.insert( id, oldId );

            blockSignals( false );
        }
        break;
    }
}


void
RedirectHttp::onRequestFinished( int id, bool error )
{
    int tId = id;

    if ( m_idTrans.contains( id ) )
    {
        tId = m_idTrans.value( id );
    }

    if ( id != tId )
        emit requestFinished( tId, error );
}


void
RedirectHttp::onRequestStarted( int id )
{
    int tId = id;

    if ( m_idTrans.contains( id ) )
    {
        tId = m_idTrans.value( id );
    }

    if ( id != tId )
        emit requestStarted( tId );
}

