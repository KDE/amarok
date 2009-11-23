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
#ifndef ATTICA_POSTJOB_H
#define ATTICA_POSTJOB_H

#include "atticaclient_export.h"

#include <kjob.h>
#include <kurl.h>

namespace KIO {
class Job;
}

namespace AmarokAttica {

class ATTICA_EXPORT PostJob : public KJob
{
    Q_OBJECT
  public:
    PostJob();

    void setUrl( const KUrl & );
    void setData( const QString &name, const QString &value );

    void start();

    QString status() const;
    QString statusMessage() const;
    
  protected slots:
    void doWork();

    void slotJobResult( KJob *job );
    void slotJobData( KIO::Job *, const QByteArray & );
    
  private:
    KUrl m_url;
    QMap<QString,QString> m_data;
    KIO::Job *m_job;
    QString m_responseData;
  
    QString m_status;
    QString m_statusMessage;
};

}

#endif
