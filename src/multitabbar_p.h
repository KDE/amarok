/***************************************************************************
                        kmultitabbar_p.h -  description
                            -------------------
    begin                :  2003
    copyright            : (C) 2003 by Joseph Wenninger <jowenn@kde.org>
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

#ifndef MULTI_TAB_BAR_P_H
#define MULTI_TAB_BAR_P_H
#include <qscrollview.h>
#include <multitabbar.h>

class MultiTabBarInternal: public QScrollView
{
        Q_OBJECT
public:
        MultiTabBarInternal(QWidget *parent,MultiTabBar::MultiTabBarMode bm);
        int appendTab(const QPixmap &,int=-1,const QString& =QString::null, const QString&identifier=QString::null);
        MultiTabBarTab *tab(int) const;
        void removeTab(int);
        void setTabVisible(int id, bool visible);
        void setPosition(enum MultiTabBar::MultiTabBarPosition pos);
        void setStyle(enum MultiTabBar::MultiTabBarStyle style);
        void showActiveTabTexts(bool show);
        QPtrList<MultiTabBarTab>* tabs(){return &m_tabs;}
        uint visibleTabCount();
        uint sizePerTab();
        void showTabSelectionMenu(QPoint pos);

private:
        friend class MultiTabBar;
        QWidget *box;
        QBoxLayout *mainLayout;
        QPtrList<MultiTabBarTab> m_tabs;
        enum MultiTabBar::MultiTabBarPosition m_position;
        bool m_showActiveTabTexts;
        enum  MultiTabBar::MultiTabBarStyle m_style;
        int m_expandedTabSize;
        int m_lines;
        MultiTabBar::MultiTabBarMode m_barMode;

protected:
        virtual bool eventFilter(QObject *,QEvent*);
        virtual void drawContents ( QPainter *, int, int, int, int);

        /**
        * [contentsM|m]ousePressEvent are reimplemented from QScrollView
        * in order to ignore all mouseEvents on the viewport, so that the
        * parent can handle them.
        */
        virtual void contentsMousePressEvent(QMouseEvent *);
        virtual void mousePressEvent(QMouseEvent *);
        virtual void resizeEvent(QResizeEvent *);
};
#endif

