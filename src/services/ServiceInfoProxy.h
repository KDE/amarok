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

class ServiceInfoProxy;

namespace The {
    AMAROK_EXPORT ServiceInfoProxy* serviceInfoProxy();
}

/**
A proxy class for relaying information from the currently active service to the ServiceEngine so it can be displayed in a plasma applet in the context view. It is a singleton and included in the "The" namespace for easy access

    Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> 
*/
class AMAROK_EXPORT ServiceInfoProxy
{
    friend ServiceInfoProxy* serviceInfoProxy();

    public:
        static ServiceInfoProxy * instance();
        ~ServiceInfoProxy();

        void subscribe( ServiceInfoObserver *observer );
        void subscribeForCloud( ServiceInfoObserver *observer );
        void unsubscribe( ServiceInfoObserver *observer );

        void setInfo( const QVariantMap &infoMap );
        void setCloud( const QVariantMap &cloudMap );

        void loadHomePage();
        
        QVariantMap info(); // info about the service
        QVariantMap cloud(); //cloud view for the service

    private slots:
        void paletteChanged( const QPalette & palette );
    
    private:
        ServiceInfoProxy();
        void notifyObservers( const QVariantMap &infoMap ) const;
        void notifyCloudObservers( const QVariantMap &cloudMap ) const;
        QSet<ServiceInfoObserver *> m_observers;
        QSet<ServiceInfoObserver *> m_cloudObservers;

        static ServiceInfoProxy * m_instance;

        QVariantMap m_storedInfo;
        QVariantMap m_storedCloud;
};

#endif
