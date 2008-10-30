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

#ifndef LASTFM_CORE_URL_H
#define LASTFM_CORE_URL_H

#include <lastfm/DllExportMacro.h>
#include <QString>
#include <QUrl>


struct LASTFM_CORE_DLLEXPORT CoreUrl : public QUrl
{
	CoreUrl( const QUrl& url ) : QUrl( url )
	{}
	
	/** www.last.fm becomes the local version, eg www.lastfm.de */
	CoreUrl localised() const;
	/** www.last.fm becomes m.last.fm, localisation is preserved */
	CoreUrl mobilised() const;

	/** Use this to URL encode any database item (artist, track, album). It
	 * internally calls UrlEncodeSpecialChars to double encode some special
	 * symbols according to the same pattern as that used on the website.
	 *
	 * &, /, ;, +, #
	 *
	 * Use for any urls that go to www.last.fm
	 * Do not use for ws.audioscrobbler.com
	 *
	 * @param[in] str String to encode.
	 */
	static QString encode( QString );
	static QString decode( QString );
	
	/** returns eg. www.lastfm.de */
	static QString localisedHostName( const class CoreLocale& );
};

#endif
