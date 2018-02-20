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

#include <KIO/Job>

#include <QDebug>
#include <QTimer>

using namespace AmarokAttica;

KnowledgeBaseJob::KnowledgeBaseJob()
  : m_job( )
{
}

void KnowledgeBaseJob::setUrl( const QUrl &url )
{
  m_url = url;
}

void KnowledgeBaseJob::start()
{
    QTimer::singleShot( 0, this, &KnowledgeBaseJob::doWork );
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

  auto job = KIO::get( m_url, KIO::NoReload, KIO::HideProgressInfo );
  connect( job, &KIO::TransferJob::result,
           this, &KnowledgeBaseJob::slotJobResult );
  connect( job, &KIO::TransferJob::data,
           this, &KnowledgeBaseJob::slotJobData );

  m_job = job;
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

