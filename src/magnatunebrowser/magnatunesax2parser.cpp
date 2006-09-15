// Author: Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
#include "magnatunesax2parser.h"

#include <stdio.h>
#include <qstring.h>
#include "debug.h"

bool MagnatuneSAX2Parser::startDocument()
{
    
    m_inAlbum = false;
    m_inTrack = false;
    m_inArtist = false;
    
    indent = "";

    m_currentAlbum = 0;
    m_currentArtist = 0;
    m_currentTrack = 0;

    return TRUE;
   
}

bool MagnatuneSAX2Parser::startElement( const QString&, const QString&,
                                    const QString& qName,
                                    const QXmlAttributes& )
{
    
    m_currentElementName = qName;

    if (qName == "Album") {
	albumStart();
    } else if (qName == "Track") {
	trackStart();
    }

    debug() << qName << endl;
    //printf( "%s%s\n", (const char*)indent.latin1(), (const char*) qName.latin1() );
    //indent += "    ";
    return TRUE;
}

bool MagnatuneSAX2Parser::endElement( const QString&, const QString&, const QString& qName)
{
    
    if (qName == "Album") {
	albumEnd();
    } if (qName == "Track") {
	trackEnd();
    }

    indent.remove( (uint)0, 4 );
    return TRUE;
}

bool MagnatuneSAX2Parser::characters( const QString & ch )
{

   //printf( "%s%s\n", (const char*)(indent + "   ").latin1(), (const char*) ch.latin1() );

   if (m_inAlbum and (!m_inTrack)) {

       handleAlbumElement(ch);

   } else if (m_inTrack) {
       handleTrackElement(ch);
   }

    return TRUE;
}

void MagnatuneSAX2Parser::albumStart()
{
    m_inAlbum = true;
    //m_currentAlbum = new MagnatuneListViewAlbumItem(m_currentArtist);
}


void MagnatuneSAX2Parser::handleAlbumElement( QString contents )
{

    if (m_currentElementName == "albumname" ) {
       m_currentAlbum->setName(contents);
    } else  if (m_currentElementName == "cover_small" ) {
       m_currentAlbum->setCoverURL(contents);
    } else  if (m_currentElementName == "launchdate" ) {
       //m_currentAlbum->setLaunchDate(contents);  needs parsing
    } else  if (m_currentElementName == "mp3genre" ) {
       m_currentAlbum->setMp3Genre(contents);
    } else  if (m_currentElementName == "magnatunegenres" ) {
       m_currentAlbum->setMagnatuneGenres(contents);
    } else  if (m_currentElementName == "albumsku" ) {
       m_currentAlbum->setAlbumCode(contents);
    }  else  if (m_currentElementName == "artist" ) {
       // hack since artists are not a well integrated part of xml structure...
       // will have to talk to Magnatune about this
       
       /*m_inArtist = true;

       if (m_currentArtist == 0) {
        
          m_currentArtist = new MagnatuneListViewArtistItem(m_targetList);
	  m_currentArtist->setName(contents);

       } else {

           m_currentArtistItem = (MagnatuneListViewArtistItem *) m_targetList->findItem(m_currentArtist->name, 0);
           if (m_currentArtistItem == 0)
           	m_currentArtist = new MagnatuneListViewArtistItem(m_targetList);
	  	m_currentArtist->setName(contents);
               
       }*/

    } 
}


void MagnatuneSAX2Parser::albumEnd()
{
    
   // m_currentAlbumItem = new QListViewItem(m_currentArtistItem, m_currentAlbum->name); 
   // m_currentAlbumItem->setName(
   // delete m_currentAlbum;
   // m_currentAlbum = 0;
    m_inAlbum = false;
}


void MagnatuneSAX2Parser::trackStart()
{
   m_currentTrack = new MagnatuneListViewTrackItem(m_currentAlbum);
    m_inTrack = true;
}

void MagnatuneSAX2Parser::handleTrackElement( QString contents )
{


    if (m_currentElementName == "trackname" ) {
       m_currentTrack->setName(contents);
    } else  if (m_currentElementName == "license" ) {
       //m_currentTrack->setLicense(contents);
    } else  if (m_currentElementName == "seconds" ) {
       m_currentTrack->setDuration(contents.toInt());
    } else  if (m_currentElementName == "mp3lofi" ) {
       m_currentTrack->setLofiURL(contents);
    } else  if (m_currentElementName == "url" ) {
       m_currentTrack->setHifiURL(contents);
    } else  if (m_currentElementName == "tracknum" ) {
       m_currentTrack->setTrackNumber(contents.toInt());
    } 

}

void MagnatuneSAX2Parser::trackEnd()
{

    m_inTrack = false;
}

void MagnatuneSAX2Parser::setTargetListView( KListView * target )
{
    m_targetList = target;
}



