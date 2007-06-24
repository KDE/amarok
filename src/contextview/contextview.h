/***************************************************************************
 * copyright     : (C) 2007 Seb Ruiz <ruiz@kde.org>                        *
 *                 (C) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> *
 *                :(C) 2007 Leonardo Franchi  <lfranchi@gmail.com>         *
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
#include "ContextItem.h"
#include "engineobserver.h"
#include "GenericInfoBox.h"

#include <QGraphicsSvgItem>
#include <QGraphicsView>
#include <QPointer>

class QGraphicsScene;
class QResizeEvent;
class QWheelEvent;

using namespace Context;

class ContextView : public QGraphicsView, public EngineObserver
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

        // this registers the item as populating the ContextView, so it gets messages etc. the item should then call {add/remove}ContextBox to actually populate the CV
        void addContextItem( ContextItem* i );
        void removeContextItem( ContextItem* i );

        void addContextBox( ContextBox *newBox, int index = -1 /*which position to place the new box*/, bool fadeIn = false, ContextItem* parent = 0);
        // add and remove take a ContextItem parent which dictates the ownership
        // of the box (if they are owned by an item)
        void removeContextBox( ContextBox *oldBox, bool fadeOut = false, ContextItem* parent = 0);

    protected:
        void engineNewMetaData( const MetaBundle&, bool );
        void engineStateChanged( Engine::State, Engine::State = Engine::Empty );
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

        void shuffleItems( QList<QGraphicsItem*> items, qreal distance, int direction );

        void notifyItems( const QString& message );

        /// Page Views ////////////////////////////////////////
        void showHome();
        void showCurrentTrack();


        /// Attributes ////////////////////////////////////////
        QGraphicsScene *m_contextScene;

        QList<ContextItem*> m_contextItems;
        QMap<qreal, ContextBox*> m_contextBoxes; // holds an ordered list of the items, from top to bottom

        // this keeps track of all items that are owned by each context item
        QMap< ContextItem*, QList< QGraphicsItem* >* > m_contextItemMap;

        ContextBox *m_testItem;

    private slots:
        void introAnimationComplete();
        void testBoxLayout();
};

#endif
