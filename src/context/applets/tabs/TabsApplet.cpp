/****************************************************************************************
 * Copyright (c) 2010 Rainer Sigle <rainer.sigle@web.de>                                *
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
 * this program.  if not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "TabsApplet"

#include "TabsApplet.h"

#include "TabsItem.h"
#include "TabsView.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "context/ContextView.h"
#include "context/widgets/TextScrollingWidget.h"

#include <KConfigDialog>
#include <KConfigGroup>
#include <KDialog>
#include <Plasma/IconWidget>
#include <Plasma/Label>

/**
 * \brief Constructor
 *
 * TabsApplet constructor
 *
 * \param parent : the TabsApplet parent (used by Context::Applet)
 * \param args   : (used by Context::Applet)
 */
TabsApplet::TabsApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_tabsView( 0 )
    , m_currentState( InitState )
    , m_layout( 0 )
    , m_fetchGuitar( true )
    , m_fetchBass( true )
    , m_showTabBrowser( false )
{
    DEBUG_BLOCK
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );

    EngineController *engine = The::engineController();
    connect( engine, SIGNAL( stopped( qint64, qint64 ) ), this, SLOT( stopped() ) );
}

TabsApplet::~TabsApplet()
{
    DEBUG_BLOCK
    delete m_tabsView;
    if( m_reloadIcon )
        delete m_reloadIcon.data();
    if( m_titleLabel )
        delete m_titleLabel.data();
}

/**
 * \brief Initialization
 *
 * Initializes the TabsApplet with default parameters
 */
void
TabsApplet::init()
{
    // applet base initializtation
    Context::Applet::init();

    // create the header label
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_titleLabel = new TextScrollingWidget( this );
    m_titleLabel.data()->setFont( labelFont );
    m_titleLabel.data()->setText( i18nc( "Guitar tablature", "Tabs" ) );
    m_titleLabel.data()->setDrawBackground( true );
    m_titleLabel.data()->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );

    // creates the tab view
    m_tabsView = new TabsView( this );

    // Set the collapse size
    qreal headerHeight = m_titleLabel.data()->size().height()
            + 2 * QApplication::style()->pixelMetric( QStyle::PM_LayoutTopMargin ) + 6;
    setCollapseHeight( headerHeight );
    setMinimumHeight( collapseHeight() );
    setPreferredHeight( collapseHeight() );

    // create the reload icon
    QAction* reloadAction = new QAction( this );
    reloadAction->setIcon( KIcon( "view-refresh" ) );
    reloadAction->setVisible( true );
    reloadAction->setEnabled( true );
    reloadAction->setText( i18nc( "Guitar tablature", "Reload tabs" ) );
    m_reloadIcon = addAction( reloadAction );
    m_reloadIcon.data()->setEnabled( false );
    connect( m_reloadIcon.data(), SIGNAL( clicked() ), this, SLOT( reloadTabs() ) );

    // create the settings icon
    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setEnabled( true );
    settingsAction->setText( i18n( "Settings" ) );
    QWeakPointer<Plasma::IconWidget> settingsIcon = addAction( settingsAction );
    connect( settingsIcon.data(), SIGNAL( clicked() ), this, SLOT( showConfigurationInterface() ) );

    QGraphicsLinearLayout *headerLayout = new QGraphicsLinearLayout( Qt::Horizontal );
    headerLayout->addItem( m_reloadIcon.data() );
    headerLayout->addItem( m_titleLabel.data() );
    headerLayout->addItem( settingsIcon.data() );
    headerLayout->setContentsMargins( 0, 4, 0, 2 );

    m_layout = new QGraphicsLinearLayout( Qt::Vertical );
    m_layout->addItem( headerLayout );
    m_layout->addItem( m_tabsView );
    setLayout( m_layout );

    // read configuration data and update the engine.
    KConfigGroup config = Amarok::config("Tabs Applet");
    m_fetchGuitar = config.readEntry( "FetchGuitar", true );
    m_fetchBass = config.readEntry( "FetchBass", true );
    dataEngine( "amarok-tabs" )->query( QString( "tabs:fetchGuitar:" ).append( QString::number( m_fetchGuitar ) ) );
    dataEngine( "amarok-tabs" )->query( QString( "tabs:fetchBass:" ).append( QString::number( m_fetchBass ) ) );

    updateInterface( InitState );

    // connect to the tabs data-engine
    connectSource( "tabs" );
    connect( dataEngine( "amarok-tabs" ), SIGNAL( sourceAdded( const QString & ) ), this, SLOT( connectSource( const QString & ) ) );
}

