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

#include "personjob.h"

#include "personparser.h"

#include <QDebug>
#include <QTimer>

#include <kio/job.h>
#include <klocale.h>


using namespace AmarokAttica;

PersonJob::PersonJob()
  : m_job( )
{
}

void PersonJob::setUrl( const QUrl &url )
{
  m_url = url;
}

void PersonJob::start()
{
    QTimer::singleShot( 0, this, &PersonJob::doWork );
}

Person PersonJob::person() const
{
  return m_person;
}

void PersonJob::doWork()
{
  qDebug() << m_url;

  auto job = KIO::get( m_url, KIO::NoReload, KIO::HideProgressInfo );
  connect( job, &KIO::TransferJob::result,
           this, &PersonJob::slotUserJobResult );
  connect( job, &KIO::TransferJob::data,
           this, &PersonJob::slotUserJobData );

  m_job = job;
}

void PersonJob::slotUserJobResult( KJob *job )
{
  m_job = 0;

  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
  
    emitResult();
  } else {
    // qDebug() << m_userData;
    m_person = PersonParser().parse( m_userData );
  
    if (!m_person.avatarUrl().isEmpty()) {
      qDebug() << "Getting avatar from" << m_person.avatarUrl();

      auto job = KIO::get( m_url, KIO::NoReload, KIO::HideProgressInfo );
      connect( job, &KIO::TransferJob::result,
               this, &PersonJob::slotAvatarJobResult );
      connect( job, &KIO::TransferJob::data,
               this, &PersonJob::slotAvatarJobData );

      m_job = job;
    } else {
      emitResult();
    }
  }
}

void PersonJob::slotUserJobData( KIO::Job *, const QByteArray &data )
{
  m_userData.append( QString::fromUtf8( data.data(), data.size() + 1 ) );
}

void PersonJob::slotAvatarJobResult( KJob *job )
{
  m_job = 0;

  if ( job->error() ) {
    qWarning() << "Error retrieving Avatar:" << job->errorText();
  } else {
    QPixmap pic;
    if ( pic.loadFromData( m_avatarData ) ) {
      m_person.setAvatar( pic );
    }
  }
  
  emitResult();
}

void PersonJob::slotAvatarJobData( KIO::Job *, const QByteArray &data )
{
  m_avatarData.append( data );
}
