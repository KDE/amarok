/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#ifndef POPUPDROPPERFACTORY_H
#define POPUPDROPPERFACTORY_H

#include "amarok_export.h"
#include "context/popupdropper/libpud/PopupDropper.h"

/**
A central place for creating a Pud that matches system colors

	@author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class PopupDropperFactory;
class PopupDropperItem;

namespace The {
    AMAROK_EXPORT PopupDropperFactory* popupDropperFactory();
}

class AMAROK_EXPORT PopupDropperFactory : public QObject
{
    Q_OBJECT

    friend PopupDropperFactory* The::popupDropperFactory();

    public:
        /**
         * Creates a new PopupDropper with correct system colors. This function creates it on top of the context view
         * Function for creating a new PopupDropper with a custom parent
         * @param parent The widget to act as the parent
         * @param ignoreEmptyParent Whether to ignore if the parent is null - use this when creating submenus where the parent is 0
         * @return The newly created PopupDropper
         */
        PopupDropper * createPopupDropper( QWidget * parent, bool ignoreEmptyParent = false );

        PopupDropperItem* createItem( QAction * action );
        void adjustItems( PopupDropper *pud );
        void adjustItem( PopupDropperItem *item );
        static void adjustItemCallback( void *pdi );

    private:
        explicit PopupDropperFactory( QObject* parent );
        ~PopupDropperFactory() override;
 };


#endif
