/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
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
 
#include "BrowserWidget.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "widgets/HorizontalDivider.h"

#include <KAction>
#include <KIcon>

BrowserWidget::BrowserWidget( QWidget * parent )
    : AmarokDockWidget(  i18n( "Context" ), parent )
{
    DEBUG_BLOCK

    setObjectName( "Context dock" );
    setAllowedAreas( Qt::AllDockWidgetAreas );

    //we have to create this here as it is used when setting up the
    //categories (unless of couse we move that to polish as well...)
    m_mainWidget = new KVBox( this );
    m_breadcrumbWidget = new BrowserBreadcrumbWidget( m_mainWidget );
    new HorizontalDivider( m_mainWidget );
    m_categoryList = new BrowserCategoryList( m_mainWidget, "root list" );
    m_breadcrumbWidget->setRootList( m_categoryList );
}


BrowserWidget::~BrowserWidget()
{}

void BrowserWidget::polish()
{
    DEBUG_BLOCK
    setWidget( m_mainWidget );

    m_categoryList->setIcon( KIcon( "user-home" ) );

    m_categoryList->setMinimumSize( 100, 300 );

    connect( m_breadcrumbWidget, SIGNAL( toHome() ), this, SLOT( home() ) );

    m_mainWidget->setFrameShape( QFrame::NoFrame );

    // Keyboard shortcut for going back one level
    KAction *action = new KAction( KIcon( "go-previous" ), i18n( "Previous Browser" ), m_mainWidget );
    Amarok::actionCollection()->addAction( "browser_previous", action );
    connect( action, SIGNAL( triggered( bool ) ), m_categoryList, SLOT( back() ) );
    action->setShortcut( KShortcut( Qt::CTRL + Qt::Key_Left ) );
}

BrowserCategoryList * BrowserWidget::list() const
{
    return m_categoryList;
}

void
BrowserWidget::navigate( const QString & target )
{
    m_categoryList->navigate( target );
}

void
BrowserWidget::home()
{
    DEBUG_BLOCK
    m_categoryList->home();
}

#include "BrowserWidget.moc"
