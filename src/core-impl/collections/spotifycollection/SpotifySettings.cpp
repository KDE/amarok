#define DEBUG_PREFIX "SpotifySettings"
#include <QFile>
#include <KLocale>
#include <KMessageBox>
#include <KPluginFactory>
#include "core/support/Debug.h"
#include "SpotifySettings.h"
#include "SpotifyController.h"
#include "ui_SpotifyConfigWidget.h"

K_PLUGIN_FACTORY( SpotifySettingsFactory, registerPlugin< SpotifySettings >(); )
K_EXPORT_PLUGIN( SpotifySettingsFactory( "kcm_amarok_spotify" ) )

SpotifySettings::SpotifySettings( QWidget* parent, const QVariantList& args )
: KCModule( SpotifySettingsFactory::componentData(), parent, args )
{
    DEBUG_BLOCK

    debug() << "Creating Spotify settings object...";
    m_configWidget = new Ui::SpotifyConfigWidget;
    m_configWidget->setupUi( this );

    connect( m_configWidget->btnLogin, SIGNAL( clicked ),
            this, SLOT( tryLogin() ) );
    connect( m_configWidget->btnClose, SIGNAL( clicked ),
            this, SLOT( cancel ) );

    load();

//    Spotify::Controller* controller = The::SpotifyController();
Spotify::Controller* controller = 0;
    if( !QFile::exists( m_config.resolverPath() ) )
    {
        int res = KMessageBox::questionYesNo( this,
                    i18n( "Spotify resolver is missing or not installed correctly, do you want to download and install it now?" ),
                    i18n( "Spotify resolver" ) );
        if( res == KMessageBox::Yes )
        {
            tryDownloadResolver();
        }
        else
        {
            // Close config dialog
            close();
        }
    }
        if( controller )
        {
        }

}

SpotifySettings::~SpotifySettings()
{
    delete m_configWidget;
}

void
SpotifySettings::load()
{
    DEBUG_BLOCK
    m_config.load();
    m_configWidget->lineUsername->setText( m_config.username() );
    m_configWidget->linePassword->setText( m_config.password() );
    m_configWidget->checkHighQuality->setChecked( m_config.highQuality() );
    KCModule::load();
}

void
SpotifySettings::save()
{
    DEBUG_BLOCK
    m_config.setUsername( m_configWidget->lineUsername->text() );
    m_config.setPassword( m_configWidget->linePassword->text() );
    m_config.setHighQuality( m_configWidget->checkHighQuality->isChecked() );
    m_config.save();
    KCModule::save();
}

void
SpotifySettings::defaults()
{
    m_config.reset();
    m_configWidget->lineUsername->setText( m_config.username() );
    m_configWidget->linePassword->setText( m_config.password() );
    m_configWidget->checkHighQuality->setChecked( m_config.highQuality() );
}

void
SpotifySettings::tryLogin()
{
    DEBUG_BLOCK
//    Spotify::Controller* controller = The::SpotifyController();
    Spotify::Controller* controller = 0;
    connect( controller, SIGNAL( loggedIn() ),
            this, SLOT( loginSuccefully() ) );

    if( !controller->running() )
    {
        controller->setFilePath( m_config.resolverPath() );
        if( !controller->loaded() )
        {
            controller->reload();
        }
        controller->start();
    }

    if( controller && controller->running() )
    {
        controller->login( m_configWidget->lineUsername->text(), m_configWidget->linePassword->text() );
    }
}

void
SpotifySettings::tryDownloadResolver()
{
}

void
SpotifySettings::cancel()
{
    close();
}
