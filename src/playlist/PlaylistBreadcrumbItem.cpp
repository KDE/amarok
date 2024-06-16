/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "PlaylistBreadcrumbItem.h"

#include "PlaylistSortWidget.h"

#include <QIcon>
#include <KLocalizedString>

namespace Playlist
{

BreadcrumbItemMenu::BreadcrumbItemMenu( Column currentColumn, QWidget *parent )
    : QMenu( parent )
{
    for( Column col = Column( 0 ); col != NUM_COLUMNS; col = Column( col + 1 ) )
    {
        if( !isSortableColumn( col ) || currentColumn == col )
            continue;

        QAction *action = addAction( QIcon::fromTheme( iconName( col ) ),
                                     QString( columnName( col ) ) );
        action->setData( internalColumnName( col ) );
    }

    addSeparator();
    QAction *shuffleAction = addAction( QIcon::fromTheme( QStringLiteral("media-playlist-shuffle") ),
                                        i18n( "Shuffle" ) );
    shuffleAction->setData( QStringLiteral( "Shuffle" ) );

    connect( this, &BreadcrumbItemMenu::triggered, this, &BreadcrumbItemMenu::actionTriggered );
}

BreadcrumbItemMenu::~BreadcrumbItemMenu()
{}

void
BreadcrumbItemMenu::actionTriggered( QAction *action )
{
    const QString actionName( action->data().toString() );
    if( actionName == QLatin1String("Shuffle") )
        Q_EMIT shuffleActionClicked();
    else
        Q_EMIT actionClicked( actionName );
}

/////// BreadcrumbItem methods begin here

BreadcrumbItem::BreadcrumbItem( BreadcrumbLevel *level, QWidget *parent )
    : BoxWidget( false, parent )
    , m_name( level->name() )
    , m_prettyName( level->prettyName() )
{
    // Let's set up the "siblings" button first...
    m_menuButton = new BreadcrumbItemMenuButton( this );
    m_menu = new BreadcrumbItemMenu( columnForName( m_name ), this );

    // Disable used levels
    QStringList usedBreadcrumbLevels = qobject_cast< SortWidget * >( parent )->levels();
    for( QAction *action : m_menu->actions() )
        if( usedBreadcrumbLevels.contains( action->data().toString() ) )
            action->setEnabled( false );

    m_menuButton->setMenu( m_menu );
    m_menu->setContentsMargins( /*offset*/ 6, 1, 1, 2 );

    // And then the main breadcrumb button...
    m_mainButton = new BreadcrumbItemSortButton( level->icon(), level->prettyName(), this );
    connect( m_mainButton, &BreadcrumbItemSortButton::clicked, this, &BreadcrumbItem::clicked );
    connect( m_mainButton, &BreadcrumbItemSortButton::arrowToggled, this, &BreadcrumbItem::orderInverted );
    connect( m_mainButton, &BreadcrumbItemSortButton::sizePolicyChanged, this, &BreadcrumbItem::updateSizePolicy );
    m_menu->hide();

    updateSizePolicy();
}

BreadcrumbItem::~BreadcrumbItem()
{}

QString
BreadcrumbItem::name() const
{
    return m_name;
}

Qt::SortOrder
BreadcrumbItem::sortOrder() const
{
    return m_mainButton->orderState();
}

void
BreadcrumbItem::invertOrder()
{
    m_mainButton->invertOrder();
}

void
BreadcrumbItem::updateSizePolicy()
{
    setSizePolicy( m_mainButton->sizePolicy() );
}

const BreadcrumbItemMenu*
BreadcrumbItem::menu()
{
    return m_menu;
}

/////// BreadcrumbAddMenuButton methods begin here

BreadcrumbAddMenuButton::BreadcrumbAddMenuButton( QWidget *parent )
    : BreadcrumbItemMenuButton( parent )
{
    setToolTip( i18n( "Add a sorting level to the playlist." ) );

    //FIXME: the menu should have the same margins as other Playlist::Breadcrumb and
    //       BrowserBreadcrumb menus.
    m_menu = new BreadcrumbItemMenu( PlaceHolder, this );
    setMenu( m_menu );
}

BreadcrumbAddMenuButton::~BreadcrumbAddMenuButton()
{}

const BreadcrumbItemMenu*
BreadcrumbAddMenuButton::menu()
{
    return m_menu;
}

void
BreadcrumbAddMenuButton::updateMenu( const QStringList &usedBreadcrumbLevels )
{
    // Enable unused, disable used levels
    for( QAction *action : m_menu->actions() )
        action->setEnabled( !usedBreadcrumbLevels.contains( action->data().toString() ) );
}

}   //namespace Playlist
