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

#include "TabsView.h"
#include "TabsItem.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "context/ContextView.h"
#include "context/widgets/TextScrollingWidget.h"
#include "PaletteHandler.h"

#include <Plasma/Theme>
#include <Plasma/IconWidget>

#include <KConfigGroup>
#include <KConfigDialog>

#include <QGraphicsProxyWidget>
#include <QScrollBar>
#include <QTreeView>

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
    , m_model( 0 )
    , m_tabsView( 0 )
    , m_currentState( InitState )
    , m_layout( 0 )
    , m_fetchGuitar( true )
    , m_fetchBass ( true )
{
    DEBUG_BLOCK
    setHasConfigurationInterface( true );
    setBackgroundHints( Plasma::Applet::NoBackground );

    EngineController *engine = The::engineController();

    connect( engine, SIGNAL( stopped( qint64, qint64 ) ),
             this, SLOT( stopped() ) );
}

TabsApplet::~TabsApplet()
{
    DEBUG_BLOCK
    delete m_model;
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

    // defining the initial height of the context view (full height)
    resize( 500, -1 );

    // create the header label
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_titleLabel = new TextScrollingWidget( this );
    m_titleLabel.data()->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_titleLabel.data()->setFont( labelFont );
    m_titleLabel.data()->setText( i18nc( "Guitar tablature", "Tabs" ) );
    m_titleLabel.data()->setDrawBackground( true );
    m_titleLabel.data()->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );

    // defines the collapse size for the context applet
    setCollapseHeight( m_titleLabel.data()->size().height() + 3 * standardPadding() );

    // creates the basic tab view and the corresponding model
    m_model = new QStandardItemModel();
    m_model->setColumnCount( 1 );

    m_tabsView = new TabsView( this );
    m_tabsView->setModel( m_model );

    // Set the collapse size
    setCollapseHeight( m_titleLabel.data()->size().height() + 2 * ( 4 + QApplication::style()->pixelMetric(QStyle::PM_LayoutTopMargin) ) + 3 );

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
    prepareGeometryChange();

    const qreal padding = standardPadding();
    const qreal scrollWidth  = size().width() - padding;
    const qreal scrollHeight = size().height() - m_titleLabel.data()->boundingRect().bottom() - 2 * padding;

    m_titleLabel.data()->setScrollingText( m_titleLabel.data()->text() );
    const QSizeF scrollSize( scrollWidth, scrollHeight );
    m_tabsView->setMaximumSize( scrollSize );

    // increase list-width when scrollbar is shown
    const QScrollBar *vTabListScrollBar = m_tabsView->tabsListView()->verticalScrollBar();
    const qreal vTabListScrollBarWidthOffset = vTabListScrollBar->isVisible() ?  vTabListScrollBar->width() : 0;
    m_tabsView->tabsListView()->setFixedWidth( 48 + vTabListScrollBarWidthOffset );
}

void
TabsApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );
    Q_UNUSED( contentsRect )

    p->setRenderHint( QPainter::Antialiasing );
    addGradientToAppletBackground( p );
}

void
TabsApplet::stopped()
{
    m_titleLabel.data()->setText( i18nc( "Guitar tablature", "Tabs: No track playing" ) );
    updateInterface( StoppedState );
}

