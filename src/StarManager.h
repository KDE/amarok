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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_STAR_MANAGER_H
#define AMAROK_STAR_MANAGER_H

#include <QImage>
#include <QPixmap>

class StarManager : public QObject
{
    Q_OBJECT

    public:
        static StarManager *instance();

        QPixmap* getStar( int num );
        QPixmap* getGreyStar() { return &m_greyedStarPix; }
        QPixmap* getHalfStar( int num = -1 );
        QImage& getStarImage( int num );
        QImage& getGreyStarImage() { return m_greyedStar; }
        QImage& getHalfStarImage( int num = -1 );

        bool setColor( int starNum, const QColor &color );
        bool setHalfColor( const QColor &color );

        void reinitStars( int height = -1, int margin = -1 );

    signals:
        void ratingsColorsChanged();

    private:
        StarManager( QObject* parent );
        ~StarManager();

        static StarManager* s_instance;

        int m_height;
        int m_margin;

        //cached stars...why both?  For faster conversion when drawing context browser
        QPixmap m_starPix;
        QImage m_star;
        QPixmap m_greyedStarPix;
        QImage m_greyedStar;
        QPixmap m_halfStarPix;
        QImage m_halfStar;

        QImage m_images[5];
        QImage m_halfimages[5];
        QPixmap m_pixmaps[5];
        QPixmap m_halfpixmaps[5];

        QColor m_colors[5];
        QColor m_halfStarColor;
};

#endif

