/*
    This file is part of KDE.

    Copyright (c) 2008 Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "postjob.h"

#include <KIO/Job>

#include <QXmlStreamReader>
#include <QDebug>
#include <QTimer>
#include <QUrlQuery>

using namespace AmarokAttica;

PostJob::PostJob()
  : m_job( )
{
}

void PostJob::setUrl( const QUrl &url )
{
  m_url = url;
}

void PostJob::setData( const QString &name, const QString &value )
{
  m_data.insert( name, value );
}

void PostJob::start()
{
    QTimer::singleShot( 0, this, &PostJob::doWork );
}

QString PostJob::status() const
{
  return m_status;
}

QString PostJob::statusMessage() const
{
  return m_statusMessage;
}

void PostJob::doWork()
{
  QString postData;

  const QStringList dataKeys = m_data.keys();
  QUrlQuery query;
  foreach( const QString &name, dataKeys ) {
    query.addQueryItem( name, m_data.value( name ) );
  }
  m_url.setQuery( query );

  qDebug() << m_url;

  auto job = KIO::http_post( m_url, postData.toUtf8(), KIO::HideProgressInfo );
  connect( job, &KIO::TransferJob::result,
           this, &PostJob::slotJobResult );
  connect( job, &KIO::TransferJob::data,
           this, &PostJob::slotJobData );

  m_job = job;
}

void PostJob::slotJobResult( KJob *job )
{
  m_job = 0;

  qDebug() << "RESPONSE" << m_responseData;

  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
  } else {
    qDebug() << "No error ";
    
    QXmlStreamReader xml( m_responseData );
    
    while ( !xml.atEnd() ) {
      xml.readNext();
      
      if ( xml.isStartElement() && xml.name() == "meta" ) {
        while ( !xml.atEnd() ) {
          xml.readNext();
         
          if ( xml.isStartElement() ) {
            if ( xml.name() == "status" ) {
              m_status = xml.readElementText();
            } else if ( xml.name() == "message" ) {
              m_statusMessage = xml.readElementText();
            }
          }
          
          if ( xml.isEndElement() && xml.name() == "meta" ) break;
        }
      }
    }
    
    qDebug() << "STATUS:" << m_status;
    
    if ( m_status != "ok" ) {
      setError( KJob::UserDefinedError );
      setErrorText( m_status + ": " + m_statusMessage );
    }
  }
  
  emitResult();
}

void PostJob::slotJobData( KIO::Job *, const QByteArray &data )
{
  m_responseData.append( QString::fromUtf8( data.data(), data.size() + 1 ) );
}
