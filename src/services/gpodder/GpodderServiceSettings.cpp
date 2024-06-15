/****************************************************************************************
 * Copyright (c) 2007 Shane King <kde@dontletsstart.com>                                *
 * Copyright (c) 2010 Stefan Derkits <stefan@derkits.at>                                *
 * Copyright (c) 2010 Christian Wagner <christian.wagner86@gmx.at>                      *
 * Copyright (c) 2010 Felix Winter <ixos01@gmail.com>                                   *
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

#define DEBUG_PREFIX "GpodderServiceSettings"

#include "GpodderServiceSettings.h"

#include "core/podcasts/PodcastProvider.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "NetworkAccessManagerProxy.h"
#include "playlistmanager/PlaylistManager.h"
#include "ui_GpodderConfigWidget.h"

#include <KMessageBox>
#include <KPluginFactory>

#include <QHostInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>


K_PLUGIN_FACTORY_WITH_JSON( GpodderServiceSettingsFactory, "amarok_service_gpodder_config.json", registerPlugin<GpodderServiceSettings>(); )

GpodderServiceSettings::GpodderServiceSettings( QWidget *parent, const QVariantList &args )
        : KCModule( parent, args )
        , m_enableProvider( false )
        , m_createDevice( nullptr )
{
    debug() << "Creating gpodder.net config object";

    m_configDialog = new Ui::GpodderConfigWidget;
    m_configDialog->setupUi( this );

    connect( m_configDialog->kcfg_GpodderUsername, &QLineEdit::textChanged,
             this, &GpodderServiceSettings::settingsChanged );
    connect( m_configDialog->kcfg_GpodderPassword, &QLineEdit::textChanged,
             this, &GpodderServiceSettings::settingsChanged );
    connect( m_configDialog->testLogin, &QPushButton::clicked,
             this, &GpodderServiceSettings::testLogin );

    load();
}

GpodderServiceSettings::~GpodderServiceSettings()
{
    if( m_createDevice )
        m_createDevice->deleteLater();

    if( m_devices )
        m_devices->deleteLater();

    delete m_configDialog;
}

void
GpodderServiceSettings::save()
{
    m_config.setUsername( m_configDialog->kcfg_GpodderUsername->text() );
    m_config.setPassword( m_configDialog->kcfg_GpodderPassword->text() );
    m_config.setEnableProvider( m_enableProvider );
    m_config.setIgnoreWallet( false );

    m_config.save();
    KCModule::save();
}

void
GpodderServiceSettings::testLogin()
{
    DEBUG_BLOCK

    if ( ( !m_configDialog->kcfg_GpodderUsername->text().isEmpty() ) &&
         ( !m_configDialog->kcfg_GpodderPassword->text().isEmpty() ) )
    {

        m_configDialog->testLogin->setEnabled( false );
        m_configDialog->testLogin->setText( i18n( "Testing..." ) );

        mygpo::ApiRequest api( m_configDialog->kcfg_GpodderUsername->text(),
                               m_configDialog->kcfg_GpodderPassword->text(),
                               The::networkAccessManager() );
        m_devices = api.listDevices( m_configDialog->kcfg_GpodderUsername->text() );

        connect( m_devices.data(), &mygpo::DeviceList::finished,
                 this, &GpodderServiceSettings::finished );
        connect( m_devices.data(), &mygpo::DeviceList::requestError,
                 this, &GpodderServiceSettings::onError );
        connect( m_devices.data(), &mygpo::DeviceList::parseError,
                 this, &GpodderServiceSettings::onParseError );
    }
    else
    {
        KMessageBox::error( this,
                            i18n( "Either the username or the password is empty, please correct and try again." ),
                            i18n( "Failed" ) );
    }
}

void
GpodderServiceSettings::finished()
{
    DEBUG_BLOCK

    debug() << "Authentication worked, got List of Devices, searching for Amarok Device";

    m_configDialog->testLogin->setText( i18nc( "The operation completed as expected", "Success" ) );
    m_configDialog->testLogin->setEnabled( false );

    bool deviceExists = false;
    QList<mygpo::DevicePtr> ptrList = m_devices->devicesList();
    mygpo::DevicePtr devPtr;

    QString hostname = QHostInfo::localHostName();
    QString deviceID = QLatin1String( "amarok-" ) % hostname;

    foreach( devPtr, ptrList )
    {
        if( devPtr->id().compare( deviceID ) == 0 )
        {
            deviceExists = true;
            break;
        }
    }
    if( !deviceExists )
    {
        debug() << "Create new device " % deviceID;

        mygpo::ApiRequest api( m_configDialog->kcfg_GpodderUsername->text(),
                               m_configDialog->kcfg_GpodderPassword->text(),
                               The::networkAccessManager() );

        m_createDevice = api.renameDevice( m_configDialog->kcfg_GpodderUsername->text(),
                                           deviceID,
                                           QLatin1String( "Amarok on " ) % hostname,
                                           mygpo::Device::OTHER );

        connect( m_createDevice, &QNetworkReply::finished,
                 this, &GpodderServiceSettings::deviceCreationFinished );
        connect( m_createDevice, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
                 this, &GpodderServiceSettings::deviceCreationError );
    }
    else
    {
        debug() << "Amarok device was found and everything looks perfect";
    }
}

void
GpodderServiceSettings::onError( QNetworkReply::NetworkError code )
{
    DEBUG_BLOCK

    debug() << code;

    if( code == QNetworkReply::NoError )
        debug() << "No Error was found, but onError was called - should not happen";
    else if( code == QNetworkReply::AuthenticationRequiredError )
    {
        debug() << "Authentication failed";

        KMessageBox::error( this,
            i18n( "Either the username or the password is incorrect, please correct and try again" ),
                            i18n( "Failed" ) );

        m_configDialog->testLogin->setText( i18n( "&Test Login" ) );
        m_configDialog->testLogin->setEnabled( true );
    }
    else
    {
        KMessageBox::error( this,
            i18n( "Unable to connect to gpodder.net service or other error occurred." ),
                            i18n( "Failed" ) );

        m_configDialog->testLogin->setText( i18n( "&Test Login" ) );
        m_configDialog->testLogin->setEnabled( true );
    }
}

void
GpodderServiceSettings::onParseError()
{
    debug() << "Couldn't parse DeviceList, should not happen if gpodder.net is working correctly";

    m_configDialog->testLogin->setText( i18n( "&Test Login" ) );
    m_configDialog->testLogin->setEnabled( true );

    KMessageBox::error( this, i18n( "Error parsing the Reply, check if gpodder.net is working correctly and report a bug" ), i18n( "Failed" ) );
}

void
GpodderServiceSettings::deviceCreationFinished()
{
    debug() << "Creation of Amarok Device finished";
}

void
GpodderServiceSettings::deviceCreationError( QNetworkReply::NetworkError code )
{
    debug() << "Error creating Amarok Device";
    debug() << code;

    m_configDialog->testLogin->setText( i18n( "&Test Login" ) );
    m_configDialog->testLogin->setEnabled( true );
}

void
GpodderServiceSettings::load()
{
    m_config.load();

    m_configDialog->kcfg_GpodderUsername->setText( m_config.username() );
    m_configDialog->kcfg_GpodderPassword->setText( m_config.password() );
    m_enableProvider = m_config.enableProvider();

    KCModule::load();
}

void
GpodderServiceSettings::defaults()
{
    m_config.reset();

    m_configDialog->kcfg_GpodderUsername->clear();
    m_configDialog->kcfg_GpodderPassword->clear();
    m_enableProvider = false;
}

void
GpodderServiceSettings::settingsChanged()
{
    m_configDialog->testLogin->setText( i18n( "&Test Login" ) );
    m_configDialog->testLogin->setEnabled( true );

    m_enableProvider = true;
    emit changed( true );
}

#include "GpodderServiceSettings.moc"
