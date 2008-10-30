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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef WS_AUTOPROXY_H
#define WS_AUTOPROXY_H

#ifdef WIN32
#include <windows.h>
#include <winhttp.h>
#endif


/** @brief simple wrapper to do per url automatic proxy detection 
  * @author <doug@last.fm>
  */
class WsAutoProxy
{
#ifdef WIN32
	HINTERNET m_hSession;
#endif

public:
	WsAutoProxy();
	~WsAutoProxy();

	bool getProxyFor( const class QString &url, 
                      const class QByteArray &userAgent, 
                      class QNetworkProxy &out,
                      const QString &pacUrl);
};

#endif 