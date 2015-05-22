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

#define DEBUG_PREFIX "ContextDock"

#include "ContextDock.h"

#include "amarokconfig.h"
#include "context/ContextScene.h"
#include "context/ContextView.h"
#include "context/ToolbarView.h"
#include "core/support/Debug.h"

#include <KVBox>

ContextDock::ContextDock( QWidget *parent )
    : AmarokDockWidget( i18n( "&Context" ), parent )
{
    setObjectName( "Context dock" );
    setAllowedAreas( Qt::AllDockWidgetAreas );
    setMinimumWidth( 50 );
    setContentsMargins( 0, 0, 0, 0 );

    m_mainWidget = new KVBox( this );
    m_mainWidget->setMinimumWidth( 400 );
    m_mainWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_mainWidget->setSpacing( 0 );
    m_mainWidget->setContentsMargins( 0, 0, 0, 0 );
    m_mainWidget->setFrameShape( QFrame::NoFrame );
    setWidget( m_mainWidget );

    m_corona = new Context::ContextScene( this );
    connect( m_corona.data(), SIGNAL(containmentAdded(Plasma::Containment*)),
            this, SLOT(createContextView(Plasma::Containment*)) );

    m_corona.data()->loadDefaultSetup(); // this method adds our containment to the scene
}

void ContextDock::polish()
{
}

void
ContextDock::createContextView( Plasma::Containment *containment )
{
    disconnect( m_corona.data(), SIGNAL(containmentAdded(Plasma::Containment*)),
            this, SLOT(createContextView(Plasma::Containment*)) );

    debug() << "Creating context view on containmend" << containment->name();
    PERF_LOG( "Creating ContexView" )
    m_contextView = new Context::ContextView( containment, m_corona.data(), m_mainWidget );
    m_contextView.data()->setFrameShape( QFrame::NoFrame );
    m_contextToolbarView = new Context::ToolbarView( containment, m_corona.data(), m_mainWidget );
    PERF_LOG( "Created ContexToolbarView" )

    connect( m_corona.data(), SIGNAL(sceneRectChanged(QRectF)), m_contextView.data(), SLOT(updateSceneRect(QRectF)) );
    connect( m_contextToolbarView.data(), SIGNAL(hideAppletExplorer()), m_contextView.data(), SLOT(hideAppletExplorer()) );
    connect( m_contextToolbarView.data(), SIGNAL(showAppletExplorer()), m_contextView.data(), SLOT(showAppletExplorer()) );
    m_contextView.data()->showHome();
    PERF_LOG( "ContexView created" )
}


