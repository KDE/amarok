/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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

#ifndef SERVICEINFOPROXY_H
#define SERVICEINFOPROXY_H

#include "amarok_export.h"
#include "ServiceInfoObserver.h"

#include <QVariant>
#include <QSet>


/**
A proxy class for relaying information from the currently active service to the ServiceEngine so it can be displayed in a plasma applet in the context view. It is a singleton and included in the "THE" namespace for easy access

	Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> 
*/
class AMAROK_EXPORT ServiceInfoProxy{
public:

    static AMAROK_EXPORT ServiceInfoProxy * instance();
    ~ServiceInfoProxy();

    void AMAROK_EXPORT subscribe( ServiceInfoObserver *observer );
    void AMAROK_EXPORT unsubscribe( ServiceInfoObserver *observer );

    void setInfo( QVariantMap infoMap );
    QVariantMap AMAROK_EXPORT info();

private:

    ServiceInfoProxy();
    void notifyObservers( QVariantMap infoMap ) const;
    QSet<ServiceInfoObserver *> m_observers;

    static ServiceInfoProxy * m_instance;

    QVariantMap m_storedInfo;
};

#endif
