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
class SideBar: public KHBox
{
    typedef KHBox super;
    Q_OBJECT

    signals:
        /**
         * This signal is emitted whenever the active page in the sidebar 
         * is changed.  The active page is in the slot denoted by by @p index.
         * If @p index is -1, the browser has been minimized.
         */
        void widgetActivated( int index );
        /**
         * This signal is emitted whenever the active page in the sidebar 
         * is changed.  The active page is the @p widget being emitted
         */
        void widgetActivated( QWidget *widget );

    public:
        /**
         * Creates a new Sidebar that contains multiple browser tabs.
         *
         * @param parent The parent widget that this sidebar belongs to
         * @param contentsWidget The widget that this sidebar places all of its children in
         */
        explicit SideBar( QWidget *parent, QWidget *contentsWidget = 0 )
            : super( parent )
            , m_bar( new SideBarWidget( this ) )
            , m_frame( new KHBox( this ) )
            , m_widgets( new QStackedWidget( m_frame ) )
            , m_current( -1 )
        {
            connect( m_bar, SIGNAL( opened( int ) ), SLOT( openWidget( int ) ) );
            connect( m_bar, SIGNAL( closed() ), SLOT( closeWidgets() ) );
            m_frame->hide();
            m_widgets->setParent( m_frame );
            setContentsWidget( contentsWidget );
            layout()->setContentsMargins( 0, 0, 0, 0 );
        }

        ~SideBar()
        {
            DEBUG_BLOCK
        }

        void deleteBrowsers()
        {
            DEBUG_BLOCK

            QWidget * widget = m_widgets->widget( 0 );
            while( widget )
            {
                m_widgets->removeWidget( widget );
                delete widget;
                widget = m_widgets->widget( 0 );
            }
        }

        void setContentsWidget( QWidget *w )
        {
            m_contentsWidget = w;
            if( w )
                w->setParent( this );
        }
        SideBarWidget *sideBarWidget() const { return m_bar; }

        QWidget *contentsWidget() const { return m_contentsWidget; }

        int addWidget( const QIcon &icon, const QString &name, QWidget *widget )
        {
            m_widgets->addWidget( widget );
            m_bar->addSideBar( icon, name );
            return m_widgets->count() - 1;
        }

        void removeWidget( QWidget *widget )
        {
            m_widgets->removeWidget( widget );
        }

        QWidget *at( int index ) const { return m_widgets->widget( index ); }

        int currentIndex() const { return m_current; }

        QWidget *currentWidget() const
        {
            if( m_current >= 0 )
                return at( m_current );
            else
                return 0;
        }

    public slots:
        void showWidget( int index )
        {
            if( index >= 0 && index < m_bar->count() )
                m_bar->open( index );
        }

    protected:
        QSize sizeHint() const
        {
            return QSize( static_cast<QWidget*>( parent() )->size().width() / 4, 400 );
        }

    private slots:
        void openWidget( int index )
        {
            m_contentsWidget->setParent( m_frame );
            m_frame->show();
            m_widgets->setCurrentIndex( index );
            m_current = index;
            emit widgetActivated( currentIndex() );
            emit widgetActivated( currentWidget() );
        }

        void closeWidgets()
        {
            m_contentsWidget->setParent( this );
            m_frame->hide();
            resize( m_bar->sizeHint() );
            m_current = -1;
            emit widgetActivated( currentIndex() );
            emit widgetActivated( currentWidget() );
        }

    private:
        QPointer<SideBarWidget> m_bar;
        KHBox *m_frame;
        QStackedWidget *m_widgets;
        QWidget *m_contentsWidget;
        int m_current;
};

#endif
