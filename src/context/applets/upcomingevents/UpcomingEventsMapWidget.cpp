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

#include "UpcomingEventsMapWidget.h"
#include "UpcomingEventsWidget.h"
#include "network/NetworkAccessManagerProxy.h"

#include <KLocale>
#include <KStandardDirs>

#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QWebView>
#include <QWebFrame>

class UpcomingEventsMapWidgetPrivate
{
public:
    UpcomingEventsMapWidgetPrivate( UpcomingEventsMapWidget *parent );
    ~UpcomingEventsMapWidgetPrivate();

    void addEvent( const LastFmEventPtr &event );
    void addMarker( const LastFmEventPtr &event );
    QString createInfoString( const LastFmEventPtr &event ) const;
    QUrl eventForMapIcon( const LastFmEventPtr &event ) const;
    void removeEvent( const LastFmEventPtr &event );
    void removeMarker( const LastFmEventPtr &event );

    void _init();
    void _linkClicked( const QUrl &url );
    void _loadFinished( bool success );
    void _centerAt( QObject *obj );

    LastFmEvent::List events;
    LastFmEvent::List eventQueue;
    QSet<UpcomingEventsListWidget*> listWidgets;
    QPointF centerWhenLoaded;
    bool isLoaded;

private:
    UpcomingEventsMapWidget *const q_ptr;
    Q_DECLARE_PUBLIC( UpcomingEventsMapWidget )
};

UpcomingEventsMapWidgetPrivate::UpcomingEventsMapWidgetPrivate( UpcomingEventsMapWidget *parent )
    : isLoaded( false )
    , q_ptr( parent )
{
}

UpcomingEventsMapWidgetPrivate::~UpcomingEventsMapWidgetPrivate()
{
}

void
UpcomingEventsMapWidgetPrivate::addEvent( const LastFmEventPtr &event )
{
    if( !isLoaded )
    {
        eventQueue << event;
        return;
    }
    events << event;
    addMarker( event );
}

void
UpcomingEventsMapWidgetPrivate::addMarker( const LastFmEventPtr &event )
{
    Q_Q( UpcomingEventsMapWidget );
    LastFmLocationPtr loc = event->venue()->location;
    QString js = QString( "javascript:addMarker(%1,%2,'%3','%4')" )
        .arg( QString::number( loc->latitude ) )
        .arg( QString::number( loc->longitude ) )
        .arg( eventForMapIcon(event).url() )
        .arg( createInfoString(event) );
    q->page()->mainFrame()->evaluateJavaScript( js );
}

void
UpcomingEventsMapWidgetPrivate::removeEvent( const LastFmEventPtr &event )
{
    eventQueue.removeAll( event );
    if( isLoaded )
    {
        events.removeAll( event );
        removeMarker( event );
    }
}

void
UpcomingEventsMapWidgetPrivate::removeMarker( const LastFmEventPtr &event )
{
    Q_Q( UpcomingEventsMapWidget );
    LastFmLocationPtr loc = event->venue()->location;
    QString js = QString( "javascript:removeMarker(%1,%2)" )
        .arg( QString::number( loc->latitude ) )
        .arg( QString::number( loc->longitude ) );
    q->page()->mainFrame()->evaluateJavaScript( js );
}

