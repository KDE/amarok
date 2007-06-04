/***************************************************************************
 * copyright            : (C) 2007 Seb Ruiz <ruiz@kde.org>                 *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_ALBUMBOX_H
#define AMAROK_ALBUMBOX_H

#include "contextbox.h"
#include <QPixmap>

class QGraphicsItem;
class QGraphicsPixmapItem;
class QGraphicsScene;

namespace Context
{
    /*
     * A group of items for displaying a "row" in the box, consisting of an image
     * and some sort of descriptive text
     */
    class AlbumItem : public QGraphicsItemGroup
    {
        public:
            explicit AlbumItem( QGraphicsItem *parent = 0, QGraphicsScene *scene = 0 );
            ~AlbumItem() { }

            void setCover( const QString &location );
            void setCover( const QPixmap &image );

            void setText( const QString &text );

            qreal bottom();

        private:
            QGraphicsPixmapItem *m_coverItem;
            QGraphicsTextItem   *m_textItem;
    };

    /*
     * Contains a bunch of those AlbumItem groups in a table like format, one on
     * each row
     */
    class AlbumBox : public ContextBox
    {
        public:
            explicit AlbumBox( QGraphicsItem *parent = 0, QGraphicsScene *scene = 0 );
            ~AlbumBox() { /* delete, disconnect and disembark */ }

            void addAlbumInfo( const QString &pixLocation, const QString &text );

        private:
            qreal m_bottom;
    };

}

#endif

