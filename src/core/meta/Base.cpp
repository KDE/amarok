/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2013 Matěj Laitl <matej@laitl.cz>                                      *
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
 ***************************************************************************************/

#include "Base.h"

#include "core/meta/Observer.h"

#include <QDebug>

using namespace Meta;

Base::Base()
    : m_observersLock( QReadWriteLock::Recursive )
{
}

Base::~Base()
{
    // we need to notify all observers that we're deleted to avoid stale pointers
    for( Observer *observer : m_observers )
    {
        observer->destroyedNotify( this );
    }
}

void
Base::subscribe( Observer *observer )
{
    if( observer )
    {
        QWriteLocker locker( &m_observersLock );
        m_observers.insert( observer );
    }
}

void
Base::unsubscribe( Observer *observer )
{
    QWriteLocker locker( &m_observersLock );
    m_observers.remove( observer );
}

QDebug
operator<<( QDebug dbg, const Base &base )
{
    dbg.nospace() << "Meta::Base(" << base.name() << " at " << &base << ")";
    return dbg.space();
}
