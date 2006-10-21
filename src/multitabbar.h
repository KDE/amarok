/***************************************************************************
                       kmultitabbar.h -  description
                           -------------------
   begin                :  2001
   copyright            : (C) 2001,2002,2003 by Joseph Wenninger <jowenn@kde.org>
                          (C) 2005           by Mark Kretschmann <markey@web.de>
***************************************************************************/

/***************************************************************************
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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
***************************************************************************/

#ifndef _Multitabbar_h_
#define _Multitabbar_h_

#include <qscrollview.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qlayout.h>
#include <qstring.h>
#include <qptrlist.h>
#include <qpushbutton.h>

class QPixmap;
class QPainter;
class QFrame;

class MultiTabBarPrivate;
class MultiTabBarTabPrivate;
class MultiTabBarButtonPrivate;
class MultiTabBarInternal;

// this exists only because dropEvent is protected
class DropProxyTarget
{
    public:
        virtual void dropProxyEvent( QDropEvent *e ) = 0;
};

/**
* A Widget for horizontal and vertical tabs.
* It is possible to add normal buttons to the top/left
* The handling if only one tab at a time or multiple tabs
* should be raisable is left to the "user".
*@author Joseph Wenninger
*/
class MultiTabBar: public QWidget
{
        Q_OBJECT
    public:
        enum MultiTabBarMode{Horizontal, Vertical};
        enum MultiTabBarPosition{Left, Right, Top, Bottom};

        /**
        * VSNET == Visual Studio .Net like (only show the text of active tabs
        * KDEV3 == Kdevelop 3 like (always show the text)
        * KONQSBC == konqy's classic sidebar style (unthemed), this one is disabled
        * 	at the moment, but will be renabled soon too
        */
        enum MultiTabBarStyle{VSNET = 0, KDEV3 = 1, KONQSBC = 2, KDEV3ICON = 3, AMAROK = 4, STYLELAST = 0xffff};

        MultiTabBar( MultiTabBarMode bm, QWidget *parent = 0, const char *name = 0 );
        virtual ~MultiTabBar();

        /**
        * append  a new button to the button area. The button can later on be accessed with button(ID)
        * eg for connecting signals to it
        * @param pic a pixmap for the button
        * @param id an arbitraty ID value. It will be emitted in the clicked signal for identifying the button
        *	if more than one button is connected to a signals.
        * @param popup A popup menu which should be displayed if the button is clicked
        * @param not_used_yet will be used for a popup text in the future
        */
        int appendButton( const QPixmap &pic, int id = -1, QPopupMenu* popup = 0, const QString& not_used_yet = QString::null );
        /**
            * remove a button with the given ID
        */
        void removeButton( int id );
        /**
        * append a new tab to the tab area. It can be accessed lateron with tabb(id);
        * @param pic a bitmap for the tab
        * @param id an arbitrary ID which can be used later on to identify the tab
        * @param text if a mode with text is used it will be the tab text, otherwise a mouse over hint
        * @param identifier for storing visibility to config file
        */
        int appendTab( const QPixmap &pic, int id = -1, const QString& text = QString::null, const QString& identifier = QString::null );
        /**
        * remove a tab with a given ID
        */
        void removeTab( int id );
        /**
        * set a tab to "raised"
        * @param id The ID of the tab to manipulate
        * @param state true == activated/raised, false == not active
        */
        void setTab( int id , bool state );
        /**
        * return the state of a tab, identified by it's ID
        */
        bool isTabRaised( int id ) const;
        /**
        * get a pointer to a button within the button area identified by its ID
        */
        class MultiTabBarButton *button( int id ) const;

        /**
        * get a pointer to a tab within the tab area, identiifed by its ID
        */
        class MultiTabBarTab *tab( int id ) const;
        /**
        * set the real position of the widget.
        * @param pos if the mode is horizontal, only use top, bottom, if it is vertical use left or right
        */
        void setPosition( MultiTabBarPosition pos );
        /**
        * set the display style of the tabs
        */
        void setStyle( MultiTabBarStyle style );
        /**
        * be carefull, don't delete tabs yourself and don't delete the list itself
        */
        QPtrList<MultiTabBarTab>* tabs();
        /**
        * be carefull, don't delete buttons yourself and don't delete the list itself
        */
        QPtrList<MultiTabBarButton>* buttons();

        /**
        * might vanish, not sure yet
        */
        void showActiveTabTexts( bool show = true );

        /**
        * @return number of visible tabs
        */
        uint visibleTabCount();
        /**
        * @return height per tab
        */
        uint sizePerTab();

        void showTabSelectionMenu(QPoint pos);

