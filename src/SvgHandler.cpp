/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2008 Jeff Mitchell <kde-dev@emailgoeshere.com>                         *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
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
 
#include "SvgHandler.h"

#include "App.h"
#include "Debug.h"
#include "MainWindow.h"
#include "PaletteHandler.h"
#include "SvgTinter.h"

#include <KColorScheme>
#include <KColorUtils>
#include <KStandardDirs>

#include <QHash>
#include <QPainter>
#include <QPalette>
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
    , m_sliderHandleCache( new KPixmapCache( "Amarok-Slider-pixmaps" ) )
    , m_themeFile( "amarok/images/default-theme-clean.svg" )  // //use default theme
    , m_customTheme( false )
{
    DEBUG_BLOCK
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette& ) ), this, SLOT( reTint() ) );
}

SvgHandler::~SvgHandler()
{
    DEBUG_BLOCK

    m_cache->deleteCache( "Amarok-pixmaps" ); 
    delete m_cache;
    m_sliderHandleCache->deleteCache( "Amarok-Slider-pixmaps" );
    delete m_sliderHandleCache;

    foreach( KSvgRenderer* item, m_renderers )
    {
        delete item;
    }
    m_renderers.clear();

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

    QReadLocker readLocker( &m_lock );
    if( ! m_renderers[name] )
    {
        readLocker.unlock();
        if( !loadSvg( name ) )
        {
            pixmap.fill( Qt::transparent );
            return pixmap;
        }
        readLocker.relock();
    }

    const QString key = QString("%1:%2x%3")
        .arg( keyname )
        .arg( width )
        .arg( height );


    if ( !m_cache->find( key, pixmap ) ) {
//         debug() << QString("svg %1 not in cache...").arg( key );

        pixmap.fill( Qt::transparent );
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

    QReadLocker readLocker( &m_lock );
    if( ! m_renderers[name] )
    {
        readLocker.unlock();
        if( ! loadSvg( name ) )
        {
            pixmap.fill( Qt::transparent );
            return pixmap;
        }
        readLocker.relock();
    }

    const QString key = QString("%1:%2x%3-div")
            .arg( keyname )
            .arg( width )
            .arg( height );


    if ( !m_cache->find( key, pixmap ) ) {
//         debug() << QString("svg %1 not in cache...").arg( key );

        pixmap.fill( Qt::transparent );
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
    
    QReadLocker readLocker( &m_lock );
    if( !m_renderers[m_themeFile] )
    {
        readLocker.unlock();
        if( !loadSvg( m_themeFile ) )
        {
            pixmap.fill( Qt::transparent );
            return pixmap;
        }
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
        if ( skipCache )
            pixmap = QPixmap( newWidth, newHeight );
        
        pixmap.fill( Qt::transparent );
        
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

#if 0
void SvgHandler::paintCustomSlider( QPainter *p, int x, int y, int width, int height, qreal percentage, bool active )
{
    int knobSize = height - 4;
    int sliderRange = width - ( knobSize + 4 );
    int knobRelPos = x + sliderRange * percentage + 2;
    int knobY = y + ( height - knobSize ) / 2 + 1;

    int sliderY = y + ( height / 2 ) - 1;


    //first draw the played part
    p->drawPixmap( x, sliderY,
                   renderSvg(
                   "new_slider_top_played",
                   width, 2,
                   "new_slider_top_played" ),
                   0, 0, knobRelPos - x, 2 );

    //and then the unplayed part
    p->drawPixmap( knobRelPos + 1, sliderY,
                   renderSvg(
                   "new_slider_top",
                   width, 2,
                   "new_slider_top" ),
                   knobRelPos + 1 - x, 0, -1, 2 );

    //and then the bottom
    p->drawPixmap( x, sliderY + 2,
                   renderSvg(
                   "new_slider_bottom",
                   width, 2,
                   "new_slider_bottom" ) );

    //draw end markers
    p->drawPixmap( x, y,
                   renderSvg(
                   "new_slider_end",
                   2, height,
                   "new_slider_end" ) );

    p->drawPixmap( x + width - 2, y,
                   renderSvg(
                   "new_slider_end",
                   2, height,
                   "new_slider_endr" ) );


    if ( active )
        p->drawPixmap( knobRelPos, knobY,
                       renderSvg(
                       "new_slider_knob_active",
                       knobSize, knobSize,
                       "new_slider_knob_active" ) );
    else
        p->drawPixmap( knobRelPos, knobY,
                       renderSvg(
                       "new_slider_knob",
                       knobSize, knobSize,
                       "new_slider_knob" ) );
}
#endif

QRect SvgHandler::sliderKnobRect( const QRect &slider, qreal percent )
{
    //NOTICE Vertical sliders are atm not supported by the API at all, neither is rtl
    const int knobSize = slider.height() - 4;
    QRect ret( 0, 0, knobSize, knobSize );
    ret.moveTo( slider.x() + qRound( ( slider.width() - knobSize ) * percent ), slider.y() + 1 );
    return ret;
}

// Experimental, using a mockup from Nuno Pinheiro (new_slider_nuno)
void SvgHandler::paintCustomSlider( QPainter *p, int x, int y, int width, int height, qreal percentage, bool active )
{
    QRect knob = sliderKnobRect( QRect( x, y, width, height), percentage );

    //debug() << "rel: " << knobRelPos << ", width: " << width << ", height:" << height << ", %: " << percentage;

    // Draw the slider background in 3 parts

    int sliderHeight = height - 6;

    p->drawPixmap( x, y + 2,
                   renderSvg(
                   "progress_slider_left",
                   sliderHeight, sliderHeight,
                   "progress_slider_left" ) );

    p->drawPixmap( x + sliderHeight, y + 2,
                   renderSvg(
                   "progress_slider_mid",
                   width - sliderHeight * 2, sliderHeight,
                   "progress_slider_mid" ) );

    p->drawPixmap( x + width - sliderHeight, y + 2,
                   renderSvg(
                   "progress_slider_right",
                   sliderHeight, sliderHeight,
                   "progress_slider_right" ) );

    //draw the played background.

    int playedBarHeight = sliderHeight - 6;

    int sizeOfLeftPlayed = qBound( 0, knob.x() - 2, playedBarHeight );

    if( sizeOfLeftPlayed > 0 ) {

        p->drawPixmap( x + 3, y + 5,
                        renderSvg(
                        "progress_slider_played_left",
                        playedBarHeight, playedBarHeight,
                        "progress_slider_played_left" ), 0, 0, sizeOfLeftPlayed + 3, playedBarHeight );

        int playedBarMidWidth = knob.x() - ( x + 3 + playedBarHeight );

        //Add 5 more pixels to avoid a "gap" between it and the top and botton of the round knob.
        playedBarMidWidth += 5;

        p->drawPixmap( x + 3 + playedBarHeight, y + 5,
                        renderSvg(
                        "progress_slider_played_mid",
                        playedBarMidWidth, playedBarHeight,
                        "progress_slider_played_mid" ) );
    }


    // Draw the knob (handle)
    if ( active )
        p->drawPixmap( knob.x(), knob.y(),
                       renderSvg(
                       "slider_knob_200911_active",
                       knob.width(), knob.height(),
                       "slider_knob_200911_active" ) );
    else
        p->drawPixmap( knob.x(), knob.y(),
                       renderSvg(
                       "slider_knob_200911",
                       knob.width(), knob.height(),
                       "slider_knob_200911" ) );
}


#include "SvgHandler.moc"
