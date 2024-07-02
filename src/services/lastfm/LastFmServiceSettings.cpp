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

#include <QCryptographicHash>

#include <XmlQuery.h>

K_PLUGIN_FACTORY_WITH_JSON( LastFmServiceSettingsFactory, "amarok_service_lastfm_config.json", registerPlugin<LastFmServiceSettings>(); )

LastFmServiceSettings::LastFmServiceSettings( QWidget *parent, const QVariantList &args )
    : KCModule( parent, args )
    , m_config( LastFmServiceConfig::instance() )
{
    m_configDialog = new Ui::LastFmConfigWidget;
    m_configDialog->setupUi( this );

    connect( m_config.data(), &LastFmServiceConfig::updated, this, &LastFmServiceSettings::onConfigUpdated );

    connect( m_configDialog->kcfg_ScrobblerUsername, &QLineEdit::textChanged, this, &LastFmServiceSettings::settingsChanged );
    connect( m_configDialog->kcfg_ScrobblerPassword, &QLineEdit::textChanged, this, &LastFmServiceSettings::settingsChanged );
    connect( m_configDialog->kcfg_SubmitPlayedSongs, &QCheckBox::stateChanged, this, &LastFmServiceSettings::settingsChanged );
    connect( m_configDialog->kcfg_RetrieveSimilarArtists, &QCheckBox::stateChanged, this, &LastFmServiceSettings::settingsChanged );
    connect( m_configDialog->kcfg_ScrobbleComposer, &QCheckBox::stateChanged, this, &LastFmServiceSettings::settingsChanged );
    connect( m_configDialog->kcfg_UseFancyRatingTags, &QCheckBox::stateChanged, this, &LastFmServiceSettings::settingsChanged );
    connect( m_configDialog->kcfg_AnnounceCorrections, &QCheckBox::stateChanged, this, &LastFmServiceSettings::settingsChanged );
    connect( m_configDialog->kcfg_FilterByLabel, &QCheckBox::stateChanged, this, &LastFmServiceSettings::settingsChanged );
    connect( m_configDialog->kcfg_FilteredLabel, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &LastFmServiceSettings::settingsChanged );
    connect( m_configDialog->testLogin, &QPushButton::clicked, this, &LastFmServiceSettings::testLogin );

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
    QString dialogUser = m_configDialog->kcfg_ScrobblerUsername->text();
    QString dialogPass = m_configDialog->kcfg_ScrobblerPassword->text();

    // clear the session key if credentials changed
    if( m_config->username() != dialogUser || m_config->password() != dialogPass )
        m_config->setSessionKey( QString() );

    m_config->setUsername( dialogUser );
    m_config->setPassword( dialogPass );
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
LastFmServiceSettings::testLogin()
{
    m_configDialog->testLogin->setEnabled( false );
    m_configDialog->testLogin->setText( i18n( "Testing..." ) );
    // set the global static Lastfm::Ws stuff
    lastfm::ws::ApiKey = Amarok::lastfmApiKey();
    lastfm::ws::SharedSecret = Amarok::lastfmApiSharedSecret();
    lastfm::ws::setScheme(lastfm::ws::Https);
    if( lastfm::nam() != The::networkAccessManager() )
        lastfm::setNetworkAccessManager( The::networkAccessManager() );

    debug() << "username:" << QString( QUrl::toPercentEncoding( m_configDialog->kcfg_ScrobblerUsername->text().toUtf8() ) );

    QMap<QString, QString> query;

    query[ QStringLiteral("method") ] = QStringLiteral("auth.getMobileSession");
    query[ QStringLiteral("password") ] = m_configDialog->kcfg_ScrobblerPassword->text().toUtf8();
    query[ QStringLiteral("username") ] = m_configDialog->kcfg_ScrobblerUsername->text().toUtf8();
    m_authQuery = lastfm::ws::post( query );

    connect( m_authQuery, &QNetworkReply::finished, this, &LastFmServiceSettings::onAuthenticated );
    connect( m_authQuery, QOverload<QNetworkReply::NetworkError>::of( &QNetworkReply::errorOccurred ),
             this, &LastFmServiceSettings::onError );
}

void
LastFmServiceSettings::onAuthenticated()
{
    lastfm::XmlQuery lfm;
    lfm.parse( m_authQuery->readAll() );

    switch( m_authQuery->error() )
    {
        case QNetworkReply::NoError:
             debug() << "NoError";
             if( lfm.children( QStringLiteral("error") ).size() > 0 )
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
LastFmServiceSettings::onConfigUpdated()
{
    load();
}

void
LastFmServiceSettings::load()
{
    m_configDialog->kcfg_ScrobblerUsername->setText( m_config->username() );
    m_configDialog->kcfg_ScrobblerPassword->setText( m_config->password() );
    m_configDialog->kcfg_SubmitPlayedSongs->setChecked( m_config->scrobble() );
    m_configDialog->kcfg_RetrieveSimilarArtists->setChecked( m_config->fetchSimilar() );
    m_configDialog->kcfg_ScrobbleComposer->setChecked( m_config->scrobbleComposer() );
    m_configDialog->kcfg_UseFancyRatingTags->setChecked( m_config->useFancyRatingTags() );
    m_configDialog->kcfg_AnnounceCorrections->setChecked( m_config->announceCorrections() );
    m_configDialog->kcfg_FilterByLabel->setChecked( m_config->filterByLabel() );
    m_configDialog->kcfg_FilteredLabel->setCurrentIndex( filteredLabelComboIndex( m_config->filteredLabel() ) );

    if( !m_config->username().isEmpty() && !m_config->password().isEmpty() )
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
    //TODO: Make pretty validation for username and password
    //with error reporting

    m_configDialog->testLogin->setText( i18n( "&Test Login" ) );
    m_configDialog->testLogin->setEnabled( true );

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