    protected:
        friend class MultiTabBarButton;
        virtual void fontChange( const QFont& );
        void updateSeparator();
    private:
        class MultiTabBarInternal *m_internal;
        QBoxLayout *m_l;
        QFrame *m_btnTabSep;
        QPtrList<MultiTabBarButton> m_buttons;
        MultiTabBarPosition m_position;
        MultiTabBarPrivate *d;
};

/**
* This class should never be created except with the appendButton call of MultiTabBar
*/
class MultiTabBarButton: public QPushButton
{
        Q_OBJECT
    public:
        MultiTabBarButton( const QPixmap& pic, const QString&, QPopupMenu *popup,
                           int id, QWidget *parent, MultiTabBar::MultiTabBarPosition pos, MultiTabBar::MultiTabBarStyle style );
        MultiTabBarButton( const QString&, QPopupMenu *popup,
                           int id, QWidget *parent, MultiTabBar::MultiTabBarPosition pos, MultiTabBar::MultiTabBarStyle style );
        virtual ~MultiTabBarButton();
        int id() const;

    public slots:
        /**
        * this is used internaly, but can be used by the user, if (s)he wants to
        * It the according call of MultiTabBar is invoked though this modifications will be overwritten
        */
        void setPosition( MultiTabBar::MultiTabBarPosition );
        /**
        * this is used internaly, but can be used by the user, if (s)he wants to
        * It the according call of MultiTabBar is invoked though this modifications will be overwritten
        */
        void setStyle( MultiTabBar::MultiTabBarStyle );

        /**
        * modify the text of the button
        */
        void setText( const QString & );

        /**
        * make this a drop proxy for finalDropTarget
        */
        void proxyDrops( DropProxyTarget *finalDropTarget );

        QSize sizeHint() const;

    protected:
        static const int ANIM_INTERVAL = 18;
        static const int ANIM_MAX = 20;

        MultiTabBar::MultiTabBarPosition m_position;
        MultiTabBar::MultiTabBarStyle m_style;
        QString m_text;
        int m_id;

        bool m_animEnter;
        int m_animCount;
        class QTimer* m_animTimer;
        class QTimer* m_dragSwitchTimer;

        virtual void hideEvent( class QHideEvent* );
        virtual void showEvent( class QShowEvent* );
        virtual void enterEvent( class QEvent* );
        virtual void leaveEvent( class QEvent* );

        virtual void dragEnterEvent ( class QDragEnterEvent * );
        virtual void dragMoveEvent ( class QDragMoveEvent * );
        virtual void dragLeaveEvent ( class QDragLeaveEvent * );
        virtual void dropEvent( class QDropEvent * );
    private:
        MultiTabBarButtonPrivate *d;
    signals:
        /**
        * this is emitted if  the button is clicked
        * @param id	the ID identifying the button
        */
        void clicked( int id );
        void initiateDrag ( int id );
    protected slots:
        virtual void slotClicked();
        virtual void slotAnimTimer();
        virtual void slotDragSwitchTimer();
};

/**
* This class should never be created except with the appendTab call of MultiTabBar
*/
class MultiTabBarTab: public MultiTabBarButton
{
        Q_OBJECT
    public:
        MultiTabBarTab( const QPixmap& pic, const QString&, int id, QWidget *parent,
                        MultiTabBar::MultiTabBarPosition pos, MultiTabBar::MultiTabBarStyle style );
        virtual ~MultiTabBarTab();
        /**
        * set the active state of the tab
        * @param  state true==active false==not active
        */
        void setState( bool state );
        /**
        * choose if the text should always be displayed
        * this is only used in classic mode if at all
        */
        void showActiveTabText( bool show );
        void resize() { setSize( neededSize() ); }
        bool visible() { return m_visible; };
        void setVisible( bool visible) { m_visible = visible; };
        void setIdentifier( const QString &id ) { m_identifier = id; }
        const QString &identifier() const { return m_identifier; }
    private:
        bool m_visible;
        bool m_showActiveTabText;
        int m_expandedSize;
        QString m_identifier;
        MultiTabBarTabPrivate *d;
    protected:
        friend class MultiTabBarInternal;
        void setSize( int );
        int neededSize();
        void updateState();
        virtual void drawButton( QPainter * );
        virtual void drawButtonLabel( QPainter * );
        void drawButtonStyled( QPainter * );
        void drawButtonClassic( QPainter * );
        void drawButtonAmarok( QPainter * );
        QColor blendColors( const QColor& color1, const QColor& color2, int percent );
    protected slots:
        virtual void slotClicked();
        void setTabsPosition( MultiTabBar::MultiTabBarPosition );

    public slots:
        virtual void setIcon( const QString& );
        virtual void setIcon( const QPixmap& );
};

#endif
