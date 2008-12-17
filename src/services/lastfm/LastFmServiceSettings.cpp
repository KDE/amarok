/***************************************************************************
 * copyright            : (C) 2007 Shane King <kde@dontletsstart.com>      *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "lastfm"

#include "LastFmServiceSettings.h"
#include "ui_LastFmConfigWidget.h"
#include "Debug.h"

#include <lastfm/Scrobbler.h> // from liblastfm
#include <lastfm/ws/WsKeys.h>
#include <lastfm/ws/WsReply.h>
#include <lastfm/ws/WsRequestBuilder.h>

#include <QCryptographicHash>
#include <QVBoxLayout>
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

    connect( m_configDialog->kcfg_ScrobblerUsername, SIGNAL( textChanged( const QString & ) ), this, SLOT( settingsChanged() ) );
    connect( m_configDialog->kcfg_ScrobblerPassword, SIGNAL( textChanged( const QString & ) ), this, SLOT( settingsChanged() ) );
    connect( m_configDialog->kcfg_SubmitPlayedSongs, SIGNAL( stateChanged( int ) ), this, SLOT( settingsChanged() ) );
    connect( m_configDialog->kcfg_RetrieveSimilarArtists, SIGNAL( stateChanged( int ) ), this, SLOT( settingsChanged() ) );
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
    m_config.save();

    KCModule::save();
}

void
LastFmServiceSettings::testLogin()
{
    m_configDialog->testLogin->setEnabled( false );
    m_configDialog->testLogin->setText( i18n( "Testing..." ) );
    // set the global static Lastfm::Ws stuff
    Ws::ApiKey = "402d3ca8e9bc9d3cf9b85e1202944ca5";
    Ws::SharedSecret = "fe0dcde9fcd14c2d1d50665b646335e9";
    Ws::Username = qstrdup( m_configDialog->kcfg_ScrobblerUsername->text().toLatin1().data() );
    
    // set up proxy
    //WsAccessManager* qnam = new KNetworkAccessManager( this );
    //WsRequestBuilder::setWAM( qnam );
    
    debug() << "username:" << QString( QUrl::toPercentEncoding( Ws::Username ) );

    QString authToken =  md5( ( m_configDialog->kcfg_ScrobblerUsername->text() + md5( m_configDialog->kcfg_ScrobblerPassword->text().toUtf8() ) ).toUtf8() );
    QString sign_key = md5( ( "api_key" + QString( Ws::ApiKey ) + "authToken" + authToken + "methodauth.getMobileSession" + QString( Ws::SharedSecret ) ).toUtf8() );
    
    // now authenticate w/ last.fm and get our session key
    WsReply* reply = WsRequestBuilder( "auth.getMobileSession" )
    .add( "username", m_configDialog->kcfg_ScrobblerUsername->text() )
    .add( "authToken", authToken )
    .add( "api_key", Ws::ApiKey )
    .add( "api_sig", sign_key )
    .get();
    
    connect( reply, SIGNAL( finished( WsReply* ) ), SLOT( onAuthenticated( WsReply* ) ) );
}

void
LastFmServiceSettings::onAuthenticated( WsReply *reply )
{
    switch( reply->error() )
    {
        case Ws::NoError:
             m_configDialog->testLogin->setText( i18nc( "The operation completed as expected", "Success" ) );
             m_configDialog->testLogin->setEnabled( false );
             break;
         case Ws::AuthenticationFailed:
            KMessageBox::error( this, i18n( "Either the username or the password is incorrect, please correct and try again" ), i18n( "Failed" ) );
            m_configDialog->testLogin->setText( i18n( "Test Login" ) );
            m_configDialog->testLogin->setEnabled( true );
            break;
            
        case Ws::UrProxyIsFuckedLol:
        case Ws::UrLocalNetworkIsFuckedLol:
            KMessageBox::sorry( this, i18n( "Unable to reach the internet, please check your firewall settings and try again" ) );
        default:
            debug() << "Unhandled WsReply state, probably not important";
            return;
    }
}

void 
LastFmServiceSettings::load()
{
    m_config.load();
    m_configDialog->kcfg_ScrobblerUsername->setText( m_config.username() );
    m_configDialog->kcfg_ScrobblerPassword->setText( m_config.password() );
    m_configDialog->kcfg_SubmitPlayedSongs->setChecked( m_config.scrobble() );
    m_configDialog->kcfg_RetrieveSimilarArtists->setChecked( m_config.fetchSimilar() );

    KCModule::load();
}


void
LastFmServiceSettings::defaults()
{
    m_config.reset();

    // By default this checkboxes is:
    m_configDialog->kcfg_SubmitPlayedSongs->setChecked( true );
    m_configDialog->kcfg_RetrieveSimilarArtists->setChecked( true );
}


void
LastFmServiceSettings::settingsChanged()
{
    //TODO: Make pretty validation for username and password
    //with error reporting

    emit changed( true );
}
