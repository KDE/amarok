/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
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
 
#include "BrowserWidget.h"

#include "Amarok.h"
#include "Debug.h"
#include "widgets/HorizontalDivider.h"

#include "KIcon"

BrowserWidget::BrowserWidget( QWidget * parent )
    : KVBox( parent )
{
    m_breadcrumbWidget = new BrowserBreadcrumbWidget( this );
    new HorizontalDivider( this );
    
    m_categoryList = new BrowserCategoryList( this, "root list" );
    m_categoryList->setPrettyName( i18n( "Home" ) );
    m_categoryList->setIcon( KIcon( "user-home" ) );

    m_breadcrumbWidget->setRootList( m_categoryList );

    m_categoryList->setMinimumSize( 100, 300 );

    connect( m_categoryList, SIGNAL( viewChanged() ), this, SLOT( categoryChanged() ) );
    connect( m_breadcrumbWidget, SIGNAL( toHome() ), this, SLOT( home() ) );

    setFrameShape( QFrame::NoFrame );

    // Keyboard shortcut for going back one level
    KAction *action = new KAction( KIcon( "go-previous" ), i18n( "Previous Browser" ), this );
    Amarok::actionCollection()->addAction( "browser_previous", action );
    connect( action, SIGNAL( triggered( bool ) ), m_categoryList, SLOT( back() ) );
    action->setShortcut( KShortcut( Qt::CTRL + Qt::Key_Left ) );
}


BrowserWidget::~BrowserWidget()
{}

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
