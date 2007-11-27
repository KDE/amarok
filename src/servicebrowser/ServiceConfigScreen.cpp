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

#include <QCheckBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

ServiceConfigScreen::ServiceConfigScreen( QWidget * parent )
 : ConfigDialogBase( parent )
{

    QList<QString> serviceNames = ServicePluginManager::instance()->factories().keys();

    //add a layout
    QVBoxLayout *layout = new QVBoxLayout;

    //setup a config item for each service

    int runningY = 0;
    foreach ( QString serviceName, serviceNames ) {

        QFrame * serviceItem = new QFrame( this );

        serviceItem->setFrameStyle(QFrame::Panel | QFrame::Raised);
        serviceItem->setLineWidth(2);

        QHBoxLayout *itemLayout = new QHBoxLayout;

        itemLayout->addWidget( new QLabel( serviceName ) );
        itemLayout->addWidget( new QCheckBox("Enable", this) );
        itemLayout->addWidget( new QCheckBox("Add to collection", this) );
        itemLayout->addWidget( new QPushButton("Configure", this) );

        serviceItem->setLayout( itemLayout );
        layout->addWidget( serviceItem );

    }

    setLayout(layout);

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


