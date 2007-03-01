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

#include "magnatuneinfoparser.h"

#include "debug.h"
#include "magnatunedatabasehandler.h"

#include <QFile>

MagnatuneInfoParser::MagnatuneInfoParser(  )
  : QObject()

{
}


MagnatuneInfoParser::~MagnatuneInfoParser()
{}


void MagnatuneInfoParser::getInfo( MagnatuneArtist *artist ) {
 
    debug() << "MagnatuneInfoParser: getInfo about artist" << endl;

    // first get the entire artist html page
   /* QString tempFile;
    QString orgHtml;*/

    m_infoDownloadJob = KIO::storedGet( artist->getHomeURL(), false, true );
    //Amarok::StatusBar::instance() ->newProgressOperation( m_infoDownloadJob ).setDescription( i18n( "Fetching Artist Info" ) );
    connect( m_infoDownloadJob, SIGNAL(result(KJob *)), SLOT( artistInfoDownloadComplete( KJob*) ) );

}


void MagnatuneInfoParser::getInfo( MagnatuneAlbum *album )
{
    const MagnatuneArtist artist = MagnatuneDatabaseHandler::instance()->getArtistById( album->getArtistId() );
    const QString artistName = artist.getName();

    QString infoHtml = "<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" "
                       "CONTENT=\"text/html; charset=iso-8859-1\"></HEAD><BODY>";

    infoHtml += "<div align=\"center\"><strong>";
    infoHtml += artistName;
    infoHtml += "</strong><br><em>";
    infoHtml += album->getName();
    infoHtml += "</em><br><br>";
    infoHtml += "<img src=\"" + album->getCoverURL() +
                "\" align=\"middle\" border=\"1\">";

    infoHtml += "<br><br>Genre: " + album->getMp3Genre();
    infoHtml += "<br>Release Year: " + QString::number( album->getLaunchDate().year() );
    infoHtml += "<br><br>From Magnatune.com</div>";
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
    debug() << "MagnatuneInfoParser: Artist info downloaded: " << infoString << endl;
    infoString = extractArtistInfo( infoString );

    //debug() << "html: " << trimmedInfo << endl;

    emit ( info( infoString ) );

}

QString
MagnatuneInfoParser::extractArtistInfo( QString artistPage )
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


#include "magnatuneinfoparser.moc"

