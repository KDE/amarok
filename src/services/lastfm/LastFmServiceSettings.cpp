/****************************************************************************************
 * Copyright (c) 2007 Shane King <kde@dontletsstart.com>                                *
 * Copyright (c) 2013 Vedant Agarwala <vedant.kota@gmail.com>                           *
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

#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "network/NetworkAccessManagerProxy.h"
#include "ui_LastFmConfigWidget.h"

#include <KMessageBox>
#include <KPluginFactory>

#include <QDesktopServices>

#include <XmlQuery.h>

K_PLUGIN_FACTORY_WITH_JSON( LastFmServiceSettingsFactory, "amarok_service_lastfm_config.json", registerPlugin<LastFmServiceSettings>(); )

LastFmServiceSettings::LastFmServiceSettings( QWidget *parent, const QVariantList &args )
    : KCModule( parent, args )
    , m_config( LastFmServiceConfig::instance() )
{
    m_configDialog = new Ui::LastFmConfigWidget;
    m_configDialog->setupUi( this );

    connect( m_config.data(), &LastFmServiceConfig::updated, this, &LastFmServiceSettings::onConfigUpdated );

    connect( m_configDialog->kcfg_SubmitPlayedSongs, &QCheckBox::stateChanged, this, &LastFmServiceSettings::settingsChanged );
    connect( m_configDialog->kcfg_RetrieveSimilarArtists, &QCheckBox::stateChanged, this, &LastFmServiceSettings::settingsChanged );
    connect( m_configDialog->kcfg_ScrobbleComposer, &QCheckBox::stateChanged, this, &LastFmServiceSettings::settingsChanged );
    connect( m_configDialog->kcfg_UseFancyRatingTags, &QCheckBox::stateChanged, this, &LastFmServiceSettings::settingsChanged );
    connect( m_configDialog->kcfg_AnnounceCorrections, &QCheckBox::stateChanged, this, &LastFmServiceSettings::settingsChanged );
    connect( m_configDialog->kcfg_FilterByLabel, &QCheckBox::stateChanged, this, &LastFmServiceSettings::settingsChanged );
    connect( m_configDialog->kcfg_FilteredLabel, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &LastFmServiceSettings::settingsChanged );

    connect( m_configDialog->connectToAccount, &QPushButton::clicked, this, &LastFmServiceSettings::initiateTokenAuth );
    connect( m_configDialog->disconnectAccount, &QPushButton::clicked, this, &LastFmServiceSettings::disconnectAccount );

    using namespace Collections;

    QueryMaker *query = CollectionManager::instance()->queryMaker();
    query->setQueryType( QueryMaker::Label );
    connect( query, &QueryMaker::newLabelsReady, this, &LastFmServiceSettings::addNewLabels );
    query->setAutoDelete( true );
    query->run();
}

LastFmServiceSettings::~LastFmServiceSettings()
{
    delete m_configDialog;
}

void
LastFmServiceSettings::save()
{
    m_config->setScrobble( m_configDialog->kcfg_SubmitPlayedSongs->isChecked() );
    m_config->setFetchSimilar( m_configDialog->kcfg_RetrieveSimilarArtists->isChecked() );
    m_config->setScrobbleComposer( m_configDialog->kcfg_ScrobbleComposer->isChecked() );
    m_config->setUseFancyRatingTags( m_configDialog->kcfg_UseFancyRatingTags->isChecked() );
    m_config->setAnnounceCorrections( m_configDialog->kcfg_AnnounceCorrections->isChecked() );
    m_config->setFilterByLabel( m_configDialog->kcfg_FilterByLabel->isChecked() );
    m_config->setFilteredLabel( m_configDialog->kcfg_FilteredLabel->currentText() );
    m_config->save();

    KCModule::save();
}

void
LastFmServiceSettings::disconnectAccount()
{
    debug() << "Disconnecting Last.fm account"<< m_config->username();
    if( KMessageBox::warningYesNo( this, i18n( "Do you want to disconnect Amarok from Last.fm account %1?", m_config->username() ),
        i18n( "Disconnect Last.fm account?" ), KStandardGuiItem::yes(), KStandardGuiItem::cancel() ) == KMessageBox::Yes )
    {
        m_config->setSessionKey( QString() );
        m_config->setUsername( QString() );
        m_config->save();
        Q_EMIT( settingsChanged() );
        load();
    }
}

void
LastFmServiceSettings::initiateTokenAuth()
{
    m_configDialog->connectToAccount->setEnabled( false );
    m_configDialog->connectToAccount->setText( i18n( "Connecting..." ) );
    // set the global static Lastfm::Ws stuff
    lastfm::ws::ApiKey = Amarok::lastfmApiKey();
    lastfm::ws::SharedSecret = Amarok::lastfmApiSharedSecret();
    lastfm::ws::setScheme(lastfm::ws::Https);
    if( lastfm::nam() != The::networkAccessManager() )
        lastfm::setNetworkAccessManager( The::networkAccessManager() );

    debug() << "username:" << QString::fromLatin1( QUrl::toPercentEncoding( m_configDialog->kcfg_ScrobblerUsername->text() ) );

    QMap<QString, QString> query;

    query[ QStringLiteral("method") ] = QStringLiteral("auth.getToken");
    m_authQuery = lastfm::ws::get( query );

    connect( m_authQuery, &QNetworkReply::finished, this, &LastFmServiceSettings::onAuthTokenReady );
    connect( m_authQuery, QOverload<QNetworkReply::NetworkError>::of( &QNetworkReply::errorOccurred ),
             this, &LastFmServiceSettings::onError );
}

void
LastFmServiceSettings::onAuthTokenReady()
{
    lastfm::XmlQuery lfm;
    lfm.parse( m_authQuery->readAll() );

    switch( m_authQuery->error() )
    {
        case QNetworkReply::NoError:
             debug() << "NoError";
             if( lfm.children( QStringLiteral("error") ).size() > 0 || lfm[ QStringLiteral("token") ].text().isEmpty() )
             {
                 debug() << "ERROR from last.fm:" << lfm.text();
                 m_configDialog->connectToAccount->setText( i18nc( "The operation was rejected by the server", "Failed" ) );
                 m_configDialog->connectToAccount->setEnabled( true );

             } else
             {
                 m_configDialog->connectToAccount->setText( i18nc( "Last.fm authentication process status", "Opening Last.fm web page" ) );
                 m_configDialog->connectToAccount->setEnabled( false );
             }
             break;

        default:
            debug() << "Unhandled QNetworkReply state, probably not important";
    }
    m_authQuery->deleteLater();
    if( QDesktopServices::openUrl( QUrl( QStringLiteral( "https://www.last.fm/api/auth/?api_key=%1&token=%2" )
        .arg( QLatin1String( Amarok::lastfmApiKey() ), lfm[ QStringLiteral("token") ].text() ) ) ) )
    {
        QTimer::singleShot( 2000, [=] () // wait a moment for the browser to open, as connecting won't succeed before interaction in browser
        {
            disconnect( m_configDialog->connectToAccount, &QPushButton::clicked, this, &LastFmServiceSettings::initiateTokenAuth );
            connect( m_configDialog->connectToAccount, &QPushButton::clicked, [=]() { this->getSessionToken( lfm[ QStringLiteral("token") ].text() ); } );
            m_configDialog->connectToAccount->setEnabled( true );
            m_configDialog->connectToAccount->setText( i18nc( "Pushbutton to complete Last.fm authentication process",
                                                              "Finish connecting account" ) );
        } );
    }
}

void
LastFmServiceSettings::getSessionToken(const QString &sessionToken)
{
    QMap<QString, QString> query;
    query[ QStringLiteral("method") ] = QStringLiteral("auth.getSession");
    query[ QStringLiteral("token") ] = sessionToken;
    m_authQuery = lastfm::ws::get( query );
    connect( m_authQuery, &QNetworkReply::finished, this, &LastFmServiceSettings::onAuthenticated );
    connect( m_authQuery, QOverload<QNetworkReply::NetworkError>::of( &QNetworkReply::errorOccurred ),
             this, &LastFmServiceSettings::onError );
}

void
LastFmServiceSettings::onAuthenticated()
{
    lastfm::XmlQuery lfm;
    lfm.parse( m_authQuery->readAll() );

    QString sessionkey = lfm[ QStringLiteral("session") ][ QStringLiteral("key") ].text();
    QString username = lfm[ QStringLiteral("session") ][ QStringLiteral("name") ].text();
    QString tokenErrorMessage = i18n( "Last.fm account connection was not successful. Please make sure you have "
            "authorized Amarok to connect to your Last.fm account on the Last.fm web page that was opened and try again." );
    switch( m_authQuery->error() )
    {
        case QNetworkReply::NoError:
            debug() << "NoError";
            if( lfm.children( QStringLiteral("error") ).size() > 0 )
            {
                debug() << "ERROR from last.fm:" << lfm.text();
                m_configDialog->connectToAccount->setText( i18nc( "The operation was rejected by the server", "Failed" ) );
                m_configDialog->connectToAccount->setEnabled( true );
                if( lfm[ QStringLiteral("error") ].attribute( QStringLiteral("code") ) == QStringLiteral("14") )
                    KMessageBox::error( this, tokenErrorMessage, i18n( "Failed" ) );
            }
            else if( sessionkey.isEmpty() || username.isEmpty() )
            {
                debug() << "Problem getting last.fm sessionkey and username, response:" << lfm.text();
                m_configDialog->connectToAccount->setText( i18nc( "The operation was rejected by the server", "Failed" ) );
                KMessageBox::error( this, tokenErrorMessage, i18n( "Failed" ) );
            }
            else
            {
                m_config->setSessionKey( sessionkey );
                m_config->setUsername( username );
                m_configDialog->kcfg_ScrobblerUsername->setText( username );
                m_configDialog->connectToAccount->setText( i18nc( "The operation completed as expected", "Success" ) );
                m_configDialog->connectToAccount->setEnabled( false );
                m_configDialog->kcfg_SubmitPlayedSongs->setEnabled( true );
                Q_EMIT( settingsChanged() );
            }
            break;

        case QNetworkReply::AuthenticationRequiredError:
            debug() << "AuthenticationFailed";
            KMessageBox::error( this, tokenErrorMessage, i18n( "Failed" ) );
            m_configDialog->connectToAccount->setText( i18n( "Connect to account" ) );
            m_configDialog->connectToAccount->setEnabled( true );
            break;

        default:
            debug() << "Unhandled QNetworkReply state, probably not important";
    }
    m_authQuery->deleteLater();
}

void
LastFmServiceSettings::onError( QNetworkReply::NetworkError code )
{
    if( code == QNetworkReply::NoError )
        return;

    if( code == QNetworkReply::AuthenticationRequiredError )
    {
        onAuthenticated();
        return;
    }

    KMessageBox::error( this, i18n( "Unable to connect to Last.fm service." ), i18n( "Failed" ) );
    m_configDialog->connectToAccount->setText( i18n( "Connect to account" ) );
    m_configDialog->connectToAccount->setEnabled( true );

    debug() << "Error occurred during network request: " << m_authQuery->errorString();
    m_authQuery->deleteLater();
}

void
LastFmServiceSettings::onConfigUpdated()
{
    load();
}

void
LastFmServiceSettings::load()
{
    if( !m_config->sessionKey().isEmpty() && !m_config->username().isEmpty() )
    {
        m_configDialog->connectToAccount->setVisible( false );
        m_configDialog->kActiveLabel1->setVisible( false );
        m_configDialog->disconnectAccount->setVisible( true );
        m_configDialog->kcfg_ScrobblerUsername->setText( m_config->username() );
    }
    else
    {
        m_configDialog->connectToAccount->setVisible( true );
        m_configDialog->kActiveLabel1->setVisible( true );
        m_configDialog->disconnectAccount->setVisible( false );
        m_configDialog->kcfg_ScrobblerUsername->setText( i18n( "Not connected" ) );
    }
    m_configDialog->kcfg_SubmitPlayedSongs->setChecked( m_config->scrobble() );
    m_configDialog->kcfg_RetrieveSimilarArtists->setChecked( m_config->fetchSimilar() );
    m_configDialog->kcfg_ScrobbleComposer->setChecked( m_config->scrobbleComposer() );
    m_configDialog->kcfg_UseFancyRatingTags->setChecked( m_config->useFancyRatingTags() );
    m_configDialog->kcfg_AnnounceCorrections->setChecked( m_config->announceCorrections() );
    m_configDialog->kcfg_FilterByLabel->setChecked( m_config->filterByLabel() );
    m_configDialog->kcfg_FilteredLabel->setCurrentIndex( filteredLabelComboIndex( m_config->filteredLabel() ) );

    if( !m_config->sessionKey().isEmpty() && !m_config->username().isEmpty() )
        m_configDialog->kcfg_SubmitPlayedSongs->setEnabled( true );

    KCModule::load();
}

void
LastFmServiceSettings::defaults()
{
    m_configDialog->kcfg_SubmitPlayedSongs->setChecked( m_config->defaultScrobble() );
    m_configDialog->kcfg_RetrieveSimilarArtists->setChecked( m_config->defaultFetchSimilar() );
    m_configDialog->kcfg_ScrobbleComposer->setChecked( m_config->defaultScrobbleComposer() );
    m_configDialog->kcfg_UseFancyRatingTags->setChecked( m_config->defaultUseFancyRatingTags() );
    m_configDialog->kcfg_AnnounceCorrections->setChecked( m_config->defaultAnnounceCorrections() );
    m_configDialog->kcfg_FilterByLabel->setChecked( m_config->defaultFilterByLabel() );
    m_configDialog->kcfg_FilteredLabel->setCurrentIndex( filteredLabelComboIndex( m_config->defaultFilteredLabel() ) );
}

void
LastFmServiceSettings::settingsChanged()
{
    Q_EMIT changed( true );
}

void
LastFmServiceSettings::addNewLabels( const Meta::LabelList &labels )
{
    for( const Meta::LabelPtr &label : labels )
        m_configDialog->kcfg_FilteredLabel->addItem( label->name() );
}

int
LastFmServiceSettings::filteredLabelComboIndex( const QString &label )
{
    int index = m_configDialog->kcfg_FilteredLabel->findText( label );
    if( index == -1)
    {
        m_configDialog->kcfg_FilteredLabel->addItem( label );
        return m_configDialog->kcfg_FilteredLabel->findText( label );
    }
    else
        return index;
}

#include "LastFmServiceSettings.moc"
