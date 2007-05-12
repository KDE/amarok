/***************************************************************************
begin                : 2004/02/07
copyright            : (C) Mark Kretschmann
email                : markey@web.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarok.h"
#include "amarokconfig.h"
#include "app.h"
#include "collectiondb.h"
#include "config.h" // Has USE_MYSQL
#include "configdialog.h"
#include "contextbrowser.h"
#include "dbsetup.h"
#include "debug.h"
#include "directorylist.h"
#include "enginecontroller.h"
#include "mediabrowser.h"
#include "mediumpluginmanager.h"
#include "Options1.h"
#include "Options2.h"
#include "Options4.h"
#include "Options5.h"
#include "Options7.h"
#include "Options8.h"
#include "osd.h"
#include "playlistwindow.h"
#include "playerwindow.h"
#include "plugin/pluginconfig.h"
#include "pluginmanager.h"

#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qobjectlist.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qtextcodec.h>
#include <qtooltip.h>
#include <qvbox.h>

#include <kapplication.h> //kapp
#include <kcombobox.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kstandarddirs.h>

namespace Amarok {
    int databaseTypeCode( const QString type )
    {
        // can't use kconfigxt for the database comboxbox since we need the DBConnection id and not the index
        int dbType = DbConnection::sqlite;
        if ( type == "MySQL")
            dbType = DbConnection::mysql;
        else if ( type == "Postgresql" )
            dbType = DbConnection::postgresql;
        return dbType;
    }
}


int AmarokConfigDialog::s_currentPage = 0;

//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

AmarokConfigDialog::AmarokConfigDialog( QWidget *parent, const char* name, KConfigSkeleton *config )
        : KConfigDialog( parent, name, config )
        , m_engineConfig( 0 )
        , m_opt4( 0 )
{
    setWFlags( WDestructiveClose );

    // IMPORTANT Don't simply change the page names, they are used as identifiers in other parts of the app.
            m_opt1 = new Options1( 0, "General" );
#ifdef Q_WS_MAC
            m_opt1->kcfg_ShowSplashscreen->setEnabled(false);
            m_opt1->kcfg_ShowTrayIcon->setEnabled(false);
            m_opt1->kcfg_AnimateTrayIcon->setEnabled(false);
            m_opt1->kcfg_ShowPlayerWindow->setEnabled(false);
#endif
            m_opt2 = new Options2( 0, "Appearance" );
            m_opt4 = new Options4( 0, "Playback" );
#ifdef Q_WS_X11
    Options5 *opt5 = new Options5( 0, "OSD" );
#endif
    QVBox    *opt6 = new QVBox;
            m_opt7 = new Options7( 0, "Collection" );
    Options8 *opt8 = new Options8( 0, "Scrobbler" );
    QVBox    *opt9 = new QVBox;

    // Sound System
    opt6->setName( "Engine" );
    opt6->setSpacing( KDialog::spacingHint() );
    QWidget *groupBox, *aboutEngineButton;
    groupBox            = new QGroupBox( 2, Qt::Horizontal, i18n("Sound System"), opt6 );
    m_engineConfigFrame = new QGroupBox( 1, Qt::Horizontal, opt6 );
    m_soundSystem       = new QComboBox( false, groupBox );
    aboutEngineButton   = new QPushButton( i18n("About"), groupBox );

    QToolTip::add( m_soundSystem, i18n("Click to select the sound system to use for playback.") );
    QToolTip::add( aboutEngineButton, i18n("Click to get the plugin information.") );

    /// Populate the engine selection combo box
    KTrader::OfferList offers = PluginManager::query( "[X-KDE-Amarok-plugintype] == 'engine'" );
    KTrader::OfferList::ConstIterator end( offers.end() );
    for( KTrader::OfferList::ConstIterator it = offers.begin(); it != end; ++it ) {
        // Don't list the <no engine> (void engine) entry if it's not currently active,
        // cause there's no point in choosing this engine (it's a dummy, after all).
        if( (*it)->property( "X-KDE-Amarok-name" ).toString() == "void-engine"
            && AmarokConfig::soundSystem() != "void-engine" ) continue;

        m_soundSystem->insertItem( (*it)->name() );
        // Save name properties in QMap for lookup
        m_pluginName[(*it)->name()] = (*it)->property( "X-KDE-Amarok-name" ).toString();
        m_pluginAmarokName[(*it)->property( "X-KDE-Amarok-name" ).toString()] = (*it)->name();
    }

    // Collection
#if !defined(USE_MYSQL) && !defined(USE_POSTGRESQL)
    m_opt7->databaseBox->hide();
#endif

#ifndef USE_MYSQL
    //FIXME we do this because this widget breaks the Apply button (always enabled).
    //It breaks because it is set to type="password" in the .kcfg file. Setting to
    //type="string" also fixes this bug, but means the password is stored in plain
    //text. This is a temporary fix so that the majority of users get a fixed Apply
    //button.
    delete m_opt7->dbSetupFrame->kcfg_MySqlPassword2;
#endif
    m_opt7->collectionFoldersBox->setColumns( 1 );
    new CollectionSetup( m_opt7->collectionFoldersBox ); //TODO this widget doesn't update the apply/ok buttons

    // Media Devices
    opt9->setName( "Media Devices" );
    opt9->setSpacing( KDialog::spacingHint() );
    QVBox *topbox = new QVBox( opt9 );
    topbox->setSpacing( KDialog::spacingHint() );
    QGroupBox *mediaBox  = new QGroupBox( 2, Qt::Horizontal, i18n("Media Devices"), topbox );
    mediaBox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
    QVBox *vbox = new QVBox( mediaBox );
    vbox->setSpacing( KDialog::spacingHint() );
    m_deviceManager = new MediumPluginManager( vbox );

    QHBox *hbox = new QHBox( topbox );
    hbox->setSpacing( KDialog::spacingHint() );
    hbox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
    KPushButton *autodetect = new KPushButton( i18n( "Autodetect Devices" ), hbox );
    autodetect->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    connect( autodetect, SIGNAL(clicked()), m_deviceManager, SLOT(redetectDevices()) );
    KPushButton *add = new KPushButton( i18n( "Add Device..." ), hbox );
    add->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    connect( add, SIGNAL(clicked()), m_deviceManager, SLOT(newDevice()) );

    QFrame *frame = new QFrame( topbox );
    frame->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    // add pages
    addPage( m_opt1, i18n( "General" ), Amarok::icon( "settings_general" ), i18n( "Configure General Options" ) );
    addPage( m_opt2, i18n( "Appearance" ), Amarok::icon( "settings_view" ), i18n( "Configure Amarok's Appearance" ) );
    addPage( m_opt4, i18n( "Playback" ), Amarok::icon( "settings_playback" ), i18n( "Configure Playback" ) );
#ifdef Q_WS_X11
    addPage( opt5,   i18n( "OSD" ), Amarok::icon( "settings_indicator" ), i18n( "Configure On-Screen-Display" ) );
#endif
    addPage( opt6,   i18n( "Engine" ), Amarok::icon( "settings_engine" ), i18n( "Configure Engine" ) );
    addPage( m_opt7, i18n( "Collection" ), Amarok::icon( "collection" ), i18n( "Configure Collection" ) );
    addPage( opt8,   i18n( "last.fm" ), Amarok::icon( "audioscrobbler" ), i18n( "Configure last.fm Support" ) );
    addPage( opt9,   i18n( "Media Devices" ), Amarok::icon( "device" ), i18n( "Configure Portable Player Support" ) );

    // Show information labels (must be done after insertions)
    QObjectList *list = queryList( "QLabel", "infoPixmap" );
    QPixmap const info = KGlobal::iconLoader()->iconPath( "messagebox_info", -KIcon::SizeHuge );
    for( QObject *label = list->first(); label; label = list->next() )
        static_cast<QLabel*>(label)->setPixmap( info );
    delete list;

    //stop KFont Requesters getting stupidly large
    list = queryList( "QLabel", "m_sampleLabel" );
    for( QObject *label = list->first(); label; label = list->next() )
        static_cast<QLabel*>(label)->setMaximumWidth( 250 );
    delete list;

    connect( m_deviceManager, SIGNAL(changed()), SLOT(updateButtons()) );
    connect( m_soundSystem, SIGNAL(activated( int )), SLOT(updateButtons()) );
    connect( aboutEngineButton, SIGNAL(clicked()), SLOT(aboutEngine()) );
#ifdef Q_WS_X11
    connect( opt5, SIGNAL(settingsChanged()), SLOT(updateButtons()) ); //see options5.ui.h
#endif
    connect( m_opt2->styleComboBox, SIGNAL( activated( int ) ), SLOT( updateButtons() ) );
    connect( m_opt7->dbSetupFrame->databaseEngine, SIGNAL( activated( int ) ), SLOT( updateButtons() ) );
    connect( m_opt1->kComboBox_browser, SIGNAL( activated( int ) ), SLOT( updateButtons() ) );
    connect( m_opt1->kLineEdit_customBrowser, SIGNAL( textChanged( const QString& ) ), SLOT( updateButtons() ) );
}

AmarokConfigDialog::~AmarokConfigDialog()
{
    DEBUG_FUNC_INFO

    s_currentPage = activePageIndex();

    delete m_engineConfig;
    delete m_deviceManager;
}


/** Reimplemented from KConfigDialog */
void AmarokConfigDialog::addPage( QWidget *page, const QString &itemName, const QString &pixmapName, const QString &header, bool manage )
{
    // Add the widget pointer to our list, for later reference
    m_pageList << page;

    KConfigDialog::addPage( page, itemName, pixmapName, header, manage );
}


