/*
  Copyright (c) 2006 Gábor Lehel <illissius@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef AMAROK_SIDEBAR_H
#define AMAROK_SIDEBAR_H

#include "browsers/BrowserCategoryList.h"

#include "Debug.h"
#include "widgets/SidebarWidget.h"

#include <KHBox>

#include <QIcon>
#include <QFrame>
#include <QLayout>
#include <QPointer>
#include <QStackedWidget>

/**
 * A widget with multiple stacked pages, accessible by vertical tabs along 
 * one side.  The text is laid out rotated 90 degrees from horizontal,
 * with the lettering traveling from bottom to top
 *
 * @author Gábor Lehel <illissius@gmail.com>
*/
class SideBar: public BrowserCategoryList
{
    typedef BrowserCategoryList super;
    Q_OBJECT

    public:
        /**
         * Creates a new Sidebar that contains multiple browser tabs.
         *
         * @param parent The parent widget that this sidebar belongs to
         * @param contentsWidget The widget that this sidebar places all of its children in
         */
        explicit SideBar( QWidget *parent )
            : super( parent, QString() )
            , m_bar( new SideBarWidget( this ) )
        {}

        ~SideBar()
        {
            DEBUG_BLOCK
        }

        SideBarWidget *sideBarWidget() const { return m_bar; }

    private:
        QPointer<SideBarWidget> m_bar;

};

#endif
