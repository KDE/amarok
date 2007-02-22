//
// C++ Implementation: starmanager
//
// Description: helper to give correct stars
//
//
// Author: Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "amarok.h"
#include "amarokconfig.h"
#include "metabundle.h"
#include "starmanager.h"

#include <kiconeffect.h>

#include <qfile.h>
#include <qimage.h>
#include <qpixmap.h>

#include <kstandarddirs.h>   //KGlobal::dirs()

StarManager* StarManager::instance()
{
    static StarManager sm;
    return &sm;
}


StarManager::StarManager()
{
    m_colors[0] = QColor( 0, 255, 0 );
    m_colors[1] = QColor( 168, 0, 172 );
    m_colors[2] = QColor( 255, 0, 0 );
    m_colors[3] = QColor( 0, 0, 255 );
    m_colors[4] = QColor( 168, 172, 0 );
    m_halfStarColor = QColor( 0, 168, 172 );
    m_margin = 1;
    m_height = 20;
}

StarManager::~StarManager() {}

void
StarManager::reinitStars( int height, int margin )
{
    if( height != -1 )
        m_height = height;
    if( margin != -1 )
        m_margin = margin;

    int hval = m_height + m_margin * 2 - 4 + ( ( m_height % 2 ) ? 1 : 0 );
    QImage star = QImage( locate( "data", "amarok/images/star.png" ) ).smoothScale( hval, hval, QImage::ScaleMin );
    m_star = star.copy();
    m_starPix.convertFromImage( star );
    QImage greystar = QImage( star );
    KIconEffect::toGray( greystar, 1.0 );
    m_greyedStar = greystar.copy();
    m_greyedStarPix.convertFromImage( greystar );
    QImage half = QImage( locate( "data", "amarok/images/smallstar.png" ) ).smoothScale( hval, hval, QImage::ScaleMin );
    m_halfStar = half.copy();
    if( AmarokConfig::customRatingsColors() )
        KIconEffect::colorize( m_halfStar, m_halfStarColor, 1.0 );
    m_halfStarPix.convertFromImage( m_halfStar );

    QImage tempstar;
    QImage temphalfstar;
    for( int i = 0; i < 5; i++ )
    {
        tempstar = star.copy();
        if( AmarokConfig::customRatingsColors() )
        {
            KIconEffect::colorize( tempstar, m_colors[i], 1.0 );
        }
        m_images[i] = tempstar.copy();
        m_pixmaps[i].convertFromImage( tempstar );
        tempstar.reset();
    }
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
StarManager::getHalfStar( int /*num*/  )
{
    return &m_halfStarPix;
}

QImage&
StarManager::getHalfStarImage( int /*num*/  )
{
    return m_halfStar;
}

bool
StarManager::setColor( int starNum, const QColor &color )
{
    if( starNum < 1 || starNum > 5 )
        return false;
    m_colors[starNum - 1] = color;
    reinitStars();
    return true;
}

bool
StarManager::setHalfColor( const QColor &color )
{
    m_halfStarColor = color;
    reinitStars();
    return true;
}

#include "starmanager.moc"
