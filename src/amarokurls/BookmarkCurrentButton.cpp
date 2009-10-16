/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include <KIcon>

#include <QMenu>
#include <QString>

BookmarkCurrentButton::BookmarkCurrentButton( QWidget *parent )
    : QToolButton( parent )
{
    setIcon( KIcon( "bookmark-new" ) );
    setText( i18n( "Bookmark current ..." ) );
    connect( this, SIGNAL( clicked ( bool ) ), this, SLOT( showMenu() ) );
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

    foreach( AmarokUrlGenerator * generator, generators )
    {
        generatorMap.insert( menu.addAction( generator->description() ), generator );
    }

    QAction * action = menu.exec( pos );

    if( action && generatorMap.contains( action ) )
    {
        AmarokUrl url = generatorMap.value( action )->createUrl();
        url.saveToDb();
        BookmarkModel::instance()->reloadFromDb();
    }
}

#include "BookmarkCurrentButton.moc"
