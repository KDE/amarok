/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef INFOPROXY_H
#define INFOPROXY_H

#include "amarok_export.h"
#include "InfoObserver.h"

#include <QVariant>
#include <QSet>

class InfoProxy;

namespace The {
    AMAROK_EXPORT InfoProxy* infoProxy();
}

/**
A proxy class for relaying information from the currently active service to the ServiceEngine so it can be displayed in a plasma applet in the context view. It is a singleton and included in the "The" namespace for easy access

    @author Nikolaj Hald Nielsen <nhn@kde.org> 
*/
class AMAROK_EXPORT InfoProxy
{
    friend InfoProxy* infoProxy();

    public:
        static InfoProxy * instance();
        ~InfoProxy();

        void subscribe( InfoObserver *observer );
        void subscribeForCloud( InfoObserver *observer );
        void unsubscribe( InfoObserver *observer );

        void setInfo( const QVariantMap &infoMap );
        void setCloud( const QVariantMap &cloudMap );

        void loadHomePage();
        
        QVariantMap info(); // info about the service
        QVariantMap cloud(); //cloud view for the service

    private slots:
        void paletteChanged( const QPalette & palette );
    
    private:
        InfoProxy();
        void notifyObservers( const QVariantMap &infoMap ) const;
        void notifyCloudObservers( const QVariantMap &cloudMap ) const;
        QSet<InfoObserver *> m_observers;
        QSet<InfoObserver *> m_cloudObservers;

        static InfoProxy * m_instance;

        QVariantMap m_storedInfo;
        QVariantMap m_storedCloud;
};

#endif
