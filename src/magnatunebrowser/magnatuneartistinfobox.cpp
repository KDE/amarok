//
// C++ Implementation: magnatuneartistinfobox
//
// Description: 
//
//
// Author: Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "magnatuneartistinfobox.h"
#include <qfile.h>
#include "debug.h"
#include "magnatunedatabasehandler.h"

MagnatuneArtistInfoBox::MagnatuneArtistInfoBox( QWidget *parentWidget, const char *widgetname)
        : KHTMLPart( parentWidget, widgetname )
{
}


MagnatuneArtistInfoBox::~MagnatuneArtistInfoBox()
{
}

bool MagnatuneArtistInfoBox::displayArtistInfo( KURL url )
{

	 debug() << "displayArtistInfo started" << endl;	

	// first get the entire artist html page
	QString tempFile;
	QString orgHtml;

        m_infoDownloadJob = KIO::storedGet( url, false, false );
        Amarok::StatusBar::instance()->newProgressOperation( m_infoDownloadJob ).setDescription( i18n( "Fetching Artist Info" ) );
        connect( m_infoDownloadJob, SIGNAL( result( KIO::Job* ) ), SLOT( infoDownloadComplete( KIO::Job* ) ) );


	return true;
	
}



bool MagnatuneArtistInfoBox::displayAlbumInfo( MagnatuneAlbum * album )
{
	
        

        MagnatuneArtist artist= MagnatuneDatabaseHandler::instance()->getArtistById(album->getArtistId());
        QString artistName = artist.getName();
 
        
        QString infoHtml = "<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=iso-8859-1\"></HEAD><BODY>";
	infoHtml += "<div align=\"center\"><strong>";
        infoHtml += artistName;
        infoHtml += "</strong><br><em>";
        infoHtml += album->getName();
        infoHtml += "</em><br><br>";
        infoHtml += "<img src=\"" + album->getCoverURL() + "\" align=\"middle\" border=\"1\">";
	infoHtml += "<br><br>From Magnatune.com</div>";
        infoHtml += "</BODY></HTML>";

	this->begin();
	this->write(infoHtml);
	this->end();
	this->show();

        return true;

}

void MagnatuneArtistInfoBox::infoDownloadComplete( KIO::Job * downLoadJob )
{

    if ( !downLoadJob->error() == 0 )
    {
       //TODO: error handling here
       return;
    }
    if ( downLoadJob != m_infoDownloadJob )
        return; //not the right job, so let's ignore it

    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( downLoadJob );
    QString info = QString( storedJob->data() );
     
    QString trimmedInfo = extractArtistInfo(info);

     //debug() << "html: " << trimmedInfo << endl;	

    this->begin();
    this->write(trimmedInfo);
    this->end();
    this->show();


}

QString MagnatuneArtistInfoBox::extractArtistInfo( QString artistPage )
{


        QString trimmedHtml;


        int sectionStart = artistPage.find("<!-- ARTISTBODY -->");
	int sectionEnd = artistPage.find("<!-- /ARTISTBODY -->", sectionStart);

	trimmedHtml = artistPage.mid(sectionStart, sectionEnd - sectionStart);   	

	int buyStartIndex = trimmedHtml.find("<!-- PURCHASE -->");
	int buyEndIndex;

	//we are going to integrate the buying of music (I hope) so remove these links

	while ( buyStartIndex != -1) {
		buyEndIndex = trimmedHtml.find("<!-- /PURCHASE -->", buyStartIndex) + 18;
		trimmedHtml.remove(buyStartIndex,buyEndIndex- buyStartIndex);
		buyStartIndex = trimmedHtml.find("<!-- PURCHASE -->", buyStartIndex);
	}


	QString infoHtml = "<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=iso-8859-1\"></HEAD><BODY>";
	infoHtml += trimmedHtml;
	infoHtml += "</BODY></HTML>";


        return infoHtml;
}

#include "magnatuneartistinfobox.moc"

