/****************************************************************************************
 * Copyright (c) 2011 Teo Mrnjavac <teo@kde.org>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "PlaylistToolBar.h"
#include "core/support/Debug.h"

#include <KLocalizedString>

#include <QToolButton>
#include <QResizeEvent>

namespace Playlist {

ToolBar::ToolBar( QWidget *parent ) :
    QToolBar( parent ),
    m_collapsed( false )
{
    setObjectName( QStringLiteral("PlaylistToolBar") );

    m_collapsibleActions = new QActionGroup( parent ); //needs to exist before adding any
                                                     //other action to the toolbar

    m_playlistOperationsMenu = new KActionMenu( QIcon::fromTheme( QStringLiteral("amarok_playlist") ),
                                                i18n( "&Playlist" ), parent );
    m_playlistOperationsMenu->setPopupMode( QToolButton::InstantPopup );
    m_playlistOperationsMenu->setVisible( false );

    addAction( m_playlistOperationsMenu );
    addSeparator();

}

void ToolBar::addCollapsibleActions( const QActionGroup *actions )
{
    foreach( QAction *a, actions->actions() )
    {
        m_collapsibleActions->addAction( a );
    }
    onActionsAdded();
}

void ToolBar::setCollapsed( bool collapsed ) //SLOT
{
    m_collapsed = collapsed;
    if( collapsed )
    {
        foreach( QAction *a, m_collapsibleActions->actions() )
        {
            removeAction( a );
            m_playlistOperationsMenu->addAction( a );
        }
    }
    else
    {
        insertActions( m_playlistOperationsMenu, m_collapsibleActions->actions() );
        foreach( QAction *a, m_collapsibleActions->actions() )
        {
            m_playlistOperationsMenu->removeAction( a );
        }
    }
    m_playlistOperationsMenu->setVisible( collapsed );
}

void ToolBar::onActionsAdded()
{
    int limit = limitWidth();
    if( width() < limit )
        setCollapsed( true );
    else if( width() >= limit )
        setCollapsed( false );
}

void ToolBar::resizeEvent( QResizeEvent *event )
{
    QToolBar::resizeEvent( event );
    int limit = limitWidth();

    if( event->oldSize().width() >= limit && event->size().width() < limit )
        setCollapsed( true );
    else if( event->oldSize().width() < limit && event->size().width() >= limit )
        setCollapsed( false );
}

void ToolBar::actionEvent( QActionEvent *event )
{
    QToolBar::actionEvent( event );
    if( ( event->type() == QEvent::ActionAdded || event->type() == QEvent::ActionRemoved )
            && !m_collapsibleActions->actions().contains( event->action() ) )
    {
        onActionsAdded();
    }
}

} // namespace Playlist
