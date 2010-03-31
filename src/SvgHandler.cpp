/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
#include "core/support/Debug.h"
#include "EngineController.h"
#include "MainWindow.h"
#include "moodbar/MoodbarManager.h"
#include "PaletteHandler.h"
#include "SvgTinter.h"

#include <KColorScheme>
#include <KColorUtils>
#include <KStandardDirs>

#include <QHash>
#include <QPainter>
#include <QPalette>
#include <QReadLocker>
#include <QStyleOptionSlider>
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
    QPixmap pixmap;

    const QString key = QString("%1:%2x%3")
            .arg( keyname )
            .arg( width )
            .arg( height );

    if ( !m_cache->find( key, pixmap ) ) {
//         debug() << QString("svg %1 not in cache...").arg( key );

        pixmap = QPixmap( width, height );
        pixmap.fill( Qt::transparent );

        QReadLocker readLocker( &m_lock );
        if( ! m_renderers[name] )
        {
            readLocker.unlock();
            if( !loadSvg( name ) )
            {
                return pixmap;
            }
            readLocker.relock();
        }

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

    QPixmap pixmap;

    const QString key = QString("%1:%2x%3-div")
            .arg( keyname )
            .arg( width )
            .arg( height );


    if ( !m_cache->find( key, pixmap ) ) {
//         debug() << QString("svg %1 not in cache...").arg( key );

        pixmap = QPixmap( width, height );
        pixmap.fill( Qt::transparent );

        QString name = m_themeFile;
        
        QReadLocker readLocker( &m_lock );
        if( ! m_renderers[name] )
        {
            readLocker.unlock();
            if( ! loadSvg( name ) )
            {
                return pixmap;
            }
            readLocker.relock();
        }
        
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

QPixmap
SvgHandler::imageWithBorder( Meta::AlbumPtr album, int size, int borderWidth )
{
    const int imageSize = size - ( borderWidth * 2 );
    const QString loc   = album->imageLocation( imageSize ).url();
    const QString key   = !loc.isEmpty() ? loc : album->name();
    const QPixmap cover = album->image( imageSize );
    if( album->hasImage() )
        return addBordersToPixmap( cover, borderWidth, key );
    else
        return cover;
}

QPixmap SvgHandler::addBordersToPixmap( QPixmap orgPixmap, int borderWidth, const QString &name, bool skipCache )
{
    int newWidth = orgPixmap.width() + borderWidth * 2;
    int newHeight = orgPixmap.height() + borderWidth *2;

    QPixmap pixmap;
    
    const QString key = QString("%1:%2x%3b%4")
            .arg( name )
            .arg( newWidth )
            .arg( newHeight )
            .arg( borderWidth );

    if( skipCache || !m_cache->find( key, pixmap ) )
    {
        // Cache miss! We need to create the pixmap
        // if skipCache is true, we might actually already have fetched the image, including borders from the cache....
        // so we really need to create a blank pixmap here as well, to not pollute the cached pixmap
        pixmap = QPixmap( newWidth, newHeight );
        pixmap.fill( Qt::transparent );

        QReadLocker readLocker( &m_lock );
        if( !m_renderers[m_themeFile] )
        {
            readLocker.unlock();
            if( !loadSvg( m_themeFile ) )
            {
                return pixmap;
            }
            readLocker.relock();
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

QRect SvgHandler::sliderKnobRect( const QRect &slider, qreal percent, bool inverse ) const
{
    if ( inverse )
        percent = 1.0 - percent;
    const int knobSize = slider.height() - 4;
    QRect ret( 0, 0, knobSize, knobSize );
    ret.moveTo( slider.x() + qRound( ( slider.width() - knobSize ) * percent ), slider.y() + 1 );
    return ret;
}

// Experimental, using a mockup from Nuno Pinheiro (new_slider_nuno)
void SvgHandler::paintCustomSlider( QPainter *p, QStyleOptionSlider *slider, qreal percentage, bool paintMoodbar )
{
    int sliderHeight = slider->rect.height() - 6;
    const bool inverse = ( slider->orientation == Qt::Vertical ) ? slider->upsideDown :
                         ( (slider->direction == Qt::RightToLeft) != slider->upsideDown );
    QRect knob = sliderKnobRect( slider->rect, percentage, inverse );
    QPoint pt = slider->rect.topLeft() + QPoint( 0, 2 );

    //debug() << "rel: " << knobRelPos << ", width: " << width << ", height:" << height << ", %: " << percentage;

    //if we should paint moodbar, paint this as the bottom layer
    bool moodbarPainted = false;
    if ( paintMoodbar )
    {
        Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
        if ( currentTrack )
        {
            if( The::moodbarManager()->hasMoodbar( currentTrack ) )
            {
                QPixmap moodbar = The::moodbarManager()->getMoodbar( currentTrack, slider->rect.width() - sliderHeight, sliderHeight, inverse );
                p->drawPixmap( pt, renderSvg( "moodbar_end_left", sliderHeight / 2, sliderHeight, "moodbar_end_left" ) );

                pt.rx() += sliderHeight / 2;
                p->drawPixmap( pt, moodbar );

                pt.rx() += slider->rect.width() - sliderHeight;
                p->drawPixmap( pt, renderSvg( "moodbar_end_right", sliderHeight / 2, sliderHeight, "moodbar_end_right" ) );

                moodbarPainted = true;
            }
        }
    }

    if( !moodbarPainted )
    {
        // Draw the slider background in 3 parts

        p->drawPixmap( pt, renderSvg( "progress_slider_left", sliderHeight, sliderHeight, "progress_slider_left" ) );

        pt.rx() += sliderHeight;
        QRect midRect(pt, QSize(slider->rect.width() - sliderHeight * 2, sliderHeight) );
        p->drawTiledPixmap( midRect, renderSvg( "progress_slider_mid", 32, sliderHeight, "progress_slider_mid" ) );

        pt = midRect.topRight() + QPoint( 1, 0 );
        p->drawPixmap( pt, renderSvg( "progress_slider_right", sliderHeight, sliderHeight, "progress_slider_right" ) );

        //draw the played background.

        int playedBarHeight = sliderHeight - 6;

        int sizeOfLeftPlayed = qBound( 0, inverse ? slider->rect.right() - knob.right() + 2 :
                                                    knob.x() - 2, playedBarHeight );

        if( sizeOfLeftPlayed > 0 )
        {
            QPoint tl, br;
            if ( inverse )
            {
                tl = knob.topRight() + QPoint( -5, 5 ); // 5px x padding to avoid a "gap" between it and the top and botton of the round knob.
                br = slider->rect.topRight() + QPoint( -3, 5 + playedBarHeight - 1 );
                QPixmap rightEnd = renderSvg( "progress_slider_played_right", playedBarHeight, playedBarHeight, "progress_slider_played_right" );
                p->drawPixmap( br.x() - rightEnd.width() + 1, tl.y(), rightEnd, qMax(0, rightEnd.width() - (sizeOfLeftPlayed + 3)), 0, sizeOfLeftPlayed + 3, playedBarHeight );
                br.rx() -= playedBarHeight;
            }
            else
            {
                tl = slider->rect.topLeft() + QPoint( 3, 5 );
                br = QPoint( knob.x() + 5, tl.y() + playedBarHeight - 1 );
                QPixmap leftEnd = renderSvg( "progress_slider_played_left", playedBarHeight, playedBarHeight, "progress_slider_played_left" );
                p->drawPixmap( tl.x(), tl.y(), leftEnd, 0, 0, sizeOfLeftPlayed + 3, playedBarHeight );
                tl.rx() += playedBarHeight;
            }
            if ( sizeOfLeftPlayed == playedBarHeight )
                p->drawTiledPixmap( QRect(tl, br), renderSvg( "progress_slider_played_mid", 32, playedBarHeight, "progress_slider_played_mid" ) );

        }
    }

    if ( slider->state & QStyle::State_Enabled )
    {   // Draw the knob (handle)
        const char *string = ( slider->activeSubControls & QStyle::SC_SliderHandle ) ?
                             "slider_knob_200911_active" : "slider_knob_200911";
        p->drawPixmap( knob.topLeft(), renderSvg( string, knob.width(), knob.height(), string ) );
    }
}


#include "SvgHandler.moc"
