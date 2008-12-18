/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#include "ProgressiveSearchWidget.h"

#include "Debug.h"

#include <KAction>
#include <KLineEdit>
#include <KLocale>

#include <QToolBar>

ProgressiveSearchWidget::ProgressiveSearchWidget( QWidget * parent )
    : KHBox( parent )
{

    m_searchEdit = new KLineEdit( this );
    m_searchEdit->setClickMessage( i18n( "Search playlist" ) );
    m_searchEdit->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_searchEdit->setClearButtonShown( true );
    m_searchEdit->setFrame( true );
    m_searchEdit->setToolTip( i18n( "Start typing to progressively search through the playlist" ) );

    connect( m_searchEdit, SIGNAL( textChanged( const QString & ) ), this, SLOT( slotFilterChanged(  const QString &  ) ) );

    QToolBar * toolbar = new QToolBar( this );

    m_nextAction = new KAction( KIcon( "go-down" ), i18n( "&Next" ), this );
    connect( m_nextAction, SIGNAL( triggered() ), this, SLOT( slotNext() ) );
    
    m_previousAction = new KAction( KIcon( "go-up" ), i18n( "&Previous" ), this );
    connect( m_previousAction, SIGNAL( triggered() ), this, SLOT( slotPrevious() ) );

    toolbar->addAction( m_nextAction );
    toolbar->addAction( m_previousAction );

    noMatch();
    
    
}


ProgressiveSearchWidget::~ProgressiveSearchWidget()
{
}

void ProgressiveSearchWidget::slotFilterChanged( const QString & filter )
{
    DEBUG_BLOCK
    debug() << "New filter: " << filter;

    if( filter.isEmpty() ) {
        emit( filterCleared() );
        noMatch();
    } else
        emit( filterChanged( filter ) );

}

void ProgressiveSearchWidget::slotNext()
{
    DEBUG_BLOCK
    emit( next( m_searchEdit->text() ) );
}

void ProgressiveSearchWidget::slotPrevious()
{
    DEBUG_BLOCK
    emit( previous( m_searchEdit->text() ) );
            
}

void ProgressiveSearchWidget::match()
{
    m_nextAction->setEnabled( true );
    m_previousAction->setEnabled( true );
}

void ProgressiveSearchWidget::noMatch()
{
    m_nextAction->setEnabled( false );
    m_previousAction->setEnabled( false );
}


#include "ProgressiveSearchWidget.moc"