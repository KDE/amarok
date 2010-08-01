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

#include "App.h"
#include "amarokurls/AmarokUrl.h"
#include "context/applets/upcomingevents/LastFmEvent.h"
#include "context/widgets/TextScrollingWidget.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "SvgHandler.h"
#include "LastFmEventXmlParser.h"
#include "UpcomingEventsMapWidget.h"
#include "UpcomingEventsCalendarWidget.h"

#include <KConfigDialog>
#include <KGlobalSettings>
#include <KStandardDirs>
#include <Plasma/Extender>
#include <Plasma/ExtenderItem>
#include <Plasma/IconWidget>
#include <Plasma/Theme>
#include <Plasma/WebView>
#include <Plasma/Svg>

#include <QDesktopServices>
#include <QGraphicsLayoutItem>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QSignalMapper>
#include <QXmlStreamReader>

UpcomingEventsApplet::UpcomingEventsApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_headerLabel( 0 )
    , m_toolBoxIconSize( 0.0 )
    , m_groupVenues( false )
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );
    updateToolBoxIconSize();
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(themeChanged()));
}

UpcomingEventsApplet::~UpcomingEventsApplet()
{
    // Remove all items from the extender, so that their configs are not saved
    // into our rc file. The default configs saved are the positions of the
    // extender items and is meant for the plasma desktop.
    QList<Plasma::ExtenderItem*> extenderItems;
    extenderItems << extender()->attachedItems() << extender()->detachedItems();
    qDeleteAll( extenderItems );
}

void
UpcomingEventsApplet::init()
{
    // Call the base implementation.
    Context::Applet::init();

    setBackgroundHints( Plasma::Applet::NoBackground );

    Plasma::Svg svg;
    svg.setImagePath( "widgets/configuration-icons" );
    m_maximizeIcon = svg.pixmap( "restore" );
    m_maximizeSignalMapper = new QSignalMapper( this );
    connect( m_maximizeSignalMapper, SIGNAL(mapped(QString)), this, SLOT(maximizeExtenderItem(QString)) );

    QAction *calendarAction = new QAction( this );
    calendarAction->setIcon( KIcon( "view-calendar" ) );
    calendarAction->setToolTip( i18n( "View Events Calendar" ) );
    Plasma::IconWidget *calendarIcon = addAction( calendarAction );
    connect( calendarIcon, SIGNAL(clicked()), this, SLOT(viewCalendar()) );

    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setToolTip( i18n( "Settings" ) );
    settingsAction->setEnabled( true );
    Plasma::IconWidget *settingsIcon = addAction( settingsAction );
    connect( settingsIcon, SIGNAL(clicked()), this, SLOT(configure()) );

    // Use the same font as the other applets
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerLabel = new TextScrollingWidget( this );
    m_headerLabel->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_headerLabel->setFont( labelFont );
    m_headerLabel->setText( i18n( "Upcoming Events" ) );
    m_headerLabel->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );

    QGraphicsLinearLayout *headerLayout = new QGraphicsLinearLayout( Qt::Horizontal );
    headerLayout->addItem( calendarIcon );
    headerLayout->addItem( m_headerLabel );
    headerLayout->addItem( settingsIcon );
    headerLayout->setContentsMargins( 0, 4, 0, 2 );

    m_artistExtenderItem = new Plasma::ExtenderItem( extender() );
    m_artistEventsList = new UpcomingEventsListWidget( m_artistExtenderItem );
    m_artistExtenderItem->setTitle( i18nc( "@title:group", "No track is currently playing" ) );
    m_artistExtenderItem->setName( "currentartistevents" );
    m_artistExtenderItem->setWidget( m_artistEventsList );
    m_artistExtenderItem->setCollapsed( true );
    m_artistExtenderItem->setIcon( KIcon("filename-artist-amarok") );
    addMaximizeAction( m_artistExtenderItem );
    connect( m_artistEventsList, SIGNAL(mapRequested(QObject*)), SLOT(handleMapRequest(QObject*)) );

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Vertical );
    layout->addItem( headerLayout );
    layout->addItem( extender() );
    setLayout( layout );
    connect( extender(), SIGNAL(geometryChanged()), SLOT(updateConstraintsSlot()) );

    // ask for all the CV height
    resize( 500, -1 );

    Plasma::DataEngine *engine = dataEngine( "amarok-upcomingEvents" );
    connect( engine, SIGNAL(sourceAdded(QString)), SLOT(engineSourceAdded(QString)) );
    engine->query( "venueevents" );
    engine->query( "artistevents" );

    // Read config and inform the engine.
    enableVenueGrouping( Amarok::config("UpcomingEvents Applet").readEntry( "groupVenues", false ) );
    QStringList venueData = Amarok::config("UpcomingEvents Applet").readEntry( "favVenues", QStringList() );
    m_favoriteVenues = venueStringToDataList( venueData );

    updateConstraints();
    update();
}

