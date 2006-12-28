//
// C++ Implementation: deviceconfiguredialog.cpp
//
// Description:
//
//
// Author: Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2006
//         Martin Aumueller <aumuell@reserv.at>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "amarok.h"
#include "debug.h"
#include "deviceconfiguredialog.h"
#include "hintlineedit.h"
#include "mediabrowser.h"
#include "medium.h"
#include "plugin/pluginconfig.h"
#include "pluginmanager.h"
#include "scriptmanager.h"

#include <qlabel.h>
#include <qtooltip.h>
#include <qvbox.h>
#include <qbuttongroup.h>
#include <qvbuttongroup.h>

#include <kapplication.h>
#include <kcombobox.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kwin.h>

DeviceConfigureDialog::DeviceConfigureDialog( const Medium &medium )
        : KDialogBase( Amarok::mainWindow(), "deviceconfiguredialog", true, QString("Select Plugin for " + medium.name()), Ok|Cancel, Ok, false )
{
    m_medium = new Medium( medium );
    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Configure Media Device" ) ) );
    showButtonApply( false );

    QVBox* vbox = makeVBoxMainWidget();
    vbox->setSpacing( KDialog::spacingHint() );

    QLabel *connectLabel = 0;
    m_connectEdit = 0;
    QLabel *disconnectLabel = 0;
    m_disconnectEdit = 0;
    m_transcodeCheck = 0;
    QButtonGroup *transcodeGroup = 0;
    m_transcodeAlways = 0;
    m_transcodeWhenNecessary = 0;
    m_transcodeRemove = 0;

    MediaDevice* device = MediaBrowser::instance()->deviceFromId( m_medium->id() );

    if( device )
    {
        device->loadConfig();

        // pre-connect/post-disconnect (mount/umount)
        connectLabel = new QLabel( vbox );
        connectLabel->setText( i18n( "Pre-&connect command:" ) );
        m_connectEdit = new HintLineEdit( device->m_preconnectcmd, vbox );
        m_connectEdit->setHint( i18n( "Example: mount %d" ) );
        connectLabel->setBuddy( m_connectEdit );
        QToolTip::add( m_connectEdit, i18n( "Set a command to be run before connecting to your device (e.g. a mount command) here.\n%d is replaced by the device node, %m by the mount point.\nEmpty commands are not executed." ) );

        disconnectLabel = new QLabel( vbox );
        disconnectLabel->setText( i18n( "Post-&disconnect command:" ) );
        m_disconnectEdit = new HintLineEdit( device->m_postdisconnectcmd, vbox );
        disconnectLabel->setBuddy( m_disconnectEdit );
        m_disconnectEdit->setHint( i18n( "Example: eject %d" ) );
        QToolTip::add( m_disconnectEdit, i18n( "Set a command to be run after disconnecting from your device (e.g. an eject command) here.\n%d is replaced by the device node, %m by the mount point.\nEmpty commands are not executed." ) );

        // transcode
        m_transcodeCheck = new QCheckBox( vbox );
        m_transcodeCheck->setText( i18n( "&Transcode before transferring to device" ) );
        m_transcodeCheck->setChecked( device->m_transcode );

        transcodeGroup = new QVButtonGroup( vbox );
        QString format = "mp3";
        if( !device->supportedFiletypes().isEmpty() )
            format = device->supportedFiletypes().first();
        transcodeGroup->setTitle( i18n( "Transcode to preferred format (%1) for device" ).arg( format ) );
        m_transcodeAlways = new QRadioButton( transcodeGroup );
        m_transcodeAlways->setText( i18n( "Whenever possible" ) );
        m_transcodeAlways->setChecked( device->m_transcodeAlways );
        m_transcodeWhenNecessary = new QRadioButton( transcodeGroup );
        m_transcodeWhenNecessary->setText( i18n( "When necessary" ) );
        m_transcodeWhenNecessary->setChecked( !device->m_transcodeAlways );
        connect( m_transcodeCheck, SIGNAL(toggled( bool )),
                transcodeGroup, SLOT(setEnabled( bool )) );
        transcodeGroup->insert( m_transcodeAlways );
        transcodeGroup->insert( m_transcodeWhenNecessary );
        m_transcodeRemove = new QCheckBox( transcodeGroup );
        m_transcodeRemove->setText( i18n( "Remove transcoded files after transfer" ) );
        m_transcodeRemove->setChecked( device->m_transcodeRemove );

        const ScriptManager *sm = ScriptManager::instance();
        m_transcodeCheck->setEnabled( sm->transcodeScriptRunning() != QString::null );
        transcodeGroup->setEnabled( sm->transcodeScriptRunning() != QString::null && device->m_transcode );
        if( sm->transcodeScriptRunning().isNull() )
        {
            QToolTip::add( m_transcodeCheck, i18n( "For this feature, a script of type \"Transcode\" has to be running" ) );
            QToolTip::add( transcodeGroup, i18n( "For this feature, a script of type \"Transcode\" has to be running" ) );
        }

        device->addConfigElements( vbox );
    }

    m_accepted = false;
}

DeviceConfigureDialog::~DeviceConfigureDialog()
{
     delete m_connectEdit;
     delete m_disconnectEdit;
     delete m_medium;
}

void
DeviceConfigureDialog::slotCancel()
{
    KDialogBase::slotCancel( );
}

void
DeviceConfigureDialog::slotOk()
{
    m_accepted = true;
    MediaDevice* device = MediaBrowser::instance()->deviceFromId( m_medium->id() );

    if( device )
    {
        device->m_preconnectcmd = m_connectEdit->text();
        device->setConfigString( "PreConnectCommand", device->m_preconnectcmd );
        device->m_postdisconnectcmd = m_disconnectEdit->text();
        device->setConfigString( "PostDisconnectCommand", device->m_postdisconnectcmd );
        device->setConfigBool( "Transcode", device->m_transcode );
        device->m_transcode = m_transcodeCheck->isChecked();
        device->setConfigBool( "Transcode", device->m_transcode );
        device->m_transcodeAlways = m_transcodeAlways->isChecked();
        device->setConfigBool( "TranscodeAlways", device->m_transcodeAlways );
        device->m_transcodeRemove = m_transcodeRemove->isChecked();
        device->setConfigBool( "TranscodeRemove", device->m_transcodeRemove );
        device->applyConfig();
    }

    MediaBrowser::instance()->updateButtons();
    MediaBrowser::instance()->updateStats();
    MediaBrowser::instance()->updateDevices();

    KDialogBase::slotOk();
}

#include "deviceconfiguredialog.moc"
