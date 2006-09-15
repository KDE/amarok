// Author: Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
#include "magnatunexmlparser.h"
#include "magnatunedatabasehandler.h"


#include "debug.h"
#include "amarok.h"
#include "statusbar.h" 


MagnatuneXmlParser::MagnatuneXmlParser(QString filename)
 : ThreadWeaver::Job( "MagnatuneXmlParser" )
{

	m_currentArtist = "";
	m_sFileName = filename;
}


MagnatuneXmlParser::~MagnatuneXmlParser()
{
}

bool MagnatuneXmlParser::doJob( )
{
	ReadConfigFile(m_sFileName); 
        return true;
}


void MagnatuneXmlParser::completeJob( )
{
  Amarok::StatusBar::instance()->longMessage( i18n( QString("Magnatune.com database update complete! Added %1 tracks on %2 albums from %3 artists").arg(m_nNumberOfTracks).arg(m_nNumberOfAlbums).arg(m_nNumberOfArtists).ascii()), KDE::StatusBar::Information );

  emit(DoneParsing());

}

void MagnatuneXmlParser::ReadConfigFile(QString filename)
{

 	m_nNumberOfTracks = 0;
	m_nNumberOfAlbums = 0;
	m_nNumberOfArtists = 0;

  	QDomDocument doc( "config" );

	QFile file(filename);
	if ( !file.open( IO_ReadOnly ) )
		return;
	if ( !doc.setContent( &file ) ) {
		file.close();
		return;
	}
	file.close();
	

        MagnatuneDatabaseHandler::instance()->destroyDatabase();
        MagnatuneDatabaseHandler::instance()->createDatabase();

	//run through all the elements
	QDomElement docElem = doc.documentElement();
	
        
	MagnatuneDatabaseHandler::instance()->begin(); //start transaction (MAJOR speedup!!)
	ParseElement(docElem);
	MagnatuneDatabaseHandler::instance()->commit(); //complete transaction

	return;



}


void MagnatuneXmlParser::ParseElement( QDomElement e )
{	

	QString sElementName = e.tagName();
	if(sElementName == "Album") {
		ParseAlbum(e);
	} else {	
		
		ParseChildren(e);
	}
		

}


void MagnatuneXmlParser::ParseChildren( QDomElement e )
{

	QDomNode n = e.firstChild();
	 
	while( !n.isNull() ) {
	
		if (n.isElement()) {
			ParseElement(n.toElement());
		}
        	
		n = n.nextSibling();
	}
}

void MagnatuneXmlParser::ParseAlbum( QDomElement e )
{

        m_pCurrentAlbum = new MagnatuneAlbum();
        m_pCurrentArtist = new MagnatuneArtist();

       
	QString sElementName;
	

	QDomNode n = e.firstChild();
	QDomElement childElement;
	 
	while( !n.isNull() ) {
	
		if (n.isElement()) {

			childElement = n.toElement();

			QString sElementName = childElement.tagName();
	

			if(sElementName == "albumname") {
				
				//printf(("|--+" + childElement.text() + "\n").ascii());
				//m_currentAlbumItem = new MagnatuneListViewAlbumItem( m_currentArtistItem); 
 				m_pCurrentAlbum->setName(childElement.text());
			} else if(sElementName == "albumsku") {
				
 				m_pCurrentAlbum->setAlbumCode(childElement.text());
			} else if(sElementName == "magnatunegenres") {
				
 				m_pCurrentAlbum->setMagnatuneGenres(childElement.text());

			} else if (sElementName == "launchdate") {

 				QString dateString = childElement.text();
				QDate date = QDate::fromString(dateString, Qt::ISODate);
 				m_pCurrentAlbum->setLaunchDate(date);

			}else if(sElementName == "cover_small") {
				
 				m_pCurrentAlbum->setCoverURL(childElement.text());
			} else if (sElementName == "artist") {
          			m_pCurrentArtist->setName(childElement.text());

			} else if (sElementName == "artistdesc") {
				m_pCurrentArtist->setDescription(childElement.text()); 

			} else if (sElementName == "artistphoto") {
				m_pCurrentArtist->setPhotoURL(childElement.text()); 
				//m_pCurrentArtist->setPhotoURL("dummy1"); 

			} else if (sElementName == "mp3genre") {
			      m_pCurrentAlbum->setMp3Genre(childElement.text());

			} else if (sElementName == "home") {

				m_pCurrentArtist->setHomeURL(childElement.text());
   				//m_pCurrentArtist->setHomeURL("dummy2");

			} else if (sElementName == "Track") {

				ParseTrack(childElement);
			}
		}

		n = n.nextSibling();
        }


        // now we should have gathered all info about current album (and artist)... 
        //Time to add stuff to the database

       	//check if artist already exists, if not, create him/her/them/it

      

	int artistId = MagnatuneDatabaseHandler::instance()->GetArtistIdByExactName(m_pCurrentArtist->getName());

	if (artistId == -1) {
		//does not exist, lets create it
		artistId = MagnatuneDatabaseHandler::instance()->insertArtist(m_pCurrentArtist);
		m_nNumberOfArtists++;
	}

	int albumId = MagnatuneDatabaseHandler::instance()->insertAlbum(m_pCurrentAlbum,artistId);
	m_nNumberOfAlbums++;



      	MagnatuneTrackList::iterator it;
        for ( it = m_currentAlbumTracksList.begin(); it != m_currentAlbumTracksList.end(); ++it ) {
	    MagnatuneDatabaseHandler::instance()->insertTrack(&(*it),albumId, artistId);
	    m_nNumberOfTracks++;

	}

         //MagnatuneDatabaseHandler::instance()->insertTracks( m_currentAlbumTracksList, albumId, artistId);
        m_currentAlbumTracksList.clear();
}



void MagnatuneXmlParser::ParseTrack( QDomElement e )
{
	
	QString trackName;
	QString trackNumber;
	QString streamingUrl;


	QString sElementName;
	QDomElement childElement;

        MagnatuneTrack currentTrack;
	

	QDomNode n = e.firstChild();
	 
	while( !n.isNull() ) {
	
		if (n.isElement()) {

			childElement = n.toElement();

			QString sElementName = childElement.tagName();
	

			if(sElementName == "trackname") {
				currentTrack.setName( childElement.text() );	
			} else if (sElementName == "url") {
				currentTrack.setHifiURL(childElement.text() );	
			} else if (sElementName == "mp3lofi") {
				currentTrack.setLofiURL(childElement.text() );	
			} else if (sElementName == "tracknum") {
				currentTrack.setTrackNumber(childElement.text().toInt() );	
			} else if (sElementName == "seconds") {
				currentTrack.setDuration(childElement.text().toInt() );	
			} 
		}
		n = n.nextSibling();
        }

        m_currentAlbumTracksList.append(currentTrack);

}

#include "magnatunexmlparser.moc"

