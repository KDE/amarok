//
// C++ Interface: starmanager
//
// Description: Small little manager to return the color stars we want
//
//
// Author: Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//


#ifndef AMAROK_STAR_MANAGER_H
#define AMAROK_STAR_MANAGER_H

#include <qimage.h>
#include <qpixmap.h>

class StarManager : public QObject
{

    Q_OBJECT

    public:
        StarManager();
        ~StarManager();
        static StarManager *instance();

        QPixmap* getStar( int num );
        QPixmap* getGreyStar() { return &m_greyedStarPix; }
        QPixmap* getHalfStar( int num = -1 );
        QImage& getStarImage( int num );
        QImage& getGreyStarImage() { return m_greyedStar; }
        QImage& getHalfStarImage( int num = -1 );

        void reinitStars( int height, int margin );

    private:

        //cached stars...why both?  For faster conversion when drawing context browser
        QPixmap m_starPix;
        QImage m_star;
        QPixmap m_greyedStarPix;
        QImage m_greyedStar;
        QPixmap m_halfStarPix;
        QImage m_halfStar;
        QPixmap m_oneStarPix;
        QImage m_oneStar;
        QPixmap m_twoStarPix;
        QImage m_twoStar;
        QPixmap m_threeStarPix;
        QImage m_threeStar;
        QPixmap m_fourStarPix;
        QImage m_fourStar;
        QPixmap m_fiveStarPix;
        QImage m_fiveStar;
};

#endif