/**
 * Connects the source to the tabs engine and calls the dataUpdated function
 */
void
TabsApplet::connectSource( const QString &source )
{
    if( source == "tabs" )
    {
        dataEngine( "amarok-tabs" )->connectSource( "tabs", this );
        dataUpdated( source, dataEngine( "tabs" )->query( "tabs" ) );
    }
}

void
TabsApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints )
    m_titleLabel.data()->setScrollingText( m_titleLabel.data()->text() );
}

void
TabsApplet::stopped()
{
    m_titleLabel.data()->setText( i18nc( "Guitar tablature", "Tabs" ) );
    updateInterface( StoppedState );
}

void
TabsApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    Q_UNUSED( name )

    // remove previously fetched stuff
    m_tabsView->clear();
    m_tabsView->clearTabBrowser();
    setBusy( false );

    if( data.empty() )
    {
        m_titleLabel.data()->setText( i18nc( "Guitar tablature", "Tabs" ) );
        updateInterface( InitState );
        return;
    }

    const QString state      = data[ "state" ].toString();
    const QString artistName = data[ "artist" ].toString();
    const QString titleName  = data[ "title" ].toString();
    if( data.contains( "state" ) && state.contains( "Fetching" ) )
    {
        if( canAnimate() )
            setBusy( true );
        m_titleLabel.data()->setText( i18n( "Tabs: Fetching ..." ) );
        updateInterface( FetchingState );
        return;
    }
    else if( data.contains( "state" ) && state.contains( "Stopped" ) )
    {
        stopped();
        return;
    }
    else if( data.contains( "state" ) && state.contains( "noTabs") )
    {
        // no tabs for the current track
        m_titleLabel.data()->setText( i18nc( "Guitar tablature", "No tabs for %1 by %2", titleName, artistName ) );
        updateInterface( NoTabsState );
        return;
    }
    else if( data.contains( "state" ) && state.contains( "FetchError") )
    {
        m_titleLabel.data()->setText( i18nc( "Guitar tablature", "Tabs: Fetch Error" ) );
        updateInterface( NoTabsState );
        return;
    }

    // getting the tab-data from the engine
    bool tabFound = false;
    for( int i = 0; i < data.size(); i++ )
    {
        const QString tabId = QString( "tabs:" ).append( QString::number( i ) );
        if( data.contains( tabId ) )
        {
            TabsInfo *item = data[ tabId ].value<TabsInfo *>() ;
            if( item )
            {
                TabsItem *tabsItem = new TabsItem();
                tabsItem->setTab( item );

                m_tabsView->appendTab( tabsItem );
                if( !tabFound )
                {
                    // update the applet and display the first tab in list
                    m_tabsView->showTab( tabsItem );

                    // update artist and title in the headerlabel
                    m_titleLabel.data()->setText( i18nc( "Guitar tablature", "Tabs : %1 - %2", titleName, artistName ) );
                    updateInterface( TabState );
                    tabFound = true;
                }
            }
        }
    }
}

