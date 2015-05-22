/****************************************************************************************
 * Copyright (c) 2009-2010 Ludovic Deveaux <deveaux.ludovic31@gmail.com>                *
 * Copyright (c) 2010 Hormiere Guillaume <hormiere.guillaume@gmail.com>                 *
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

#define DEBUG_PREFIX "UpcomingEventsWidget"

#include "UpcomingEventsWidget.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "SvgHandler.h"

#include <KDateTime>
#include <KIcon>
#include <KLocale>

#include <Plasma/Label>
#include <Plasma/PushButton>
#include <Plasma/Separator>

#include <QDesktopServices>
#include <QLabel>
#include <QPixmap>
#include <QPixmapCache>
#include <QGraphicsGridLayout>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QSignalMapper>

UpcomingEventsWidget::UpcomingEventsWidget( const LastFmEventPtr &event,
                                            QGraphicsItem *parent,
                                            Qt::WindowFlags wFlags )
    : QGraphicsWidget( parent, wFlags )
    , m_mapButton( 0 )
    , m_urlButton( 0 )
    , m_image( new QLabel )
    , m_event( event )
{
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Maximum );

    m_image->setText( i18n("Loading picture...") );
    m_image->setAttribute( Qt::WA_NoSystemBackground );
    m_image->setAlignment( Qt::AlignCenter );
    m_image->setFixedSize( 128, 128 );
    QGraphicsProxyWidget *imageProxy = new QGraphicsProxyWidget( this );
    imageProxy->setWidget( m_image );

    m_attendance   = createLabel();
    m_date         = createLabel();
    m_location     = createLabel();
    m_name         = createLabel();
    m_participants = createLabel();
    m_tags         = createLabel();
    m_venue        = createLabel();

    QGraphicsLinearLayout *buttonsLayout = new QGraphicsLinearLayout( Qt::Horizontal );
    buttonsLayout->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Fixed );
    if( event && event->venue() && event->venue()->location )
    {
        QPointF geo( event->venue()->location->longitude, event->venue()->location->latitude );
        if( !geo.isNull() )
        {
            m_mapButton = new Plasma::PushButton( this );
            m_mapButton->setMaximumSize( QSizeF( 22, 22 ) );
            m_mapButton->setIcon( KIcon("edit-find") ); // TODO: a map icon would be nice
            m_mapButton->setToolTip( i18n( "View map" ) );
            buttonsLayout->addItem( m_mapButton );
        }
    }

    if( event && event->url().isValid() )
    {
        m_urlButton = new Plasma::PushButton( this );
        m_urlButton->setMaximumSize( QSizeF( 22, 22 ) );
        m_urlButton->setIcon( KIcon("applications-internet") );
        m_urlButton->setToolTip( i18n( "Open Last.fm webpage for this event" ) );
        connect( m_urlButton, SIGNAL(clicked()), this, SLOT(openUrl()) );
        buttonsLayout->addItem( m_urlButton );
    }

    QSizePolicy::Policy minPol = QSizePolicy::Minimum;
    QGraphicsWidget *supportLabel, *venueLabel, *locationLabel, *dateLabel, *attendLabel, *tagsLabel;
    supportLabel  = createLabel( i18nc("@label:textbox Supporing acts for an event", "Supporting:"), minPol );
    venueLabel    = createLabel( i18nc("@label:textbox", "Venue:"), minPol );
    locationLabel = createLabel( i18nc("@label:textbox", "Location:"), minPol );
    dateLabel     = createLabel( i18nc("@label:textbox", "Date:"), minPol );
    attendLabel   = createLabel( i18nc("@label:textbox", "Attending:"), minPol );
    tagsLabel     = createLabel( i18nc("@label:textbox", "Tags:"), minPol );

    QGraphicsGridLayout *infoLayout = new QGraphicsGridLayout;
    infoLayout->addItem( supportLabel, 0, 0 );
    infoLayout->addItem( venueLabel, 1, 0 );
    infoLayout->addItem( locationLabel, 2, 0 );
    infoLayout->addItem( dateLabel, 3, 0 );
    infoLayout->addItem( attendLabel, 4, 0 );
    infoLayout->addItem( tagsLabel, 5, 0 );
    infoLayout->addItem( m_participants, 0, 1 );
    infoLayout->addItem( m_venue, 1, 1 );
    infoLayout->addItem( m_location, 2, 1 );
    infoLayout->addItem( m_date, 3, 1 );
    infoLayout->addItem( m_attendance, 4, 1 );
    infoLayout->addItem( m_tags, 5, 1 );

    QGraphicsGridLayout *layout = new QGraphicsGridLayout;
    layout->addItem( imageProxy, 0, 0, 2, 1, Qt::AlignCenter );
    layout->addItem( m_name, 0, 1 );
    layout->addItem( buttonsLayout, 0, 2, Qt::AlignRight );
    layout->addItem( infoLayout, 1, 1, 1, 2 );
    setLayout( layout );

    QString name = event->name();
    if( event->isCancelled() )
        name = i18nc( "@label:textbox Title for a canceled upcoming event", "<s>%1</s> (Canceled)", name );
    setName( name );
    setDate( event->date() );
    setLocation( event->venue()->location );
    setVenue( event->venue() );
    setAttendance( event->attendance() );
    setParticipants( event->participants() );
    setTags( event->tags() );
    setImage( event->imageUrl(LastFmEvent::Large) );
}

UpcomingEventsWidget::~UpcomingEventsWidget()
{
}

QGraphicsProxyWidget *
UpcomingEventsWidget::createLabel( const QString &text, QSizePolicy::Policy hPolicy )
{
    QLabel *label = new QLabel;
    label->setAttribute( Qt::WA_NoSystemBackground );
    label->setMinimumWidth( 10 );
    label->setSizePolicy( hPolicy, QSizePolicy::Preferred );
    label->setText( text );
    label->setWordWrap( false );
    QGraphicsProxyWidget *proxy = new QGraphicsProxyWidget( this );
    proxy->setWidget( label );
    return proxy;
}

void
UpcomingEventsWidget::setImage( const KUrl &url )
{
    if( url.isValid() )
    {
        m_imageUrl = url;
        QPixmap pixmap;
        if( QPixmapCache::find(url.url(), &pixmap) )
        {
            m_image->setPixmap( pixmap );
            return;
        }
        QNetworkReply *reply = The::networkAccessManager()->get( QNetworkRequest(url) );
        connect( reply, SIGNAL(finished()), SLOT(loadImage()), Qt::QueuedConnection );
    }
    m_image->setPixmap( Amarok::semiTransparentLogo( 120 ) );
}

void
UpcomingEventsWidget::loadImage()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender() );
    if( !reply )
        return;

    reply->deleteLater();
    const KUrl &url = reply->request().url();
    if( m_imageUrl != url )
        return;

    if( reply->error() != QNetworkReply::NoError )
        return;

    QPixmap image;
    if( image.loadFromData( reply->readAll() ) )
    {
        image = image.scaled( 116, 116, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        image = The::svgHandler()->addBordersToPixmap( image, 6, QString(), true );
        QPixmapCache::insert( url.url(), image );
        m_image->setPixmap( image );
    }
}

void
UpcomingEventsWidget::openUrl()
{
    if( m_event->url().isValid() )
        QDesktopServices::openUrl( m_event->url() );
}

void
UpcomingEventsWidget::setTags( const QStringList &tags )
{
    QLabel *tagsLabel = static_cast<QLabel*>( m_tags->widget() );
    tagsLabel->setText( tags.isEmpty() ? i18n( "none" ) : tags.join(", ") );
    QStringList tooltips;
    if( tags.count() > 10 )
    {
        for( int i = 0; i < 10; ++i )
            tooltips << tags.value( i );
    }
    else
        tooltips = tags;
    tagsLabel->setToolTip( i18nc( "@info:tooltip", "<strong>Tags:</strong><nl/>%1", tooltips.join(", ") ) );
}

void
UpcomingEventsWidget::setAttendance( int count )
{
    static_cast<QLabel*>(m_attendance->widget())->setText( QString::number(count) );
}

void
UpcomingEventsWidget::setParticipants( const QStringList &participants )
{
    QLabel *participantsLabel = static_cast<QLabel*>( m_participants->widget() );
    if( participants.isEmpty() )
    {
        participantsLabel->setText( i18n( "none" ) );
    }
    else
    {
        QString combined = participants.join( ", " );
        participantsLabel->setText( combined );
        if( participants.size() > 1 )
        {
            QString tooltip = i18nc( "@info:tooltip Supporting artists for an event",
                                     "<strong>Supporting artists:</strong><nl/>%1", combined );
            participantsLabel->setToolTip( tooltip );
        }
    }
}

void
UpcomingEventsWidget::setDate( const KDateTime &date )
{
    QLabel *dateLabel = static_cast<QLabel*>( m_date->widget() );
    dateLabel->setText( KGlobal::locale()->formatDateTime( date, KLocale::FancyLongDate ) );
    KDateTime currentDT = KDateTime::currentLocalDateTime();
    if( currentDT.compare(date) == KDateTime::Before )
    {
        int daysTo = currentDT.daysTo( date );
        dateLabel->setToolTip( i18ncp( "@info:tooltip Number of days till an event",
                                       "Tomorrow", "In <strong>%1</strong> days", daysTo ) );
    }
}

void
UpcomingEventsWidget::setLocation( const LastFmLocationPtr &loc )
{
    QString text = QString( "%1, %2" ).arg( loc->city, loc->country );
    if( !loc->street.isEmpty() )
        text.prepend( loc->street + ", " );
    QLabel *locLabel = static_cast<QLabel*>( m_location->widget() );
    locLabel->setText( text );
    locLabel->setToolTip( i18nc( "@info:tooltip", "<strong>Location:</strong><nl/>%1", text ) );
}

void
UpcomingEventsWidget::setVenue( const LastFmVenuePtr &venue )
{
    static_cast<QLabel*>( m_venue->widget() )->setText( venue->name );
}

void
UpcomingEventsWidget::setName( const QString &name )
{
    QLabel *nameLabel = static_cast<QLabel*>( m_name->widget() );
    QFont nameFont;
    nameFont.setBold( true );
    nameFont.setPointSize( nameLabel->font().pointSize() + 2 );
    nameLabel->setFont( nameFont );
    nameLabel->setText( name );
}

UpcomingEventsListWidget::UpcomingEventsListWidget( QGraphicsWidget *parent )
    : Plasma::ScrollWidget( parent )
    , m_sigmap( new QSignalMapper( this ) )
{
    // The widgets are displayed line by line with only one column
    m_layout = new QGraphicsLinearLayout( Qt::Vertical );
    QGraphicsWidget *content = new QGraphicsWidget( this );
    content->setLayout( m_layout );
    setWidget( content );
    connect( m_sigmap, SIGNAL(mapped(QObject*)), this, SIGNAL(mapRequested(QObject*)) );
}

UpcomingEventsListWidget::~UpcomingEventsListWidget()
{
    clear();
}

int
UpcomingEventsListWidget::count() const
{
    return m_events.count();
}

LastFmEvent::List
UpcomingEventsListWidget::events() const
{
    return m_events;
}

void
UpcomingEventsListWidget::addEvent( const LastFmEventPtr &event )
{
    m_events << event;
    UpcomingEventsWidget *widget = new UpcomingEventsWidget( event );
    const uint eventTime = event->date().toTime_t();
    QMap<uint, UpcomingEventsWidget*>::const_iterator iBound( m_sortMap.insertMulti( eventTime, widget ) );
    QMap<uint, UpcomingEventsWidget*>::const_iterator i( m_sortMap.constBegin() );
    int index = 0;
    while( i++ != iBound )
        ++index; // find the right index to insert the widget
    index *= 2;  // take separators into account
    m_layout->insertItem( index, widget );
    m_layout->insertItem( index + 1, new Plasma::Separator );
    if( widget->m_mapButton )
    {
        connect( widget->m_mapButton, SIGNAL(clicked()), m_sigmap, SLOT(map()) );
        m_sigmap->setMapping( widget->m_mapButton, widget );
    }
    emit eventAdded( event );
}

void
UpcomingEventsListWidget::addEvents( const LastFmEvent::List &events )
{
    foreach( const LastFmEventPtr &event, events )
        addEvent( event );
}

void
UpcomingEventsListWidget::clear()
{
    foreach( const LastFmEventPtr &event, m_events )
        emit eventRemoved( event );
    m_events.clear();
    qDeleteAll( m_sortMap.values() );
    m_sortMap.clear();
    int count = m_layout->count();
    while( --count >= 0 )
    {
        QGraphicsLayoutItem *child = m_layout->itemAt( 0 );
        m_layout->removeItem( child );
        delete child;
    }
}

bool
UpcomingEventsListWidget::isEmpty() const
{
    return count() == 0;
}

QString
UpcomingEventsListWidget::name() const
{
    return m_name;
}

void
UpcomingEventsListWidget::setName( const QString &name )
{
    m_name = name;
}

