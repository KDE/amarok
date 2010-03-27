/****************************************************************************************
 * Copyright (c) 2007 Jeff Mitchell <kde-dev@emailgoeshere.com>                         *
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

#include "StarManager.h"

#include "Amarok.h"
#include <config-amarok.h>  
#include "core/support/Debug.h"
#include "MainWindow.h"

#include <KIconEffect>
#include <KStandardDirs>   //KGlobal::dirs()

#include <QImage>
#include <QPixmap>



StarManager* StarManager::s_instance = 0;


StarManager* StarManager::instance()
{
    return s_instance ? s_instance : new StarManager( The::mainWindow() );
}


StarManager::StarManager( QObject* parent )
    : QObject( parent )
{
    DEBUG_BLOCK

    s_instance = this;

    /*if( AmarokConfig::customRatingsColors() )
        AmarokConfig::setCustomRatingsColors( false );
    m_colors[0] = AmarokConfig::starColorOne();
    m_colors[1] = AmarokConfig::starColorTwo();
    m_colors[2] = AmarokConfig::starColorThree();
    m_colors[3] = AmarokConfig::starColorFour();
    m_colors[4] = AmarokConfig::starColorFive();
    m_halfStarColor = AmarokConfig::starColorHalf();*/
    m_margin = 1;
    m_height = 20;
    reinitStars();
}

StarManager::~StarManager()
{
    DEBUG_BLOCK
}

void
StarManager::reinitStars( int height, int margin )
{
    if( height != -1 )
        m_height = height;
    if( margin != -1 )
        m_margin = margin;

    int hval = m_height + m_margin * 2 - 4 + ( ( m_height % 2 ) ? 1 : 0 );
    QImage star = QImage( KStandardDirs::locate( "data", "amarok/images/star.png" ) ).scaled( hval, hval, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    m_star = star.copy();
    m_starPix = QPixmap::fromImage( star );
    m_greyedStar = star.copy();
    KIconEffect::toGray( m_greyedStar, 1.0 );
    m_greyedStarPix = QPixmap::fromImage( m_greyedStar );
    QImage half = QImage( KStandardDirs::locate( "data", "amarok/images/smallstar.png" ) ).scaled( hval, hval, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    m_halfStar = half.copy();
    /*if( AmarokConfig::customRatingsColors() )
        KIconEffect::colorize( m_halfStar, m_halfStarColor, 1.0 );*/
    m_halfStarPix = QPixmap::fromImage( m_halfStar );

    QImage tempstar;
    QImage temphalfstar;
    for( int i = 0; i < 5; i++ )
    {
        tempstar = star.copy();
        temphalfstar = half.copy();
        /*if( AmarokConfig::customRatingsColors() )
        {
            KIconEffect::colorize( tempstar, m_colors[i], 1.0 );
            if( !AmarokConfig::fixedHalfStarColor() )
                KIconEffect::colorize( temphalfstar, m_colors[i], 1.0 );
        }*/
        m_images[i] = tempstar.copy();
        m_halfimages[i] = temphalfstar.copy();
        m_pixmaps[i] = QPixmap::fromImage( tempstar );
        m_halfpixmaps[i] = QPixmap::fromImage( temphalfstar );
        tempstar = QImage();
        temphalfstar = QImage();
    }
    //TODO:PORT
//     if( Playlist::instance() ) Playlist::instance()->qscrollview()->viewport()->update();
/*PORT 2.0
    if( CollectionView::instance() &&
            CollectionView::instance()->viewMode() == CollectionView::modeFlatView )
        CollectionView::instance()->triggerUpdate(); */
    emit ratingsColorsChanged();
}

QPixmap*
StarManager::getStar( int num )
{
    if( num < 1 || num > 5 )
        return &m_starPix;
    else
        return &m_pixmaps[num - 1];
}

QImage&
StarManager::getStarImage( int num )
{
    if( num < 1 || num > 5 )
        return m_star;
    else
        return m_images[num - 1];
}

QPixmap*
StarManager::getHalfStar( int num )
{
    /*if( AmarokConfig::fixedHalfStarColor() || num == -1 )
        return &m_halfStarPix;
    else*/
        return &m_halfpixmaps[num - 1];
}

QImage&
StarManager::getHalfStarImage( int num  )
{
    /*if( AmarokConfig::fixedHalfStarColor() || num == -1 )
        return m_halfStar;
    else*/
        return m_halfimages[num - 1];
}

bool
StarManager::setColor( int starNum, const QColor &color )
{
    if( starNum < 1 || starNum > 5 )
        return false;
    m_colors[starNum - 1] = color;
    return true;
}

bool
StarManager::setHalfColor( const QColor &color )
{
    m_halfStarColor = color;
    return true;
}

#include "StarManager.moc"

