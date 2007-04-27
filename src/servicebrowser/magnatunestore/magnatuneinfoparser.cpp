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

#include "magnatuneinfoparser.h"

#include "debug.h"
#include "magnatunedatabasehandler.h"
#include "statusbar.h"

#include <QFile>

MagnatuneInfoParser::MagnatuneInfoParser(  )
  : m_dbHandler ( 0 )

{
}


MagnatuneInfoParser::~MagnatuneInfoParser()
{}


void MagnatuneInfoParser::getInfo( SimpleServiceArtist *artist ) {
 
    MagnatuneArtist * magnatuneArtist = dynamic_cast<MagnatuneArtist *>( artist );

    debug() << "MagnatuneInfoParser: getInfo about artist" << endl;

    // first get the entire artist html page
   /* QString tempFile;
    QString orgHtml;*/

    m_infoDownloadJob = KIO::storedGet( magnatuneArtist->getHomeURL(), false, true );
    //Amarok::StatusBar::instance() ->newProgressOperation( m_infoDownloadJob ).setDescription( i18n( "Fetching Artist Info" ) );
    connect( m_infoDownloadJob, SIGNAL(result(KJob *)), SLOT( artistInfoDownloadComplete( KJob*) ) );

    Amarok::StatusBar::instance() ->newProgressOperation( m_infoDownloadJob )
    .setDescription( i18n( "Fetching artist info..." ) );

}


void MagnatuneInfoParser::getInfo( SimpleServiceAlbum *album )
{
    MagnatuneAlbum * magnatuneAlbum = dynamic_cast<MagnatuneAlbum *>( album );
   
    SimpleServiceArtist * artist = m_dbHandler->getArtistById( album->getArtistId() );
    const QString artistName = artist->getName();

    QString infoHtml = "<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" "
                       "CONTENT=\"text/html; charset=iso-8859-1\"></HEAD><BODY>";

    infoHtml += "<div align=\"center\"><strong>";
    infoHtml += artistName;
    infoHtml += "</strong><br><em>";
    infoHtml += magnatuneAlbum->getName();
    infoHtml += "</em><br><br>";
    infoHtml += "<img src=\"" + magnatuneAlbum->getCoverURL() +
                "\" align=\"middle\" border=\"1\">";

    infoHtml += "<br><br>Genre: " + magnatuneAlbum->getMp3Genre();
    infoHtml += "<br>Release Year: " + QString::number( magnatuneAlbum->getLaunchDate().year() );

    if ( !magnatuneAlbum->getDescription().isEmpty() ) {
 
        //debug() << "MagnatuneInfoParser: Writing description: '" << album->getDescription() << "'" << endl;
       infoHtml += "<br><br><b>Description:</b><br><p align=\"left\" >" + magnatuneAlbum->getDescription();

    }

    infoHtml += "</p><br><br>From Magnatune.com</div>";
    infoHtml += "</BODY></HTML>";

    emit ( info( infoHtml ) );
}

void
MagnatuneInfoParser::artistInfoDownloadComplete( KJob *downLoadJob )
{

    if ( !downLoadJob->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }
    if ( downLoadJob != m_infoDownloadJob )
        return ; //not the right job, so let's ignore it



    QString infoString = ((KIO::StoredTransferJob* )downLoadJob)->data();
    //debug() << "MagnatuneInfoParser: Artist info downloaded: " << infoString << endl;
    infoString = extractArtistInfo( infoString );

    //debug() << "html: " << trimmedInfo << endl;

    emit ( info( infoString ) );

}

QString
MagnatuneInfoParser::extractArtistInfo( const QString &artistPage )
{
    QString trimmedHtml;


    int sectionStart = artistPage.indexOf( "<!-- ARTISTBODY -->" );
    int sectionEnd = artistPage.indexOf( "<!-- /ARTISTBODY -->", sectionStart );

    trimmedHtml = artistPage.mid( sectionStart, sectionEnd - sectionStart );

    int buyStartIndex = trimmedHtml.indexOf( "<!-- PURCHASE -->" );
    int buyEndIndex;

    //we are going to integrate the buying of music (I hope) so remove these links

    while ( buyStartIndex != -1 )
    {
        buyEndIndex = trimmedHtml.indexOf( "<!-- /PURCHASE -->", buyStartIndex ) + 18;
        trimmedHtml.remove( buyStartIndex, buyEndIndex - buyStartIndex );
        buyStartIndex = trimmedHtml.indexOf( "<!-- PURCHASE -->", buyStartIndex );
    }


    QString infoHtml = "<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" "
                       "CONTENT=\"text/html; charset=iso-8859-1\"></HEAD><BODY>";

    infoHtml += trimmedHtml;
    infoHtml += "</BODY></HTML>";


    return infoHtml;
}

/*void MagnatuneInfoParser::resetScrollBars( )
{
    //note: the scrollbar methods never return 0
    view()->horizontalScrollBar()->setValue(0);
    view()->verticalScrollBar()->setValue(0);
}*/

void MagnatuneInfoParser::setDbHandler(MagnatuneDatabaseHandler * dbHandler)
{
    m_dbHandler = dbHandler;
}


#include "magnatuneinfoparser.moc"

