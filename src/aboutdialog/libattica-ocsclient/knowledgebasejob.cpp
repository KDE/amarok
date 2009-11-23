/*
    This file is part of KDE.

    Copyright (c) 2008 Cornelius Schumacher <schumacher@kde.org>
    Copyright (c) 2009 Marco Martin <notmart@gmail.com>

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

#include "knowledgebasejob.h"

#include "knowledgebaseparser.h"

#include <kio/job.h>
#include <klocale.h>
#include <QDebug>
#include <QTimer>

using namespace AmarokAttica;

KnowledgeBaseJob::KnowledgeBaseJob()
  : m_job( 0 )
{
}

void KnowledgeBaseJob::setUrl( const KUrl &url )
{
  m_url = url;
}

void KnowledgeBaseJob::start()
{
  QTimer::singleShot( 0, this, SLOT( doWork() ) );
}

KnowledgeBase KnowledgeBaseJob::knowledgeBase() const
{
  return m_knowledgeBase;
}

KnowledgeBase::Metadata KnowledgeBaseJob::metadata() const
{
  return m_metadata;
}

void KnowledgeBaseJob::doWork()
{
  qDebug() << m_url;

  m_job = KIO::get( m_url, KIO::NoReload, KIO::HideProgressInfo );
  connect( m_job, SIGNAL( result( KJob * ) ),
    SLOT( slotJobResult( KJob * ) ) );
  connect( m_job, SIGNAL( data( KIO::Job *, const QByteArray & ) ),
    SLOT( slotJobData( KIO::Job *, const QByteArray & ) ) );
}

void KnowledgeBaseJob::slotJobResult( KJob *job )
{
  m_job = 0;

  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
  } else {
    qDebug() << m_data;
    KnowledgeBaseParser parser;
    m_knowledgeBase = parser.parse( QString::fromUtf8( m_data.data() ) );
    m_metadata = parser.lastMetadata();
  }

  emitResult();
}

void KnowledgeBaseJob::slotJobData( KIO::Job *, const QByteArray &data )
{
  m_data.append( data );
}

