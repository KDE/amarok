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

#include "categorylistjob.h"

#include "categoryparser.h"

#include <kio/job.h>
#include <klocale.h>


using namespace Attica;

CategoryListJob::CategoryListJob()
  : m_job( 0 )
{
}

void CategoryListJob::setUrl( const KUrl &url )
{
  m_url = url;
}

void CategoryListJob::start()
{
  QTimer::singleShot( 0, this, SLOT( doWork() ) );
}

Category::List CategoryListJob::categoryList() const
{
  return m_categoryList;
}

void CategoryListJob::doWork()
{
  qDebug() << m_url;

  m_job = KIO::get( m_url, KIO::NoReload, KIO::HideProgressInfo );
  connect( m_job, SIGNAL( result( KJob * ) ),
    SLOT( slotJobResult( KJob * ) ) );
  connect( m_job, SIGNAL( data( KIO::Job *, const QByteArray & ) ),
    SLOT( slotJobData( KIO::Job *, const QByteArray & ) ) );
}

void CategoryListJob::slotJobResult( KJob *job )
{
  m_job = 0;

  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
  
    emitResult();
  } else {
    qDebug() << m_data;
    m_categoryList = CategoryParser().parseList(
      QString::fromUtf8( m_data.data() ) );

    emitResult();
  }
}

void CategoryListJob::slotJobData( KIO::Job *, const QByteArray &data )
{
  m_data.append( data );
}
