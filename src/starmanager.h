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

        QPixmap* getStar( int num, bool full = false );
        QPixmap* getGreyStar() { return &m_greyedStarPix; }
        QPixmap* getHalfStar( int num = -1, bool full = false  );
        QImage& getStarImage( int num, bool full = false  );
        QImage& getGreyStarImage() { return m_greyedStar; }
        QImage& getHalfStarImage( int num = -1, bool full = false  );

        bool setColor( int starNum, const QColor &color );
        bool setHalfColor( const QColor &color );

        void reinitStars( int height = -1, int margin = -1 );

    signals:
        void ratingsColorsChanged();

    private:

        int m_height;
        int m_margin;

        //cached stars...why both?  For faster conversion when drawing context browser
        QPixmap m_starPix;
        QImage m_star;
        QPixmap m_fullStarPix;
        QImage m_fullStar;
        QPixmap m_greyedStarPix;
        QImage m_greyedStar;
        QPixmap m_halfStarPix;
        QPixmap m_fullHalfStarPix;
        QImage m_halfStar;
        QImage m_fullHalfStar;

        QImage m_images[5];
        QImage m_halfimages[5];
        QPixmap m_pixmaps[5];
        QPixmap m_halfpixmaps[5];

        QColor m_colors[5];
        QColor m_halfStarColor;
};

#endif

