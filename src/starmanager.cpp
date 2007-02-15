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
#include "debug.h"
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


//nothing to initialize here, reinitStars will be called by Playlist constructor

StarManager::StarManager() {}

StarManager::~StarManager() {}

void
StarManager::reinitStars( int height, int margin )
{
    int hval = height + margin * 2 - 4 + ( ( height % 2 ) ? 1 : 0 );
    QImage star = QImage( locate( "data", "amarok/images/star.png" ) ).smoothScale( hval, hval, QImage::ScaleMin );
    m_star = star.copy();
    m_starPix.convertFromImage( star );
    QImage greystar = QImage( star );
    KIconEffect::toGray( greystar, 1.0 );
    m_greyedStar = greystar.copy();
    m_greyedStarPix.convertFromImage( greystar );
    QImage half = QImage( locate( "data", "amarok/images/smallstar.png" ) ).smoothScale( hval, hval, QImage::ScaleMin );
    KIconEffect::colorize( half, QColor( 168, 0, 172 ), 0.5 );
    m_halfStar = half.copy();
    m_halfStarPix.convertFromImage( half );

    QImage tempstar;
    QImage temphalfstar;
    for( int i = 1; i < 6; i++ )
    {
        tempstar = star.copy();
        switch( i )
        {
            case 1:
                KIconEffect::colorize( tempstar, QColor( 0, 255, 0 ), 1.0 );
                m_oneStar = tempstar.copy();
                m_oneStarPix.convertFromImage( tempstar );
                break;
            case 2:
                KIconEffect::colorize( tempstar, QColor( 168, 0, 172 ), 1.0 );
                m_twoStar = tempstar.copy();
                m_twoStarPix.convertFromImage( tempstar );
                break;
            case 3:
                KIconEffect::colorize( tempstar, QColor( 255, 0, 0 ), 1.0 );
                m_threeStar = tempstar.copy();
                m_threeStarPix.convertFromImage( tempstar );
                break;
            case 4:
                KIconEffect::colorize( tempstar, QColor( 0, 0, 255  ), 1.0 );
                m_fourStar = tempstar.copy();
                m_fourStarPix.convertFromImage( tempstar );
                break;
            case 5:
                KIconEffect::colorize( tempstar, QColor( 168, 172, 0 ), 1.0 );
                m_fiveStar = tempstar.copy();
                m_fiveStarPix.convertFromImage( tempstar );
                break;
            default:
                break;
        }
        tempstar.reset();
    }
}

QPixmap*
StarManager::getStar( int num )
{
    switch( num )
    {
        case 1:
            return &m_oneStarPix;
        case 2:
            return &m_twoStarPix;
        case 3:
            return &m_threeStarPix;
        case 4:
            return &m_fourStarPix;
        case 5:
            return &m_fiveStarPix;
        default:
            return &m_starPix;
    }
}

QImage&
StarManager::getStarImage( int num )
{
    switch( num )
    {
        case 1:
            return m_oneStar;
        case 2:
            return m_twoStar;
        case 3:
            return m_threeStar;
        case 4:
            return m_fourStar;
        case 5:
            return m_fiveStar;
        default:
            return m_star;
    }
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

#include "starmanager.moc"
