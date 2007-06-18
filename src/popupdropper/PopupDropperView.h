/*
 *  Copyright (c) 2007 Jeff Mitchell <kde-dev@emailgoeshere.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef AMAROK_POPUPDROPPER_VIEW_H
#define AMAROK_POPUPDROPPER_VIEW_H

#include "amarok_export.h"

#include <QGraphicsView>
#include <QObject>
#include <QtGlobal>

class QGraphicsScene;
class QPainter;
class QRectF;

/**
  * This class contructs the PopupDropperView
  * @author Jeff Mitchell <kde-dev@emailgoeshere.com>
  */

namespace PopupDropperNS {

    class PopupDropperView : public QGraphicsView
    {
        Q_OBJECT
    
        public:
    
            /**
            * Creates a new PopupDropperView.
            * 
            */
            PopupDropperView( QGraphicsScene *scene, QWidget *parent  );
            ~PopupDropperView();
    
            void drawBackground( QPainter *painter, const QRectF &rect );
    };
}
#endif /* AMAROK_POPUPDROPPER_VIEW_H */

