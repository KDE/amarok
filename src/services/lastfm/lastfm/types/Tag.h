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

#ifndef LASTFM_TAG_H
#define LASTFM_TAG_H

#include <lastfm/DllExportMacro.h>
#include <lastfm/core/WeightedStringList.h>
#include <QString>
#include <QUrl>


class LASTFM_TYPES_DLLEXPORT Tag
{
	QString m_name;
	
public:
	Tag( const QString& name ) : m_name( name )
	{}
	
	operator QString() const { return m_name; }
	
	/** the global tag page at www.last.fm */
	QUrl www() const;
	/** the tag page for user @p user at www.last.fm */
	QUrl www( const class User& user ) const;
	
	class WsReply* search() const;
	static QStringList search( WsReply* );
    
    /** you can in fact pass any *.get*Tags result to this
      * though you must be aware, the weighting may be empty */
    static WeightedStringList list( WsReply* );
};

#endif
