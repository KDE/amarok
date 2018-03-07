/****************************************************************************************
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#include "UpcomingEventsCalendarWidget.h"
#include "PaletteHandler.h"

#include <KColorUtils>
#include <KDateTime>
#include <KGlobal>
#include <QIcon>
#include <KLocale>
#include <KSystemTimeZones>

#include <QAction>
#include <QCalendarWidget>
#include <QTextCharFormat>
#include <QTimer>

class UpcomingEventsCalendarWidgetPrivate
{
public:
    UpcomingEventsCalendarWidgetPrivate( UpcomingEventsCalendarWidget* parent )
        : todayAction( 0 )
        , q_ptr( parent )
    {
        calendar = new QCalendarWidget;
        calendar->setGridVisible( true );
        calendar->setNavigationBarVisible( true );
        calendar->setFirstDayOfWeek( Qt::DayOfWeek(KGlobal::locale()->weekStartDay()) );
    }

    ~UpcomingEventsCalendarWidgetPrivate() {}

    void addEvent( const LastFmEventPtr &e )
    {
        events << e;
        QDate dt = e->date().date();
        QTextCharFormat format = calendar->dateTextFormat( dt );
        format.setFontUnderline( true );
        format.setToolTip( e->name() );
        format.setBackground( eventBackground );
        calendar->setDateTextFormat( dt, format );
    }

    void addEvents( const LastFmEvent::List &e )
    {
        QSet<LastFmEventPtr> newEvents = e.toSet().subtract( events );
        foreach( const LastFmEventPtr &event, newEvents )
            addEvent( event );
    }

    void _updateToday()
    {
        Q_Q( UpcomingEventsCalendarWidget );
        QDateTime now = QDateTime::currentDateTime();
        int updateIn = (24 * 60 * 60) - (now.toTime_t() + KSystemTimeZones::local().currentOffset()) % (24 * 60 * 60);
        QTimer::singleShot( updateIn * 1000, q, SLOT(_updateToday()) );

        if( !todayDate.isNull() )
        {
            QTextCharFormat format = calendar->dateTextFormat( todayDate );
            format.setFontWeight( QFont::Normal );
            calendar->setDateTextFormat( todayDate, format );
        }

        todayDate = now.date();
        QTextCharFormat format = calendar->dateTextFormat( todayDate );
        format.setFontWeight( QFont::Bold );
        calendar->setDateTextFormat( todayDate, format );
    }

    void _jumpToToday()
    {
        calendar->showToday();
        calendar->setSelectedDate( todayDate );
    }

    void _paletteChanged( const QPalette &palette )
    {
        QColor base = palette.color( QPalette::Base );
        QColor high = palette.color( QPalette::Highlight );
        eventBackground = QBrush( KColorUtils::tint( base, high, 0.4 ) );

        QList<QDate> eventDates;
        foreach( const LastFmEventPtr &event, events )
            eventDates << event->date().date();

        foreach( const QDate &date, eventDates )
        {
            QTextCharFormat format = calendar->dateTextFormat( date );
            format.setBackground( eventBackground );
            calendar->setDateTextFormat( date, format );
        }
    }

    QAction *todayAction;
    QDate todayDate;
    QBrush eventBackground;
    QCalendarWidget *calendar;
    QSet<LastFmEventPtr> events;

private:
    UpcomingEventsCalendarWidget *const q_ptr;
    Q_DECLARE_PUBLIC( UpcomingEventsCalendarWidget )
};

UpcomingEventsCalendarWidget::UpcomingEventsCalendarWidget( QGraphicsItem *parent, Qt::WindowFlags wFlags )
    : QGraphicsProxyWidget( parent, wFlags )
    , d_ptr( new UpcomingEventsCalendarWidgetPrivate( this ) )
{
    Q_D( UpcomingEventsCalendarWidget );
    setWidget( d->calendar );
    QColor base = The::paletteHandler()->palette().color( QPalette::Base );
    QColor high = The::paletteHandler()->palette().color( QPalette::Highlight );
    d->eventBackground = QBrush( KColorUtils::tint( base, high, 0.4 ) );
    d->_updateToday();
    connect( The::paletteHandler(), SIGNAL(newPalette(QPalette)), SLOT(_paletteChanged(QPalette)) );
}

UpcomingEventsCalendarWidget::~UpcomingEventsCalendarWidget()
{
    delete d_ptr;
}

void
UpcomingEventsCalendarWidget::clear()
{
    Q_D( UpcomingEventsCalendarWidget );
    d->calendar->setDateTextFormat( QDate(), QTextCharFormat() );
    d->events.clear();
}

LastFmEvent::List
UpcomingEventsCalendarWidget::events() const
{
    Q_D( const UpcomingEventsCalendarWidget );
    return d->events.toList();
}

QAction *
UpcomingEventsCalendarWidget::todayAction()
{
    Q_D( UpcomingEventsCalendarWidget );
    if( !d->todayAction )
    {
        d->todayAction = new QAction( QIcon::fromTheme("go-jump-today"), QString(), this );
        d->todayAction->setToolTip( i18nc( "@info:tooltip Calendar action", "Jump to Today" ) );
        connect( d->todayAction, SIGNAL(triggered()), SLOT(_jumpToToday()) );
    }
    return d->todayAction;
}

void
UpcomingEventsCalendarWidget::addEvent( const LastFmEventPtr &event )
{
    Q_D( UpcomingEventsCalendarWidget );
    d->addEvent( event );
}

void
UpcomingEventsCalendarWidget::addEvents( const LastFmEvent::List &events )
{
    Q_D( UpcomingEventsCalendarWidget );
    d->addEvents( events );
}

