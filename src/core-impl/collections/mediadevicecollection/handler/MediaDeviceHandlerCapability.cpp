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

Handler::Capability::Capability( QObject *parent )
    : QObject()
{
    /* we may be created in non-gui or non-eventloop thread. Let's move ourselves into
     * parent's thread so that we can be children. */
    if( thread() != parent->thread() )
        moveToThread( parent->thread() );
    setParent( parent );
}

Handler::Capability::~Capability()
{
    //nothing to do
}

#include "MediaDeviceHandlerCapability.moc"
