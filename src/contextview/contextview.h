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

#ifndef AMAROK_CONTEXTVIEW_H
#define AMAROK_CONTEXTVIEW_H

#include "contextbox.h"

#include <QGraphicsView>

class QGraphicsScene;
class QWheelEvent;

using namespace Context;

class ContextView : public QGraphicsView
{
    Q_OBJECT
    static ContextView *s_instance;

    public:
        /*
         * Destroys the context view
         */
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
        void wheelEvent( QWheelEvent *event );

    private:
        static const qreal BOX_PADDING = 20;
        /*
         * Creates a new context view widget with parent \p parent
         * Constructor is private since the view is a singleton class
         */
        ContextView();

        void initiateScene();

        void scaleView( qreal factor );
        static bool higherThan( const QGraphicsItem *i1, const QGraphicsItem *i2 );

        /// Page Views ////////////////////////////////////////
        //

        /*
         * Shows the introductory home page with a small number of statistics,
         * such as most recent and favourite albums
         */
        void showHome();


        /// Attributes ////////////////////////////////////////
        //
        QGraphicsScene *m_contextScene; ///< Pointer to the scene which holds all our items

    private slots:

        void introAnimationComplete();

};

#endif
