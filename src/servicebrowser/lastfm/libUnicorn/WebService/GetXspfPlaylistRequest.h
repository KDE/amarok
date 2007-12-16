/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Erik Jalevik, Last.fm Ltd <eriklast.fm>                            *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef GETXSPFPLAYLISTREQUEST_H
#define GETXSPFPLAYLISTREQUEST_H

#include "Request.h"
#include "UnicornDllExportMacro.h"


/** @author <erik@last.fm> */

class UNICORN_DLLEXPORT GetXspfPlaylistRequest : public Request
{
public:
    GetXspfPlaylistRequest( const QString& session,
                            const QString& basePath,
                            const QString& version,
                            bool discovery );

    virtual void start();

protected:
    virtual bool headerReceived( const QHttpResponseHeader& header );

    virtual void success( QByteArray );

private:
    QString m_session;
    QString m_basePath;
    QString m_version;
    bool m_discovery;
};

#endif // GETXSPFPLAYLISTREQUEST_H
