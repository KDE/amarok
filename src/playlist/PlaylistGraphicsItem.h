/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "meta.h"
#include <QGraphicsItem>

class QFontMetricsF;

namespace PlaylistNS {

    class GraphicsItem : public QGraphicsItem
    {
        class ActiveItems;
        public:
            GraphicsItem();
            ~GraphicsItem();
            void paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget );
            QRectF boundingRect() const;
            void setupItem();
            static qreal height() { return s_height; }
        protected:
            void mouseDoubleClickEvent( QGraphicsSceneMouseEvent* event );
            void dragEnterEvent( QGraphicsSceneDragDropEvent *event );
            void dropEvent( QGraphicsSceneDragDropEvent * event );
        private:
            void init( Meta::TrackPtr track );
            void resize( Meta::TrackPtr track, int totalWidth );
            int getRow() const { return int( ( mapToScene( 0.0, 0.0 ).y() ) / s_height ); }
            ActiveItems* m_items;
            static const qreal ALBUM_WIDTH;
            static const qreal MARGIN;
            static qreal s_height;
            static QFontMetricsF* s_fm;
    };

}
