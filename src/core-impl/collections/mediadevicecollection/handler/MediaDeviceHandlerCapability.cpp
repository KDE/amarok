/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#include "MediaDeviceHandlerCapability.h"

Handler::Capability::Capability( QObject *handler )
    : QObject()
{
    if( thread() != handler->thread() ) {
        /* we need to be in handler's thread so that we can become children */
        moveToThread( handler->thread() );
    }

    /* moveToThread( hander->thread() ); setParent( handler ); fails on assert in debug
     * Qt builds. This is a workaround that is safe as long as this object is only deleted
     * using deleteLater() or form the handler's thread */
    connect( this, SIGNAL(signalSetParent(QObject*)), this, SLOT(slotSetParent(QObject*)) );
    emit signalSetParent( handler );
}

Handler::Capability::~Capability()
{
    // nothing to do
}

void Handler::Capability::slotSetParent( QObject *parent )
{
    setParent( parent );
}

