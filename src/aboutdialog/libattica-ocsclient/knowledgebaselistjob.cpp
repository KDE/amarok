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

#include "knowledgebaselistjob.h"

#include "knowledgebaseparser.h"

#include <KIO/Job>

#include <QDebug>
#include <QTimer>

using namespace AmarokAttica;

KnowledgeBaseListJob::KnowledgeBaseListJob()
  : m_job( )
{
}

void KnowledgeBaseListJob::setUrl( const QUrl &url )
{
  m_url = url;
}

void KnowledgeBaseListJob::start()
{
    QTimer::singleShot( 0, this, &KnowledgeBaseListJob::doWork );
}

KnowledgeBase::List KnowledgeBaseListJob::knowledgeBaseList() const
{
  return m_knowledgeBaseList;
}

KnowledgeBase::Metadata KnowledgeBaseListJob::metadata() const
{
  return m_metadata;
}

void KnowledgeBaseListJob::doWork()
{
  qDebug() << m_url;

  auto job = KIO::get( m_url, KIO::NoReload, KIO::HideProgressInfo );
  connect( job, &KIO::TransferJob::result,
           this, &KnowledgeBaseListJob::slotJobResult );
  connect( job, &KIO::TransferJob::data,
           this, &KnowledgeBaseListJob::slotJobData );

  m_job = job;
}

void KnowledgeBaseListJob::slotJobResult( KJob *job )
{
  m_job = 0;

  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
  
    emitResult();
  } else {
    qDebug() << m_data;
    KnowledgeBaseParser parser;
    m_knowledgeBaseList = parser.parseList(
      QString::fromUtf8( m_data.data() ) );
    m_metadata = parser.lastMetadata();

    emitResult();
  }
}

void KnowledgeBaseListJob::slotJobData( KIO::Job *, const QByteArray &data )
{
  m_data.append( data );
}
