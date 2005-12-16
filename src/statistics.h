/***************************************************************************
 * copyright            : (C) 2005 Seb Ruiz <me@sebruiz.net>               *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_STATISTICS_H
#define AMAROK_STATISTICS_H

#include "playlistwindow.h"

#include <kdialogbase.h>    //baseclass
#include <klistview.h>      //baseclass

#include <qtimer.h>

class QColor;

class StatisticsList;
class StatisticsItem;
class StatisticsDetailedItem;

class Statistics : public KDialogBase
{
        Q_OBJECT

    public:
        Statistics( QWidget *parent = 0, const char *name = 0 );
        ~Statistics();

        static Statistics *instance() { return s_instance; }

    private:
        StatisticsList *m_listview;

        static Statistics *s_instance;
};

class StatisticsList : public KListView
{
        Q_OBJECT

    public:
        StatisticsList( QWidget *parent, const char *name=0 );
        ~StatisticsList() {};

    private slots:
        void    clearHover();
        void    itemClicked( QListViewItem *item );
        void    startHover( QListViewItem *item );

    private:
//         void    contentsDragEnterEvent( QDragEnterEvent *e );
//         void    contentsDragMoveEvent( QDragMoveEvent* e );
//         void    contentsDropEvent( QDropEvent *e );
//         void    keyPressEvent( QKeyEvent *e );
        void    viewportPaintEvent( QPaintEvent* );

        void    initDisplay();
        void    expandInformation( StatisticsDetailedItem *item );

        StatisticsItem *m_titleItem;
        StatisticsItem *m_trackItem;
        StatisticsItem *m_artistItem;
        StatisticsItem *m_albumItem;
        StatisticsItem *m_genreItem;

        QListViewItem  *m_currentItem;

};

class StatisticsItem : public QObject, public KListViewItem
{
        Q_OBJECT

    public:
        StatisticsItem( QString text, StatisticsList *parent, KListViewItem *after=0, const char *name=0 );
        ~StatisticsItem() {};

        void    paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align );
        void    paintFocus( QPainter*, const QColorGroup& , const QRect& ) {};  //reimp
        void    setPixmap( const QString &pix );

        void    enterHover();
        void    leaveHover();

        void       setOn( const bool b ) { m_on = b; }
        const bool isOn() { return m_on; }

        void       setTitleItem( const bool b ) { m_isTitleItem = b; }
        const bool isTitleItem() { return m_isTitleItem; }

    protected:
        static const int ANIM_INTERVAL = 18;
        static const int ANIM_MAX = 20;

    private slots:
        void slotAnimTimer();

    private:
        QColor  blendColors( const QColor& color1, const QColor& color2, int percent );

        QTimer *m_animTimer;
        bool    m_animEnter;
        int     m_animCount;

        bool    m_isActive;
        bool    m_isTitleItem;

        bool    m_on;
};

class StatisticsDetailedItem : public KListViewItem
{
    public:
        StatisticsDetailedItem::StatisticsDetailedItem( QString &text, StatisticsItem *parent,
                                                        StatisticsDetailedItem *after=0, const char *name=0 );
        ~StatisticsDetailedItem() {};

        enum ItemType { NONE, SHOW_MORE, SHOW_LESS };

        void    setItemType( const ItemType t );
        const   ItemType itemType() { return m_type; }

        void    paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align );
        void    paintFocus( QPainter*, const QColorGroup& , const QRect& ) {};  //reimp

    private:
        ItemType m_type;
};


#endif /* AMAROK_STATISTICS_H */
