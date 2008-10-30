/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
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

#ifndef LASTFM_WS_REQUEST_MANAGER_H
#define LASTFM_WS_REQUEST_MANAGER_H

#include <lastfm/DllExportMacro.h>
#include <lastfm/ws/WsReply.h> //for your convenience
#include <lastfm/ws/WsRequestParameters.h>
   

class LASTFM_WS_DLLEXPORT WsRequestBuilder
{
    enum RequestMethod
    {
        GET,
        POST
    };

    /** DO NOT MAKE THIS ACCESSIBLE TO OTHER PARTS OF THE APPLICATION 
      * Talk to max if you wanted to */
    static class WsAccessManager* nam;

    RequestMethod request_method;
    WsRequestParameters params;

    /** starts the request, connect to finished() to get the results */
    WsReply* start();

public:
    WsRequestBuilder( const QString& methodName );
    
    WsReply* get() { request_method = GET; return start(); }
    WsReply* post() { request_method = POST; return start(); }

    /** add a parameter to the request */
    WsRequestBuilder& add( const QString& key, const QString& value ) { params.add( key, value ); return *this; }
    /** adds the parameter if @p value is not empty */
    WsRequestBuilder& addIfNotEmpty( const QString& key, const QString& v ) { if (v.size()) params.add( key, v ); return *this; }
  
    /** connects the receiver and slot to the reply's done() signal */
    WsRequestBuilder& connect( QObject* receiver, const char* slot );
};

#endif
