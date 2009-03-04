/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd                                       *
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

#include "lastfm/DllExportMacro.h"


/** you need to assign these in your application, they start off 
  * assigned as null strings, if you are using UnicornApplication, don't
  * worry about it. */
namespace Ws
{
    LASTFM_WS_DLLEXPORT extern const char* SessionKey;
    LASTFM_WS_DLLEXPORT extern const char* SharedSecret;
    LASTFM_WS_DLLEXPORT extern const char* ApiKey;
	LASTFM_WS_DLLEXPORT extern const char* Username;
	
	/** if you don't assign this, we create one for you, this is so we can
	  * create pretty logs with your app's usage information */
	LASTFM_WS_DLLEXPORT extern const char* UserAgent;
}
