/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Jeff Mitchell <kde-dev@emailgoeshere.com>                         *
 * Copyright (c) 2009-2013 Mark Kretschmann <kretschmann@kde.org>                       *
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

#define DEBUG_PREFIX "SvgHandler"
 
#include "SvgHandler.h"

#include "App.h"
#include "EngineController.h"
#include "MainWindow.h"
#include "PaletteHandler.h"
#include "SvgTinter.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "covermanager/CoverCache.h"
#include "moodbar/MoodbarManager.h"

#include <QPainter>
#include <QReadLocker>
#include <QStandardPaths>
#include <QStyleOptionSlider>
#include <QSvgRenderer>
#include <QWriteLocker>


namespace The {
    static SvgHandler* s_SvgHandler_instance = nullptr;

    SvgHandler* svgHandler()
    {
        if( !s_SvgHandler_instance )
            s_SvgHandler_instance = new SvgHandler();

        return s_SvgHandler_instance;
    }
}

SvgHandler::SvgHandler( QObject* parent )
    : QObject( parent )
    , m_cache( new KImageCache( QStringLiteral("Amarok-pixmaps"), 20 * 1024 ) )
    , m_themeFile( QStringLiteral("amarok/images/default-theme-clean.svg") )  // //use default theme
    , m_customTheme( false )
{
    DEBUG_BLOCK
    connect( The::paletteHandler(), &PaletteHandler::newPalette, this, &SvgHandler::discardCache );
}

SvgHandler::~SvgHandler()
{
    DEBUG_BLOCK
    delete m_cache;
    qDeleteAll( m_renderers );
    m_renderers.clear();

    The::s_SvgHandler_instance = nullptr;
}


