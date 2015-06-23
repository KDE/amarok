/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "PlaylistViewUrlGenerator.h"

#include "core/support/Debug.h"
#include "layouts/LayoutManager.h"
#include "MainWindow.h"
#include "PlaylistDock.h"
#include "PlaylistSortWidget.h"
#include "ProgressiveSearchWidget.h"

#include <KLocale>
#include <KStandardDirs>

namespace Playlist
{

ViewUrlGenerator * ViewUrlGenerator::s_instance = 0;

ViewUrlGenerator * ViewUrlGenerator::instance()
{
    if( s_instance == 0)
        s_instance = new ViewUrlGenerator();

    return s_instance;
}


ViewUrlGenerator::ViewUrlGenerator()
{}

ViewUrlGenerator::~ViewUrlGenerator()
{}

AmarokUrl
ViewUrlGenerator::createUrl()
{
    DEBUG_BLOCK
    AmarokUrl url;
    url.setCommand( "playlist" );
    url.setPath( "view" );
    Dock * playlistDock = The::mainWindow()->playlistDock();

    QString filterExpr = playlistDock->searchWidget()->currentFilter();
    QString onlyMatches = playlistDock->searchWidget()->onlyMatches() ? "true" : "false";
    QString sortPath = playlistDock->sortWidget()->sortPath();
    QString prettySortPath = playlistDock->sortWidget()->prettySortPath();
    QString layout = LayoutManager::instance()->activeLayoutName();
    debug()<< "The filter is "<< filterExpr;
    debug()<< "OnlyMatches is "<< onlyMatches;
    debug()<< "The sortPath is "<< sortPath;
    debug()<< "The layout is "<< layout;

    QString prettyUrlName;

    if( !sortPath.isEmpty() )
    {
        url.setArg( "sort", sortPath );
        prettyUrlName.append( prettySortPath );
    }
    if( !filterExpr.isEmpty() )
    {
        url.setArg( "filter", filterExpr );
        url.setArg( "matches", onlyMatches );
        if( !prettyUrlName.isEmpty() )
            prettyUrlName.append( "  |  " );
        QString prettyFilterExpr = "\"" + filterExpr + "\"";
        prettyUrlName.append( ( onlyMatches == QString( "true" ) )
                              ? i18n( "Filter %1", prettyFilterExpr )
                              : i18n( "Search %1", prettyFilterExpr ) );
    }
    if( !layout.isEmpty() )
    {
        url.setArg( "layout", layout );
        if( !prettyUrlName.isEmpty() )
            prettyUrlName.append( "  |  " );
        prettyUrlName.append( i18n( "%1 layout", layout ) );
    }

    url.setName( prettyUrlName );
    debug()<< "Url is "<<url.url();
    return url;
}

QString
ViewUrlGenerator::description()
{
    return i18n( "Bookmark Playlist Setup" );
}

QIcon ViewUrlGenerator::icon()
{
    return QIcon( QPixmap( KStandardDirs::locate( "data", "amarok/images/playlist-bookmark-16.png" ) ) );
}

} //namespace Playlist
