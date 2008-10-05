/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Max Howell, Last.fm Ltd <max@last.fm>                              *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "logger.h"
#include "Request.h"
#include "Settings.h"
#include "WebService.h"


ChangeStationRequest::ChangeStationRequest()
        : Request( TypeChangeStation, "ChangeStation" ),
          m_hasXspf( false )
{}

void
ChangeStationRequest::start()
{
    QString lang_code = language();
    if (lang_code.isEmpty()) 
        lang_code = "en";

    QString urlWithoutProtocol = m_stationUrl;
    if ( m_stationUrl.startsWith( "lastfm://" ) )
    {
        urlWithoutProtocol = m_stationUrl.right( m_stationUrl.size() - 9 );
    }

    QString url = urlWithoutProtocol.contains( "%" ) //FIXME can't we just always do this?
        ? urlWithoutProtocol
        : QString( QUrl::toPercentEncoding( urlWithoutProtocol, "/," ) );

    QString path;
    if ( m_stationUrl.isPlaylist() )
    {
        // It's a preview/playlist, use getresourceplaylist
        path = "/1.0/webclient/getresourceplaylist.php?sk=" + m_session +
               "&url=lastfm://" + url +
               "&desktop=1"; // plain text

        m_hasXspf = true;
    }
    else
    {
        // It's a normal station, use adjust
        path = m_basePath +
               "/adjust.php?session=" + m_session +
               "&url=lastfm://" + url +
               "&lang=" + lang_code;
    }

    get( path );
}
    
void
ChangeStationRequest::success( QByteArray data )
{
    qDebug() << "ChangeStation response: " << data;

    if ( hasXspf() )
    {
        m_xspf = data;
    }
    else
    {
        if (parameter( "response", data ) != "OK")
        {
            int error_code = parameter( "error", data ).toInt();
            
            switch ( error_code )
            {
                case 1:
                    setFailed( ChangeStation_NotEnoughContent,
                        tr( "Sorry, there is not enough content to play this station. Please choose a different one." ) );
                    break;
                    
                case 2:
                    setFailed( ChangeStation_TooFewGroupMembers,
                        tr( "This group does not have enough members for radio." ) );
                    break;

                case 3:
                    setFailed( ChangeStation_TooFewFans,
                        tr( "This artist does not have enough fans for radio." ) );
                    break;                    

                case 4:
                    setFailed( ChangeStation_Unavailable,
                        tr( "This item is not available for streaming." ) );
                    break;

                case 5:
                    setFailed( ChangeStation_SubscribersOnly,
                               tr( "This station is available to subscribers only."
                                   "<p>" "You can subscribe here: <a href='http://www.last.fm/subscribe/'>http://www.last.fm/subscribe/</a>" ) );
                    break;

                case 6:
                    setFailed( ChangeStation_TooFewNeighbours,
                        tr( "There are not enough neighbours for this radio mode." ) );
                    break;

                case 7:
                    setFailed( ChangeStation_StreamerOffline,
                        tr( "The streaming system is offline for maintenance, please try again later." ) );
                    break;

                case 8:
                    setFailed( ChangeStation_InvalidSession,
                        tr( "The streaming system is offline for maintenance, please try again later." ) );
                    break;

                default:
                    setFailed( ChangeStation_UnknownError,
                        tr( "Starting radio failed. Unknown error." ) );
                    break;
            }

            LOGL( 1, "Change station failed: " << errorMessage() );
        }
        else
        {
            QString url = parameter( "url", data );
            m_stationUrl = StationUrl( url );
            m_stationName = parameter( "stationname", data );
            m_stationName = m_stationName.trimmed();
            m_discoverable = parameter( "discovery", data ) == "true";
            if ( !m_stationName.isEmpty() )
                m_stationName[0] = m_stationName[0].toUpper();
        }
    }
}
