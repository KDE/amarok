/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *             (c) 2008  Jeff Mitchell <kde-dev@emailgoeshere.com>         *
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
 
#include "SvgHandler.h"

#include "App.h"
#include "Debug.h"
#include "MainWindow.h"
#include "SvgTinter.h"

#include <KStandardDirs>

#include <QHash>
#include <QPainter>
#include <QReadLocker>
#include <QWriteLocker>


namespace The {
    static SvgHandler* s_SvgHandler_instance = 0;

    SvgHandler* svgHandler()
    {
        if( !s_SvgHandler_instance )
            s_SvgHandler_instance = new SvgHandler();

        return s_SvgHandler_instance;
    }
}


SvgHandler::SvgHandler( QObject* parent )
    : QObject( parent )
    , m_cache( new KPixmapCache( "Amarok-pixmaps" ) )
    , m_themeFile( "amarok/images/default-theme-clean.svg" )  // //use default theme
    , m_customTheme( false )
{
    DEBUG_BLOCK
}

SvgHandler::~SvgHandler()
{
    DEBUG_BLOCK

    m_cache->deleteCache( "Amarok-pixmaps" ); 
    delete m_cache;

    The::s_SvgHandler_instance = 0;
}


bool SvgHandler::loadSvg( const QString& name )
{
    QString svgFilename;
    
    if ( !m_customTheme )
        svgFilename = KStandardDirs::locate( "data", name );
    else
        svgFilename = name;
    
    KSvgRenderer *renderer = new KSvgRenderer( The::svgTinter()->tint( svgFilename ).toAscii() );

    if ( !renderer->isValid() )
    {
        debug() << "Bluddy 'ell mateys, aye canna' load ya Ess Vee Gee at " << svgFilename;
        delete renderer;
        return false;
    }
    QWriteLocker writeLocker( &m_lock );

    if( m_renderers[name] )
        delete m_renderers[name];

    m_renderers[name] = renderer;
    return true;
}

KSvgRenderer* SvgHandler::getRenderer( const QString& name )
{
    QReadLocker readLocker( &m_lock );
    if( ! m_renderers[name] )
    {
        readLocker.unlock();
        if( !loadSvg( name ) )
        {
            QWriteLocker writeLocker( &m_lock );
            m_renderers[name] = new KSvgRenderer();
        }
        readLocker.relock();
    }
    return m_renderers[name];
}

KSvgRenderer * SvgHandler::getRenderer()
{
    return getRenderer( m_themeFile );
}

QPixmap SvgHandler::renderSvg( const QString &name, const QString& keyname, int width, int height, const QString& element )
{
    QPixmap pixmap( width, height );
    pixmap.fill( Qt::transparent );

    QReadLocker readLocker( &m_lock );
    if( ! m_renderers[name] )
    {
        readLocker.unlock();
        if( !loadSvg( name ) )
            return pixmap;
        readLocker.relock();
    }

    const QString key = QString("%1:%2x%3")
        .arg( keyname )
        .arg( width )
        .arg( height );


    if ( !m_cache->find( key, pixmap ) ) {
//         debug() << QString("svg %1 not in cache...").arg( key );

        QPainter pt( &pixmap );
        if ( element.isEmpty() )
            m_renderers[name]->render( &pt, QRectF( 0, 0, width, height ) );
        else
            m_renderers[name]->render( &pt, element, QRectF( 0, 0, width, height ) );
  
        m_cache->insert( key, pixmap );
    }

    return pixmap;
}

QPixmap SvgHandler::renderSvg(const QString & keyname, int width, int height, const QString & element)
{
    return renderSvg( m_themeFile, keyname, width, height, element );
}

