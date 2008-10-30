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

#ifndef LASTFM_CORE_LOCALE_H
#define LASTFM_CORE_LOCALE_H

#include <lastfm/DllExportMacro.h>
#include <QLocale>


class LASTFM_CORE_DLLEXPORT CoreLocale
{
	QLocale::Language m_language;
	
public:
	/** constructs a CoreLocale that returns true from isNull() */
	CoreLocale( const QLocale& l ) : m_language( l.language() ) {}
	CoreLocale( QLocale::Language l ) : m_language( l ) {}

	/** @returns the equivalent ISO language code as used in HTTP headers */
	QString iso639() const;
	
	/** the two letter langauge codes we use on the side */
	QString code() const;
	
	QLocale qlocale() const { return QLocale( m_language ); }
	
	/** this is the system locale, or the user specified locale, the user can
	  * change this from the official last.fm client */
	static CoreLocale system();
};

#endif
