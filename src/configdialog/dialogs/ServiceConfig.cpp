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
 
#include "ServiceConfig.h"

#include "debug.h"

#include <KSharedConfig>

#include <QCheckBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>


ServiceConfig::ServiceConfig( QWidget * parent )
 : ConfigDialogBase( parent )
 , m_configChanged( false )
{

    m_serviceSelector = new KPluginSelector( this );
//     m_serviceSelector->setMaximumSize( QSize( 3500000, 600 ) );
    m_serviceSelector->setSizePolicy( QSizePolicy:: Expanding, QSizePolicy::Expanding );

    QVBoxLayout *layout = new QVBoxLayout( this );
    layout->setMargin( 0 );
    layout->addWidget( m_serviceSelector );

    connect( m_serviceSelector, SIGNAL( changed( bool ) ), SLOT( slotConfigChanged( bool ) ) );

    
    QList< ServiceFactory * > serviceFactories = ServicePluginManager::instance()->factories().values();

    QList<KPluginInfo> pluginInfoList;
    foreach ( ServiceFactory * factory, serviceFactories ) {
        pluginInfoList.append( factory->info() );
    }

    m_serviceSelector->addPlugins( pluginInfoList, KPluginSelector::ReadConfigFile, "Services" );
}


ServiceConfig::~ServiceConfig()
{} 


void ServiceConfig::updateSettings()
{
    m_serviceSelector->save();
    ServicePluginManager::instance()->settingsChanged();
}


bool ServiceConfig::hasChanged()
{
    DEBUG_BLOCK;
    return m_configChanged;
}


bool ServiceConfig::isDefault()
{
    DEBUG_BLOCK;
    return false;
}


void ServiceConfig::slotConfigChanged( bool changed )
{
    DEBUG_BLOCK
    m_configChanged = changed;
}

#include "ServiceConfig.moc"
