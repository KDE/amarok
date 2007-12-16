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
#include "debug.h"

#include <QVBoxLayout>

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
}


LastFmServiceSettings::~LastFmServiceSettings()
{
}


void 
LastFmServiceSettings::save()
{
}


void 
LastFmServiceSettings::load()
{
}


void
LastFmServiceSettings::defaults()
{
}
