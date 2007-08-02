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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#include "jamendoservice.h"

#include "debug.h"
#include "JamendoInfoParser.h"
#include "jamendoxmlparser.h"
#include "ServiceSqlRegistry.h"


#include <KTemporaryFile>

using namespace Meta;

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
    if ( m_polished ) return;

    m_updateListButton = new QPushButton;
    m_updateListButton->setParent( m_bottomPanel );
    m_updateListButton->setText( i18n( "Update" ) );
    m_updateListButton->setObjectName( "updateButton" );
    m_updateListButton->setIcon( KIcon( Amarok::icon( "rescan" ) ) );

    connect( m_updateListButton, SIGNAL( clicked() ), this, SLOT( updateButtonClicked() ) );

    m_debugButton = new QPushButton;
    m_debugButton->setParent( m_bottomPanel );
    m_debugButton->setText( "Debug"  );
    m_debugButton->setObjectName( "debugButton" );
    m_debugButton->setIcon( KIcon( Amarok::icon( "rescan" ) ) );

    connect( m_debugButton, SIGNAL( clicked() ), this, SLOT( debugSlot() ) );


    m_infoParser = new JamendoInfoParser();

    //TODO: move this to base class?
    connect ( m_infoParser, SIGNAL( info( QString) ), this, SLOT( infoChanged( QString ) ) );

    //m_model = new DatabaseDrivenContentModel();
    //m_dbHandler = new JamendoDatabaseHandler();
    //m_model->setDbHandler( m_dbHandler );

    QList<int> levels;
    //levels << CategoryId::Artist << CategoryId::Album << CategoryId::None;
    levels << CategoryId::Genre << CategoryId::Artist << CategoryId::Album;


    ServiceMetaFactory * metaFactory = new JamendoMetaFactory( "jamendo" );
    ServiceSqlRegistry * registry = new ServiceSqlRegistry( metaFactory );
    m_collection = new ServiceSqlCollection( "jamendo", "Jamendo.com", metaFactory, registry );

    setModel( new SingleCollectionTreeItemModel( m_collection, levels ) );

    m_polished = true;


}

void JamendoService::updateButtonClicked()
{
    m_updateListButton->setEnabled( false );

    debug() << "JamendoService: start downloading xml file";

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
    debug() << "JamendoService: xml file download complete";


    //testing



    if ( !downloadJob->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }

    //system( "gzip -df /tmp/dbdump.en.xml.gz" ); //FIXME!!!!!!!!!

    debug() << "JamendoService: create xml parser";
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
    debug() << "Aborted xml download";

    m_updateListButton->setEnabled( true );
}

void JamendoService::doneParsing()
{
    debug() << "JamendoService: done parsing";
    m_updateListButton->setEnabled( true );
    // getModel->setGenre("All");
    //delete sender
    sender()->deleteLater();
    m_collection->emitUpdated();
}

void JamendoService::debugSlot()
{

    debug() << "JamendoService: create xml parser";
    JamendoXmlParser * parser = new JamendoXmlParser( "/tmp/dbdump.en.xml" );
    connect( parser, SIGNAL( doneParsing() ), SLOT( doneParsing() ) );

    ThreadManager::instance()->queueJob( parser );
    //downloadJob->deleteLater();
   // m_listDownloadJob = 0;
}

#include "jamendoservice.moc"


