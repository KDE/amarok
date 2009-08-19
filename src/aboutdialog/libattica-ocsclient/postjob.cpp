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

#include <kio/job.h>
#include <klocale.h>

#include <QXmlStreamReader>
#include <QDebug>
#include <QTimer>
using namespace Attica;

PostJob::PostJob()
  : m_job( 0 )
{
}

void PostJob::setUrl( const KUrl &url )
{
  m_url = url;
}

void PostJob::setData( const QString &name, const QString &value )
{
  m_data.insert( name, value );
}

void PostJob::start()
{
  QTimer::singleShot( 0, this, SLOT( doWork() ) );
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

  foreach( const QString &name, m_data.keys() ) {
    m_url.addQueryItem( name, m_data.value( name ) );
  }

  qDebug() << m_url;

  m_job = KIO::http_post( m_url, postData.toUtf8(), KIO::HideProgressInfo );
  connect( m_job, SIGNAL( result( KJob * ) ),
    SLOT( slotJobResult( KJob * ) ) );
  connect( m_job, SIGNAL( data( KIO::Job *, const QByteArray & ) ),
    SLOT( slotJobData( KIO::Job *, const QByteArray & ) ) );
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
