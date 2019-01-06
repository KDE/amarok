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

#include "context/ContextView.h"
#include "core/support/Debug.h"

#include <KLocalizedString>

ContextDock::ContextDock( QWidget *parent )
    : AmarokDockWidget( i18n( "&Context" ), parent )
{
    setObjectName( QStringLiteral("Context dock") );
    setAllowedAreas( Qt::AllDockWidgetAreas );
    setMinimumWidth( 50 );
    setContentsMargins( 0, 0, 0, 0 );

    createContextView();
}

void ContextDock::polish()
{
}

void
ContextDock::createContextView()
{
    auto mainWidget = new Context::ContextView();
    mainWidget->setMinimumWidth( 400 );
    mainWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    mainWidget->setContentsMargins( 0, 0, 0, 0 );
    setWidget( mainWidget );

    PERF_LOG( "ContexView created" )
}


