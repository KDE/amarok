/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#include "Debug.h"
#include "layouts/LayoutManager.h"
#include "MainWindow.h"
#include "PlaylistWidget.h"
#include "PlaylistSortWidget.h"
#include "ProgressiveSearchWidget.h"

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
    Widget * playlistWidget = The::mainWindow()->playlistWidget();

    QString filterExpr = playlistWidget->searchWidget()->currentFilter();
    QString onlyMatches = playlistWidget->searchWidget()->onlyMatches() ? "true" : "false";
    QString sortPath = playlistWidget->sortWidget()->sortPath();
    QString prettySortPath = playlistWidget->sortWidget()->prettySortPath();
    QString layout = LayoutManager::instance()->activeLayoutName();
    debug()<< "The filter is "<< filterExpr;
    debug()<< "OnlyMatches is "<< onlyMatches;
    debug()<< "The sortPath is "<< sortPath;
    debug()<< "The layout is "<< layout;

    QString prettyUrlName;

    if( !sortPath.isEmpty() )
    {
        url.appendArg( "sort", sortPath );
        prettyUrlName.append( prettySortPath );
    }
    if( !filterExpr.isEmpty() )
    {
        url.appendArg( "filter", filterExpr );
        url.appendArg( "matches", onlyMatches );
        if( !prettyUrlName.isEmpty() )
            prettyUrlName.append( "  |  " );
        QString prettyFilterExpr = "\"" + filterExpr + "\"";
        prettyUrlName.append( ( onlyMatches == QString( "true" ) )
                              ? i18n( "Filter %1", prettyFilterExpr )
                              : i18n( "Search %1", prettyFilterExpr ) );
    }
    if( !layout.isEmpty() )
    {
        url.appendArg( "layout", layout );
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

} //namespace Playlist
