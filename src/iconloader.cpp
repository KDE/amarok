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

#include "amarokconfig.h"
#include "iconloader.h"

#include <qmap.h>


QString
amaroK::icon( const QString& name )
{
    // We map our amaroK icon theme names to system icons, instead of using the same
    // naming scheme. This has two advantages:
    // 1. Our icons can have simpler and more meaningful names
    // 2. We can map several of our icons to one system icon, if necessary
    static QMap<QString, QString> iconMap;

    if( iconMap.empty() ) {
        iconMap["time"]             = "history";
        iconMap["clear_playlist"]   = "view_remove";
        iconMap["playlist"]         = "player_playlist_2";
        iconMap["play"]             = "player_play";
        iconMap["stop"]             = "player_stop";
        iconMap["pause"]            = "player_pause";
        iconMap["previous"]         = "player_start";
        iconMap["next"]             = "player_end";
        iconMap["rescan"]           = "reload";
        iconMap["refresh"]          = "reload";
        iconMap["random"]           = "random";
        iconMap["refresh_playlist"] = "rebuild";
    }


    if( AmarokConfig::useCustomIconTheme() )
        return QString( "amarok_" ) + name;

    else if( iconMap.contains( name ) )
        return iconMap[name];

    return name;
}


