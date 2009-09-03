/****************************************************************************************
 * Copyright (c) 2004 Michael Pyne <michael.pyne@kdemail.net>                           *
 * Copyright (c) 2004 Pierpaolo Di Panfilo <pippo_dp@libero.it>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "k3bexporter.h"

#include "Amarok.h"

#include "AmarokProcess.h"

#include <KLocale>
#include <KMessageBox>
#include <KStandardDirs>

#include <QByteArray>
#include <QStringList>
#include <Q3ValueList>


K3bExporter *K3bExporter::s_instance = 0;

// FIXME: implement me!
#if 0
bool K3bExporter::isAvailable() //static
{
    return !KStandardDirs::findExe( "k3b" ).isNull();
}

void K3bExporter::exportTracks( const KUrl::List &urls, int openmode )
{
    if( urls.empty() )
        return;

    DCOPClient *client = DCOPClient::mainClient();
    QByteArray appId, appObj;
    QByteArray data;

    if( openmode == -1 )
        //ask to open a data or an audio cd project
        openmode = openMode();

    if( !client->findObject( "k3b-*", "K3bInterface", "", data, appId, appObj) )
        exportViaCmdLine( urls, openmode );
    else {
        DCOPRef ref( appId, appObj );
        exportViaDCOP( urls, ref, openmode );
    }
}

void K3bExporter::exportCurrentPlaylist( int openmode )
{
    Playlist::instance()->burnPlaylist( openmode );
}

void K3bExporter::exportSelectedTracks( int openmode )
{
    Playlist::instance()->burnSelectedTracks( openmode );
}

void K3bExporter::exportAlbum( const QString &album, int openmode )
{
    exportAlbum( QString(), album, openmode );
}

void K3bExporter::exportAlbum( const QString &artist, const QString &album, int openmode )
{
    QString albumId = QString::number( CollectionDB::instance()->albumID( album, false, false, true ) );
    QString artistId;
    if( !artist.isNull() )
        artistId = QString::number( CollectionDB::instance()->artistID( artist, false, false, true ) );

    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
    qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valAlbumID, albumId );
    if( !artist.isNull() )
        qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, artistId );
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );

    QStringList values( qb.run() );

    if( !values.isEmpty() )
    {
        KUrl::List urls;

        oldForeach( values )
            urls << KUrl( *it );

        exportTracks( urls, openmode );
    }
}

void K3bExporter::exportArtist( const QString &artist, int openmode )
{
    const QString artistId = QString::number( CollectionDB::instance()->artistID( artist, false, false, true ) );

    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
    qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valArtistID, artistId );
    qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );

    QStringList values( qb.run() );

    if( !values.isEmpty() )
    {
        KUrl::List urls;

        oldForeach( values )
            urls << KUrl( *it );

        exportTracks( urls, openmode );
    }
}

void K3bExporter::exportComposer( const QString &composer, int openmode )
{
    const QString composerId = QString::number( CollectionDB::instance()->composerID( composer, false, false, true ) );

    QueryBuilder qb;
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );
    qb.addMatch( QueryBuilder::tabSong, QueryBuilder::valComposerID, composerId );
    qb.sortBy( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valDiscNumber );
    qb.sortBy( QueryBuilder::tabSong, QueryBuilder::valTrack );

    QStringList values( qb.run() );

    if( !values.isEmpty() )
    {
        KUrl::List urls;

        oldForeach( values )
            urls << KUrl( *it );

        exportTracks( urls, openmode );
    }
}

void K3bExporter::exportViaCmdLine( const KUrl::List &urls, int openmode )
{
    QByteArray cmdOption;

    switch( openmode ) {
    case AudioCD:
        cmdOption = "--audiocd";
        break;

    case DataCD:
        cmdOption = "--datacd";
        break;

    case Abort:
        return;
    }

    K3Process *process = new K3Process;

    *process << "k3b";
    *process << cmdOption;

    KUrl::List::ConstIterator it;
    KUrl::List::ConstIterator constEnd( urls.end() );
    for( it = urls.constBegin(); it != end; ++it )
        *process << ( *it ).path();

    if( !process->start( K3Process::DontCare ) )
        KMessageBox::error( 0, i18n("Unable to start K3b.") );
}

void K3bExporter::exportViaDCOP( const KUrl::List &urls, DCOPRef &ref, int openmode )
{
    Q3ValueList<DCOPRef> projectList;
    DCOPReply projectListReply = ref.call("projects()");

    if( !projectListReply.get<Q3ValueList<DCOPRef> >(projectList, "QValueList<DCOPRef>") ) {
        DCOPErrorMessage();
        return;
    }

    if( projectList.count() == 0 && !startNewK3bProject(ref, openmode) )
        return;

    if( !ref.send( "addUrls(KUrl::List)", DCOPArg(urls, "KUrl::List") ) ) {
        DCOPErrorMessage();
        return;
    }
}

void K3bExporter::DCOPErrorMessage()
{
    KMessageBox::error( 0, i18n("There was a DCOP communication error with K3b."));
}

bool K3bExporter::startNewK3bProject( DCOPRef &ref, int openmode )
{
    QByteArray request;
    //K3bOpenMode mode = openMode();

    switch( openmode ) {
    case AudioCD:
        request = "createAudioCDProject()";
        break;

    case DataCD:
        request = "createDataCDProject()";
        break;

    case Abort:
        return false;
    }

    KMessageBox::sorry(0,request);
    if( !ref.send( request ) ) {
        DCOPErrorMessage();
        return false;
    }

    return true;
}

K3bExporter::K3bOpenMode K3bExporter::openMode()
{
    int reply = KMessageBox::questionYesNoCancel(
        0,
        i18n("Create an audio mode CD suitable for CD players, or a data "
             "mode CD suitable for computers and other digital music "
             "players?"),
        i18n("Create K3b Project"),
        i18n("Audio Mode"),
        i18n("Data Mode")
    );

    switch(reply) {
    case KMessageBox::Cancel:
        return Abort;

    case KMessageBox::No:
        return DataCD;

    case KMessageBox::Yes:
        return AudioCD;
    }

    return Abort;
}
#endif
