/****************************************************************************************
 * Copyright (c) 2007 Shane King <kde@dontletsstart.com>                                *
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

#define DEBUG_PREFIX "LastFmServiceSettings"

#include "LastFmServiceSettings.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "NetworkAccessManagerProxy.h"
#include "ui_LastFmConfigWidget.h"

#include <lastfm/XmlQuery.h>

#include <QCryptographicHash>
#include <QRegExpValidator>

#include <KMessageBox>
#include <KPluginFactory>


K_PLUGIN_FACTORY( LastFmServiceSettingsFactory, registerPlugin<LastFmServiceSettings>(); )
K_EXPORT_PLUGIN( LastFmServiceSettingsFactory( "kcm_amarok_lastfm" ) )

QString md5( const QByteArray& src )
{
    QByteArray const digest = QCryptographicHash::hash( src, QCryptographicHash::Md5 );
    return QString::fromLatin1( digest.toHex() ).rightJustified( 32, '0' );
}


LastFmServiceSettings::LastFmServiceSettings( QWidget *parent, const QVariantList &args )
    : KCModule( LastFmServiceSettingsFactory::componentData(), parent, args )
{
    debug() << "Creating Last.fm config object";

    QVBoxLayout* l = new QVBoxLayout( this );
    QWidget *w = new QWidget;
    m_configDialog = new Ui::LastFmConfigWidget;
    m_configDialog->setupUi( w );
    l->addWidget( w );

    // whenever this gets opened, we'll assume the user wants to change something,
    // so blow away the saved session key
    m_config.clearSessionKey();

    connect( m_configDialog->kcfg_ScrobblerUsername, SIGNAL( textChanged( const QString & ) ), this, SLOT( settingsChanged() ) );
    connect( m_configDialog->kcfg_ScrobblerPassword, SIGNAL( textChanged( const QString & ) ), this, SLOT( settingsChanged() ) );
    connect( m_configDialog->kcfg_SubmitPlayedSongs, SIGNAL( stateChanged( int ) ), this, SLOT( settingsChanged() ) );
    connect( m_configDialog->kcfg_RetrieveSimilarArtists, SIGNAL( stateChanged( int ) ), this, SLOT( settingsChanged() ) );
    connect( m_configDialog->kcfg_ScrobbleComposer, SIGNAL( stateChanged( int ) ), this, SLOT( settingsChanged() ) );
    connect( m_configDialog->testLogin, SIGNAL( clicked() ), this, SLOT( testLogin() ) );

    load();
}


LastFmServiceSettings::~LastFmServiceSettings()
{
}


void
LastFmServiceSettings::save()
{
    m_config.setUsername( m_configDialog->kcfg_ScrobblerUsername->text() );
    m_config.setPassword( m_configDialog->kcfg_ScrobblerPassword->text() );
    m_config.setScrobble( m_configDialog->kcfg_SubmitPlayedSongs->isChecked() );
    m_config.setFetchSimilar( m_configDialog->kcfg_RetrieveSimilarArtists->isChecked() );
    m_config.setScrobbleComposer( m_configDialog->kcfg_ScrobbleComposer->isChecked() );
    m_config.save();

    KCModule::save();
}

void
LastFmServiceSettings::testLogin()
{
    DEBUG_BLOCK

    m_configDialog->testLogin->setEnabled( false );
    m_configDialog->testLogin->setText( i18n( "Testing..." ) );
    // set the global static Lastfm::Ws stuff
    lastfm::ws::ApiKey = Amarok::lastfmApiKey();
    lastfm::ws::SharedSecret = Amarok::lastfmApiSharedSecret();
    lastfm::ws::Username = qstrdup( m_configDialog->kcfg_ScrobblerUsername->text().toLatin1().data() );
    if( lastfm::nam() != The::networkAccessManager() )
        lastfm::setNetworkAccessManager( The::networkAccessManager() );

    debug() << "username:" << QString( QUrl::toPercentEncoding( lastfm::ws::Username ) );

    const QString authToken = md5( QString( "%1%2" ).arg( m_configDialog->kcfg_ScrobblerUsername->text() )
                                                    .arg( md5( m_configDialog->kcfg_ScrobblerPassword->text().toUtf8() ) ).toUtf8() );

    // now authenticate w/ last.fm and get our session key
    QMap<QString, QString> query;
    query[ "method" ] = "auth.getMobileSession";
    query[ "username" ] = m_configDialog->kcfg_ScrobblerUsername->text();
    query[ "authToken" ] = authToken;
    m_authQuery = lastfm::ws::post( query );

    connect( m_authQuery, SIGNAL( finished() ), SLOT( onAuthenticated() ) );
    connect( m_authQuery, SIGNAL( error( QNetworkReply::NetworkError ) ), SLOT( onError( QNetworkReply::NetworkError ) ) );
}

void
LastFmServiceSettings::onAuthenticated()
{
    DEBUG_BLOCK

    lastfm::XmlQuery lfm;
    lfm.parse( m_authQuery->readAll() );

    switch( m_authQuery->error() )
    {
        case QNetworkReply::NoError:
             debug() << "NoError";
             if( lfm.children( "error" ).size() > 0 )
             {
                 debug() << "ERROR from last.fm:" << lfm.text();
                 m_configDialog->testLogin->setText( i18nc( "The operation was rejected by the server", "Failed" ) );
                 m_configDialog->testLogin->setEnabled( true );

             } else
             {
                 m_configDialog->testLogin->setText( i18nc( "The operation completed as expected", "Success" ) );
                 m_configDialog->testLogin->setEnabled( false );
                 m_configDialog->kcfg_SubmitPlayedSongs->setEnabled( true );
             }
             break;

        case QNetworkReply::AuthenticationRequiredError:
            debug() << "AuthenticationFailed";
            KMessageBox::error( this, i18n( "Either the username or the password is incorrect, please correct and try again" ), i18n( "Failed" ) );
            m_configDialog->testLogin->setText( i18n( "Test Login" ) );
            m_configDialog->testLogin->setEnabled( true );
            break;

        default:
            debug() << "Unhandled QNetworkReply state, probably not important";
    }
    m_authQuery->deleteLater();
}

void
LastFmServiceSettings::onError( QNetworkReply::NetworkError code )
{
    DEBUG_BLOCK

    if( code == QNetworkReply::NoError )
        return;

    if( code == QNetworkReply::AuthenticationRequiredError )
    {
        onAuthenticated();
        return;
    }

    KMessageBox::error( this, i18n( "Unable to connect to Last.fm service." ), i18n( "Failed" ) );
    m_configDialog->testLogin->setText( i18n( "Test Login" ) );
    m_configDialog->testLogin->setEnabled( true );

    debug() << "Error occurred during network request: " << m_authQuery->errorString();
    m_authQuery->deleteLater();
}


void
LastFmServiceSettings::load()
{
    m_config.load();
    m_configDialog->kcfg_ScrobblerUsername->setText( m_config.username() );
    m_configDialog->kcfg_ScrobblerPassword->setText( m_config.password() );
    m_configDialog->kcfg_SubmitPlayedSongs->setChecked( m_config.scrobble() );
    m_configDialog->kcfg_RetrieveSimilarArtists->setChecked( m_config.fetchSimilar() );
    m_configDialog->kcfg_ScrobbleComposer->setChecked( m_config.scrobbleComposer() );

    if( !m_config.username().isEmpty() && !m_config.password().isEmpty() )
        m_configDialog->kcfg_SubmitPlayedSongs->setEnabled( true );

    KCModule::load();
}


void
LastFmServiceSettings::defaults()
{
    m_config.reset();

    // By default this checkboxes is:
    m_configDialog->kcfg_SubmitPlayedSongs->setChecked( false );
    m_configDialog->kcfg_RetrieveSimilarArtists->setChecked( false );
    m_configDialog->kcfg_ScrobbleComposer->setChecked( false );
}


void
LastFmServiceSettings::settingsChanged()
{
    //TODO: Make pretty validation for username and password
    //with error reporting

    m_configDialog->testLogin->setText( i18n( "&Test Login" ) );
    m_configDialog->testLogin->setEnabled( true );

    emit changed( true );
}
