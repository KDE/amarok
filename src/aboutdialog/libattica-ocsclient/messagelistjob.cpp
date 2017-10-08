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

#include "messagelistjob.h"

#include "messageparser.h"

#include <QDebug>
#include <QTimer>

#include <kio/job.h>
#include <klocale.h>


using namespace AmarokAttica;

MessageListJob::MessageListJob()
  : m_job( )
{
}

void MessageListJob::setUrl( const QUrl &url )
{
  m_url = url;
}

void MessageListJob::start()
{
    QTimer::singleShot( 0, this, &MessageListJob::doWork );
}

Message::List MessageListJob::messageList() const
{
  return m_messageList;
}

void MessageListJob::doWork()
{
  qDebug() << m_url;

  auto job = KIO::get( m_url, KIO::NoReload, KIO::HideProgressInfo );
  connect( job, &KIO::TransferJob::result,
           this, &MessageListJob::slotJobResult );
  connect( job, &KIO::TransferJob::data,
           this, &MessageListJob::slotJobData );

  m_job = job;
}

void MessageListJob::slotJobResult( KJob *job )
{
  m_job = 0;

  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
  
    emitResult();
  } else {
    qDebug() << m_data;
    m_messageList = MessageParser().parseList(
      QString::fromUtf8( m_data.data() ) );

    emitResult();
  }
}

void MessageListJob::slotJobData( KIO::Job *, const QByteArray &data )
{
  m_data.append( data );
}
