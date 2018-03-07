/*
    This file is part of KDE.

    Copyright (c) 2009 Eckhart Wörner <ewoerner@kde.org>

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

#include "eventjob.h"

#include <QTimer>

#include <KIO/Job>

#include "eventparser.h"


using namespace AmarokAttica;


EventJob::EventJob()
    : m_job(0)
{
}


void EventJob::setUrl(const QUrl &url)
{
    m_url = url;
}


void EventJob::start()
{
    QTimer::singleShot(0, this, &EventJob::doWork);
}


Event EventJob::event() const
{
    return m_event;
}


void EventJob::doWork()
{
    auto job = KIO::get( m_url, KIO::NoReload, KIO::HideProgressInfo );
    connect( job, &KIO::TransferJob::result,
             this, &EventJob::slotJobResult );
    connect( job, &KIO::TransferJob::data,
             this, &EventJob::slotJobData );

    m_job = job;
}


void EventJob::slotJobResult(KJob* job)
{
    m_job = 0;

    if (job->error()) {
        setError(job->error());
        setErrorText(job->errorText());
    
        emitResult();
    } else {
        m_event = EventParser().parse(QString::fromUtf8(m_data.data()));

        emitResult();
    }
}


void EventJob::slotJobData(KIO::Job* job, const QByteArray& data)
{
    Q_UNUSED(job);

    m_data.append(data);
}


