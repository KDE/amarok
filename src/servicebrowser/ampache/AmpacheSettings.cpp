/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#include "AmpacheSettings.h"

#include "ui_AmpacheConfigWidget.h"

#include <kdebug.h>
#include <kgenericfactory.h>

#include <QVBoxLayout>

K_PLUGIN_FACTORY(AmpacheSettingsFactory, registerPlugin<AmpacheSettings>();)
K_EXPORT_PLUGIN(AmpacheSettingsFactory( "kcm_amarok_ampache" ))


AmpacheSettings::AmpacheSettings(QWidget * parent, const QVariantList & args)
    : KCModule( AmpacheSettingsFactory::componentData(), parent, args )
{

    kDebug( 14310 ) << "Creating Ampache config object";

    QVBoxLayout* l = new QVBoxLayout( this );
    QWidget *w = new QWidget;
    m_configDialog = new Ui::AmpacheConfigWidget;
    m_configDialog->setupUi( w );
    l->addWidget( w );

    load();
}

AmpacheSettings::~AmpacheSettings()
{
}

void AmpacheSettings::save()
{
    kDebug( 14310 ) << "save";
}

void AmpacheSettings::load()
{

    kDebug( 14310 ) << "load";
   // KCModule::load();
}

void AmpacheSettings::defaults()
{
    kDebug( 14310 ) << "defaults";
}
