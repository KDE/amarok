/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef LASTFM_WS_ERROR_H
#define LASTFM_WS_ERROR_H

namespace Ws
{
    enum Error
    {
        NoError = 1,

        /** see http://last.fm/api/ */
        InvalidService = 2,
        InvalidMethod,
        AuthenticationFailed,
        InvalidFormat,
        InvalidParameters,
        InvalidResourceSpecified,
        OperationFailed,
        InvalidSessionKey,
        InvalidApiKey,
        ServiceOffline,
        SubscribersOnly,

		NotEnoughContent = 20,
		NotEnoughMembers,
		NotEnoughFans,
		NotEnoughNeighbours,
		
        /** Last.fm sucks, or something weird happened. 
          * Call networkError() for more details
		  * Advise the user to try again in a _few_minutes_.
		  * For some cases, you may want to try again yourself, at this point
		  * in the API you will have to. Eventually we will discourage this and
		  * do it for you, as we don't want to strain Last.fm's servers
		  */
        TryAgainLater = 100,

        /** Last.fm fucked up, or something mangled the response on its way */
        MalformedResponse,

        /** call networkError() for more details */
        UrLocalNetworkIsFuckedLol,
        UrProxyIsFuckedLol,

        /** you aborted the request, the lib never does, we promise! */
        Aborted
    };
}

#endif
