/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "PlaylistBreadcrumbItem.h"

#include "PlaylistDefines.h"

#include <KIcon>
#include <KLocale>

#include <QMenu>

namespace Playlist
{

BreadcrumbItem::BreadcrumbItem( BreadcrumbLevel *level, QWidget *parent )
    : KHBox( parent )
{
    //Let's set up the "siblings" button first...
    m_menuButton = new BreadcrumbItemMenuButton( this );
    QMenu *menu = new QMenu( this );
    QMap< QString, QPair< KIcon, QString > > siblings = level->siblings();
    for( QMap< QString, QPair< KIcon, QString > >::const_iterator i = siblings.begin(); i != siblings.end(); ++i )
    {
        QAction *action = menu->addAction( i.value().first, i.value().second );
        action->setData( i.key() );
    }
    m_menuButton->setMenu( menu );
    const int offset = 6;
    menu->setContentsMargins( offset, 1, 1, 2 );
    connect( menu, SIGNAL( triggered( QAction* ) ), this, SLOT( siblingTriggered( QAction* ) ) );

    //And then the main breadcrumb button...
    m_mainButton = new BreadcrumbItemButton( level->icon(), level->name(), this );

    connect( m_mainButton, SIGNAL( clicked() ), this, SIGNAL( clicked() ) );

    connect( m_mainButton, SIGNAL( sizePolicyChanged() ), this, SLOT( updateSizePolicy() ) );
    menu->hide();

    updateSizePolicy();

    m_name = level->name();
}

BreadcrumbItem::~BreadcrumbItem()
{}

QString
BreadcrumbItem::name()
{
    return m_name;
}

void
BreadcrumbItem::updateSizePolicy()
{
    setSizePolicy( m_mainButton->sizePolicy() );
}

void
BreadcrumbItem::siblingTriggered( QAction * action )
{
    emit siblingClicked( action );
}

BreadcrumbAddMenuButton::BreadcrumbAddMenuButton( QWidget *parent )
    : BreadcrumbItemMenuButton( parent )
{
    setFixedWidth( 20 );
    setToolTip( i18n( "Add a sorting level to the playlist." ) );

    QMenu *menu = new QMenu( this );

    for( int i = 0; i < NUM_COLUMNS; ++i )  //might be faster if it used a const_iterator
    {
        if( !sortableCategories.contains( internalColumnNames.at( i ) ) )
            continue;
        QAction *action = menu->addAction( KIcon( iconNames.at( i ) ), QString( columnNames.at( i ) ) );
        action->setData( internalColumnNames.at( i ) );
        //FIXME: this menu should have the same margins as other Playlist::Breadcrumb and
        //       BrowserBreadcrumb menus.
    }
    setMenu( menu );
    connect( menu, SIGNAL( triggered( QAction* ) ), this, SLOT( siblingTriggered( QAction* ) ) );
}

BreadcrumbAddMenuButton::~BreadcrumbAddMenuButton()
{}

void
BreadcrumbAddMenuButton::siblingTriggered( QAction *action )
{
    emit siblingClicked( action->data().toString() );
}

}   //namespace Playlist
