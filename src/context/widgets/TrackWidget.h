/*******************************************************************************
* copyright              : (C) 2008 William Viana Soares <vianasw@gmail.com>   *
* copyright              : (C) 2008 Mark Kretschmann <kretschmann@kde.org>     *
*                                                                              *
********************************************************************************/

/*******************************************************************************
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
********************************************************************************/

#ifndef TRACK_WIDGET_H
#define TRACK_WIDGET_H

#include "amarok_export.h"
#include "meta/Meta.h"
#include "RatingWidget.h"
#include "ToolBoxIcon.h"

#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

/**
 * @class TrackWidget
 * @short A widget to show track information
 */
class AMAROK_EXPORT TrackWidget: public ToolBoxIcon
{
    Q_OBJECT

    static const int PADDING = 15;

    public:
        TrackWidget( QGraphicsItem *parent = 0 );
        ~TrackWidget();
      
        void hide();

        /**
        * Sets the track to show.
        * @param track The track pointer to associate this widget with.
        */
        void setTrack( Meta::TrackPtr track );
        
        /**
        * @return the track associated with this widget.
        */
        Meta::TrackPtr track() const;        
        
    protected:
        virtual void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0 );
        virtual void mouseReleaseEvent( QGraphicsSceneMouseEvent *event );
        
    private slots:
        /**
        * Changes the rating of the track associated to the widget.
        * @param rating the new rating to give to the track.
        */
        void changeTrackRating( int rating );
        
    private:
        Meta::TrackPtr m_track;
        RatingWidget *m_rating;    
};

#endif
