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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "PlaylistViewUrlGenerator.h"

#include "Debug.h"
#include "layouts/LayoutManager.h"
#include "MainWindow.h"
#include "PlaylistGroupingAction.h"
#include "PlaylistWidget.h"
#include "PlaylistSortWidget.h"
#include "ProgressiveSearchWidget.h"

namespace Playlist
{

ViewUrlGenerator::ViewUrlGenerator()
{}

ViewUrlGenerator::~ViewUrlGenerator()
{}

AmarokUrl
ViewUrlGenerator::createAmarokUrl()
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
    QString groupingCategory = playlistWidget->groupingAction()->currentGroupingCategory();
        QString prettyGroupingCategory = playlistWidget->groupingAction()->prettyGroupingCategory().remove( "&" );
    QString layout = LayoutManager::instance()->activeLayoutName();
    debug()<< "The filter is "<< filterExpr;
    debug()<< "OnlyMatches is "<< onlyMatches;
    debug()<< "The sortPath is "<< sortPath;
    debug()<< "The grouping is "<< groupingCategory;
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
    if( !groupingCategory.isEmpty() )
    {
        url.appendArg( "group", groupingCategory );
        if( !prettyUrlName.isEmpty() )
            prettyUrlName.append( "  |  " );
        prettyUrlName.append( i18n( "Group by %1", prettyGroupingCategory ) );
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

} //namespace Playlist
