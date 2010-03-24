/****************************************************************************************
 * Copyright (c) 2005 Martin Aumueller <aumuell@reserv.at>                              *
 * Copyright (c) 2006 Jeff Mitchell <kde-dev@emailgoeshere.com>                         *
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

#include "deviceconfiguredialog.h"

#include "Amarok.h"
#include "Debug.h"
#include "hintlineedit.h"
#include "mediabrowser.h"
#include "MediaDevice.h"
#include "core/plugins/PluginManager.h"
#include "ScriptManager.h"

#include <KLocale>
#include <KVBox>

#include <QLabel>
#include <q3buttongroup.h>


DeviceConfigureDialog::DeviceConfigureDialog( MediaDevice *device )
        : KDialog( Amarok::mainWindow() )
        , m_device( device )
{
    setCaption( i18n("Select Plugin for %1", m_device->name() ) );
    setModal( true );
    setButtons( Ok | Cancel );
    showButtonSeparator( true );

    kapp->setTopWidget( this );
    setCaption( KDialog::makeStandardCaption( i18n( "Configure Media Device" ) ) );
    showButton( KDialog::Apply, false );

    KVBox* vbox = new KVBox( this );
    setMainWidget( vbox );
    vbox->setSpacing( KDialog::spacingHint() );

    QLabel *connectLabel = 0;
    m_connectEdit = 0;
    QLabel *disconnectLabel = 0;
    m_disconnectEdit = 0;
    m_transcodeCheck = 0;
    Q3ButtonGroup *transcodeGroup = 0;
    m_transcodeAlways = 0;
    m_transcodeWhenNecessary = 0;
    m_transcodeRemove = 0;

    if( device )
    {
        device->loadConfig();

        // pre-connect/post-disconnect (mount/umount)
        connectLabel = new QLabel( vbox );
        connectLabel->setText( i18n( "Pre-&connect command:" ) );
        m_connectEdit = new HintLineEdit( device->m_preconnectcmd, vbox );
        m_connectEdit->setHint( i18n( "Example: mount %d" ) );
        connectLabel->setBuddy( m_connectEdit );
        m_connectEdit->setToolTip( i18n( "Set a command to be run before connecting to your device (e.g. a mount command) here.\n%d is replaced by the device node, %m by the mount point.\nEmpty commands are not executed." ) );

        disconnectLabel = new QLabel( vbox );
        disconnectLabel->setText( i18n( "Post-&disconnect command:" ) );
        m_disconnectEdit = new HintLineEdit( device->m_postdisconnectcmd, vbox );
        disconnectLabel->setBuddy( m_disconnectEdit );
        m_disconnectEdit->setHint( i18n( "Example: eject %d" ) );
        m_disconnectEdit->setToolTip( i18n( "Set a command to be run after disconnecting from your device (e.g. an eject command) here.\n%d is replaced by the device node, %m by the mount point.\nEmpty commands are not executed." ) );

        // transcode
        m_transcodeCheck = new QCheckBox( vbox );
        m_transcodeCheck->setText( i18n( "&Transcode before transferring to device" ) );
        m_transcodeCheck->setChecked( device->m_transcode );

        transcodeGroup = new Q3VButtonGroup( vbox );
        QString format = "mp3";
        if( !device->supportedFiletypes().isEmpty() )
            format = device->supportedFiletypes().first();
        transcodeGroup->setTitle( i18n( "Transcode to preferred format (%1) for device", format ) );
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
        m_transcodeCheck->setEnabled( !sm->transcodeScriptRunning().isEmpty() );
        transcodeGroup->setEnabled( !sm->transcodeScriptRunning().isEmpty() && device->m_transcode );
        if( sm->transcodeScriptRunning().isNull() )
        {
            m_transcodeCheck->setToolTip( i18n( "For this feature, a script of type \"Transcode\" has to be running" ) );
            transcodeGroup->setToolTip( i18n( "For this feature, a script of type \"Transcode\" has to be running" ) );
        }

        device->addConfigElements( vbox );
    }

    m_accepted = false;
}

DeviceConfigureDialog::~DeviceConfigureDialog()
{
     delete m_connectEdit;
     delete m_disconnectEdit;
}

void
DeviceConfigureDialog::slotButtonClicked( KDialog::ButtonCode button )
{
    if ( button != KDialog::Ok )
        KDialog::slotButtonClicked( button );
    m_accepted = true;

    if( m_device )
    {
        m_device->m_preconnectcmd = m_connectEdit->text();
        m_device->setConfigString( "PreConnectCommand", m_device->m_preconnectcmd );
        m_device->m_postdisconnectcmd = m_disconnectEdit->text();
        m_device->setConfigString( "PostDisconnectCommand", m_device->m_postdisconnectcmd );
        m_device->setConfigBool( "Transcode", m_device->m_transcode );
        m_device->m_transcode = m_transcodeCheck->isChecked();
        m_device->setConfigBool( "Transcode", m_device->m_transcode );
        m_device->m_transcodeAlways = m_transcodeAlways->isChecked();
        m_device->setConfigBool( "TranscodeAlways", m_device->m_transcodeAlways );
        m_device->m_transcodeRemove = m_transcodeRemove->isChecked();
        m_device->setConfigBool( "TranscodeRemove", m_device->m_transcodeRemove );
        m_device->applyConfig();
    }

    MediaBrowser::instance()->updateButtons();
    MediaBrowser::instance()->updateStats();
    MediaBrowser::instance()->updateDevices();

    KDialog::slotButtonClicked( button );
}

#include "deviceconfiguredialog.moc"

