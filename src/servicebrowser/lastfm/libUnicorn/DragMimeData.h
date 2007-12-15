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

#ifndef DRAGMIMEDATA_H
#define DRAGMIMEDATA_H

#include "UnicornDllExportMacro.h"

#include "Track.h"
#include "Station.h"

#include <QMimeData>

/** 
 * @author <max@last.fm>
 */
class UNICORN_DLLEXPORT DragMimeData : public QMimeData
{
public:
    Track track() const;
    QString username() const;
    QString tag() const;
    Station station() const;
    
    bool hasTrack() const { return hasFormat( "item/track" ); }
    bool hasUser() const { return hasFormat( "item/user" ); }
    bool hasTag() const { return hasFormat( "item/tag" ); }
    bool hasStation() const { return hasFormat( "item/station" ); }
    
    UnicornEnums::ItemType itemType() const;
    QString toString() const;
};


#endif // DRAGMIMEDATA_H
