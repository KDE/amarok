/****************************************************************************************
 * Copyright (c) 2009  Nikolaj Hald Nielsen <nhn@kde.org>                               *
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
 
#include "ExpandingControlsWidget.h"

#include <KIcon>

ExpandingControlsWidget::ExpandingControlsWidget( QWidget * parent )
 : KVBox( parent )
 , m_mainWidget( 0 )
 , m_expanded( false )
{

    setContentsMargins( 0, 0, 0, 0 );
    
    m_divider = new HorizontalDivider( this );
    
    m_expandButton = new QToolButton( this );
    m_expandButton->setFixedHeight( 16 );
    m_expandButton->setAutoRaise( true );
    m_expandButton->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Fixed );
    m_expandButton->setIcon( KIcon( "arrow-up" ) );
    m_expandButton->setEnabled( false );

    connect( m_expandButton, SIGNAL( clicked() ), this, SLOT( toggleExpanded() ) );
}

ExpandingControlsWidget::~ExpandingControlsWidget()
{
}

void ExpandingControlsWidget::setMainWidget( QWidget * mainWidget )
{

    if ( !mainWidget )
        return;
    
    m_mainWidget = mainWidget;
    
    m_expandButton->setParent( 0 );

    m_mainWidget->setParent( this );
    if ( m_expanded )
        m_mainWidget->show();
    else
        m_mainWidget->hide();

    m_expandButton->setParent( this );
    m_expandButton->show();
    m_expandButton->setEnabled( true );
}

void ExpandingControlsWidget::toggleExpanded()
{
    setExpanded( !m_expanded );
}

void ExpandingControlsWidget::setExpanded( bool expanded )
{
   if ( !m_mainWidget )
       return;

   if ( m_expanded == expanded )
       return;

   m_expanded = expanded;

   if( expanded )
   {
       m_expanded = true;
       m_mainWidget->show();
       m_expandButton->setIcon( KIcon( "arrow-down" ) );
   }
   else
   {
       m_expanded = false;
       m_mainWidget->hide();
       m_expandButton->setIcon( KIcon( "arrow-up" ) );
   }
    
}

#include "ExpandingControlsWidget.moc"


