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
#include "collectionbrowser.h"
#include "contextbrowser.h"
#include "debug.h"
#include "metabundle.h"
#include "playlist.h"
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
    if( AmarokConfig::customRatingsColors() )
        AmarokConfig::setCustomRatingsColors( false );
    m_colors[0] = AmarokConfig::starColorOne();
    m_colors[1] = AmarokConfig::starColorTwo();
    m_colors[2] = AmarokConfig::starColorThree();
    m_colors[3] = AmarokConfig::starColorFour();
    m_colors[4] = AmarokConfig::starColorFive();
    m_halfStarColor = AmarokConfig::starColorHalf();
    m_margin = 1;
    m_height = 20;
    reinitStars();
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
    QImage fullStar = QImage( locate( "data", "amarok/images/star.png" ) );
    m_star = star.copy();
    m_fullStar = fullStar.copy();
    m_starPix.convertFromImage( star );
    m_fullStarPix.convertFromImage( fullStar );
    m_greyedStar = star.copy();
    KIconEffect::toGray( m_greyedStar, 1.0 );
    m_greyedStarPix.convertFromImage( m_greyedStar );
    QImage half = QImage( locate( "data", "amarok/images/smallstar.png" ) ).smoothScale( hval, hval, QImage::ScaleMin );
    QImage fullHalf = QImage( locate( "data", "amarok/images/smallstar.png" ) );
    m_halfStar = half.copy();
    m_fullHalfStar = fullHalf.copy();
    if( AmarokConfig::customRatingsColors() )
        KIconEffect::colorize( m_halfStar, m_halfStarColor, 1.0 );
    m_halfStarPix.convertFromImage( m_halfStar );
    m_fullHalfStarPix.convertFromImage( m_fullHalfStar );

    QImage tempstar;
    QImage temphalfstar;
    for( int i = 0; i < 5; i++ )
    {
        tempstar = star.copy();
        temphalfstar = half.copy();
        if( AmarokConfig::customRatingsColors() )
        {
            KIconEffect::colorize( tempstar, m_colors[i], 1.0 );
            if( !AmarokConfig::fixedHalfStarColor() )
                KIconEffect::colorize( temphalfstar, m_colors[i], 1.0 );
        }
        m_images[i] = tempstar.copy();
        m_halfimages[i] = temphalfstar.copy();
        m_pixmaps[i].convertFromImage( tempstar );
        m_halfpixmaps[i].convertFromImage( temphalfstar );
        tempstar.reset();
        temphalfstar.reset();
    }
    if( Playlist::instance() ) Playlist::instance()->qscrollview()->viewport()->update();
    if( CollectionView::instance() &&
            CollectionView::instance()->viewMode() == CollectionView::modeFlatView )
        CollectionView::instance()->triggerUpdate();
    emit ratingsColorsChanged();
}

QPixmap*
StarManager::getStar( int num, bool full )
{
    if(full)
	return &m_fullStarPix;
    else if( num < 1 || num > 5 )
        return &m_starPix;
    else
        return &m_pixmaps[num - 1];
}

QImage&
StarManager::getStarImage( int num, bool full )
{
    if(full)
        return m_fullStar;
    else if( num < 1 || num > 5 )
        return m_star;
    else
        return m_images[num - 1];
}

QPixmap*
StarManager::getHalfStar( int num, bool full )
{
    if( full )
        return &m_fullHalfStarPix;
    else if( AmarokConfig::fixedHalfStarColor() || num == -1 )
        return &m_halfStarPix;
    else
        return &m_halfpixmaps[num - 1];
}

QImage&
StarManager::getHalfStarImage( int num, bool full )
{
    if( full )
        return m_fullHalfStar;
    else if( AmarokConfig::fixedHalfStarColor() || num == -1 )
        return m_halfStar;
    else
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

#include "starmanager.moc"
