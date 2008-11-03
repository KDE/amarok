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

#include <QVBoxLayout>
#include <QRegExpValidator>

#include <KPluginFactory>

K_PLUGIN_FACTORY( LastFmServiceSettingsFactory, registerPlugin<LastFmServiceSettings>(); )
K_EXPORT_PLUGIN( LastFmServiceSettingsFactory( "kcm_amarok_lastfm" ) )


LastFmServiceSettings::LastFmServiceSettings( QWidget *parent, const QVariantList &args )
    : KCModule( LastFmServiceSettingsFactory::componentData(), parent, args )
{
    debug() << "Creating Last.fm config object";

    QVBoxLayout* l = new QVBoxLayout( this );
    QWidget *w = new QWidget;
    m_configDialog = new Ui::LastFmConfigWidget;
    m_configDialog->setupUi( w );
    l->addWidget( w );
    
    QRegExp rx( "[A-Za-z][A-Za-z0-9]{1,14}" );
    QValidator *validator = new QRegExpValidator( rx, this );
    m_configDialog->kcfg_ScrobblerUsername->setValidator( validator );

    connect( m_configDialog->kcfg_ScrobblerUsername, SIGNAL( textChanged( const QString & ) ), this, SLOT( settingsChanged() ) );
    connect( m_configDialog->kcfg_ScrobblerPassword, SIGNAL( textChanged( const QString & ) ), this, SLOT( settingsChanged() ) );
    connect( m_configDialog->kcfg_SubmitPlayedSongs, SIGNAL( stateChanged( int ) ), this, SLOT( settingsChanged() ) );
    connect( m_configDialog->kcfg_RetrieveSimilarArtists, SIGNAL( stateChanged( int ) ), this, SLOT( settingsChanged() ) );

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
    load();
}


void
LastFmServiceSettings::settingsChanged()
{
    emit changed( true );
}
