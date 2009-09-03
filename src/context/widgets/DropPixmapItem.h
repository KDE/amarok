/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
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

#ifndef DROPPIXMAPITEM_H
#define DROPPIXMAPITEM_H

#include "amarok_export.h"

#include <QGraphicsPixmapItem>

//forward
class QGraphicsSceneDragDropEvent;
class KJob;

/**
* \brief A QGraphicsPixmapItem on which you can drop an image
*
* Used for drag'n drop support for the cover. Will download the file if it's a link (from webrowser)
*
* \sa QGraphicsPixmapItem
*
* \author Simon Esneault <simon.esneault@gmail.com>
*/

class AMAROK_EXPORT DropPixmapItem : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
    public:
        
        DropPixmapItem( QGraphicsItem* parent = 0 );

    signals:
        void imageDropped( QPixmap );
        
    public slots:
        /**
        * Result of the image fetching stuff
        */
        void imageDownloadResult( KJob * );
        
    protected slots:
        /**
        * Reimplement dropEvent
        */
        virtual void dropEvent( QGraphicsSceneDragDropEvent* );

    private:
        KJob *m_job;

};

#endif // DROPPIXMAPITEM_H