QString
UpcomingEventsMapWidgetPrivate::createInfoString( const LastFmEventPtr &event ) const
{
    QString name = event->name();
    if( event->isCancelled() )
        name = i18nc( "@label:textbox Title for a canceled upcoming event", "<s>%1</s> (Canceled)", name );

    QStringList artists = event->artists();
    artists.removeDuplicates();

    QString desc = event->description();
    KDateTime dt = event->date();
    QStringList tags = event->tags();
    LastFmVenuePtr venue = event->venue();
    QString venueWebsite = venue->website.url();
    QString venueLastFmUrl = venue->url.url();
    QString location = venue->location->city;
    if( !venue->location->street.isEmpty() )
        location.prepend( venue->location->street + ", " );

    QString html = QString(
        "<div><img src=\"%1\" alt=\"\" style=\"float:right;margin:5px;clear:right\"/></div>" \
        "<div><img src=\"%2\" alt=\"\" style=\"float:right;margin:5px;clear:right\"/></div>" \
        "<div id=\"bodyContent\">" \
        "<small>" \
        "<b>Event:</b> %3<br/>" \
        "<b>Artists:</b> %4<br/>" \
        "<b>Time:</b> %5<br/>" \
        "<b>Date:</b> %6<br/>" \
        "<b>Venue:</b> %7<br/>" \
        "<b>Location:</b> %8<br/>" \
        "<b>Description:</b> %9<br/>" \
        "<b>Tags:</b> %10<br/>" \
        "<b>Event Website:</b> <a href=\"%11\">Last.fm</a><br/>" \
        "<b>Venue Website:</b> <a href=\"%12\">URL</a>, <a href=\"%13\">Last.fm</a><br/>" \
        "</small>" \
        "</div>")
        .arg( event->imageUrl(LastFmEvent::Medium).url() )
        .arg( venue->imageUrls[LastFmEvent::Medium].url() )
        .arg( name )
        .arg( artists.join(", ") )
        .arg( KGlobal::locale()->formatTime( dt.time() ) )
        .arg( KGlobal::locale()->formatDate( dt.date(), KLocale::FancyShortDate ) )
        .arg( venue->name )
        .arg( location )
        .arg( desc.isEmpty() ? i18n("none") : desc )
        .arg( tags.isEmpty() ? i18n("none") : tags.join(", ") )
        .arg( event->url().url() )
        .arg( venueWebsite.isEmpty() ? i18n("none") : venueWebsite )
        .arg( venueLastFmUrl.isEmpty() ? i18n("none") : venueLastFmUrl );
    return html;
}

QUrl
UpcomingEventsMapWidgetPrivate::eventForMapIcon( const LastFmEventPtr &event ) const
{
    // Thanks a whole bunch to Nicolas Mollet, Matthias Stasiak at google-maps-icons
    // pack (http://code.google.com/p/google-maps-icons/wiki/CultureIcons)
    const QStringList &tags = event->tags();
    QString name;
    if( tags.contains( "festival", Qt::CaseInsensitive ) )
        name = "festival.png";
    else if( !tags.filter( QRegExp("rock|metal") ).isEmpty() )
        name = "music-rock.png";
    else if( !tags.filter( QRegExp("hip.?hop|rap") ).isEmpty() )
        name = "music-hiphop.png";
    else if( !tags.filter( QRegExp("orchest.*|classical|symphon.*") ).isEmpty() )
        name = "music-classical.png";
    else if( !tags.filter( QRegExp("choir|chorus|choral") ).isEmpty() )
        name = "choral.png";
    else if( !tags.filter( QRegExp("danc(e|ing)|disco|electronic") ).isEmpty() )
        name = "dancinghall.png";
    else
        name = "music-live.png";
    return QUrl( "http://google-maps-icons.googlecode.com/files/" + name );
}

void
UpcomingEventsMapWidgetPrivate::_centerAt( QObject *obj )
{
    Q_Q( UpcomingEventsMapWidget );
    UpcomingEventsWidget *widget = static_cast<UpcomingEventsWidget*>( obj );
    LastFmVenuePtr venue = widget->eventPtr()->venue();
    q->centerAt( venue );
}

void
UpcomingEventsMapWidgetPrivate::_init()
{
    Q_Q( UpcomingEventsMapWidget );
    q->connect( q, SIGNAL(loadFinished(bool)), q, SLOT(_loadFinished(bool)) );
    QFile mapHtml( KStandardDirs::locate( "data", "amarok/data/upcoming-events-map.html" ) );
    if( mapHtml.open( QIODevice::ReadOnly | QIODevice::Text ) )
        q->setHtml( mapHtml.readAll() );
}

void
UpcomingEventsMapWidgetPrivate::_linkClicked( const QUrl &url )
{
    QDesktopServices::openUrl( url );
}

