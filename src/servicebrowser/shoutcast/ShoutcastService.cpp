/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *             (c) 2007 Adam Pigg <adam@piggz.co.uk>                       *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/

#include "ShoutcastService.h"

#include "debug.h"
#include "amarok.h"
#include "statusbar.h"

#include <KTemporaryFile>



using namespace Meta;

ShoutcastService::ShoutcastService( const char *name )
    : ServiceBase( "Shoutcast Directory" )
{

    setShortDescription("The biggest damn list of online radio stations on the net :-)");
    setIcon( KIcon( Amarok::icon( "download" ) ) );


}


ShoutcastService::~ShoutcastService()
{
}

void ShoutcastService::polish()
{

    DEBUG_BLOCK

    m_collection = new ServiceCollection();

    m_storedTransferJob =  KIO::storedGet(  KUrl( "http://www.shoutcast.com/sbin/newxml.phtml" ), false, true );
    connect( m_storedTransferJob, SIGNAL( result( KJob * ))
        , this, SLOT(genreDownloadComplete(KJob *)));

    Amarok::StatusBar::instance() ->newProgressOperation( m_storedTransferJob )
    .setDescription( i18n( "Downloading Shoutcasst genres" ) );

}


void ShoutcastService::genreDownloadComplete( KJob *job )
{

    DEBUG_BLOCK

    QDomDocument doc( "genres" );

    doc.setContent( m_storedTransferJob->data() );

    debug() << "So far so good... Got this data: " << m_storedTransferJob->data();


    // We use this list to filter out some obscure genres
    QStringList bannedGenres;
    bannedGenres << "alles" << "any" << "anything" << "autopilot" << "backup" << "bandas" << "beer";
    bannedGenres << "catholic" << "chr" << "das" << "domaca" << "everything" << "fire" << "her" << "hollands";
    bannedGenres << "http" << "just" << "lokale" << "middle" << "noticias" << "only" << "scanner" << "shqip";
    bannedGenres << "good" << "super" << "wusf" << "www" << "zabavna" << "zouk" << "whatever" << "varios";
    bannedGenres << "varius" << "video" << "opm" << "non" << "narodna" << "muzyka" << "muzica" << "muzika";
    bannedGenres << "musique" << "music" << "multi" << "online" << "mpb" << "musica" << "musik" << "manele";
    bannedGenres << "paranormal" << "todos" << "soca" << "the" << "toda" << "trova" << "italo";
    bannedGenres << "auto" << "alternativo" << "best" << "clasicos" << "der" << "desi" << "die" << "emisora";
    bannedGenres << "voor" << "post" << "playlist" << "ned" << "gramy" << "deportes" << "bhangra" << "exitos";
    bannedGenres << "doowop" << "radio" << "radyo" << "railroad" << "program" << "mostly" << "hot";
    bannedGenres << "deejay" << "cool" << "big" << "exitos" << "mp3" << "muzyczne" << "nederlandstalig";
    bannedGenres << "max" << "informaci" << "halk" << "dobra" << "welcome" << "genre";

    // This maps genres that should be combined together
  /*  QMap<QString, QString> genreMapping;
    genreMapping["Romania"] = "Romanian";
    genreMapping["Turk"] = "Turkish";
    genreMapping["Turkce"] = "Turkish";
    genreMapping["Polskie"] = "Polska";
    genreMapping["Polski"] = "Polish";
    genreMapping["Greece"] = "Greek";
    genreMapping["Dnb"] = "Drum&bass";
    genreMapping["Classic"] = "Classical";
    genreMapping["Goth"] = "Gothic";
    genreMapping["Alt"] = "Alternative";
    genreMapping["Italiana"] = "Italian";
    genreMapping["Japan"] = "Japanese";
    genreMapping["Oldie"] = "Oldies";
    genreMapping["Nederlands"] = "Dutch";
    genreMapping["Variety"] = "Various";
    genreMapping["Soundtracks"] = "Soundtrack";
    genreMapping["Gaming"] = "Game";
    genreMapping["Sports"] = "Sport";
    genreMapping["Spain"] = "Spanish";*/

    GenreMap genreMap;
    TrackMap trackMap;

    QDomElement docElem = doc.documentElement();
    QDomNode n = docElem.firstChild();
    while( !n.isNull() )
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        const QString name = e.attribute( "name" );
        if( !name.isNull() && !bannedGenres.contains( name.toLower() ) && !genreMap.contains( name ) )
        {

            debug() << "add genre: " << name;

            ServiceGenre * genre = new ServiceGenre( name );
            GenrePtr genrePtr( genre );
            genreMap.insert(name,  genrePtr );


            ServiceTrack * track = new ServiceTrack ( "dummy" );
            TrackPtr trackPtr ( track );
            track->setGenre( genrePtr );
            genre->addTrack( trackPtr );

            trackMap.insert( "dummy - " + name, trackPtr );


            //genreCache[ name ] = last; // so we can append genres later if needed
        }
        n = n.nextSibling();
    }
    // Process the mapped (alternate) genres
   /* for( QMap<QString, QString>::iterator it = genreMapping.begin(); it != genreMapping.end(); ++it )
    {
        // Find the target genre
        ShoutcastGenre *existingGenre = dynamic_cast<ShoutcastGenre *> ( genreCache[ it.data() ] );
        if( existingGenre != 0 )
            existingGenre->appendAlternateGenre( it.key() );
    }*/

    m_collection->setGenreMap( genreMap );
    m_collection->setTrackMap( trackMap );

    QList<int> levels;
    levels << CategoryId::Genre;
    setModel( new SingleCollectionTreeItemModel( m_collection, levels ) );
    m_collection->forceUpdate();


}



/*void ShoutcastGenre::startGenreDownload( QString genre, QString tmppath )
{
    QString tmpfile = tmppath + "/amarok-list-" + genre + '-' + KRandom::randomString(10) + ".xml";
    KIO::CopyJob *cj = KIO::copy( "http://www.shoutcast.com/sbin/newxml.phtml?genre=" + genre, tmpfile, false );
    connect( cj, SIGNAL( copyingDone     ( KIO::Job*, const KUrl&, const KUrl&, time_t, bool, bool ) ),
             this,   SLOT( doneListDownload( KIO::Job*, const KUrl&, const KUrl&, time_t, bool, bool ) ) );
    connect( cj, SIGNAL( result     ( KJob* ) ),
             this,   SLOT( jobFinished( KJob* ) ) );
    m_totalJobs++;
}*/

#include "ShoutcastService.moc"





