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

#include "personlistjob.h"

#include "personparser.h"

#include <QDebug>
#include <QTimer>

#include <KIO/Job>
#include <KLocalizedString>


using namespace AmarokAttica;

PersonListJob::PersonListJob()
  : m_job( )
{
}

void PersonListJob::setUrl( const QUrl &url )
{
  m_url = url;
}

void PersonListJob::start()
{
    QTimer::singleShot( 0, this, &PersonListJob::doWork );
}

Person::List PersonListJob::personList() const
{
  return m_personList;
}

void PersonListJob::doWork()
{
  qDebug() << m_url;

  auto job = KIO::get( m_url, KIO::NoReload, KIO::HideProgressInfo );
  connect( job, &KIO::TransferJob::result,
           this, &PersonListJob::slotUserJobResult );
  connect( job, &KIO::TransferJob::data,
           this, &PersonListJob::slotUserJobData );

  m_job = job;
}

void PersonListJob::slotUserJobResult( KJob *job )
{
  m_job = 0;

  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
  
    emitResult();
  } else {
    // qDebug() << m_userData;
    m_personList = PersonParser().parseList( m_userData );

#if 0
    m_job = KIO::get( m_person.avatarUrl(), KIO::NoReload,
      KIO::HideProgressInfo );
    connect( m_job, &KIO::Job::result,
        this, &PersonListJob::slotAvatarJobResult );
    connect( m_job, &KIO::Job::data,
        this, &PersonListJob::slotAvatarJobData );
#else
    emitResult();
#endif
  }
}

void PersonListJob::slotUserJobData( KIO::Job *, const QByteArray &data )
{
  m_userData.append( QString::fromUtf8( data.data(), data.size() + 1 ) );
}

void PersonListJob::slotAvatarJobResult( KJob *job )
{
  m_job = 0;

  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
  } else {
    QPixmap pic;
    if ( !pic.loadFromData( m_avatarData ) ) {
      setError( UserDefinedError );
      setErrorText( i18n("Unable to parse avatar image data.") );
    } else {
//      m_person.setAvatar( pic );
    }
  }
  
  emitResult();
}

void PersonListJob::slotAvatarJobData( KIO::Job *, const QByteArray &data )
{
  m_avatarData.append( data );
}
