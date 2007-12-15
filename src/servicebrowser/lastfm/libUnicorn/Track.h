/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
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

#ifndef TRACK_H
#define TRACK_H

#include "UnicornDllExportMacro.h"


#include "UnicornCommon.h"

#include <QString>
#include <QVariant>

/** 
 * @author <max@last.fm>
 */
class UNICORN_DLLEXPORT Track 
{
    // TODO we have issues with naming and many different classes for this kind of data
    // main naming issue is class name and the track member data string

    PROP_GET_SET( QString, artist, Artist )
    PROP_GET_SET( QString, title, Title )
    PROP_GET_SET( QString, album, Album )
    
public:

    bool
    operator==( const Track& other )
    {   
        return other.m_artist == m_artist && m_title == other.m_title;
    }

    QString
    displayText() const
    {
        //FIXME duplicated in TrackInfo.cpp
        if (m_artist.isEmpty())
            return m_title; //NOTE could be empty too
        else if (m_title.isEmpty())
            return m_artist;
        else 
            return m_artist + " - " + m_title;
    }
    
    QString
    toString() const { return displayText(); }
    
    operator QString() const { return displayText(); }
    operator QVariant() const { return QVariant(displayText()); }
    
    bool
    isEmpty() const 
    {
        return m_title.isEmpty() && m_artist.isEmpty();
    }
};


#endif // TRACK_H