bool SvgHandler::loadSvg( const QString& name, bool forceCustomTheme )
{
    const QString &svgFilename = m_customTheme || forceCustomTheme ? name : QStandardPaths::locate( QStandardPaths::GenericDataLocation, name );
    QSvgRenderer *renderer = new QSvgRenderer( The::svgTinter()->tint( svgFilename ) );

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

QSvgRenderer* SvgHandler::getRenderer( const QString& name )
{
    QReadLocker readLocker( &m_lock );
    if( ! m_renderers[name] )
    {
        readLocker.unlock();
        if( !loadSvg( name ) )
        {
            QWriteLocker writeLocker( &m_lock );
            m_renderers[name] = new QSvgRenderer();
        }
        readLocker.relock();
    }
    return m_renderers[name];
}

QSvgRenderer * SvgHandler::getRenderer()
{
    return getRenderer( m_themeFile );
}

QPixmap SvgHandler::renderSvg( const QString &name,
                               const QString& keyname,
                               int width,
                               int height,
                               const QString& element,
                               bool skipCache,
                               const qreal opacity )
{
    QString key;
    if( !skipCache )
    {
        key = QStringLiteral("%1:%2x%3")
            .arg( keyname )
            .arg( width )
            .arg( height );
    }

    QPixmap pixmap;
    if( skipCache || !m_cache->findPixmap( key, &pixmap ) )
    {
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
        pt.setOpacity( opacity );

        if ( element.isEmpty() )
            m_renderers[name]->render( &pt, QRectF( 0, 0, width, height ) );
        else
            m_renderers[name]->render( &pt, element, QRectF( 0, 0, width, height ) );
  
        if( !skipCache )
            m_cache->insertPixmap( key, pixmap );
    }

    return pixmap;
}

QPixmap SvgHandler::renderSvg( const QUrl& url, const QString& keyname, int width, int height, const QString& element, bool skipCache, const qreal opacity )
{
    QString key;
    if( !skipCache )
    {
        key = QStringLiteral("%1:%2x%3")
        .arg( keyname )
        .arg( width )
        .arg( height );
    }

    QPixmap pixmap;
    if( skipCache || !m_cache->findPixmap( key, &pixmap ) )
    {
        pixmap = QPixmap( width, height );
        pixmap.fill( Qt::transparent );

        QString name = url.isLocalFile() ? url.toLocalFile() : QStringLiteral(":") + url.path();
        QReadLocker readLocker( &m_lock );
        if( ! m_renderers[name] )
        {
            readLocker.unlock();
            if( !loadSvg( name, true ) )
            {
                return pixmap;
            }
            readLocker.relock();
        }

        QPainter pt( &pixmap );
        pt.setOpacity( opacity );

        if ( element.isEmpty() )
            m_renderers[name]->render( &pt, QRectF( 0, 0, width, height ) );
        else
            m_renderers[name]->render( &pt, element, QRectF( 0, 0, width, height ) );

        if( !skipCache )
            m_cache->insertPixmap( key, pixmap );
    }

    return pixmap;
}

QPixmap SvgHandler::renderSvg(const QString & keyname, int width, int height, const QString & element, bool skipCache, const qreal opacity )
{
    return renderSvg( m_themeFile, keyname, width, height, element, skipCache, opacity );
}

QPixmap SvgHandler::renderSvgWithDividers(const QString & keyname, int width, int height, const QString & element)
{
    const QString key = QStringLiteral("%1:%2x%3-div")
            .arg( keyname )
            .arg( width )
            .arg( height );

    QPixmap pixmap;
    if ( !m_cache->findPixmap( key, &pixmap ) ) {
//         debug() << QStringLiteral("svg %1 not in cache...").arg( key );

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

        m_renderers[name]->render( &pt, QStringLiteral("divider_top"), QRectF( margin, 0 , width - 1 * margin, 1 ) );
        m_renderers[name]->render( &pt, QStringLiteral("divider_bottom"), QRectF( margin, height - 1 , width - 2 * margin, 1 ) );
    
        m_cache->insertPixmap( key, pixmap );
    }

    return pixmap;
}


void SvgHandler::reTint()
{
    The::svgTinter()->init();
    if ( !loadSvg( m_themeFile ))
        warning() << "Unable to load theme file: " << m_themeFile;
    Q_EMIT retinted();
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
    discardCache();
}

void SvgHandler::discardCache()
{
    //redraw entire app....
    reTint();
    m_cache->clear();

    if( auto window = pApp->mainWindow() )
        window->update();
}

QPixmap
SvgHandler::imageWithBorder( Meta::AlbumPtr album, int size, int borderWidth )
{
    const int imageSize = size - ( borderWidth * 2 );
    const QString &loc  = album->imageLocation( imageSize ).url();
    const QString &key  = !loc.isEmpty() ? loc : album->name();
    return addBordersToPixmap( The::coverCache()->getCover( album, imageSize ), borderWidth, key );
}

QPixmap SvgHandler::addBordersToPixmap( const QPixmap &orgPixmap, int borderWidth, const QString &name, bool skipCache )
{
    int newWidth = orgPixmap.width() + borderWidth * 2;
    int newHeight = orgPixmap.height() + borderWidth *2;

    QString key;
    if( !skipCache )
    {
        key = QStringLiteral("%1:%2x%3b%4")
            .arg( name )
            .arg( newWidth )
            .arg( newHeight )
            .arg( borderWidth );
    }

    QPixmap pixmap;
    if( skipCache || !m_cache->findPixmap( key, &pixmap ) )
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

        m_renderers[m_themeFile]->render( &pt, QStringLiteral("cover_border_topleft"), QRectF( 0, 0, borderWidth, borderWidth ) );
        m_renderers[m_themeFile]->render( &pt, QStringLiteral("cover_border_top"), QRectF( borderWidth, 0, orgPixmap.width(), borderWidth ) );
        m_renderers[m_themeFile]->render( &pt, QStringLiteral("cover_border_topright"), QRectF( newWidth - borderWidth , 0, borderWidth, borderWidth ) );
        m_renderers[m_themeFile]->render( &pt, QStringLiteral("cover_border_right"), QRectF( newWidth - borderWidth, borderWidth, borderWidth, orgPixmap.height() ) );
        m_renderers[m_themeFile]->render( &pt, QStringLiteral("cover_border_bottomright"), QRectF( newWidth - borderWidth, newHeight - borderWidth, borderWidth, borderWidth ) );
        m_renderers[m_themeFile]->render( &pt, QStringLiteral("cover_border_bottom"), QRectF( borderWidth, newHeight - borderWidth, orgPixmap.width(), borderWidth ) );
        m_renderers[m_themeFile]->render( &pt, QStringLiteral("cover_border_bottomleft"), QRectF( 0, newHeight - borderWidth, borderWidth, borderWidth ) );
        m_renderers[m_themeFile]->render( &pt, QStringLiteral("cover_border_left"), QRectF( 0, borderWidth, borderWidth, orgPixmap.height() ) );
    
        if( !skipCache )
            m_cache->insertPixmap( key, pixmap );
    }

    return pixmap;
}


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

    p->setRenderHint( QPainter::SmoothPixmapTransform );
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
                p->drawPixmap( pt, renderSvg( QStringLiteral("moodbar_end_left"), sliderHeight / 2, sliderHeight, QStringLiteral("moodbar_end_left") ) );

                pt.rx() += sliderHeight / 2;
                p->drawPixmap( pt, moodbar );

                pt.rx() += slider->rect.width() - sliderHeight;
                p->drawPixmap( pt, renderSvg( QStringLiteral("moodbar_end_right"), sliderHeight / 2, sliderHeight, QStringLiteral("moodbar_end_right") ) );

                moodbarPainted = true;
            }
        }
    }

    if( !moodbarPainted )
    {
        // Draw the slider background in 3 parts

        p->drawPixmap( pt, renderSvg( QStringLiteral("progress_slider_left"), sliderHeight, sliderHeight, QStringLiteral("progress_slider_left") ) );

        pt.rx() += sliderHeight;
        QRect midRect(pt, QSize(slider->rect.width() - sliderHeight * 2, sliderHeight) );
        p->drawTiledPixmap( midRect, renderSvg( QStringLiteral("progress_slider_mid"), 32, sliderHeight, QStringLiteral("progress_slider_mid") ) );

        pt = midRect.topRight() + QPoint( 1, 0 );
        p->drawPixmap( pt, renderSvg( QStringLiteral("progress_slider_right"), sliderHeight, sliderHeight, QStringLiteral("progress_slider_right") ) );

        //draw the played background.

        int playedBarHeight = sliderHeight - 6;

        int sizeOfLeftPlayed = qBound( 0, inverse ? slider->rect.right() - knob.right() + 2 :
                                                    knob.x() - 2, playedBarHeight );

        if( sizeOfLeftPlayed > 0 )
        {
            QPoint tl, br;
            if ( inverse )
            {
                tl = knob.topRight() + QPoint( -5, 5 ); // 5px x padding to avoid a "gap" between it and the top and bottom of the round knob.
                br = slider->rect.topRight() + QPoint( -3, 5 + playedBarHeight - 1 );
                QPixmap rightEnd = renderSvg( QStringLiteral("progress_slider_played_right"), playedBarHeight, playedBarHeight, QStringLiteral("progress_slider_played_right") );
                p->drawPixmap( br.x() - rightEnd.width() + 1, tl.y(), rightEnd, qMax(0, rightEnd.width() - (sizeOfLeftPlayed + 3)), 0, sizeOfLeftPlayed + 3, playedBarHeight );
                br.rx() -= playedBarHeight;
            }
            else
            {
                tl = slider->rect.topLeft() + QPoint( 3, 5 );
                br = QPoint( knob.x() + 5, tl.y() + playedBarHeight - 1 );
                QPixmap leftEnd = renderSvg( QStringLiteral("progress_slider_played_left"), playedBarHeight, playedBarHeight, QStringLiteral("progress_slider_played_left") );
                p->drawPixmap( tl.x(), tl.y(), leftEnd, 0, 0, sizeOfLeftPlayed + 3, playedBarHeight );
                tl.rx() += playedBarHeight;
            }
            if ( sizeOfLeftPlayed == playedBarHeight )
                p->drawTiledPixmap( QRect(tl, br), renderSvg( QStringLiteral("progress_slider_played_mid"), 32, playedBarHeight, QStringLiteral("progress_slider_played_mid") ) );

        }
    }

    if ( slider->state & QStyle::State_Enabled )
    {   // Draw the knob (handle)
        QString string = ( slider->activeSubControls & QStyle::SC_SliderHandle ) ?
                             QStringLiteral("slider_knob_200911_active") : QStringLiteral("slider_knob_200911");
        p->drawPixmap( knob.topLeft(), renderSvg( string, knob.width(), knob.height(), string ) );
    }
}

