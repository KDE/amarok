/****************************************************************************************
 * Copyright (c) 2010 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "ContextDock.h"


#include "amarokconfig.h"
#include "context/ContextScene.h"
#include "context/ContextView.h"
#include "context/ToolbarView.h"

#include "core/support/Debug.h"

ContextDock::ContextDock( QWidget *parent )
    :AmarokDockWidget(  i18n( "Context" ), parent )
{
    setObjectName( "Context dock" );
    setAllowedAreas( Qt::AllDockWidgetAreas );
}

QSize ContextDock::sizeHint() const
{
    return QSize( static_cast<QWidget*>( parent() )->size().width() / 3, 300 );
}

void ContextDock::polish()
{
    DEBUG_BLOCK

    m_mainWidget = new KVBox( this );

    m_mainWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_mainWidget->setSpacing( 0 );
    m_mainWidget->setFrameShape( QFrame::NoFrame );
    m_mainWidget->setFrameShadow( QFrame::Sunken );
    m_mainWidget->setMinimumSize( 100, 100 );

    setWidget( m_mainWidget );

    m_corona = new Context::ContextScene( this );
    connect( m_corona, SIGNAL( containmentAdded( Plasma::Containment* ) ),
            this, SLOT( createContextView( Plasma::Containment* ) ) );

    m_corona->loadDefaultSetup(); // this method adds our containment to the scene

}


void
ContextDock::createContextView( Plasma::Containment *containment )
{
    DEBUG_BLOCK
    disconnect( m_corona, SIGNAL( containmentAdded( Plasma::Containment* ) ),
            this, SLOT( createContextView( Plasma::Containment* ) ) );
    PERF_LOG( "Creating ContexView" )
    m_contextView = new Context::ContextView( containment, m_corona, m_mainWidget );
    m_contextView->setFrameShape( QFrame::NoFrame );
    m_contextToolbarView = new Context::ToolbarView( containment, m_corona, m_mainWidget );
    m_contextToolbarView->setFrameShape( QFrame::NoFrame );

    connect( m_contextToolbarView, SIGNAL( hideAppletExplorer() ), m_contextView, SLOT( hideAppletExplorer() ) );
    connect( m_contextToolbarView, SIGNAL( showAppletExplorer() ), m_contextView, SLOT( showAppletExplorer() ) );
    m_contextView->showHome();
    m_contextView->resize( sizeHint() );
    PERF_LOG( "ContexView created" )

}

#include "ContextDock.moc"
