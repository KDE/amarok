/****************************************************************************************
 * Copyright (c) 2009 Joffrey Clavel <jclavel@clabert.info>                             *
 * Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
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

#define DEBUG_PREFIX "UpcomingEventsApplet"

#include "UpcomingEventsApplet.h"

#include "amarokurls/AmarokUrl.h"
#include "context/applets/upcomingevents/LastFmEvent.h"
#include "context/widgets/AppletHeader.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "SvgHandler.h"
#include "LastFmEventXmlParser.h"
#include "UpcomingEventsMapWidget.h"
#include "UpcomingEventsCalendarWidget.h"
#include "UpcomingEventsStack.h"
#include "UpcomingEventsStackItem.h"

#include <KConfigDialog>
#include <KGlobalSettings>
#include <Plasma/IconWidget>
#include <Plasma/Theme>
#include <Plasma/Svg>

#include <QDesktopServices>
#include <QGraphicsLinearLayout>
#include <QXmlStreamReader>

UpcomingEventsApplet::UpcomingEventsApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_groupVenues( false )
    , m_stack( 0 )
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );
}

UpcomingEventsApplet::~UpcomingEventsApplet()
{
}

void
UpcomingEventsApplet::init()
{
    DEBUG_BLOCK

    Context::Applet::init();

    enableHeader( true );
    setHeaderText( i18n( "Upcoming Events" ) );

    m_stack = new UpcomingEventsStack( this );
    m_stack->setContentsMargins( 0, 0, 0, 0 );

    connect( m_stack, SIGNAL(collapseStateChanged()), SLOT(collapseStateChanged()) );
    connect( this, SIGNAL(listWidgetRemoved(UpcomingEventsListWidget*)),
             m_stack, SLOT(cleanupListWidgets()) );

    QAction *calendarAction = new QAction( this );
    calendarAction->setIcon( KIcon( "view-calendar" ) );
    calendarAction->setToolTip( i18n( "View Events Calendar" ) );
    Plasma::IconWidget *calendarIcon = addLeftHeaderAction( calendarAction );
    connect( calendarIcon, SIGNAL(clicked()), this, SLOT(viewCalendar()) );

    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setToolTip( i18n( "Settings" ) );
    settingsAction->setEnabled( true );
    Plasma::IconWidget *settingsIcon = addRightHeaderAction( settingsAction );
    connect( settingsIcon, SIGNAL(clicked()), this, SLOT(configure()) );

    m_artistStackItem = m_stack->create( QLatin1String("currentartistevents") );
    m_artistEventsList = new UpcomingEventsListWidget( m_artistStackItem );
    m_artistStackItem->setTitle( i18nc( "@title:group", "No track is currently playing" ) );
    m_artistStackItem->setWidget( m_artistEventsList );
    m_artistStackItem->setCollapsed( true );
    m_artistStackItem->setIcon( KIcon("filename-artist-amarok") );
    connect( m_artistEventsList, SIGNAL(mapRequested(QObject*)), SLOT(handleMapRequest(QObject*)) );

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Vertical );
    layout->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    layout->addItem( m_header );
    layout->addItem( m_stack );
    setLayout( layout );

    // Read config and inform the engine.
    enableVenueGrouping( Amarok::config("UpcomingEvents Applet").readEntry( "groupVenues", false ) );
    QStringList venueData = Amarok::config("UpcomingEvents Applet").readEntry( "favVenues", QStringList() );
    m_favoriteVenues = venueStringToDataList( venueData );

    Plasma::DataEngine *engine = dataEngine( "amarok-upcomingEvents" );
    connect( engine, SIGNAL(sourceAdded(QString)), SLOT(engineSourceAdded(QString)) );
    engine->query( "artistevents" );
    engine->query( "venueevents" );

    updateConstraints();
}

void
UpcomingEventsApplet::engineSourceAdded( const QString &source )
{
    if( source == "artistevents" || source == "venueevents" )
        dataEngine( "amarok-upcomingEvents" )->connectSource( source, this );
}

void
UpcomingEventsApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Context::Applet::constraintsEvent( constraints );
    prepareGeometryChange();
    setHeaderText( i18n( "Upcoming Events" ) );
    update();
}

void
UpcomingEventsApplet::dataUpdated( const QString &source, const Plasma::DataEngine::Data &data )
{
    const LastFmEvent::List &events = data[ "events" ].value< LastFmEvent::List >();
    if( source == "artistevents" )
    {
        QString artistName = data[ "artist" ].toString();
        m_artistEventsList->clear();
        m_artistEventsList->setName( artistName );
        addToStackItem( m_artistStackItem, events, artistName );
        if( !m_artistStackItem->action( "showinmediasources" ) )
        {
            QAction *act = new QAction( KIcon("edit-find"), QString(), m_artistStackItem );
            act->setToolTip( i18n( "Show in Media Sources" ) );
            connect( act, SIGNAL(triggered()), this, SLOT(navigateToArtist()) );
            m_artistStackItem->addAction( "showinmediasources", act );
        }
        m_artistStackItem->setCollapsed( events.isEmpty() );
    }
    else if( source == "venueevents" )
    {
        if( !events.isEmpty() )
        {
            LastFmVenuePtr venue = data[ "venue" ].value<LastFmVenuePtr>();
            if( m_groupVenues && m_stack->hasItem("favoritevenuesgroup") )
            {
                QString title = i18n( "Favorite Venues" );
                addToStackItem( m_stack->item("favoritevenuesgroup"), events, title );
            }
            else
            {
                UpcomingEventsStackItem *stackItem( 0 );
                UpcomingEventsListWidget *listWidget( 0 );
                LastFmEvent::List newEvents;
                if( !m_stack->hasItem( venue->name ) )
                {
                    stackItem = m_stack->create( venue->name );
                    listWidget = new UpcomingEventsListWidget( stackItem );
                    listWidget->setName( venue->name );
                    stackItem->setWidget( listWidget );
                    stackItem->setCollapsed( true );
                    stackItem->setIcon( KIcon("favorites") );
                    stackItem->showCloseButton();
                    connect( listWidget, SIGNAL(mapRequested(QObject*)), SLOT(handleMapRequest(QObject*)) );
                    connect( listWidget, SIGNAL(destroyed(QObject*)), SLOT(listWidgetDestroyed(QObject*)) );
                    emit listWidgetAdded( listWidget );
                    newEvents = events;
                }
                else
                {
                    stackItem = m_stack->item( venue->name );
                    typedef UpcomingEventsListWidget UELW;
                    UELW *widget = static_cast<UELW*>( stackItem->widget() );
                    newEvents = events.toSet().subtract( widget->events().toSet() ).toList();
                }
                addToStackItem( stackItem, newEvents, venue->name );
            }
            update();
        }
        else if( m_groupVenues && m_stack->hasItem( QLatin1String("favoritevenuesgroup") ) )
        {
            m_stack->remove( QLatin1String("favoritevenuesgroup" ) );
        }
        else
        {
            // remove all venue lists
            const QRegExp pattern( QLatin1String("^(?!(currentartistevents|venuemapview|calendar)).*$") );
            QList<UpcomingEventsStackItem*> eventItems = m_stack->items( pattern );
            qDeleteAll( eventItems );
        }
    }
}

void
UpcomingEventsApplet::clearVenueItems()
{
    m_stack->remove( QLatin1String("favoritevenuesgroup" ) );
    m_stack->remove( QLatin1String("venuemapview" ) );
}

void
UpcomingEventsApplet::addToStackItem( UpcomingEventsStackItem *item,
                                      const LastFmEvent::List &events,
                                      const QString &name )
{
    UpcomingEventsListWidget *listWidget = static_cast<UpcomingEventsListWidget*>( item->widget() );
    listWidget->addEvents( events );

    QString title;
    int added = listWidget->count();
    if( added == 0 )
    {
        title = name.isEmpty() ? i18n( "No upcoming events" ) : i18n( "%1: No upcoming events", name );
    }
    else
    {
        title = name.isEmpty()
            ? i18ncp( "@title:group Number of upcoming events", "1 event", "%1 events", added )
            : i18ncp( "@title:group Number of upcoming events", "%1: 1 event", "%1: %2 events", name, added );
    }
    item->setTitle( title );
    item->layout()->invalidate();
}

void
UpcomingEventsApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option )
    Q_UNUSED( contentsRect )
    addGradientToAppletBackground( p );
}

void
UpcomingEventsApplet::configure()
{
    DEBUG_BLOCK
    showConfigurationInterface();
}

void
UpcomingEventsApplet::createConfigurationInterface( KConfigDialog *parent )
{
    QWidget *generalSettings = new QWidget;
    QWidget *venueSettings = new QWidget;
    ui_GeneralSettings.setupUi( generalSettings );
    ui_VenueSettings.setupUi( venueSettings );

    // TODO bad, it's done manually ...
    QString timeSpan = Amarok::config("UpcomingEvents Applet").readEntry( "timeSpan", "AllEvents" );
    if( timeSpan == "AllEvents" )
        ui_GeneralSettings.filterComboBox->setCurrentIndex( 0 );
    else if( timeSpan == "ThisWeek" )
        ui_GeneralSettings.filterComboBox->setCurrentIndex( 1 );
    else if( timeSpan == "ThisMonth" )
        ui_GeneralSettings.filterComboBox->setCurrentIndex( 2 );
    else if( timeSpan == "ThisYear" )
        ui_GeneralSettings.filterComboBox->setCurrentIndex( 3 );

    connect( ui_VenueSettings.searchLineEdit, SIGNAL(returnPressed(QString)), SLOT(searchVenue(QString)) );
    connect( ui_VenueSettings.searchResultsList, SIGNAL(itemClicked(QListWidgetItem*)), SLOT(showVenueInfo(QListWidgetItem*)) );
    connect( ui_VenueSettings.selectedVenuesList, SIGNAL(itemClicked(QListWidgetItem*)), SLOT(showVenueInfo(QListWidgetItem*)) );
    connect( ui_VenueSettings.searchResultsList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), SLOT(venueResultDoubleClicked(QListWidgetItem*)) );
    connect( ui_VenueSettings.selectedVenuesList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), SLOT(selectedVenueDoubleClicked(QListWidgetItem*)) );
    connect( ui_VenueSettings.urlValue, SIGNAL(leftClickedUrl(QString)), SLOT(openUrl(QString)) );
    connect( ui_VenueSettings.urlValue, SIGNAL(rightClickedUrl(QString)), SLOT(openUrl(QString)) );
    connect( ui_VenueSettings.websiteValue, SIGNAL(leftClickedUrl(QString)), SLOT(openUrl(QString)) );
    connect( ui_VenueSettings.websiteValue, SIGNAL(rightClickedUrl(QString)), SLOT(openUrl(QString)) );
    connect( parent, SIGNAL(okClicked()), SLOT(saveSettings()) );

    ui_VenueSettings.photoLabel->hide();
    ui_VenueSettings.infoGroupBox->setFont( KGlobalSettings::smallestReadableFont() );
    ui_GeneralSettings.groupVenueCheckBox->setCheckState( m_groupVenues ? Qt::Checked : Qt::Unchecked );

    ui_VenueSettings.countryCombo->insertSeparator( 1 );
    const QStringList &countryCodes = KGlobal::locale()->allCountriesList();
    foreach( const QString &code, countryCodes )
        ui_VenueSettings.countryCombo->addItem( KGlobal::locale()->countryCodeToName(code), code );

    foreach( const VenueData &data, m_favoriteVenues )
    {
        QListWidgetItem *item = new QListWidgetItem;
        item->setData( VenueIdRole, data.id );
        item->setData( VenueCityRole, data.city );
        item->setData( VenueNameRole, data.name );
        item->setText( QString( "%1, %2" )
                       .arg( item->data( VenueNameRole ).toString() )
                       .arg( item->data( VenueCityRole ).toString() ) );
        ui_VenueSettings.selectedVenuesList->addItem( item );
    }

    parent->addPage( generalSettings, i18n( "Upcoming Events Settings" ), "preferences-system");
    parent->addPage( venueSettings, i18n( "Favorite Venues" ), "favorites" );
}

void
UpcomingEventsApplet::venueResultDoubleClicked( QListWidgetItem *item )
{
    if( !item )
        return;

    int row = ui_VenueSettings.searchResultsList->row( item );
    QListWidgetItem *moveItem = ui_VenueSettings.searchResultsList->takeItem( row );
    ui_VenueSettings.searchResultsList->clearSelection();
    ui_VenueSettings.selectedVenuesList->addItem( moveItem );
    ui_VenueSettings.selectedVenuesList->setCurrentItem( moveItem );
}

void
UpcomingEventsApplet::selectedVenueDoubleClicked( QListWidgetItem *item )
{
    if( !item )
        return;

    int row = ui_VenueSettings.selectedVenuesList->row( item );
    QListWidgetItem *moveItem = ui_VenueSettings.selectedVenuesList->takeItem( row );
    ui_VenueSettings.selectedVenuesList->clearSelection();
    ui_VenueSettings.searchResultsList->addItem( moveItem );
    ui_VenueSettings.searchResultsList->setCurrentItem( moveItem );
}

void
UpcomingEventsApplet::showVenueInfo( QListWidgetItem *item )
{
    if( !item )
        return;

    const QString &name    = item->data( VenueNameRole ).toString();
    const QString &city    = item->data( VenueCityRole ).toString();
    const QString &country = item->data( VenueCountryRole ).toString();
    const QString &street  = item->data( VenueStreetRole ).toString();
    const KUrl &url        = item->data( VenueUrlRole ).value<KUrl>();
    const KUrl &website    = item->data( VenueWebsiteRole ).value<KUrl>();
    const KUrl &photoUrl   = item->data( VenuePhotoUrlRole ).value<KUrl>();

    ui_VenueSettings.nameValue->setText( name );
    ui_VenueSettings.cityValue->setText( city );
    ui_VenueSettings.countryValue->setText( country );
    ui_VenueSettings.streetValue->setText( street );

    if( url.isValid() )
    {
        ui_VenueSettings.urlValue->setText( i18nc("@label:textbox Url label", "link") );
        ui_VenueSettings.urlValue->setTipText( url.url() );
        ui_VenueSettings.urlValue->setUrl( url.url() );
    }
    else
        ui_VenueSettings.urlValue->clear();

    if( website.isValid() )
    {
        ui_VenueSettings.websiteValue->setText( i18nc("@label:textbox Url label", "link") );
        ui_VenueSettings.websiteValue->setTipText( website.url() );
        ui_VenueSettings.websiteValue->setUrl( website.url() );
    }
    else
        ui_VenueSettings.websiteValue->clear();

    if( photoUrl.isValid() )
    {
        The::networkAccessManager()->getData( photoUrl, this,
             SLOT(venuePhotoResult(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
    }
    else
    {
        ui_VenueSettings.photoLabel->hide();
        ui_VenueSettings.photoLabel->clear();
    }
}

void
UpcomingEventsApplet::searchVenue( const QString &text )
{
    KUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "method", "venue.search" );
    url.addQueryItem( "api_key", Amarok::lastfmApiKey() );
    url.addQueryItem( "venue", text );
    int currentCountryIndex = ui_VenueSettings.countryCombo->currentIndex();
    const QString &countryCode = ui_VenueSettings.countryCombo->itemData( currentCountryIndex ).toString();
    if( !countryCode.isEmpty() )
        url.addQueryItem( "country", countryCode );
    The::networkAccessManager()->getData( url, this,
         SLOT(venueResults(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

void
UpcomingEventsApplet::venueResults( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    Q_UNUSED( url )
    if( e.code != QNetworkReply::NoError )
    {
        debug() << "Failed to get venue results:" << e.description;
        return;
    }

    ui_VenueSettings.searchResultsList->clear();
    QXmlStreamReader xml( data );
    while( !xml.atEnd() )
    {
        xml.readNext();
        if( xml.isStartElement() && xml.name() == "venue" )
        {
            LastFmVenueXmlParser venueParser( xml );
            if( venueParser.read() )
            {
                QListWidgetItem *item = new QListWidgetItem;

                LastFmVenuePtr venue = venueParser.venue();
                item->setData( VenueIdRole, venue->id );
                item->setData( VenueNameRole, venue->name );
                item->setData( VenuePhotoUrlRole, venue->imageUrls[LastFmEvent::Large] );
                item->setData( VenueUrlRole, venue->url );
                item->setData( VenueWebsiteRole, venue->website );

                LastFmLocationPtr location = venue->location;
                item->setData( VenueCityRole, location->city );
                item->setData( VenueCountryRole, location->country );
                item->setData( VenueStreetRole, location->street );

                item->setText( QString( "%1, %2" )
                               .arg( item->data( VenueNameRole ).toString() )
                               .arg( item->data( VenueCityRole ).toString() ) );
                ui_VenueSettings.searchResultsList->addItem( item );
            }
        }
    }
}

void
UpcomingEventsApplet::venuePhotoResult( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    Q_UNUSED( url )
    if( e.code != QNetworkReply::NoError )
    {
        debug() << "Failed to get venue photo:" << e.description;
        return;
    }

    QPixmap photo;
    if( photo.loadFromData( data ) )
    {
        photo = photo.scaled( 140, 140, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        photo = The::svgHandler()->addBordersToPixmap( photo, 5, QString(), true );
        ui_VenueSettings.photoLabel->setPixmap( photo );
        ui_VenueSettings.photoLabel->show();
    }
}

QList<UpcomingEventsApplet::VenueData>
UpcomingEventsApplet::venueStringToDataList( const QStringList &list )
{
    // config qstringlist is stored as format: QString(id;name;city), QString(id;name;city), ...
    QList<VenueData> dataList;
    foreach( const QString &item, list )
    {
        const QStringList &frag = item.split( QChar(';') );
        VenueData data = { frag.at( 0 ).toInt(), frag.at( 1 ), frag.at( 2 ) };
        dataList << data;
    }
    return dataList;
}

void
UpcomingEventsApplet::openUrl( const QString &url )
{
    QDesktopServices::openUrl( QUrl(url) );
}

UpcomingEventsMapWidget *
UpcomingEventsApplet::mapView()
{
    if( m_stack->hasItem("venuemapview") )
    {
        UpcomingEventsStackItem *item = m_stack->item( "venuemapview" );
        return static_cast<UpcomingEventsMapWidget*>( item->widget() );
    }

    UpcomingEventsStackItem *stackItem = m_stack->create( QLatin1String("venuemapview") );
    UpcomingEventsMapWidget *view = new UpcomingEventsMapWidget( stackItem );
    stackItem->setIcon( KIcon( "edit-find" ) );
    stackItem->setTitle( i18n( "Map View" ) );
    stackItem->setWidget( view );
    stackItem->setMinimumWidth( 50 );
    stackItem->showCloseButton();
    m_stack->setMinimumWidth( 50 );
    const QRegExp pattern( QLatin1String("^(?!(venuemapview|calendar)).*$") );
    QList<UpcomingEventsStackItem*> eventItems = m_stack->items( pattern );
    foreach( UpcomingEventsStackItem *item, eventItems )
    {
        typedef UpcomingEventsListWidget LW;
        if( item )
            view->addEventsListWidget( qgraphicsitem_cast<LW*>( item->widget() ) );
    }
    connect( this, SIGNAL(listWidgetAdded(UpcomingEventsListWidget*)),
             view, SLOT(addEventsListWidget(UpcomingEventsListWidget*)) );
    connect( this, SIGNAL(listWidgetRemoved(UpcomingEventsListWidget*)),
             view, SLOT(removeEventsListWidget(UpcomingEventsListWidget*)) );
    return view;
}

void
UpcomingEventsApplet::collapseStateChanged()
{
    emit sizeHintChanged( Qt::PreferredSize );
}

void
UpcomingEventsApplet::viewCalendar()
{
    if( m_stack->hasItem("calendar") )
    {
        m_stack->item("calendar")->setCollapsed( false );
        return;
    }

    UpcomingEventsStackItem *stackItem = m_stack->create( QLatin1String("calendar") );
    UpcomingEventsCalendarWidget *calendar = new UpcomingEventsCalendarWidget( stackItem );
    stackItem->setIcon( KIcon( "view-calendar" ) );
    stackItem->setTitle( i18n( "Events Calendar" ) );
    stackItem->setWidget( calendar );
    stackItem->setMinimumWidth( 50 );
    stackItem->showCloseButton();
    stackItem->addAction( "jumptotoday", calendar->todayAction() );
    const QRegExp pattern( QLatin1String("^(?!(venuemapview|calendar)).*$") );
    QList<UpcomingEventsStackItem*> eventItems = m_stack->items( pattern );
    foreach( UpcomingEventsStackItem *item, eventItems )
    {
        typedef UpcomingEventsListWidget LW;
        if( item )
            calendar->addEvents( qgraphicsitem_cast<LW*>( item->widget() )->events() );
    }
}

QString
UpcomingEventsApplet::currentTimeSpan()
{
    QString span = ui_GeneralSettings.filterComboBox->currentText();
    if( span == i18n("This week") )
        return "ThisWeek";
    else if( span == i18n("This month") )
        return "ThisMonth";
    else if( span == i18n("This year") )
        return "ThisYear";
    else
        return "AllEvents";
}

void
UpcomingEventsApplet::navigateToArtist()
{
    if( m_artistEventsList->name().isEmpty() )
        return;

    AmarokUrl url;
    url.setCommand( "navigate" );
    url.setPath( "collections" );
    url.setArg( "filter", "artist:\"" + m_artistEventsList->name() + "\"" );
    url.run();
}

void
UpcomingEventsApplet::handleMapRequest( QObject *widget )
{
    if( !mapView()->isLoaded() )
    {
        UpcomingEventsWidget *eventWidget = static_cast<UpcomingEventsWidget*>( widget );
        LastFmVenuePtr venue = eventWidget->eventPtr()->venue();
        mapView()->centerAt( venue );
        m_stack->maximizeItem( QLatin1String("venuemapview") );
    }
}

void
UpcomingEventsApplet::listWidgetDestroyed( QObject *obj )
{
    UpcomingEventsListWidget *widget = static_cast<UpcomingEventsListWidget*>( obj );
    emit listWidgetRemoved( widget );
}

void
UpcomingEventsApplet::saveTimeSpan()
{
    DEBUG_BLOCK
    Amarok::config("UpcomingEvents Applet").writeEntry( "timeSpan", currentTimeSpan() );
    dataEngine( "amarok-upcomingEvents" )->query( QString( "timespan:update" ) );
}

void
UpcomingEventsApplet::saveSettings()
{
    clearVenueItems();
    saveTimeSpan();

    // save venue settings
    QStringList venueConfig;
    m_favoriteVenues.clear();
    for( int i = 0, count = ui_VenueSettings.selectedVenuesList->count() ; i < count; ++i )
    {
        int itemId = ui_VenueSettings.selectedVenuesList->item( i )->data( VenueIdRole ).toString().toInt();
        QString itemCity = ui_VenueSettings.selectedVenuesList->item( i )->data( VenueCityRole ).toString();
        QString itemName = ui_VenueSettings.selectedVenuesList->item( i )->data( VenueNameRole ).toString();
        VenueData data = { itemId, itemName, itemCity };
        m_favoriteVenues << data;
        venueConfig << (QStringList() << QString::number(itemId) << itemName << itemCity).join( QChar(';') );
    }
    Amarok::config("UpcomingEvents Applet").writeEntry( "favVenues", venueConfig );

    enableVenueGrouping( ui_GeneralSettings.groupVenueCheckBox->checkState() == Qt::Checked );
    Amarok::config("UpcomingEvents Applet").writeEntry( "groupVenues", m_groupVenues );

    if( !m_favoriteVenues.isEmpty() )
        dataEngine( "amarok-upcomingEvents" )->query( "venueevents:update" );
}

void
UpcomingEventsApplet::enableVenueGrouping( bool enable )
{
    m_groupVenues = enable;
    if( enable )
    {
        if( !m_stack->hasItem("favoritevenuesgroup") )
        {
            UpcomingEventsStackItem *item = m_stack->create( QLatin1String("favoritevenuesgroup") );
            UpcomingEventsListWidget *listWidget = new UpcomingEventsListWidget( item );
            listWidget->setName( i18nc( "@title:group", "Favorite Venues" ) );
            QString title = i18ncp("@title:group Number of upcoming events",
                                   "%1: 1 event", "%1: %2 events",
                                   listWidget->name(), listWidget->count());
            item->setTitle( title );
            item->setIcon( "favorites" );
            item->setWidget( listWidget );
            connect( listWidget, SIGNAL(mapRequested(QObject*)), SLOT(handleMapRequest(QObject*)) );
            connect( listWidget, SIGNAL(destroyed(QObject*)), SLOT(listWidgetDestroyed(QObject*)) );
            emit listWidgetAdded( listWidget );
        }
    }
    else
    {
        m_stack->remove( QLatin1String("favoritevenuesgroup" ) );
    }
    updateConstraints();
}

