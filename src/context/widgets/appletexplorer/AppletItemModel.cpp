/****************************************************************************************
 * Copyright (c) 2009 William Viana Soares <vianasw@gmail.com>                          *
 *                       Significant parts of this code is inspired                     *
 *                       and/or copied from KDE Plasma sources, available               *
 *                       at kdebase/workspace/libs/plasmagenericshell                   *
 *                                                                                      *
 ****************************************************************************************/

/****************************************************************************************
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


#include "AppletItemModel.h"

#include <plasma/applet.h>

#include <KIcon>
#include <KPluginInfo>

#include <QIcon>

namespace Context
{

//AppletItem

AppletItem::AppletItem( AppletItemModel *model, const QMap< QString, QVariant > &info )
{
    m_model = model;
    setText( info[ "name" ].toString() );
    setData( info );
    setIcon( qvariant_cast< QIcon >( info[ "icon" ] ) );
}


QString
AppletItem::name() const
{
    return data().toMap()[ "name" ].toString();
}


QString
AppletItem::pluginName() const
{
    return data().toMap()[ "pluginName" ].toString();
}

int
AppletItem::running() const
{
    return data().toMap()[ "runningCount" ].toInt();
}

void
AppletItem::setRunning( int count )
{
    QMap< QString, QVariant > info = data().toMap();
    info.insert( "runningCount", count );
    setData( QVariant( info ) );
}



//AppletItemModel

AppletItemModel::AppletItemModel( QObject *parent )
    : QStandardItemModel( parent )
{
    populateModel();
}


void
AppletItemModel::populateModel()
{
    clear();

    foreach( const KPluginInfo &info, Plasma::Applet::listAppletInfo( QString(), "amarok" ) )
    {
        if( info.property( "NoDisplay" ).toBool() || info.category() == i18n( "Containments" ) )
        {
            continue;
        }

        QMap< QString, QVariant > attrs;
        attrs.insert( "name", info.name() );
        attrs.insert( "pluginName", info.pluginName() );
        attrs.insert( "icon",
                      static_cast< QIcon >( KIcon( info.icon().isEmpty() ?
                            "application-x-plasma": info.icon() ) ) );
        appendRow( new AppletItem( this, attrs ) );
    }
}

}

#include "AppletItemModel.moc"
