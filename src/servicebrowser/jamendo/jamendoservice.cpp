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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#include "jamendoservice.h"
#include "servicesqlcollection.h"

#include "jamendoxmlparser.h"

#include <KTemporaryFile>

JamendoService::JamendoService(const QString & name)
 : ServiceBase( name )
{

    setShortDescription("Another very friendly service!");
    setIcon( KIcon( Amarok::icon( "download" ) ) );

}


JamendoService::~JamendoService()
{
}

void JamendoService::polish()
{

    m_updateListButton = new QPushButton;
    m_updateListButton->setParent( m_bottomPanel );
    m_updateListButton->setText( i18n( "Update" ) );
    m_updateListButton->setObjectName( "updateButton" );
    m_updateListButton->setIcon( KIcon( Amarok::icon( "rescan" ) ) );

    connect( m_updateListButton, SIGNAL( clicked() ), this, SLOT( updateButtonClicked() ) );

     
    //m_model = new DatabaseDrivenContentModel();
    //m_dbHandler = new JamendoDatabaseHandler();
    //m_model->setDbHandler( m_dbHandler );

    QList<int> levels;
    //levels << CategoryId::Artist << CategoryId::Album << CategoryId::None;
    levels << CategoryId::Artist << CategoryId::Album;


    ServiceMetaFactory * metaFactory = new ServiceMetaFactory( "jamendo" );
    ServiceSqlCollection * collection = new ServiceSqlCollection( "jamendo", "Jamendo.com", metaFactory );

    setModel( new SingleCollectionTreeItemModel( collection, levels ) );


 

}

void JamendoService::updateButtonClicked()
{
    m_updateListButton->setEnabled( false );

    debug() << "JamendoService: start downloading xml file" << endl;

    KTemporaryFile tempFile;
    tempFile.setSuffix( ".gz" );
    tempFile.setAutoRemove( false );  //file will be removed in JamendoXmlParser
    if( !tempFile.open() )
    {
        return; //error
    }

    m_tempFileName = tempFile.fileName();
    m_listDownloadJob = KIO::file_copy( KUrl( "http://img.jamendo.com/data/dbdump.en.xml.gz" ), KUrl( m_tempFileName ), 0774 , true, false, true );
    Amarok::StatusBar::instance() ->newProgressOperation( m_listDownloadJob )
    .setDescription( i18n( "Downloading Jamendo.com Database" ) )
    .setAbortSlot( this, SLOT( listDownloadCancelled() ) );

    connect( m_listDownloadJob, SIGNAL( result( KJob * ) ),
            this, SLOT( listDownloadComplete( KJob * ) ) );


    //return true;
}

void JamendoService::listDownloadComplete(KJob * downloadJob)
{
    

    if ( downloadJob != m_listDownloadJob )
        return ; //not the right job, so let's ignore it
    debug() << "MagnatuneBrowser: xml file download complete" << endl;
   

    //testing

   

    if ( !downloadJob->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }

    //system( "gzip -df /tmp/dbdump.en.xml.gz" ); //FIXME!!!!!!!!! 

    debug() << "JamendoService: create xml parser" << endl;
    JamendoXmlParser * parser = new JamendoXmlParser( m_tempFileName );
    connect( parser, SIGNAL( doneParsing() ), SLOT( doneParsing() ) );

    ThreadManager::instance()->queueJob( parser );
    downloadJob->deleteLater();
    m_listDownloadJob = 0;

}

void JamendoService::listDownloadCancelled()
{

    Amarok::StatusBar::instance()->endProgressOperation( m_listDownloadJob );
    m_listDownloadJob->kill();
    delete m_listDownloadJob;
    m_listDownloadJob = 0;
    debug() << "Aborted xml download" << endl;

    m_updateListButton->setEnabled( true );
}

void JamendoService::doneParsing()
{
    debug() << "JamendoService: done parsing" << endl;
    m_updateListButton->setEnabled( true );
    // getModel->setGenre("All");
    //delete sender
    sender()->deleteLater();
}

#include "jamendoservice.moc"


