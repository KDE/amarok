/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
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

#include "logger.h"
#include "Request.h"
#include "WebService.h"
#include "Settings.h"

ReportRebufferingRequest::ReportRebufferingRequest() :
    Request( TypeReportRebuffering, "ReportRebuffering" )
{}


void
ReportRebufferingRequest::start()
{
    setHost( "www.last.fm" );

    QString path = QString( "/log/client/radio/buffer_underrun" ) +
                   QString( "?userid=" ) + userName() +
                   QString( "&hostname=" ) + streamerHost();

    get( path );
}


void
ReportRebufferingRequest::success( QByteArray /* data */ )
{
    // Code will never get here as the service returns a 404.
    // Which it should. Oddly enough.
}
