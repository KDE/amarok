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

#include "servicebase.h"

#include "amarok.h"
#include "debug.h"

#include <khbox.h>

#include <QFrame>
#include <QLabel>
#include <QSplitter>


ServiceBase *ServiceBase::s_instance = 0;

ServiceBase::ServiceBase( QString name )
        : KVBox( 0)
{

    DEBUG_BLOCK

    m_name = name;

    m_topPanel = new KVBox( this );

    m_topPanel->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
    m_topPanel->setLineWidth(2);
    //m_topPanel->setFixedHeight( 50 );

    KHBox * commonPanel = new KHBox ( m_topPanel );


    m_homeButton = new QPushButton( "Home", commonPanel );
    connect( m_homeButton, SIGNAL( clicked( bool ) ), this, SLOT( homeButtonClicked( ) ) );

    QLabel * nameLabel = new QLabel( commonPanel );
    nameLabel->setText( m_name );
    nameLabel->setAlignment(Qt::AlignTop | Qt::AlignCenter);
   
    QSplitter *splitter = new QSplitter( Qt::Vertical, this );
    m_contentList = new KListWidget( splitter );

    m_infoBox = new KHTMLPart( splitter );
    //m_infoBox->view()->widget()->setFrameStyle(QFrame::StyledPanel | QFrame::Plain); //kdelibs error?


    QString infoHtml = "<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" " 
                       "CONTENT=\"text/html; charset=iso-8859-1\"></HEAD><BODY>";
    infoHtml += "<div align=\"center\"><strong>";
    infoHtml += "Hello there";
    infoHtml += "</strong><br><br>I am ";
    infoHtml += m_name;
    infoHtml += "</div></BODY></HTML>";

    m_infoBox->begin();
    m_infoBox->write(infoHtml);
    m_infoBox->end();

    m_bottomPanel = new KVBox( this );
    m_bottomPanel->setFixedHeight( 50 );
    m_bottomPanel->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
    m_bottomPanel->setLineWidth(2);

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

void ServiceBase::homeButtonClicked( ) 
{

    emit( home() );

}






#include "servicebase.moc"
