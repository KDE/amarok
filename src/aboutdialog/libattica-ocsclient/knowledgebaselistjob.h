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
#ifndef ATTICA_KNOWLEDGEBASELISTJOB_H
#define ATTICA_KNOWLEDGEBASELISTJOB_H

#include "knowledgebase.h"

#include <kurl.h>
#include <kjob.h>

namespace KIO {
class Job;
}

namespace AmarokAttica {

class ATTICA_EXPORT KnowledgeBaseListJob : public KJob
{
    Q_OBJECT
  public:
    KnowledgeBaseListJob();

    void setUrl( const KUrl & );

    void start();

    KnowledgeBase::List knowledgeBaseList() const;
    KnowledgeBase::Metadata metadata() const;

  protected slots:
    void doWork();

    void slotJobResult( KJob *job );
    void slotJobData( KIO::Job *job, const QByteArray &data );

  private:
    KUrl m_url;
    KIO::Job *m_job;
    QByteArray m_data;

    KnowledgeBase::List m_knowledgeBaseList;
    KnowledgeBase::Metadata m_metadata;
};

}

#endif
