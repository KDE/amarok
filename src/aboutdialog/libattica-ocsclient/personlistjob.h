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
#ifndef ATTICA_PERSONLISTJOB_H
#define ATTICA_PERSONLISTJOB_H

#include "person.h"

#include <kjob.h>

namespace KIO {
class Job;
}

namespace AmarokAttica {

class ATTICA_EXPORT PersonListJob : public KJob
{
    Q_OBJECT
  public:
    PersonListJob();

    void setUrl( const KUrl & );

    void start();

    Person::List personList() const;
    
  protected slots:
    void doWork();

    void slotUserJobResult( KJob *job );
    void slotUserJobData( KIO::Job *job, const QByteArray &data );
    
    void slotAvatarJobResult( KJob *job );
    void slotAvatarJobData( KIO::Job *job, const QByteArray &data );
    
  private:
    KUrl m_url;
    KIO::Job *m_job;
    QString m_userData;
    QByteArray m_avatarData;
  
    Person::List m_personList;
};

}

#endif