void
UpcomingEventsMapWidgetPrivate::_loadFinished( bool success )
{
    if( !success )
        return;

    Q_Q( UpcomingEventsMapWidget );
    isLoaded = true;
    LastFmEvent::List queue = eventQueue;
    eventQueue.clear();

    foreach( const LastFmEventPtr &event, queue )
        addEvent( event );

    if( !centerWhenLoaded.isNull() )
    {
        q->centerAt( centerWhenLoaded.y(), centerWhenLoaded.x() );
        centerWhenLoaded *= 0.0;
    }
}

UpcomingEventsMapWidget::UpcomingEventsMapWidget( QGraphicsItem *parent )
    : KGraphicsWebView( parent )
    , d_ptr( new UpcomingEventsMapWidgetPrivate( this ) )
{
    page()->setLinkDelegationPolicy( QWebPage::DelegateAllLinks );
    page()->setNetworkAccessManager( The::networkAccessManager() );
    connect( page(), SIGNAL(linkClicked(QUrl)), this, SLOT(_linkClicked(QUrl)) );
    QTimer::singleShot( 0, this, SLOT(_init()) );
}

UpcomingEventsMapWidget::~UpcomingEventsMapWidget()
{
    delete d_ptr;
}

void
UpcomingEventsMapWidget::addEvent( const LastFmEventPtr &event )
{
    Q_D( UpcomingEventsMapWidget );
    d->addEvent( event );
}

void
UpcomingEventsMapWidget::addEvents( const LastFmEvent::List &events )
{
    foreach( const LastFmEventPtr &event, events )
        addEvent( event );
}

void
UpcomingEventsMapWidget::addEventsListWidget( UpcomingEventsListWidget *widget )
{
    Q_D( UpcomingEventsMapWidget );
    if( widget )
    {
        d->listWidgets << widget;
        addEvents( widget->events() );
        connect( widget, SIGNAL(eventAdded(LastFmEventPtr)), this, SLOT(addEvent(LastFmEventPtr)) );
        connect( widget, SIGNAL(eventRemoved(LastFmEventPtr)), this, SLOT(removeEvent(LastFmEventPtr)) );
        connect( widget, SIGNAL(mapRequested(QObject*)), this, SLOT(_centerAt(QObject*)) );
    }
}

void
UpcomingEventsMapWidget::removeEventsListWidget( UpcomingEventsListWidget *widget )
{
    Q_D( UpcomingEventsMapWidget );
    if( d->listWidgets.contains( widget ) )
    {
        foreach( const LastFmEventPtr &event, widget->events() )
            removeEvent( event );
        d->listWidgets.remove( widget );
        widget->disconnect( this );
    }
}

void
UpcomingEventsMapWidget::removeEvent( const LastFmEventPtr &event )
{
    Q_D( UpcomingEventsMapWidget );
    d->removeEvent( event );
}

bool
UpcomingEventsMapWidget::isLoaded() const
{
    Q_D( const UpcomingEventsMapWidget );
    return d->isLoaded;
}

int
UpcomingEventsMapWidget::eventCount() const
{
    Q_D( const UpcomingEventsMapWidget );
    return d->events.count();
}

LastFmEvent::List
UpcomingEventsMapWidget::events() const
{
    Q_D( const UpcomingEventsMapWidget );
    return d->events;
}

void
UpcomingEventsMapWidget::centerAt( double latitude, double longitude )
{
    Q_D( UpcomingEventsMapWidget );
    if( !d->isLoaded )
    {
        QPointF geo( longitude, latitude );
        d->centerWhenLoaded = geo;
        return;
    }

    QString lat( QString::number( latitude ) );
    QString lng( QString::number( longitude ) );
    QString js = QString( "javascript:centerAt(%1,%2)" ).arg( lat ).arg( lng );
    page()->mainFrame()->evaluateJavaScript( js );
}

void
UpcomingEventsMapWidget::centerAt( const LastFmVenuePtr &venue )
{
    LastFmLocationPtr loc = venue->location;
    centerAt( loc->latitude, loc->longitude );
}

void
UpcomingEventsMapWidget::clear()
{
    Q_D( UpcomingEventsMapWidget );
    d->events.clear();
    page()->mainFrame()->evaluateJavaScript( "javascript:clearMarkers()" );
}

