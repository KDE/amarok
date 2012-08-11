/****************************************************************************************
 * Copyright (c) 2010 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2012 Riccardo Iaconelli <riccardo@kde.org>                             *
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
#include <kdeclarative.h>
#include <QDeclarativeView>
#include <QDeclarativeError>
#include <QDeclarativeEngine>

#include <KStandardDirs>

ContextDock::ContextDock( QWidget *parent )
    : AmarokDockWidget( i18n( "&Context" ), parent )
{
    setObjectName( "Context dock" );
    setAllowedAreas( Qt::AllDockWidgetAreas );
    setMinimumWidth( 50 );
    setContentsMargins( 0, 0, 0, 0 );

    m_mainWidget = new KVBox( this );
    m_mainWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_mainWidget->setSpacing( 0 );
    m_mainWidget->setContentsMargins( 0, 0, 0, 0 );
    m_mainWidget->setFrameShape( QFrame::NoFrame );
    setWidget( m_mainWidget );


    m_view = new QDeclarativeView(m_mainWidget);
    m_declarative = new KDeclarative;
    connect(m_view->engine(), SIGNAL(warnings(QList<QDeclarativeError>)), this, SLOT(printWarnings(QList<QDeclarativeError>)));

    m_declarative->setDeclarativeEngine(m_view->engine());
    m_declarative->initialize();
    m_declarative->setupBindings();
    m_view->setFrameShape(QFrame::NoFrame);
    m_view->setSource(KStandardDirs::locate("appdata", "qml/Context.qml"));
    m_view->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    m_view->viewport()->setAutoFillBackground(false);
}

void ContextDock::polish()
{
    DEBUG_BLOCK
}

void ContextDock::printWarnings(const QList<QDeclarativeError>& warnings)
{
    foreach (const QDeclarativeError warning, warnings) {
        kDebug() << warning.toString();
    }
}

#include "ContextDock.moc"
