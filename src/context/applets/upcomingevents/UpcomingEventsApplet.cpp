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
{
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );
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
    m_scrollWidget = new Plasma::ExtenderItem( extender() );
    m_scrollWidget->setName( "currentartistevents" );
    m_scrollWidget->setWidget( m_artistEventsList );

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Vertical );
    layout->addItem( headerLayout );
    layout->addItem( extender() );
    setLayout( layout );

    // ask for all the CV height
    resize( 500, -1 );

    Plasma::DataEngine *engine = dataEngine( "amarok-upcomingEvents" );
    connect( engine, SIGNAL(sourceAdded(QString)), SLOT(engineSourceAdded(QString)) );

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
    if( source == "artistevents" )
        dataEngine( "amarok-upcomingEvents" )->connectSource( "artistevents", this );
}

void
UpcomingEventsApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints );
    prepareGeometryChange();
    qreal padding = standardPadding();
    qreal iconWidth = m_settingsIcon->size().width();
    qreal widmax = size().width() - 2 * iconWidth - 6 * padding;
    QRectF rect( ( size().width() - widmax ) / 2, 0 , widmax, 15 );
    m_headerLabel->setScrollingText( i18n( "Upcoming Events" ) );

    qreal scrollAreaW = size().width();
    qreal scrollAreaH = size().height() - m_headerLabel->boundingRect().height() - extender()->pos().y();
    QSizeF scrollAreaSize( scrollAreaW - 2 * padding, scrollAreaH - 2 * padding );
    UpcomingEventsListWidget *scrollArea = static_cast<UpcomingEventsListWidget*>( m_scrollWidget->widget() );
    scrollArea->setMinimumHeight( scrollAreaSize.height() );
    scrollArea->setMaximumHeight( scrollAreaSize.height() );
    m_scrollWidget->setMaximumWidth( scrollAreaSize.width() );
}

void
UpcomingEventsApplet::dataUpdated( const QString &source, const Plasma::DataEngine::Data &data )
{
    const LastFmEvent::List &events = data[ "LastFmEvent" ].value< LastFmEvent::List >();
    if( source == "artistevents" )
    {
        m_artistEventsList->clear();
        for( int i = 0, count = events.count(); i < count; ++i )
        {
            KDateTime limite(KDateTime::currentLocalDateTime());
            bool timeSpanDisabled = false;

            if ( this->m_timeSpan == "ThisWeek")
                limite = limite.addDays( 7 );
            else if( this->m_timeSpan == "ThisMonth" )
                limite = limite.addMonths( 1 );
            else if( this->m_timeSpan == "ThisYear" )
                limite = limite.addYears( 1 );
            else
                timeSpanDisabled = true;

            if( timeSpanDisabled || events.at( i )->date() < limite )
                m_artistEventsList->addEvent( events.at( i ) );
        }

        int eventsAdded = m_artistEventsList->count();
        QString artistName = data[ "artist" ].toString();
        if( 0 == eventsAdded && !artistName.isEmpty() )
        {
            QString title = i18n( "No upcoming events for %1", artistName );
            m_scrollWidget->setTitle( title );
        }
        else
        {
            QString title = artistName.isEmpty()
                ? i18ncp( "@title:group Number of upcoming events", "1 event", "%1 events", eventsAdded )
                : i18ncp( "@title:group Number of upcoming events for an artist",
                          "%1: 1 event", "%1: %2 events", artistName, eventsAdded );
            m_scrollWidget->setTitle( title );
        }
    }
    updateConstraints();
    update();
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
}

#include "UpcomingEventsApplet.moc"
