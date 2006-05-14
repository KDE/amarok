/***************************************************************************
 *   Copyright (C) 2006 by Mark Kretschmann <markey@web.de>                *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#include "amarok.h"
#include "amarokconfig.h"

#include <qmap.h>


QString
amaroK::icon( const QString& name ) //declared in amarok.h
{
    // We map our amaroK icon theme names to system icons, instead of using the same
    // naming scheme. This has two advantages:
    // 1. Our icons can have simpler and more meaningful names
    // 2. We can map several of our icons to one system icon, if necessary
    static QMap<QString, QString> iconMap;

    if( iconMap.empty() ) {
        iconMap["back"]             = "player_start";
        iconMap["clock"]            = "history";
        iconMap["collection"]       = "collection";
        iconMap["device"]           = "usbpendrive_unmount";
        iconMap["fastforward"]      = "2rightarrow";
        iconMap["info"]             = "info";
        iconMap["next"]             = "player_end";
        iconMap["pause"]            = "player_pause";
        iconMap["play"]             = "player_play";
        iconMap["playlist"]         = "player_playlist_2";
        iconMap["playlist_clear"]   = "view_remove";
        iconMap["playlist_refresh"] = "rebuild";
        iconMap["random"]           = "random";
        iconMap["redo"]             = "redo";
        iconMap["refresh"]          = "reload";
        iconMap["rescan"]           = "reload";
        iconMap["rewind"]           = "2leftarrow";
        iconMap["stop"]             = "player_stop";
        iconMap["podcast"]          = "sound";
        iconMap["podcast2"]         = "favorites";
        iconMap["undo"]             = "undo";
    }

    if( iconMap.contains( name ) )
    {
        if( AmarokConfig::useCustomIconTheme() || name.contains( "podcast" ) )
            return QString( "amarok_" ) + name;
        else
            return iconMap[name];
    }

    return name;
}


