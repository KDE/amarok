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

#include "amarokconfig.h"
#include "config.h" // Has USE_MYSQL
#include "configdialog.h"
#include "debug.h"
#include "directorylist.h"
#include "enginecontroller.h"
#include "Options1.h"
#include "Options2.h"
#include "Options4.h"
#include "Options5.h"
#include "Options7.h"
#include "Options8.h"
#include "osd.h"
#include "plugin/pluginconfig.h"
#include "pluginmanager.h"

#include <qcombobox.h>
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
#include <kiconloader.h>
#include <klineedit.h>
#include <klocale.h>
#include <kstandarddirs.h>

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
    Options2 *opt2 = new Options2( 0, "Appearance" );
            m_opt4 = new Options4( 0, "Playback" );
    Options5 *opt5 = new Options5( 0, "OSD" );
    QVBox    *opt6 = new QVBox;
            m_opt7 = new Options7( 0, "Collection" );
    Options8 *opt8 = new Options8( 0, "Scrobbler" );

    // Sound System
    opt6->setName( "Engine" );
    opt6->setSpacing( 12 );
    opt6->setMargin( 11 );
    QWidget *groupBox, *aboutEngineButton;
    groupBox            = new QGroupBox( 2, Qt::Horizontal, i18n("Sound System"), opt6 );
    m_engineConfigFrame = new QGroupBox( 1, Qt::Horizontal, opt6 );
    m_soundSystem       = new QComboBox( false, groupBox );
    aboutEngineButton   = new QPushButton( i18n("About"), groupBox );

    QToolTip::add( m_soundSystem, i18n("Click to select the sound system to use for playback.") );
    QToolTip::add( aboutEngineButton, i18n("Click to get the plugin information.") );

    KTrader::OfferList offers = PluginManager::query( "[X-KDE-amaroK-plugintype] == 'engine'" );

    for( KTrader::OfferList::ConstIterator it = offers.begin(); it != offers.end(); ++it ) {
        m_soundSystem->insertItem( (*it)->name() );
        // Save name properties in QMap for lookup
        m_pluginName[(*it)->name()] = (*it)->property( "X-KDE-amaroK-name" ).toString();
        m_pluginAmarokName[(*it)->property( "X-KDE-amaroK-name" ).toString()] = (*it)->name();
    }

    // KConfigXT doesn't like Comboboxes, still, 2 versions after
    // it was created. How shit. Hence we can't make recodeEncoding
    // a string (which we need to because by index is not consistent)
    QTextCodec *codec;
    m_opt1->kcfg_RecodeEncoding->insertItem( QString::null );
    for( int i = 1; (codec = QTextCodec::codecForIndex( i )); i++ )
        m_opt1->kcfg_RecodeEncoding->insertItem( codec->name() );

    // Collection
#ifdef USE_MYSQL
    m_opt7->kcfg_DatabaseEngine->insertItem( "MySQL", 1 );
#else
    m_opt7->databaseBox->hide();

    //FIXME we do this because this widget breaks the Apply button (always enabled).
    //It breaks because it is set to type="password" in the .kcfg file. Setting to
    //type="string" also fixes this bug, but means the password is stored in plain
    //text. This is a temporary fix so that the majority of users get a fixed Apply
    //button.
    delete m_opt7->kcfg_MySqlPassword;
#endif
    m_opt7->collectionFoldersBox->setColumns( 1 );
    new CollectionSetup( m_opt7->collectionFoldersBox ); //TODO this widget doesn't update the apply/ok buttons

    // add pages
    addPage( m_opt1, i18n( "General" ), "misc", i18n( "Configure General Options" ) );
    addPage( opt2,   i18n( "Appearance" ), "colors", i18n( "Configure amaroK's Appearance" ) );
    addPage( m_opt4, i18n( "Playback" ), "kmix", i18n( "Configure Playback" ) );
    addPage( opt5,   i18n( "OSD" ), "tv", i18n( "Configure On-Screen-Display" ) );
    addPage( opt6,   i18n( "Engine" ), "amarok", i18n( "Configure Engine" ) );
    addPage( m_opt7, i18n( "Collection" ), "connect_creating", i18n( "Configure Collection" ) );
    addPage( opt8,   i18n( "Scrobbler" ), locate( "data", "amarok/images/audioscrobbler.png" ), i18n( "Configure Audioscrobbler" ) );

    // Show information labels (must be done after insertions)
    QObjectList *list = queryList( "QLabel", "infoPixmap" );
    for( QObject *label = list->first(); label; label = list->next() )
        static_cast<QLabel*>(label)->setPixmap( QMessageBox::standardIcon( QMessageBox::Information ) );
    delete list;

    //stop KFont Requesters getting stupidly large
    list = queryList( "QLabel", "m_sampleLabel" );
    for( QObject *label = list->first(); label; label = list->next() )
        static_cast<QLabel*>(label)->setMaximumWidth( 250 );
    delete list;

    connect( m_soundSystem, SIGNAL(activated( int )), SLOT(updateButtons()) );
    connect( aboutEngineButton, SIGNAL(clicked()), this, SLOT(aboutEngine()) );
    connect( opt5, SIGNAL(settingsChanged()), SLOT(updateButtons()) ); //see options5.ui.h
    connect( m_opt7->kcfg_DatabaseEngine, SIGNAL(activated( int )), SLOT(databaseEngineChanged()) );
}