/** Show page by object name */
void AmarokConfigDialog::showPageByName( const QCString& page )
{
    for( uint index = 0; index < m_pageList.count(); index++ ) {
        if ( m_pageList[index]->name() == page ) {
            KConfigDialog::showPage( index );
            return;
        }
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

/**
 * Update the buttons.
 * REIMPLEMENTED
 */

void AmarokConfigDialog::updateButtons()
{
    KConfigDialog::updateButtons();
}

/**
 * Update the settings from the dialog.
 * Example use: User clicks Ok or Apply button in a configure dialog.
 * REIMPLEMENTED
 */
void AmarokConfigDialog::updateSettings()
{
#ifdef Q_WS_X11
    OSDPreviewWidget *osd = static_cast<OSDPreviewWidget*>( child( "osdpreview" ) );
    AmarokConfig::setOsdAlignment( osd->alignment() );
    AmarokConfig::setOsdYOffset( osd->y() );
    Amarok::OSD::instance()->applySettings();
#endif

    static_cast<CollectionSetup*>(child("CollectionSetup"))->writeConfig();

    if ( m_engineConfig ) m_engineConfig->save();

    AmarokConfig::setExternalBrowser( externalBrowser() );

    // When sound system has changed, update engine config page
    if ( m_soundSystem->currentText() != m_pluginAmarokName[AmarokConfig::soundSystem()] ) {
        AmarokConfig::setSoundSystem( m_pluginName[m_soundSystem->currentText()] );
        emit settingsChanged();
        soundSystemChanged();
    }

    if ( m_opt2->styleComboBox->currentText() != AmarokConfig::contextBrowserStyleSheet() ) {
        //can't use kconfigxt for the style comboxbox's since we need the string, not the index
        AmarokConfig::setContextBrowserStyleSheet( m_opt2->styleComboBox->currentText() );
        ContextBrowser::instance()->reloadStyleSheet();
    }

    int dbType = Amarok::databaseTypeCode( m_opt7->dbSetupFrame->databaseEngine->currentText() );
    if ( dbType != AmarokConfig::databaseEngine().toInt() ) {
        AmarokConfig::setDatabaseEngine( QString::number( dbType ) );
        emit settingsChanged();
    }

    m_deviceManager->finished();

    if( MediaBrowser::isAvailable() )
    {
        PlaylistWindow::self()->addBrowser( "MediaBrowser", MediaBrowser::instance(), i18n( "Media Device" ), Amarok::icon( "device" ) );
        //to re-enable mediabrowser hiding, uncomment this:
        //connect( MediaBrowser::instance(), SIGNAL( availabilityChanged( bool ) ),
        //         PlaylistWindow::self(), SLOT( mbAvailabilityChanged( bool ) ) );

    }

    Amarok::setUseScores( m_opt1->kcfg_UseScores->isChecked() );
    Amarok::setUseRatings( m_opt1->kcfg_UseRatings->isChecked() );

    // The following makes everything with a moodbar redraw itself.
    Amarok::setMoodbarPrefs( m_opt1->kcfg_ShowMoodbar->isChecked(),
                             m_opt1->kcfg_MakeMoodier->isChecked(),
                             m_opt1->kcfg_AlterMood->currentItem(),
                             m_opt1->kcfg_MoodsWithMusic->isChecked() );
}


/**
 * Update the configuration-widgets in the dialog based on Amarok's current settings.
 * Example use: Initialisation of dialog.
 * Example use: User clicks Reset button in a configure dialog.
 * REIMPLEMENTED
 */
void AmarokConfigDialog::updateWidgets()
{
    m_soundSystem->setCurrentText( m_pluginAmarokName[AmarokConfig::soundSystem()] );
    soundSystemChanged();
}


/**
 * Update the configuration-widgets in the dialog based on the default values for Amarok's settings.
 * Example use: User clicks Defaults button in a configure dialog.
 * REIMPLEMENTED
 */
void AmarokConfigDialog::updateWidgetsDefault()
{
    m_soundSystem->setCurrentItem( 0 );
}


//////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
//////////////////////////////////////////////////////////////////////////////////////////

/**
 * @return true if any configuration items we are managing changed from Amarok's stored settings
 * We manage the engine combo box and some of the OSD settings
 * REIMPLEMENTED
 */
bool AmarokConfigDialog::hasChanged()
{
#ifdef Q_WS_X11
    OSDPreviewWidget *osd = static_cast<OSDPreviewWidget*>( child( "osdpreview" ) );
#endif

    return  m_soundSystem->currentText() != m_pluginAmarokName[AmarokConfig::soundSystem()] ||
#ifdef Q_WS_X11
            osd->alignment() != AmarokConfig::osdAlignment() ||
            osd->alignment() != OSDWidget::Center && osd->y() != AmarokConfig::osdYOffset() ||
#endif
            m_opt2->styleComboBox->currentText() != AmarokConfig::contextBrowserStyleSheet() ||
            Amarok::databaseTypeCode(  m_opt7->dbSetupFrame->databaseEngine->currentText()  ) != AmarokConfig::databaseEngine().toInt() ||
            m_engineConfig && m_engineConfig->hasChanged() ||
            m_deviceManager && m_deviceManager->hasChanged() ||
            externalBrowser() != AmarokConfig::externalBrowser();
}


/** REIMPLEMENTED */
bool AmarokConfigDialog::isDefault()
{
    AMAROK_NOTIMPLEMENTED

    //TODO hard to implement - what are default settings for OSD etc?

    return false;
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
//////////////////////////////////////////////////////////////////////////////////////////

void AmarokConfigDialog::aboutEngine() //SLOT
{
    PluginManager::showAbout( QString( "Name == '%1'" ).arg( m_soundSystem->currentText() ) );
}


//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE
//////////////////////////////////////////////////////////////////////////////////////////

void AmarokConfigDialog::soundSystemChanged()
{
    ///A new sound system has been LOADED
    ///If only the sound system widget has been changed don't call this!

    // remove old engine config widget
    // will delete the view if implementation is done correctly
    delete m_engineConfig;

    if( EngineController::hasEngineProperty( "HasConfigure" ) )
    {
        m_engineConfig = EngineController::engine()->configure();
        m_engineConfig->view()->reparent( m_engineConfigFrame, QPoint() );
        m_engineConfig->view()->show();
        m_engineConfigFrame->setTitle( i18n( "to change settings", "Configure %1" ).arg( m_soundSystem->currentText() ) );
        m_engineConfigFrame->show();

        connect( m_engineConfig, SIGNAL(viewChanged()), SLOT(updateButtons()) );
    }
    else {
        m_engineConfig = 0;
        m_engineConfigFrame->hide();
    }

    const bool hasCrossfade = EngineController::hasEngineProperty( "HasCrossfade" );
    const bool crossfadeOn = m_opt4->kcfg_Crossfade->isOn();
    // Enable crossfading option when available
    m_opt4->kcfg_Crossfade->setEnabled( hasCrossfade );
    m_opt4->kcfg_CrossfadeLength->setEnabled( hasCrossfade && crossfadeOn );
    m_opt4->crossfadeLengthLabel->setEnabled( hasCrossfade && crossfadeOn );
    m_opt4->kcfg_CrossfadeType->setEnabled( hasCrossfade && crossfadeOn );

    if (!hasCrossfade)
    {
        m_opt4->radioButtonNormalPlayback->setChecked( true );
    }
}

QString AmarokConfigDialog::externalBrowser() const
{
    return   m_opt1->kComboBox_browser->isEnabled() ?
#ifdef Q_WS_MAC
                 m_opt1->kComboBox_browser->currentText() == i18n( "Default Browser" ) ?
                 "open" :
#else
                 m_opt1->kComboBox_browser->currentText() == i18n( "Default KDE Browser" ) ?
                 "kfmclient openURL" :
#endif
                 m_opt1->kComboBox_browser->currentText().lower() :
             m_opt1->kLineEdit_customBrowser->text().lower();
}


#include "configdialog.moc"
