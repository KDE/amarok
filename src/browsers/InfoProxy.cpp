/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "InfoProxy.h"

#include "App.h"
#include "Debug.h"
#include "PaletteHandler.h"

#include <KStandardDirs>

#include <QFile>

InfoProxy * InfoProxy::m_instance = 0;

InfoProxy * InfoProxy::instance()
{
    if ( m_instance == 0 )
        m_instance = new InfoProxy();

    return m_instance;
}

InfoProxy::InfoProxy()
{
    DEBUG_BLOCK;
    //for testing

    QList<QVariant> strings;
    QList<QVariant> weights;

    strings << "This" << "is" << "just" << "a" << "very" << "small" << "and" << "quite" << "silly" << "defalt" << "text"
            << "as" << "I" << "currently" << "have" <<  "nothing" << "better" << "to" << "show";

    weights << 10 << 4 << 8 << 2 << 6 << 5 << 10 << 9 << 3 << 1 << 3 << 5 << 7 << 9 << 3 << 2 << 10 << 6 << 4;

    m_storedCloud["cloud_name"] = QVariant( "test cloud" );
    m_storedCloud["cloud_strings"] = QVariant( strings );
    m_storedCloud["cloud_weights"] = QVariant( weights );

    loadHomePage();
}

InfoProxy::~InfoProxy()
{
}


void
InfoProxy::subscribe( InfoObserver * observer )
{
    DEBUG_BLOCK;
    if( observer )
    {
        m_observers.insert( observer );
        observer->infoChanged( m_storedInfo );
    }
}

void
InfoProxy::subscribeForCloud( InfoObserver * observer )
{
    DEBUG_BLOCK;
    if( observer )
    {
        m_cloudObservers.insert( observer );
        observer->infoChanged( m_storedCloud );
    }
}

void
InfoProxy::unsubscribe( InfoObserver * observer )
{
    m_observers.remove( observer );
    m_cloudObservers.remove( observer );
}

void
InfoProxy::notifyObservers( const QVariantMap &infoMap ) const
{
    foreach( InfoObserver *observer, m_observers )
        observer->infoChanged( infoMap );
}

void
InfoProxy::notifyCloudObservers( const QVariantMap &cloudMap ) const
{
    foreach( InfoObserver *observer, m_cloudObservers )
        observer->infoChanged( cloudMap );
}

void
InfoProxy::setInfo( const QVariantMap &infoMap )
{
    m_storedInfo = infoMap;
    notifyObservers( m_storedInfo );
}

void
InfoProxy::setCloud( const QVariantMap &cloudMap )
{
    m_storedCloud = cloudMap;
    notifyCloudObservers( m_storedCloud );
}

QVariantMap
InfoProxy::info()
{
    return m_storedInfo;
}

QVariantMap
InfoProxy::cloud()
{
    return m_storedCloud;
}

void
InfoProxy::loadHomePage()
{
    DEBUG_BLOCK

    KUrl dataUrl( KStandardDirs::locate( "data", "amarok/data/" ) );
    QString dataPath = dataUrl.path();

    //load html

    QString htmlPath = dataPath + "service_info_frontpage.html";
    QFile file( htmlPath );
    if ( !file.open( QIODevice::ReadOnly | QIODevice::Text) )
    {
        debug() << "error opening file. Error: " << file.error();
        return;
    }

    QString html = file.readAll();

    KUrl imageUrl( KStandardDirs::locate( "data", "amarok/images/" ) );
    QString imagePath = imageUrl.url();

    html.replace( "_PATH_", imagePath );

    html.replace( "{background_color}",PaletteHandler::highlightColor().lighter( 150 ).name() );
    html.replace( "{border_color}", PaletteHandler::highlightColor().lighter( 150 ).name() );
    html.replace( "{text_color}", App::instance()->palette().brush( QPalette::Text ).color().name() );
    QColor highlight( App::instance()->palette().highlight().color() );
    highlight.setHsvF( highlight.hueF(), 0.3, .95, highlight.alphaF() );
    html.replace( "{header_background_color}", highlight.name() );


    m_storedInfo["service_name"] =  i18n( "Home" );
    m_storedInfo["main_info"] = html;

    notifyObservers( m_storedInfo );
}


namespace The {
    AMAROK_EXPORT InfoProxy* infoProxy() { return InfoProxy::instance(); }
}

