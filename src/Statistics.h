/***************************************************************************
 * copyright            : (C) 2005-2006 Seb Ruiz <ruiz@kde.org>            *
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

#include "MainWindow.h"

#include <K3ListView>   //baseclass
#include <KCalendarSystem>
#include <KDialog>      //baseclass


class KLineEdit;
class QColor;
class QTimer;

class StatisticsList;
class StatisticsItem;
class StatisticsDetailedItem;


namespace Amarok {

    //Dropped these in the .h because we are not building the .cpp.. can go back when statistics is reenabled.
    /*
    * Transform to be usable within HTML/HTML attributes
    */
    QString escapeHTMLAttr( const QString &s )
    {
        return QString(s).replace( "%", "%25" ).replace( "'", "%27" ).replace( "\"", "%22" ).
                replace( "#", "%23" ).replace( "?", "%3F" );
    }
    QString unescapeHTMLAttr( const QString &s )
    {
        return QString(s).replace( "%3F", "?" ).replace( "%23", "#" ).replace( "%22", "\"" ).
                replace( "%27", "'" ).replace( "%25", "%" );
    }

    /**
     * Function that must be used when separating contextBrowser escaped urls
     * detail can contain track/discnumber
     */
    void albumArtistTrackFromUrl( QString url, QString &artist, QString &album, QString &detail )
    {
        if ( !url.contains("@@@") ) return;
        //KHTML removes the trailing space!
        if ( url.endsWith( " @@@" ) )
            url += ' ';

        const QStringList list = url.split( " @@@ ", QString::KeepEmptyParts );

        int size = list.count();

        if( size<=0 )
            error() << "size<=0";

        artist = size > 0 ? unescapeHTMLAttr( list[0] ) : "";
        album  = size > 1 ? unescapeHTMLAttr( list[1] ) : "";
        detail = size > 2 ? unescapeHTMLAttr( list[2] ) : "";
    }

    QString verboseTimeSince( const QDateTime &datetime )
    {
        const QDateTime now = QDateTime::currentDateTime();
        const int datediff = datetime.daysTo( now );

        if( datediff >= 6*7 /*six weeks*/ ) {  // return absolute month/year
            const KCalendarSystem *cal = KGlobal::locale()->calendar();
            const QDate date = datetime.date();
            return i18nc( "monthname year", "%1 %2", cal->monthName(date),
                          cal->yearString(date, KCalendarSystem::LongFormat) );
        }

        //TODO "last week" = maybe within 7 days, but prolly before last sunday

        if( datediff >= 7 )  // return difference in weeks
            return i18np( "One week ago", "%1 weeks ago", (datediff+3)/7 );

        if( datediff == -1 )
            return i18nc( "When this track was last played", "Tomorrow" );

        const int timediff = datetime.secsTo( now );

        if( timediff >= 24*60*60 /*24 hours*/ )  // return difference in days
            return datediff == 1 ?
                    i18n( "Yesterday" ) :
                    i18np( "One day ago", "%1 days ago", (timediff+12*60*60)/(24*60*60) );

        if( timediff >= 90*60 /*90 minutes*/ )  // return difference in hours
            return i18np( "One hour ago", "%1 hours ago", (timediff+30*60)/(60*60) );

        //TODO are we too specific here? Be more fuzzy? ie, use units of 5 minutes, or "Recently"

        if( timediff >= 0 )  // return difference in minutes
            return timediff/60 ?
                    i18np( "One minute ago", "%1 minutes ago", (timediff+30)/60 ) :
                    i18n( "Within the last minute" );

        return i18n( "The future" );
    }

    QString verboseTimeSince( uint time_t )
    {
        if( !time_t )
            return i18nc( "The amount of time since last played", "Never" );

        QDateTime dt;
        dt.setTime_t( time_t );
        return verboseTimeSince( dt );
    }
}


class Statistics : public KDialog
{
        Q_OBJECT

    public:
        explicit Statistics( QWidget *parent = 0, const char *name = 0 );
        ~Statistics();

        static Statistics *instance() { return s_instance; }

    private slots:
        void    slotSetFilter();
        void    slotSetFilterTimeout();

    private:
        StatisticsList *m_listView;
        KLineEdit  *m_lineEdit;
        QTimer         *m_timer;

        static Statistics *s_instance;
};

class StatisticsList : public K3ListView
{
        Q_OBJECT

    public:
        explicit StatisticsList( QWidget *parent, const char *name=0 );
        ~StatisticsList() {}

        QString filter()                           { return m_filter; }
        void    setFilter( const QString &filter ) { m_filter = filter; }
        void    renderView();
        void    refreshView();

    private slots:
        void    clearHover();
        void    itemClicked( Q3ListViewItem *item );
        void    showContextMenu( Q3ListViewItem *item, const QPoint &p, int );
        void    startHover( Q3ListViewItem *item );

    private:
        void    startDrag();
        void    viewportPaintEvent( QPaintEvent* );
        void    expandInformation( StatisticsItem *item, bool refresh=false );
        static QString subText( const QString &score, const QString &rating );

        StatisticsItem *m_trackItem;
        StatisticsItem *m_mostplayedItem;
        StatisticsItem *m_artistItem;
        StatisticsItem *m_albumItem;
        StatisticsItem *m_genreItem;
        StatisticsItem *m_newestItem;

        Q3ListViewItem  *m_currentItem;
        QString         m_filter;
        bool            m_expanded;
};

/// The listview items which are the headers for the categories
class StatisticsItem : public QObject, public K3ListViewItem
{
        Q_OBJECT

    public:
        StatisticsItem( QString text, StatisticsList *parent, K3ListViewItem *after=0, const char *name=0 );
        ~StatisticsItem() {}

        void    paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align );
        void    paintFocus( QPainter*, const QColorGroup& , const QRect& ) {}  //reimp
        void    setIcon( const QString &icon );

        void    enterHover();
        void    leaveHover();

        void       setExpanded( const bool b ) { m_isExpanded = b; }
        const bool isExpanded() { return m_isExpanded; }

        void    setSubtext( QString t ) { m_subText = t; }

        int     rtti() const { return RTTI; }
        static  const int RTTI = 1000;    //header item

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
        bool    m_isExpanded;

        QString m_subText;
};

/// Listview items for the children of expanded items (the actual results)
class StatisticsDetailedItem : public K3ListViewItem
{
    public:
        StatisticsDetailedItem( const QString &text, const QString &subtext, StatisticsItem *parent,
                                StatisticsDetailedItem *after=0, const char *name=0 );
        ~StatisticsDetailedItem() {}

        enum    ItemType { NONE, TRACK, ARTIST, ALBUM, GENRE, HISTORY };

        void    setup();
        void    paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align );

        void    setItemType( const ItemType t ) { m_type = t; }
        const   ItemType itemType() { return m_type; }

        void    setUrl( QString &url ) { m_url = url; }
        const   QString url() { return m_url; }

        void    setSubtext( QString t ) { m_subText = t; }
        QString getSQL(); //get the sql query for all the urls the item represents
        KUrl::List getURLs();

        void    paintFocus( QPainter*, const QColorGroup& , const QRect& ) {}  //reimp

        int     rtti() const { return RTTI; }
        static  const int RTTI = 1001;    //detailed item

    private:
        ItemType m_type;
        QString  m_url;
        QString  m_subText;
};


#endif /* AMAROK_STATISTICS_H */