void
UpcomingEventsApplet::engineSourceAdded( const QString &source )
{
    if( source == "artistevents" || source == "venueevents" )
        dataEngine( "amarok-upcomingEvents" )->connectSource( source, this );
}

void
UpcomingEventsApplet::updateConstraintsSlot()
{
    updateConstraints();
    update();
}

void
UpcomingEventsApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints );
    prepareGeometryChange();
    qreal padding = standardPadding();
    m_headerLabel->setScrollingText( i18n( "Upcoming Events" ) );

    // how many extender items are expanded
    int expandedCount( 0 );
    QList<Plasma::ExtenderItem*> existingItems = extender()->items();
    foreach( Plasma::ExtenderItem *item, existingItems )
    {
        if( !item->isCollapsed() )
            ++expandedCount;
    }

    // set the dimensions correctly for all extender items depending
    // on whether they are collapsed or not.
    int itemsCount = extender()->items().count();
    qreal verticalMargins = 8;
    qreal scrollAreaH = size().height() - extender()->pos().y();
    qreal itemHeight = scrollAreaH - itemsCount * (m_toolBoxIconSize + verticalMargins + padding + 2);
    if( expandedCount > 0 )
        itemHeight /= expandedCount;

    qreal rightMargin;
    layout()->getContentsMargins( 0, 0, &rightMargin, 0 );
    qreal scrollAreaW = size().width() - rightMargin / 2.0 - padding;
    foreach( Plasma::ExtenderItem *item, existingItems )
    {
        QGraphicsWidget *widget = static_cast<QGraphicsWidget*>(item->widget());
        widget->setMaximumWidth( scrollAreaW - padding );
        item->setMaximumWidth( scrollAreaW );
        if( expandedCount > 0 && !item->isCollapsed() )
        {
            widget->setMinimumHeight( itemHeight );
            widget->setMaximumHeight( itemHeight );
            widget->resize( scrollAreaW - padding, itemHeight );
        }
    }
}

