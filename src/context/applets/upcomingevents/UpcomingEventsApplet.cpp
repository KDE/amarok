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

#include <KConfigDialog>
#include <KDateTime>
#include <KGlobalSettings>
#include <Plasma/Extender>
#include <Plasma/ExtenderItem>
#include <Plasma/IconWidget>
#include <Plasma/ScrollWidget>
#include <Plasma/Theme>

#include <QDesktopServices>
#include <QGraphicsLayoutItem>
#include <QGraphicsLinearLayout>
#include <QXmlStreamReader>

UpcomingEventsApplet::UpcomingEventsApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_headerLabel( 0 )
    , m_settingsIcon( 0 )
    , m_toolBoxIconSize( 0.0 )
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );
    updateToolBoxIconSize();
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(themeChanged()));
}

void
UpcomingEventsApplet::init()
{
    // Call the base implementation.
    Context::Applet::init();

    setBackgroundHints( Plasma::Applet::NoBackground );

    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setEnabled( true );
    m_settingsIcon = addAction( settingsAction );
    m_settingsIcon->setToolTip( i18n( "Settings" ) );
    connect( m_settingsIcon, SIGNAL( clicked() ), this, SLOT( configure() ) );

    // Use the same font as the other applets
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerLabel = new TextScrollingWidget( this );
    m_headerLabel->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_headerLabel->setFont( labelFont );
    m_headerLabel->setText( i18n( "Upcoming Events" ) );
    m_headerLabel->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );

    QGraphicsLinearLayout *headerLayout = new QGraphicsLinearLayout( Qt::Horizontal );
    headerLayout->addItem( m_headerLabel );
    headerLayout->addItem( m_settingsIcon );
    headerLayout->setContentsMargins( 0, 4, 0, 2 );

    m_artistEventsList = new UpcomingEventsListWidget( this );
    m_artistExtenderItem = new Plasma::ExtenderItem( extender() );
    m_artistExtenderItem->setTitle( i18nc( "@title:group", "No track is currently playing" ) );
    m_artistExtenderItem->setName( "currentartistevents" );
    m_artistExtenderItem->setWidget( m_artistEventsList );
    m_artistExtenderItem->setCollapsed( true );

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
    m_timeSpan = Amarok::config("UpcomingEvents Applet").readEntry( "timeSpan", "AllEvents" );
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

    qreal scrollAreaW = size().width() - padding;
    m_artistExtenderItem->setMaximumWidth( scrollAreaW );
    m_artistEventsList->setMaximumWidth( scrollAreaW );
    extender()->setMaximumWidth( scrollAreaW );

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
    foreach( Plasma::ExtenderItem *item, existingItems )
    {
        UpcomingEventsListWidget *widget = static_cast<UpcomingEventsListWidget*>(item->widget());
        widget->setMaximumWidth( scrollAreaW );
        item->setMaximumWidth( scrollAreaW );
        if( expandedCount > 0 && !item->isCollapsed() )
        {
            widget->setMinimumHeight( itemHeight );
            widget->setMaximumHeight( itemHeight );
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
        LastFmEvent::List newEvents = filterEvents( events );
        addToExtenderItem( m_artistExtenderItem, newEvents, artistName );
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
        LastFmEvent::List newEvents = filterEvents( events );
        if( !newEvents.isEmpty() )
        {
            LastFmVenuePtr venue = data[ "venue" ].value<LastFmVenuePtr>();
            if( extender()->hasItem( venue->name ) )
                extender()->item( venue->name )->destroy();

            Plasma::ExtenderItem *extenderItem = new Plasma::ExtenderItem( extender() );
            UpcomingEventsListWidget *listWidget = new UpcomingEventsListWidget( extenderItem );
            listWidget->setName( venue->name );
            extenderItem->setName( venue->name );
            extenderItem->setWidget( listWidget );
            extenderItem->setCollapsed( true );
            extenderItem->showCloseButton();
            addToExtenderItem( extenderItem, newEvents, venue->name );
        }
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

LastFmEvent::List
UpcomingEventsApplet::filterEvents( const LastFmEvent::List &events ) const
{
    KDateTime limite( KDateTime::currentLocalDateTime() );
    bool timeSpanDisabled = false;

    if( m_timeSpan == "ThisWeek")
        limite = limite.addDays( 7 );
    else if( m_timeSpan == "ThisMonth" )
        limite = limite.addMonths( 1 );
    else if( m_timeSpan == "ThisYear" )
        limite = limite.addYears( 1 );
    else
        timeSpanDisabled = true;

    LastFmEvent::List newEvents;
    foreach( const LastFmEventPtr &event, events )
    {
        if( timeSpanDisabled || event->date() < limite )
            newEvents << event;
    }
    return newEvents;
}

void
UpcomingEventsApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option )
    Q_UNUSED( contentsRect )

    p->setRenderHint( QPainter::Antialiasing );

    addGradientToAppletBackground( p );

    //draw background of wiki text
    p->save();
    QColor bg( App::instance()->palette().highlight().color() );
    bg.setHsvF( bg.hueF(), 0.07, 1, bg.alphaF() );

    if( !m_headerLabel->isAnimating() )
        drawRoundedRectAroundText( p, m_headerLabel );

    p->restore();
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

    m_temp_timeSpan = m_timeSpan;

    // TODO bad, it's done manually ...
    if( m_timeSpan == "AllEvents" )
        ui_GeneralSettings.comboBox->setCurrentIndex( 0 );
    else if( m_timeSpan == "ThisWeek" )
        ui_GeneralSettings.comboBox->setCurrentIndex( 1 );
    else if( m_timeSpan == "ThisMonth" )
        ui_GeneralSettings.comboBox->setCurrentIndex( 2 );
    else if( m_timeSpan == "ThisYear" )
        ui_GeneralSettings.comboBox->setCurrentIndex( 3 );

    connect( ui_GeneralSettings.comboBox, SIGNAL(currentIndexChanged(QString)), SLOT(changeTimeSpan(QString)) );
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

void
UpcomingEventsApplet::changeTimeSpan( const QString &span )
{
    DEBUG_BLOCK
    // TODO change this b/c it's BAAADDD !!!

    if (span == i18nc("automatic time span", "Automatic") )
        m_temp_timeSpan = "AllEvents";

    else if (span == i18n("This week") )
        m_temp_timeSpan = "ThisWeek";

    else if (span == i18n("This month") )
        m_temp_timeSpan = "ThisMonth";

    else if (span == i18n("This year") )
        m_temp_timeSpan = "ThisYear";

    else if (span == i18n("All events") )
        m_temp_timeSpan = "AllEvents";
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
UpcomingEventsApplet::saveTimeSpan()
{
    DEBUG_BLOCK

    m_timeSpan = m_temp_timeSpan;
    dataEngine( "amarok-upcomingEvents" )->query( QString( "timespan:" ) + m_timeSpan );

    Amarok::config("UpcomingEvents Applet").writeEntry( "timeSpan", m_timeSpan );
    dataEngine( "amarok-upcomingEvents" )->query( QString( "timespan:" ) + m_timeSpan );
}

void
UpcomingEventsApplet::saveSettings()
{
    saveTimeSpan();

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

    if( !m_favoriteVenues.isEmpty() )
        dataEngine( "amarok-upcomingEvents" )->query( "venueevents:update" );
}

#include "UpcomingEventsApplet.moc"
