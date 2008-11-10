/*
 *   Copyright 2008 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "servicejob.h"

namespace Plasma
{

class ServiceJobPrivate
{
public:
    ServiceJobPrivate(ServiceJob *owner,
                      const QString &dest,
                      const QString &op,
                      const QMap<QString, QVariant> &params)
        : q(owner),
          destination(dest),
          operation(op),
          parameters(params)
    {
    }

    void slotStart()
    {
        q->start();
    }

    ServiceJob *q;
    QString destination;
    QString operation;
    QMap<QString, QVariant> parameters;
    QVariant result;
};

ServiceJob::ServiceJob(const QString &destination, const QString &operation,
                       const QMap<QString, QVariant> &parameters, QObject *parent)
    : KJob(parent),
      d(new ServiceJobPrivate(this, destination, operation, parameters))
{
}

ServiceJob::~ServiceJob()
{
    delete d;
}

QString ServiceJob::destination() const
{
    return d->destination;
}

QString ServiceJob::operationName() const
{
    return d->operation;
}

QMap<QString, QVariant> ServiceJob::parameters() const
{
    return d->parameters;
}

QVariant ServiceJob::result() const
{
    return d->result;
}

void ServiceJob::setResult(const QVariant &result)
{
    d->result = result;
    emitResult();
}

void ServiceJob::start()
{
    setResult(false);
}

} // namespace Plasma

#include "servicejob.moc"

