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
#include "context/widgets/AppletHeader.h"

#include <KConfigDialog>
#include <KConfigGroup>
#include <QDialog>
#include <Plasma/IconWidget>
#include <Plasma/Label>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

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

    EngineController *engine = The::engineController();
    connect( engine, SIGNAL(stopped(qint64,qint64)), this, SLOT(stopped()) );
}

TabsApplet::~TabsApplet()
{
    DEBUG_BLOCK
    delete m_tabsView;
    if( m_reloadIcon )
        delete m_reloadIcon.data();
}

/**
 * \brief Initialization
 *
 * Initializes the TabsApplet with default parameters
 */
void
TabsApplet::init()
{
    // applet base initialization
    Context::Applet::init();

    // create the header label
    enableHeader( true );
    setHeaderText( i18nc( "Guitar tablature", "Tabs" ) );

    // creates the tab view
    m_tabsView = new TabsView( this );

    // Set the collapse size
    setCollapseOffHeight( -1 );
    setCollapseHeight( m_header->height() );
    setMinimumHeight( collapseHeight() );
    setPreferredHeight( collapseHeight() );

    // create the reload icon
    QAction* reloadAction = new QAction( this );
    reloadAction->setIcon( QIcon::fromTheme( "view-refresh" ) );
    reloadAction->setVisible( true );
    reloadAction->setEnabled( true );
    reloadAction->setText( i18nc( "Guitar tablature", "Reload tabs" ) );
    m_reloadIcon = addLeftHeaderAction( reloadAction );
    m_reloadIcon.data()->setEnabled( false );
    connect( m_reloadIcon.data(), SIGNAL(clicked()), this, SLOT(reloadTabs()) );

    // create the settings icon
    QAction* settingsAction = new QAction( this );
    settingsAction->setIcon( QIcon::fromTheme( "preferences-system" ) );
    settingsAction->setEnabled( true );
    settingsAction->setText( i18n( "Settings" ) );
    QWeakPointer<Plasma::IconWidget> settingsIcon = addRightHeaderAction( settingsAction );
    connect( settingsIcon.data(), SIGNAL(clicked()), this, SLOT(showConfigurationInterface()) );

    m_layout = new QGraphicsLinearLayout( Qt::Vertical );
    m_layout->addItem( m_header );
    m_layout->addItem( m_tabsView );
    setLayout( m_layout );

    // read configuration data and update the engine.
    KConfigGroup config = Amarok::config("Tabs Applet");
    m_fetchGuitar = config.readEntry( "FetchGuitar", true );
    m_fetchBass = config.readEntry( "FetchBass", true );

    Plasma::DataEngine *engine = dataEngine( "amarok-tabs" );
    engine->setProperty( "fetchGuitarTabs", m_fetchGuitar );
    engine->setProperty( "fetchBassTabs", m_fetchBass );
    engine->connectSource( "tabs", this );

    updateInterface( InitState );
}

void
TabsApplet::stopped()
{
    setHeaderText( i18nc( "Guitar tablature", "Tabs" ) );
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
        setHeaderText( i18nc( "Guitar tablature", "Tabs" ) );
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
        setHeaderText( i18nc( "Guitar tablature", "Tabs: Fetching..." ) );
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
        setHeaderText( i18nc( "Guitar tablature", "No Tabs for %1 by %2", titleName, artistName ) );
        updateInterface( NoTabsState );
        return;
    }
    else if( data.contains( "state" ) && state.contains( "FetchError") )
    {
        setHeaderText( i18nc( "Guitar tablature", "Tabs: Fetch Error" ) );
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
                    setHeaderText( i18nc( "Guitar tablature", "Tabs: %1 - %2", titleName, artistName ) );
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
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    parent->setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    parent->connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    parent->connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    mainLayout->addWidget(buttonBox);

    QWidget *settings = new QWidget;
    ui_Settings.setupUi( settings );

    if( m_fetchGuitar )
        ui_Settings.cbFetchGuitar->setChecked( true );
    if( m_fetchBass )
        ui_Settings.cbFetchBass->setChecked( true );

    parent->addPage( settings, i18nc( "Guitar tablature settings", "Tabs Settings" ), "preferences-system" );
    connect( parent, SIGNAL(accepted()), this, SLOT(saveSettings()) );
}

void
TabsApplet::saveSettings()
{
    DEBUG_BLOCK
    KConfigGroup config = Amarok::config("Tabs Applet");

    bool fetchGuitar = ui_Settings.cbFetchGuitar->isChecked();
    bool fetchBass = ui_Settings.cbFetchBass->isChecked();

    // check if any setting has changed
    bool forceUpdate = false;
    if( m_fetchGuitar != fetchGuitar || m_fetchBass != fetchBass )
        forceUpdate = true;

    if( forceUpdate )
    {
        m_fetchGuitar = fetchGuitar;
        m_fetchBass = fetchBass;

        config.writeEntry( "FetchGuitar", m_fetchGuitar );
        config.writeEntry( "FetchBass", m_fetchBass );

        Plasma::DataEngine *engine = dataEngine( "amarok-tabs" );
        engine->setProperty( "fetchGuitarTabs", m_fetchGuitar );
        engine->setProperty( "fetchBassTabs", m_fetchBass );
        engine->query( QLatin1String( "tabs:forceUpdate" ) );
    }
}

void
TabsApplet::reloadTabs()
{
    DEBUG_BLOCK
    QDialog reloadDialog;
    QWidget *reloadWidget = new QWidget( &reloadDialog );

    Ui::ReloadEditDialog *reloadUI = new Ui::ReloadEditDialog();
    reloadUI->setupUi( reloadWidget );

    reloadDialog.setWindowTitle( i18nc( "Guitar tablature", "Reload Tabs" ) );

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    reloadDialog.setLayout(mainLayout);
    mainLayout->addWidget(reloadWidget);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    reloadDialog.connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    reloadDialog.connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    mainLayout->addWidget(buttonBox);

    // query engine for current artist and title
    Plasma::DataEngine *engine = dataEngine( "amarok-tabs" );
    QString artistName = engine->property( "artistName" ).toString();
    QString titleName = engine->property( "titleName" ).toString();

    // update ui
    reloadUI->artistLineEdit->setText( artistName );
    reloadUI->titleLineEdit->setText( titleName );

    if( reloadDialog.exec() == QDialog::Accepted )
    {
        QString newArtist = reloadUI->artistLineEdit->text();
        QString newTitle = reloadUI->titleLineEdit->text();
        if ( newArtist != artistName || newTitle != titleName )
        {
            engine->setProperty( "artistName", newArtist );
            engine->setProperty( "titleName", newTitle );
            engine->query( QLatin1String( "tabs:forceUpdateSpecificTitleArtist" ) );
        }
    }
}

