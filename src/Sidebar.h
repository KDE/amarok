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

#include <KHBox>
#include <KIcon>

#include <QPushButton>
#include <QIcon>
#include <QPointer>
#include <QWidget>
#include <QStackedWidget>

/**
 * A widget with multiple stacked pages, accessible by vertical tabs along 
 * one side.  The text is laid out rotated 90 degrees from horizontal,
 * with the lettering traveling from bottom to top
 *
 * @author Gábor Lehel <illissius@gmail.com>
*/
class SideBar: public KHBox
{
    typedef KHBox super;
    Q_OBJECT

    public:
        /**
         * Creates a new Sidebar that contains multiple browser tabs.
         *
         * @param parent The parent widget that this sidebar belongs to
         * @param contentsWidget The widget that this sidebar places all of its children in
         */
        explicit SideBar( QWidget *parent )
            : KHBox( parent )
        {
            m_backButton = new QPushButton( this );
            m_categoryList = new BrowserCategoryList( this, "root list" );
            
            m_backButton->setFixedWidth( 20 );
            m_backButton->setIcon( KIcon( "go-previous-amarok" ) );
            
            m_categoryList->setMinimumSize( 100, 300 );
            
            m_backButton->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding );
            //m_categoryList->setSizePolicy( QSizePolicy::Expanding );

            connect( m_backButton, SIGNAL( clicked( bool ) ), m_categoryList, SLOT( back() ) );
            setFrameShape( QFrame::StyledPanel );
            setFrameShadow( QFrame::Sunken );
        }

        ~SideBar()
        {
            DEBUG_BLOCK
        }

        QPushButton *backButton() const { return m_backButton; }
        BrowserCategoryList *list() const { return m_categoryList; }

    private:
        QPointer<QPushButton> m_backButton;
        QPointer<BrowserCategoryList> m_categoryList;


};

#endif
