/*
Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public License
along with this library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.
*/


#include "amarok.h"
#include "debug.h"


ServiceBase *ServiceBase::s_instance = 0;

ServiceBase::ServiceBase( QString name )
        : KVBox( 0)
{

    DEBUG_BLOCK

    m_name = name;
    m_topPanel = new KVBox( this);
    QSplitter *spliter = new QSplitter( Qt::Vertical, this );
    m_contentList = new KListWidget( spliter );
    m_infoBox = new m_infoBox( spliter, "infoBox" );
    m_bottomPanel = new KVBox( this );

}


void ServiceBase::showInfo( bool show )
{
    if ( show )
    {
        m_isInfoShown = true;
        m_infoBox->widget()->setMaximumHeight( 2000 );
    }
    else
    {
        m_infoBox->widget()->setMaximumHeight( 0 );
        m_isInfoShown = false;
    }
}

QString ServiceBase::getName( )
{
    return m_name;
}

void ServiceBase::setShortDescription( QString shortDescription )
{
    m_shortDescription = shortDescription;
}

QString ServiceBase::getShortDescription( )
{
    return m_shortDescription;
}

void ServiceBase::setLongDescription( QString longDescription )
{
    m_longDescription = longDescription;
}

QString ServiceBase::getLongDescription( )
{
    return m_longDescription;
}

void ServiceBase::setPixmap( QPixmap pixmap )
{
    m_pixmap = pixmap;
}

QPixmap ServiceBase::getPixmap( )
{
    return m_pixmap;
}






#include "servicebase.moc"
