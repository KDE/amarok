/***************************************************************************
 * copyright     : (C) 2007 Seb Ruiz <ruiz@kde.org>                        *
 *                 (C) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> *
 *                :(C) 2007 Leonardo Franchi  <lfranchi@gmail.com>         *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_CONTEXTVIEW_H
#define AMAROK_CONTEXTVIEW_H

#include "contextbox.h"
#include "ContextObserver.h"
#include "engineobserver.h"
#include "GenericInfoBox.h"
#include "graphicsitemfader.h"

#include <QGraphicsSvgItem>
#include <QGraphicsView>
#include <QPointer>

class QGraphicsScene;
class QMouseEvent;
class QResizeEvent;
class QWheelEvent;

using namespace Context;

class ContextView : public QGraphicsView, public EngineObserver, public ContextSubject 
{
    Q_OBJECT
    static ContextView *s_instance;

    public:

        static const int BOX_PADDING = 20;
        ~ContextView() { /* delete, disconnect and disembark */ }

        static ContextView *instance()
        {
            if( !s_instance )
                return new ContextView();
            return s_instance;
        }

        void clear();

        void addContextBox( ContextBox *newBox, int index = -1 /*which position to place the new box*/, bool fadeIn = false );
        
        void removeContextBox( ContextBox *oldBox, bool fadeOut = false );

        void showPopupDropper();
        void hidePopupDropper();

    protected:
        void engineNewMetaData( const MetaBundle&, bool );
        void engineStateChanged( Engine::State, Engine::State = Engine::Empty );
        void mouseMoveEvent( QMouseEvent *event );
        void resizeEvent( QResizeEvent *event );
        void wheelEvent( QWheelEvent *event );


    private:
        enum ShuffleDirection { ShuffleUp, ShuffleDown };
        /**
         * Creates a new context view widget with parent \p parent
         * Constructor is private since the view is a singleton class
         */
        ContextView();

        void initiateScene();

        void scaleView( qreal factor );
        static bool boxHigherThan( const ContextBox *c1, const ContextBox *c2 );

        void shuffleItems( QList<QGraphicsItem*> items, qreal distance, int direction = ShuffleDown );

        /// Page Views ////////////////////////////////////////
        void showHome();
        void showCurrentTrack();


        /// Attributes ////////////////////////////////////////
        QGraphicsScene *m_contextScene;

        QList<ContextBox*>  m_contextBoxes; // holds an ordered list of the items, from top to bottom

        ContextBox *m_testItem;

        QList<GraphicsItemFader*> m_pudFaders;
        bool m_pudShown;

    private slots:
        void introAnimationComplete();
        void boxHeightChanged(qreal change);
        void testBoxLayout();
};

#endif