void
TabsApplet::updateInterface( const AppletState appletState )
{
    // return if state has not changed (except for init state)
    if( m_currentState == appletState && appletState != InitState )
        return;

    debug() << "updating interface from state " << m_currentState << " to " << appletState;
    m_currentState = appletState;
    bool collapse = false;

    switch( m_currentState )
    {
        case InitState:
        case StoppedState:
            m_reloadIcon.data()->setEnabled( false );
            m_showTabBrowser = false;
            collapse = true;
            break;
        case NoTabsState:
            m_reloadIcon.data()->setEnabled( true );
            m_showTabBrowser = false;
            collapse = true;
            break;
        case FetchingState:
            m_reloadIcon.data()->setEnabled( false );
            m_showTabBrowser = false;
            break;
        case TabState:
            m_reloadIcon.data()->setEnabled( true );
            m_showTabBrowser = true;
            break;
    }

    QGraphicsLinearLayout *lo = static_cast<QGraphicsLinearLayout*>( layout() );

    m_showTabBrowser ? lo->addItem( m_tabsView ) : lo->removeItem( m_tabsView );
    m_showTabBrowser ? m_tabsView->show() : m_tabsView->hide();

    collapse ? setCollapseOn() : setCollapseOff();

    updateConstraints();
    update();
}

void
TabsApplet::createConfigurationInterface( KConfigDialog *parent )
{
    QWidget *settings = new QWidget;
    ui_Settings.setupUi( settings );

    if( m_fetchGuitar )
        ui_Settings.cbFetchGuitar->setChecked( true );
    if( m_fetchBass )
        ui_Settings.cbFetchBass->setChecked( true );

    parent->addPage( settings, i18nc( "Guitar tablature settings", "Tabs Settings" ), "preferences-system" );
    connect( parent, SIGNAL( accepted() ), this, SLOT( saveSettings() ) );
}

void
TabsApplet::saveSettings()
{
    DEBUG_BLOCK
    KConfigGroup config = Amarok::config("Tabs Applet");

    m_fetchGuitar = ui_Settings.cbFetchGuitar->isChecked();
    m_fetchBass = ui_Settings.cbFetchBass->isChecked();

    config.writeEntry( "FetchGuitar", m_fetchGuitar );
    config.writeEntry( "FetchBass", m_fetchBass );

    dataEngine( "amarok-tabs" )->query( QString( "tabs:fetchGuitar:" ).append( QString::number( m_fetchGuitar ) ) );
    dataEngine( "amarok-tabs" )->query( QString( "tabs:fetchBass:" ).append( QString::number( m_fetchBass ) ) );
}

void
TabsApplet::reloadTabs()
{
    DEBUG_BLOCK
    KDialog reloadDialog;
    QWidget *reloadWidget = new QWidget( &reloadDialog );

    Ui::ReloadEditDialog *reloadUI = new Ui::ReloadEditDialog();
    reloadUI->setupUi( reloadWidget );

    reloadDialog.setCaption( i18n( "Reload Tabs" ) );
    reloadDialog.setButtons( KDialog::Ok|KDialog::Cancel );
    reloadDialog.setDefaultButton( KDialog::Ok );
    reloadDialog.setMainWidget( reloadWidget );

    // query engine for current artist and title
    Plasma::DataEngine::Data data = dataEngine( "amarok-tabs" )->query( "tabs" );
    QString artistName;
    QString titleName;
    if( data.contains( "artist" ) )
        artistName = data[ "artist" ].toString();
    if( data.contains( "title" ) )
        titleName  = data[ "title" ].toString();

    // update ui
    reloadUI->artistLineEdit->setText( artistName );
    reloadUI->titleLineEdit->setText( titleName );

    if( reloadDialog.exec() == KDialog::Accepted )
    {
        artistName = reloadUI->artistLineEdit->text();
        titleName = reloadUI->titleLineEdit->text();
        if( !artistName.isEmpty() && !titleName.isEmpty() )
            dataEngine( "amarok-tabs" )->query( QString( "tabs:AMAROK_TOKEN:%1:AMAROK_TOKEN:%2").arg( artistName ).arg( titleName ) );
    }
}

#include "TabsApplet.moc"