void
UpcomingEventsApplet::dataUpdated( const QString &source, const Plasma::DataEngine::Data &data )
{
    const LastFmEvent::List &events = data[ "events" ].value< LastFmEvent::List >();
    if( source == "artistevents" )
    {
        m_artistEventsList->clear();
        QString artistName = data[ "artist" ].toString();
        addToExtenderItem( m_artistExtenderItem, events, artistName );
        m_artistEventsList->setName( artistName );
        if( !m_artistExtenderItem->action( "showinmediasources" ) )
        {
            QAction *act = new QAction( KIcon("edit-find"), QString(), m_artistExtenderItem );
            act->setToolTip( i18n( "Show in Media Sources" ) );
            connect( act, SIGNAL(triggered()), this, SLOT(navigateToArtist()) );
            m_artistExtenderItem->addAction( "showinmediasources", act );
        }
    }
    else if( source == "venueevents" )
    {
        if( !events.isEmpty() )
        {
            LastFmVenuePtr venue = data[ "venue" ].value<LastFmVenuePtr>();
            if( m_groupVenues )
            {
                QString title = i18n( "Favorite Venues" );
                addToExtenderItem( extender()->item("favoritevenuesgroup"), events, title );
            }
            else
            {
                if( extender()->hasItem( venue->name ) )
                    extender()->item( venue->name )->destroy();
                Plasma::ExtenderItem *extenderItem = new Plasma::ExtenderItem( extender() );
                UpcomingEventsListWidget *listWidget = new UpcomingEventsListWidget( extenderItem );
                listWidget->setName( venue->name );
                extenderItem->setName( venue->name );
                extenderItem->setWidget( listWidget );
                extenderItem->setCollapsed( true );
                extenderItem->setIcon( KIcon("favorites") );
                extenderItem->showCloseButton();
                addMaximizeAction( extenderItem );
                addToExtenderItem( extenderItem, events, venue->name );
                connect( listWidget, SIGNAL(mapRequested(QObject*)), SLOT(handleMapRequest(QObject*)) );
                connect( listWidget, SIGNAL(destroyed(QObject*)), SLOT(listWidgetDestroyed(QObject*)) );
                emit listWidgetAdded( listWidget );
            }
            update();
        }
    }
}

void
UpcomingEventsApplet::clearVenueItems()
{
    foreach( Plasma::ExtenderItem *item, extender()->items() )
    {
        if( item == m_artistExtenderItem )
            continue;
        if( item->name() == "calendar" )
            continue;
        if( item->name() == "favoritevenuesgroup" )
        {
            static_cast<UpcomingEventsListWidget*>( item->widget() )->clear();
            continue;
        }
        if( item->name() == "venuemapview" )
        {
            static_cast<UpcomingEventsMapWidget*>( item->widget() )->clear();
            continue;
        }
        item->destroy();
    }
}

void
UpcomingEventsApplet::addToExtenderItem( Plasma::ExtenderItem *item,
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
}

void
UpcomingEventsApplet::addMaximizeAction( Plasma::ExtenderItem *item )
{
    if( !item->action( "maximize" ) )
    {
        QAction *act = new QAction( m_maximizeIcon, QString(), item );
        act->setToolTip( i18n( "Maximize" ) );
        connect( act, SIGNAL(triggered()), m_maximizeSignalMapper, SLOT(map()) );
        m_maximizeSignalMapper->setMapping( act, item->name() );
        item->addAction( "maximize", act );
    }
}

void
UpcomingEventsApplet::maximizeExtenderItem( const QString &name )
{
    if( extender()->hasItem( name ) )
    {
        extender()->item( name )->setCollapsed( false );
        foreach( Plasma::ExtenderItem *item, extender()->items() )
        {
            if( item->name() != name )
                item->setCollapsed( true );
        }
    }
}

