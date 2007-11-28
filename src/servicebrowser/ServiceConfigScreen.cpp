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
 
#include "ServiceConfigScreen.h"

#include <KPluginSelector>

#include <QCheckBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>



ServiceConfigScreen::ServiceConfigScreen( QWidget * parent )
 : ConfigDialogBase( parent )
{

    KPluginSelector * selector = new KPluginSelector( this );

    QVBoxLayout *layout = new QVBoxLayout( this );
    layout->setMargin( 0 );
    layout->addWidget( selector );

    
    QList< ServiceFactory * > serviceFactories = ServicePluginManager::instance()->factories().values();

    QList<KPluginInfo> pluginInfoList;
    foreach ( ServiceFactory * factory, serviceFactories ) {
        pluginInfoList.append( factory->info() );
    }

    selector->addPlugins( pluginInfoList );
}


ServiceConfigScreen::~ServiceConfigScreen()
{
}


void ServiceConfigScreen::updateSettings()
{
}


bool ServiceConfigScreen::hasChanged()
{
    return false;
}


bool ServiceConfigScreen::isDefault()
{
    return true;
}