QPixmap SvgHandler::renderSvgWithDividers(const QString & keyname, int width, int height, const QString & element)
{

    QString name = m_themeFile;
    
    QPixmap pixmap( width, height );
    pixmap.fill( Qt::transparent );

    QReadLocker readLocker( &m_lock );
    if( ! m_renderers[name] )
    {
        readLocker.unlock();
        if( ! loadSvg( name ) )
            return pixmap;
        readLocker.relock();
    }

    const QString key = QString("%1:%2x%3-div")
            .arg( keyname )
            .arg( width )
            .arg( height );


    if ( !m_cache->find( key, pixmap ) ) {
//         debug() << QString("svg %1 not in cache...").arg( key );

        QPainter pt( &pixmap );
        if ( element.isEmpty() )
            m_renderers[name]->render( &pt, QRectF( 0, 0, width, height ) );
        else
            m_renderers[name]->render( &pt, element, QRectF( 0, 0, width, height ) );


        //add dividers. 5% spacing on each side
        int margin = width / 20;
        
        m_renderers[name]->render( &pt, "divider_top", QRectF( margin, 0 , width - 1 * margin, 1 ) );
        m_renderers[name]->render( &pt, "divider_bottom", QRectF( margin, height - 1 , width - 2 * margin, 1 ) );
    
        m_cache->insert( key, pixmap );
    }

    return pixmap;

}


void SvgHandler::reTint()
{
    The::svgTinter()->init();
    if ( !loadSvg( m_themeFile ))
        warning() << "Unable to load theme file: " << m_themeFile;
}

QString SvgHandler::themeFile()
{
    return m_themeFile;
}

void SvgHandler::setThemeFile( const QString & themeFile )
{
    DEBUG_BLOCK
    debug() << "got new theme file: " << themeFile;
    m_themeFile = themeFile;
    m_customTheme = true;
    reTint();
    
    //redraw entire app....
    m_cache->discard();
    App::instance()->mainWindow()->update();
}


QPixmap SvgHandler::addBordersToPixmap( QPixmap orgPixmap, int borderWidth, const QString &name, bool skipCache )
{
    int newWidth = orgPixmap.width() + borderWidth * 2;
    int newHeight = orgPixmap.height() + borderWidth *2;

    QPixmap pixmap( newWidth, newHeight );
    pixmap.fill( Qt::transparent );
    
    QReadLocker readLocker( &m_lock );
    if( !m_renderers[m_themeFile] )
    {
        readLocker.unlock();
        if( !loadSvg( m_themeFile ) )
            return pixmap;
        readLocker.relock();
    }

    const QString key = QString("%1:%2x%3b%4")
            .arg( name )
            .arg( newWidth )
            .arg( newHeight )
            .arg( borderWidth );

    if( !m_cache->find( key, pixmap ) || skipCache )
    {
        // Cache miss! We need to create the pixmap

        //whoops... if skipCache is true, we might actually already have fetched the image, including borders from the cache....
        //so we really need to create a blank pixmap here so we don't paint several layers of borders on top of each other
        if ( skipCache ) {
            pixmap = QPixmap( newWidth, newHeight );
            pixmap.fill( Qt::transparent );
        }
        
        QPainter pt( &pixmap );

        pt.drawPixmap( borderWidth, borderWidth, orgPixmap.width(), orgPixmap.height(), orgPixmap );

        m_renderers[m_themeFile]->render( &pt, "cover_border_topleft", QRectF( 0, 0, borderWidth, borderWidth ) );
        m_renderers[m_themeFile]->render( &pt, "cover_border_top", QRectF( borderWidth, 0, orgPixmap.width(), borderWidth ) );
        m_renderers[m_themeFile]->render( &pt, "cover_border_topright", QRectF( newWidth - borderWidth , 0, borderWidth, borderWidth ) );
        m_renderers[m_themeFile]->render( &pt, "cover_border_right", QRectF( newWidth - borderWidth, borderWidth, borderWidth, orgPixmap.height() ) );
        m_renderers[m_themeFile]->render( &pt, "cover_border_bottomright", QRectF( newWidth - borderWidth, newHeight - borderWidth, borderWidth, borderWidth ) );
        m_renderers[m_themeFile]->render( &pt, "cover_border_bottom", QRectF( borderWidth, newHeight - borderWidth, orgPixmap.width(), borderWidth ) );
        m_renderers[m_themeFile]->render( &pt, "cover_border_bottomleft", QRectF( 0, newHeight - borderWidth, borderWidth, borderWidth ) );
        m_renderers[m_themeFile]->render( &pt, "cover_border_left", QRectF( 0, borderWidth, borderWidth, orgPixmap.height() ) );
    
        m_cache->insert( key, pixmap );
    }

    return pixmap;
}
