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

#include <KLocalizedString>
#include <QStandardPaths>

namespace Playlist
{

ViewUrlGenerator * ViewUrlGenerator::s_instance = nullptr;

ViewUrlGenerator * ViewUrlGenerator::instance()
{
    if( s_instance == nullptr)
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
    url.setCommand( QStringLiteral("playlist") );
    url.setPath( QStringLiteral("view") );
    Dock * playlistDock = The::mainWindow()->playlistDock();

    QString filterExpr = playlistDock->searchWidget()->currentFilter();
    QString onlyMatches = playlistDock->searchWidget()->onlyMatches() ? QStringLiteral("true") : QStringLiteral("false");
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
        url.setArg( QStringLiteral("sort"), sortPath );
        prettyUrlName.append( prettySortPath );
    }
    if( !filterExpr.isEmpty() )
    {
        url.setArg( QStringLiteral("filter"), filterExpr );
        url.setArg( QStringLiteral("matches"), onlyMatches );
        if( !prettyUrlName.isEmpty() )
            prettyUrlName.append( QStringLiteral("  |  ") );
        QString prettyFilterExpr = QStringLiteral("\"") + filterExpr + QStringLiteral("\"");
        prettyUrlName.append( ( onlyMatches == QStringLiteral( "true" ) )
                              ? i18n( "Filter %1", prettyFilterExpr )
                              : i18n( "Search %1", prettyFilterExpr ) );
    }
    if( !layout.isEmpty() )
    {
        url.setArg( QStringLiteral("layout"), layout );
        if( !prettyUrlName.isEmpty() )
            prettyUrlName.append( QStringLiteral("  |  ") );
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
    return QIcon( QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/images/playlist-bookmark-16.png") ) ) );
}

} //namespace Playlist
