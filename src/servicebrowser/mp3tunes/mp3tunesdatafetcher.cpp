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
#include "mp3tunesdatafetcher.h"

#include "amarok.h"
#include "statusbar.h"


#include <kpassworddialog.h>

Mp3tunesDataFetcher::Mp3tunesDataFetcher()
 : QObject()
 , DatabaseHandlerBase()
{

    m_apiOutputFormat = "xml"; //get replies as xml
    m_partnerToken = "7359149936"; //Amaroks partner token

    m_sessionId = "";

}


Mp3tunesDataFetcher::~Mp3tunesDataFetcher()
{
}

int Mp3tunesDataFetcher::authenticate(const QString & uname, const QString & passwd)
{
    QString username, password;
   
    if ( uname.isEmpty() || passwd.isEmpty() ) {
        KPasswordDialog dlg( 0 , KPasswordDialog::ShowUsernameLine );  //FIXME 0x02 = KPasswordDialog::showUsername according to api, but that does not work
        dlg.setPrompt( i18n( "Enter a login and a password" ) );
        if( !dlg.exec() )
            return 0; //the user canceled

        username = dlg.username();
        password = dlg.password();

    } else {
        username = uname;
        password = passwd;
    }

    QString authenticationString = "https://shop.mp3tunes.com/api/v0/login?username=<username>&password=<password>&partner_token=<partner token>&output=<output format>";

    authenticationString.replace(QString("<username>"), username);
    authenticationString.replace(QString("<password>"), password);
    authenticationString.replace(QString("<partner token>"), m_partnerToken);
    authenticationString.replace(QString("<output format>"), m_apiOutputFormat);

    debug() << "Authenticating with string: " << authenticationString << endl;



    m_xmlDownloadJob = KIO::storedGet( authenticationString, false, false );
    connect( m_xmlDownloadJob, SIGNAL(result(KJob *)), SLOT( authenticationComplete( KJob*) ) );
    Amarok::StatusBar::instance() ->newProgressOperation( m_xmlDownloadJob )
    .setDescription( i18n( "Authenticating" ) );

    return 1;

}

void Mp3tunesDataFetcher::authenticationComplete(KJob * job)
{

    if ( !job->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }
    if ( job != m_xmlDownloadJob )
        return ; //not the right job, so let's ignore it


    QString xmlReply = ((KIO::StoredTransferJob* )job)->data();
    debug() << "Authentication reply: " << xmlReply << endl;

}



void Mp3tunesDataFetcher::getArtistsComplete(KJob * job)
{
}

void Mp3tunesDataFetcher::getAlbumsComplete(KJob * job)
{
}

void Mp3tunesDataFetcher::getTracksComplete(KJob * job)
{
}


// all this stuff needs to be implemeted...

int Mp3tunesDataFetcher::getArtistIdByExactName(const QString & name)
{
    return 0;
}

SimpleServiceArtistList Mp3tunesDataFetcher::getArtistsByGenre(const QString & genre)
{
    return SimpleServiceArtistList();
}

SimpleServiceArtist * Mp3tunesDataFetcher::getArtistById(int id)
{
   return new SimpleServiceArtist();
}

SimpleServiceAlbum * Mp3tunesDataFetcher::getAlbumById(int id)
{
    return new SimpleServiceAlbum();
}

SimpleServiceTrack * Mp3tunesDataFetcher::getTrackById(int id)
{
    return new SimpleServiceTrack();
}

SimpleServiceAlbumList Mp3tunesDataFetcher::getAlbumsByArtistId(int id, const QString & genre)
{
    return SimpleServiceAlbumList();
}

SimpleServiceTrackList Mp3tunesDataFetcher::getTracksByAlbumId(int id)
{
    return SimpleServiceTrackList();
}

QStringList Mp3tunesDataFetcher::getAlbumGenres()
{
    return QStringList();
}

#include "mp3tunesdatafetcher.moc"