void
UpcomingEventsApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option )
    Q_UNUSED( contentsRect )

    p->setRenderHint( QPainter::Antialiasing );
    addGradientToAppletBackground( p );
    if( !m_headerLabel->isAnimating() )
        drawRoundedRectAroundText( p, m_headerLabel );
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
        ui_VenueSettings.urlValue->setText( i18n("link") );
        ui_VenueSettings.urlValue->setUrl( url.url() );
    }
    else
        ui_VenueSettings.urlValue->clear();

    if( website.isValid() )
    {
        ui_VenueSettings.websiteValue->setText( i18n("link") );
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

void
UpcomingEventsApplet::updateToolBoxIconSize()
{
    Plasma::FrameSvg *background = new Plasma::FrameSvg(this);
    background->setImagePath("widgets/extender-dragger");
    background->resize();
    QSizeF size = background->elementSize("hint-preferred-icon-size");
    size = size.expandedTo(QSizeF(KIconLoader::SizeSmall,KIconLoader::SizeSmall));
    Plasma::Theme *theme = Plasma::Theme::defaultTheme();
    QFont font = theme->font(Plasma::Theme::DefaultFont);
    QFontMetrics fm(font);
    m_toolBoxIconSize = qMax(size.height(), (qreal) fm.height());
}

void
UpcomingEventsApplet::themeChanged()
{
    updateToolBoxIconSize();
}

UpcomingEventsMapWidget *
UpcomingEventsApplet::mapView( bool expand )
{
    if( extender()->hasItem("venuemapview") )
    {
        Plasma::ExtenderItem *item = extender()->item( "venuemapview" );
        item->setCollapsed( !expand );
        return static_cast<UpcomingEventsMapWidget*>( item->widget() );
    }

    Plasma::ExtenderItem *extenderItem = new Plasma::ExtenderItem( extender() );
    UpcomingEventsMapWidget *view = new UpcomingEventsMapWidget( extenderItem );
    extenderItem->setIcon( KIcon( "edit-find" ) );
    extenderItem->setTitle( i18n( "Map View" ) );
    extenderItem->setName( "venuemapview" );
    extenderItem->setWidget( view );
    extenderItem->setMinimumWidth( 50 );
    extenderItem->showCloseButton();
    addMaximizeAction( extenderItem );
    extender()->setMinimumWidth( 50 );
    foreach( Plasma::ExtenderItem *item, extender()->items() )
    {
        if( item->name() == "venuemapview" )
            continue;
        if( item->name() == "calendar" )
            continue;
        typedef UpcomingEventsListWidget LW;
        view->addEventsListWidget( qgraphicsitem_cast<LW*>( item->widget() ) );
    }
    connect( this, SIGNAL(listWidgetAdded(UpcomingEventsListWidget*)),
             view, SLOT(addEventsListWidget(UpcomingEventsListWidget*)) );
    connect( this, SIGNAL(listWidgetRemoved(UpcomingEventsListWidget*)),
             view, SLOT(removeEventsListWidget(UpcomingEventsListWidget*)) );
    return view;
}

void
UpcomingEventsApplet::viewCalendar()
{
    if( extender()->hasItem("calendar") )
    {
        extender()->item("calendar")->setCollapsed( false );
        return;
    }

    Plasma::ExtenderItem *extenderItem = new Plasma::ExtenderItem( extender() );
    UpcomingEventsCalendarWidget *calendar = new UpcomingEventsCalendarWidget( extenderItem );

    extenderItem->setIcon( KIcon( "view-calendar" ) );
    extenderItem->setTitle( i18n( "Events Calender" ) );
    extenderItem->setName( "calendar" );
    extenderItem->setWidget( calendar );
    extenderItem->setMinimumWidth( 50 );
    extenderItem->showCloseButton();
    extenderItem->addAction( "jumptotoday", calendar->todayAction() );
    addMaximizeAction( extenderItem );
    foreach( Plasma::ExtenderItem *item, extender()->items() )
    {
        if( item->name() == "venuemapview" )
            continue;
        if( item->name() == "calendar" )
            continue;
        typedef UpcomingEventsListWidget LW;
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
    url.appendArg( "filter", "artist:\"" + m_artistEventsList->name() + "\"" );
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
        if( !extender()->hasItem("favoritevenuesgroup") )
        {
            Plasma::ExtenderItem *item = new Plasma::ExtenderItem( extender() );
            UpcomingEventsListWidget *listWidget = new UpcomingEventsListWidget( item );
            listWidget->setName( i18nc( "@title:group", "Favorite Venues" ) );
            item->setName( "favoritevenuesgroup" );
            item->setIcon( "favorites" );
            item->setWidget( listWidget );
            addMaximizeAction( item );
            connect( listWidget, SIGNAL(mapRequested(QObject*)), SLOT(handleMapRequest(QObject*)) );
            connect( listWidget, SIGNAL(destroyed(QObject*)), SLOT(listWidgetDestroyed(QObject*)) );
            emit listWidgetAdded( listWidget );
        }
    }
    else
    {
        if( extender()->hasItem("favoritevenuesgroup") )
            extender()->item("favoritevenuesgroup")->destroy();
    }
    updateConstraints();
}

#include "UpcomingEventsApplet.moc"
