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

#include <QGraphicsView>
#include <QGraphicsScene>


class ContextView : public QGraphicsView
{

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

    private:
        /*
         * Creates a new context view widget with parent \p parent
         * Constructor is private since the view is a singleton class
         */
        ContextView();

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

};

#endif