AmarokConfigDialog::~AmarokConfigDialog()
{
    delete m_engineConfig;
}


/** Reimplemented from KConfigDialog */
void AmarokConfigDialog::addPage( QWidget *page, const QString &itemName, const QString &pixmapName, const QString &header, bool manage )
{
    // Add the widget pointer to our list, for later reference
    m_pageList << page;

    KConfigDialog::addPage( page, itemName, pixmapName, header, manage );
}


/** Show page by object name */
void AmarokConfigDialog::showPage( const QCString& page )
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
 * Update the settings from the dialog.
 * Example use: User clicks Ok or Apply button in a configure dialog.
 * REIMPLEMENTED
 */
void AmarokConfigDialog::updateSettings()
{
    OSDPreviewWidget *osd = (OSDPreviewWidget*)child( "osdpreview" );
    AmarokConfig::setOsdAlignment( osd->alignment() );
    AmarokConfig::setOsdYOffset( osd->y() );
    amaroK::OSD::instance()->applySettings();

    static_cast<CollectionSetup*>(child("CollectionSetup"))->writeConfig();

    if ( m_engineConfig ) m_engineConfig->save();

    // When sound system has changed, update engine config page
    if ( m_soundSystem->currentText() != m_pluginAmarokName[AmarokConfig::soundSystem()] ) {
        AmarokConfig::setSoundSystem( m_pluginName[m_soundSystem->currentText()] );
        emit settingsChanged();
        soundSystemChanged();
    }
}


/**
 * Update the configuration-widgets in the dialog based on amaroK's current settings.
 * Example use: Initialisation of dialog.
 * Example use: User clicks Reset button in a configure dialog.
 * REIMPLEMENTED
 */
void AmarokConfigDialog::updateWidgets()
{
    m_soundSystem->setCurrentText( m_pluginAmarokName[AmarokConfig::soundSystem()] );

    soundSystemChanged();

    bool dbConfigEnabled = false;
    if ( AmarokConfig::databaseEngine() == "1" )
    {
        dbConfigEnabled = true;
    }
    m_opt7->mysqlConfig->setEnabled( dbConfigEnabled );
}


/**
 * Update the configuration-widgets in the dialog based on the default values for amaroK's settings.
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
 * @return true if any configuration items we are managing changed from amaroK's stored settings
 * We manage the engine combo box and some of the OSD settings
 * REIMPLEMENTED
 */
bool AmarokConfigDialog::hasChanged()
{
    OSDPreviewWidget *osd = (OSDPreviewWidget*) child( "osdpreview" );

    return  m_soundSystem->currentText() != m_pluginAmarokName[AmarokConfig::soundSystem()] ||
            osd->alignment() != AmarokConfig::osdAlignment() ||
            osd->alignment() != OSDWidget::Center && osd->y() != AmarokConfig::osdYOffset() ||
            m_engineConfig && m_engineConfig->hasChanged();
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


void AmarokConfigDialog::databaseEngineChanged() // SLOT
{
    m_opt7->mysqlConfig->setEnabled( m_opt7->kcfg_DatabaseEngine->currentItem() != 0 );
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
        m_engineConfigFrame->setTitle( i18n( "Configure %1" ).arg( m_soundSystem->currentText() ) );
        m_engineConfigFrame->show();

        connect( m_engineConfig, SIGNAL(viewChanged()), SLOT(updateButtons()) );
    }
    else {
        m_engineConfig = 0;
        m_engineConfigFrame->hide();
    }

    const bool hasCrossfade = EngineController::hasEngineProperty( "HasCrossfade" );
    // Enable crossfading option when available
    m_opt4->kcfg_Crossfade->setEnabled( hasCrossfade );
    m_opt4->kcfg_CrossfadeLength->setEnabled( hasCrossfade );
    m_opt4->crossfadeLengthLabel->setEnabled( hasCrossfade );

}


#include "configdialog.moc"
