/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
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

#ifndef SUBMITFINGERPRINTREQUEST_H
#define SUBMITFINGERPRINTREQUEST_H

#include "Request.h"

/** @author <petgru@last.fm> */

class UNICORN_DLLEXPORT SubmitFullFingerprintRequest : public Request
{
    PROP_GET_SET( QString, username, Username );
    PROP_GET_SET( QString, passwordMd5, PasswordMd5 );
    PROP_GET_SET( QString, passwordMd5Lower, PasswordMd5Lower );
    PROP_GET_SET( int, size, Size );
    PROP_GET_SET( QString, sha256, Sha256 );
    PROP_GET_SET( QString, fpVersion, FpVersion );

    public:
        SubmitFullFingerprintRequest();
        SubmitFullFingerprintRequest( TrackInfo, QByteArray& ); ///convenience
        
        virtual void start();
        
        TrackInfo& track() { return m_track; }

        void setData( QByteArray& data ) { m_data = data; }
        QByteArray& data() { return m_data; }
        
    private:
        QByteArray m_data;
        TrackInfo m_track;
};

#endif