void
TabsApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    Q_UNUSED( name )

    // remove the previously fetched stuff
    m_model->clear();
    m_tabsView->setTabTextContent( "" );

    if( data.empty() )
    {
        m_titleLabel.data()->setText( i18nc( "Guitar tablature", "Tabs" ) );
        updateInterface ( InitState );
        return;
    }

    const QString artistName = data[ "artist" ].toString();
    const QString titleName  = data[ "title" ].toString();
    const QString state      = data[ "state" ].toString();
    const QString message    = data[ "message" ].toString();

    // update artist and title in the headerlabel
    if( !artistName.isEmpty()  && !titleName.isEmpty() )
        m_titleLabel.data()->setText( i18nc( "Guitar tablature", "Tabs : %1 - %2", titleName, artistName ) );
    else
        m_titleLabel.data()->setText( i18nc( "Guitar tablature", "Tabs" ) );

    if( data.contains( "state" ) && state.contains( "Fetching" ) )
    {
        updateInterface( FetchingState );
        return;
    }
    else if( data.contains( "state" ) && state.contains( "Stopped") )
    {
        stopped();
        return;
    }
    else if( data.contains( "state" ) && state.contains( "noTabs") )
    {
        if( data.contains( "message" ) )
        {
            // if we've found no tabs and got a message from the engine (e.g. connectivity issues)
            m_tabsView->setTabTextContent( message );
            updateInterface( MsgState );
        }
        else
        {
            // no tabs for the current song were found
            m_titleLabel.data()->setText( i18nc( "Guitar tablature", "No tabs for %1 by %2", titleName, artistName ) );
            updateInterface( NoTabsState );
        }
        return;
    }
    else if( data.contains( "message" ) )
    {
        // if(we get a message, show it
        m_tabsView->setTabTextContent( message );
        updateInterface( MsgState );
        return;
    }

    // getting the tab-data from the engine
    bool tabFound = false;
    for ( int i = 0; i < data.size(); i++ )
    {
        const QString tabId = QString( "tabs:" ).append( QString::number( i ) );
        if( data.contains( tabId ) )
        {
            TabsInfo *item = data[ tabId ].value<TabsInfo *>() ;
            if( item )
            {
                TabsItem *tabsItem = new TabsItem();
                tabsItem->setTab( item );

                m_model->appendRow( tabsItem );
                if( !tabFound )
                {
                    // update the applet and display the first tab in list
                    m_tabsView->showTab( tabsItem );
                    updateInterface ( TabState );
                    tabFound = true;
                }
            }
        }
    }
}

void
TabsApplet::updateInterface( AppletState appletState )
{
    DEBUG_BLOCK
    m_currentState = appletState;

    if( appletState == FetchingState )
        setBusy( true );
    else
        setBusy( false );

    switch ( m_currentState )
    {
        case InitState:
        case StoppedState:
            m_tabsView->hide();
            setCollapseOn();
            m_reloadIcon.data()->setEnabled( false );
            break;
        case NoTabsState:
            m_tabsView->hide();
            setCollapseOn();
            m_reloadIcon.data()->setEnabled( true );
            break;
        case MsgState:
            m_tabsView->show();
            resize( 500, -1 );
            setCollapseOff();
            m_reloadIcon.data()->setEnabled( true );
            break;
        case FetchingState:
            m_tabsView->show();
            resize( 500, -1 );
            setCollapseOff();
            m_reloadIcon.data()->setEnabled( false );
            break;
        case TabState:
            m_tabsView->show();
            resize( 500, -1 );
            setCollapseOff();
            m_reloadIcon.data()->setEnabled( true );
            break;
    }
    emit sizeHintChanged( Qt::PreferredSize );

    constraintsEvent();
}

void
TabsApplet::createConfigurationInterface( KConfigDialog *parent )
{
    KConfigGroup configuration = config();
    QWidget *settings = new QWidget;
    ui_Settings.setupUi( settings );

    if( m_fetchGuitar )
        ui_Settings.cbFetchGuitar->setChecked( true );
    if( m_fetchBass )
        ui_Settings.cbFetchBass->setChecked( true );

    parent->addPage( settings, i18nc( "Guitar tablature settings", "Tabs Settings" ), "preferences-system");
    connect( parent, SIGNAL( accepted() ), this, SLOT( saveSettings( ) ) );
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

    QString artistName = dataEngine( "amarok-tabs" )->query( "tabs")[ "artist" ].toString();
    QString titleName = dataEngine( "amarok-tabs" )->query( "tabs")[ "title" ].toString();

    QDialog *reloadDialogWidget = new QDialog();
    ui_reloadDialog.setupUi( reloadDialogWidget );
    ui_reloadDialog.artistLineEdit->setText( artistName );
    ui_reloadDialog.titleLineEdit->setText( titleName );

    if( reloadDialogWidget->exec() == QDialog::Accepted )
    {
        artistName = ui_reloadDialog.artistLineEdit->text();
        titleName = ui_reloadDialog.titleLineEdit->text();
        if( !artistName.isEmpty() && !titleName.isEmpty() )
            dataEngine( "amarok-tabs" )->query( QString( "tabs:AMAROK_TOKEN:" ).append( artistName ).append( QString( ":AMAROK_TOKEN:").append( titleName ) ) );
    }
}

#include "TabsApplet.moc"
