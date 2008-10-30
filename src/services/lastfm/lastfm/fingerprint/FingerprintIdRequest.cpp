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

#include "FingerprintIdRequest.h"
#include "FingerprintGenerator.h"
#include "Collection.h"

#include <QApplication>


FingerprintIdRequest::FingerprintIdRequest( const Track& track, QObject* parent ) 
            :QObject( parent ),
             m_track( track )
{
    m_networkManager = new QNetworkAccessManager( this );

    connect( this, SIGNAL( cachedFpIDFound( QString)), 
                   SIGNAL( FpIDFound( QString)), Qt::QueuedConnection );
    
    
    QString fpId = Collection::instance().getFingerprint( track.url().toLocalFile() );
    
    if ( !fpId.isEmpty() )
    {
        qDebug() << "Fingerprint found in cache for" << track;
        emit cachedFpIDFound( fpId );
        return;
    }
    
    fingerprint();
}


FingerprintIdRequest::~FingerprintIdRequest()
{}


void
FingerprintIdRequest::fingerprint()
{
    qDebug() << "Beginning..";
    FingerprintGenerator* fingerprinter = new FingerprintGenerator( m_track.url().toLocalFile(), FingerprintGenerator::Query, this );

    connect( fingerprinter, SIGNAL( success( QByteArray)), 
                            SLOT( onFingerprintSuccess( QByteArray)) );
}


void
FingerprintIdRequest::onFingerprintSuccess( const QByteArray& fp )
{
    FingerprintGenerator* fingerprinter = static_cast< FingerprintGenerator* >( sender());
    
    time_t now;
    time( &now );
    QString time = QString::number( now );
    
    QUrl queryUrl( "http://www.last.fm/fingerprint/query/" );
    

	//Parameters understood by the server according to the MIR team: 
	//{ "trackid", "recordingid", "artist", "album", "track", "duration", 
	//  "tracknum", "username", "sha256", "ip", "fpversion", "mbid", 
	//  "filename", "genre", "year", "samplerate", "noupdate", "fulldump" }
	
	#define QUERYLIST QList<QPair<QString, QString> >()
    #define QUERYITEM( X, Y ) QPair<QString, QString>( #X, Y )
    #define QUERYITEMENCODED( X, Y ) QUERYITEM( X, QUrl::toPercentEncoding( Y ))
    queryUrl.setQueryItems( QUERYLIST <<
                            QUERYITEMENCODED( artist,        m_track.artist() ) <<
                            QUERYITEMENCODED( album,         m_track.album() ) <<
                            QUERYITEMENCODED( track,         m_track.title() ) <<
                            QUERYITEM(        duration,      QString::number( m_track.duration()) ) <<
                            QUERYITEM(        mbid,          m_track.mbid() ) <<
                            QUERYITEMENCODED( filename,      QFileInfo( m_track.url().toLocalFile() ).completeBaseName() ) <<
                            QUERYITEM(        tracknum,      QString::number( m_track.trackNumber() ) ) <<
                            QUERYITEM(        sha256,        fingerprinter->sha256() ) <<
                            QUERYITEM(        time,          time ) <<
							
						    QUERYITEMENCODED( fpversion,     QString::number( fingerprint::FingerprintExtractor::getVersion() ) ) <<
                            QUERYITEM(        fulldump,      "false" ) <<
                            QUERYITEM(        noupdate,      "true" ));
	//FIXME: talk to mir about submitting fplibversion
                            
    #undef QUERYITEMENCODED
    #undef QUERYITEM
    #undef QUERYLIST
    
    QNetworkRequest fingerprintQueryRequest( queryUrl );
    fingerprintQueryRequest.setHeader( QNetworkRequest::ContentTypeHeader, "multipart/form-data; boundary=----------------------------8e61d618ca16" );

    QByteArray bytes;
    bytes.append( "------------------------------8e61d618ca16\r\n" );
    bytes.append( "Content-Disposition: " );
    bytes.append( "form-data; name=\"fpdata\"" );
    bytes.append( "\r\n\r\n" );

    bytes.append( fp );
    bytes.append( "\r\n");
    bytes.append( "------------------------------8e61d618ca16--\r\n");
    
    QNetworkReply* reply = m_networkManager->post( fingerprintQueryRequest, bytes );
    connect( reply, SIGNAL( finished()), SLOT( onFingerprintQueryFetched()) );

}


void
FingerprintIdRequest::onFingerprintQueryFetched()
{
    QNetworkReply* queryReq = static_cast<QNetworkReply*>( sender() );

    if ( queryReq->error() )
    {
        qDebug() << "Network error: " << queryReq->error();
    
        // TODO: clean up these signals, they're weird
        if ( queryReq->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 400 )
        {
            emit networkError( QNetworkReply::ProtocolInvalidOperationError, 
                               queryReq->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString() );
        }
        else
            emit networkError( queryReq->error(), 
                               queryReq->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString() );

        return;
    }

    // The response data will consist of a number and a string.
    // The number is the fpid and the string is either FOUND or NEW
    // (or NOT FOUND when noupdate was used). NEW means we should
    // schedule a full fingerprint.
    //
    // In the case of an error, there will be no initial number, just
    // an error string.op

    QString response( queryReq->readAll() );
    QStringList list = response.split( " " );
    
    if( list.isEmpty() )
    {
        //TODO: Emit failure        
    }
    
    QString fpid = list.at( 0 );
    bool isANumber;
    fpid.toUInt( &isANumber );
    if ( !isANumber )
    {
        //TODO: Emit failure
        return;
    }

    QString status = list.at( 1 );

    Collection::instance().setFingerprint( m_track.url().toLocalFile(), fpid );
    
    if( status == "NEW" )
    {
        emit unknownFingerprint( fpid );
    }
    else
    {
        emit FpIDFound( fpid );
    }
}
