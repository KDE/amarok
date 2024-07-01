/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "BookmarkCurrentButton.h"

#include "AmarokUrlHandler.h"
#include "BookmarkModel.h"

#include <KLocalizedString>

#include <QIcon>
#include <QMenu>
#include <QString>

BookmarkCurrentButton::BookmarkCurrentButton( QWidget *parent )
    : QToolButton( parent )
{
    setIcon( QIcon::fromTheme( QStringLiteral("bookmark-new") ) );
    setText( i18n( "New Bookmark" ) );
    setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
    connect( this, &BookmarkCurrentButton::clicked, this, &BookmarkCurrentButton::showMenu );
}

BookmarkCurrentButton::~BookmarkCurrentButton()
{
}

void BookmarkCurrentButton::showMenu()
{
    QPoint pos( 0, height() );
    generateMenu( mapToGlobal( pos ) );
}

void BookmarkCurrentButton::generateMenu( const QPoint &pos )
{

    QList<AmarokUrlGenerator *> generators = The::amarokUrlHandler()->generators();

    QMenu menu;

    QMap<QAction *, AmarokUrlGenerator *> generatorMap;

    for( AmarokUrlGenerator * generator : generators )
    {
        generatorMap.insert( menu.addAction( generator->icon() ,generator->description() ), generator );
    }

    QAction * action = menu.exec( pos );

    if( action && generatorMap.contains( action ) )
    {
        AmarokUrl url = generatorMap.value( action )->createUrl();
        url.saveToDb();
        BookmarkModel::instance()->reloadFromDb();
    }
}

