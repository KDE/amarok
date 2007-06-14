/***************************************************************************
 * copyright     : (C) 2007 Seb Ruiz <ruiz@kde.org>                        *
 *                 (C) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> *
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
#include "engineobserver.h"

#include <QGraphicsView>

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

        void addContextBox( QGraphicsItem *newBox, int after = -1 /*which position to place the new box*/, bool fadeIn = false);

    protected:
        void engineNewMetaData( const MetaBundle&, bool );
        void engineStateChanged( Engine::State, Engine::State = Engine::Empty );
        void resizeEvent( QResizeEvent *event );
        void wheelEvent( QWheelEvent *event );

    private:
        /*
         * Creates a new context view widget with parent \p parent
         * Constructor is private since the view is a singleton class
         */
        ContextView();

        void initiateScene();

        void scaleView( qreal factor );
        static bool higherThan( const QGraphicsItem *i1, const QGraphicsItem *i2 );

        /// Page Views ////////////////////////////////////////
        void showHome();
        void showCurrentTrack();


        /// Attributes ////////////////////////////////////////
        QGraphicsScene *m_contextScene; ///< Pointer to the scene which holds all our items

    private slots:

        void introAnimationComplete();

};

#endif
