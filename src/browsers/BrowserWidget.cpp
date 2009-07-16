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

#include "Debug.h"
#include "widgets/HorizontalDivider.h"
#include "PaletteHandler.h"

#include "KIcon"
#include "KPushButton"
#include "KVBox"


BrowserWidget::BrowserWidget( QWidget * parent )
    : KHBox( parent )
{
    KPushButton *backButton = new KPushButton( this );
    backButton->setIcon( KIcon( "go-previous" ) );
    backButton->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding );
    backButton->setFixedWidth( 25 );
    backButton->setToolTip( i18n( "Back" ) );
    backButton->setFocusPolicy( Qt::NoFocus );
    //backButton->setStyleSheet( QString( "QPushButton { background-color: %1; } " ).arg( PaletteHandler::highlightColor().name() ) );

    KVBox *verticalBox = new KVBox( this );

    m_breadcrumbWidget = new BreadcrumbWidget( verticalBox );
    new HorizontalDivider( verticalBox );
    
    m_categoryList = new BrowserCategoryList( verticalBox, "root list" );
    m_categoryList->setPrettyName( i18n( "Home" ) );
    m_categoryList->setIcon( KIcon( "user-home" ) );

    m_breadcrumbWidget->setRootList( m_categoryList );

    m_categoryList->setMinimumSize( 100, 300 );

    connect( m_categoryList, SIGNAL( viewChanged() ), this, SLOT( categoryChanged() ) );
    connect( m_breadcrumbWidget, SIGNAL( toHome() ), this, SLOT( home() ) );
    connect( backButton, SIGNAL( clicked() ), m_categoryList, SLOT( back() ) );

    setFrameShape( QFrame::NoFrame );
}


BrowserWidget::~BrowserWidget()
{
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
