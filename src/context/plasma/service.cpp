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

#include "service.h"
#include "private/service_p.h"

#include <QFile>
#include <QTimer>

#include <KDebug>
#include <KService>
#include <KServiceTypeTrader>
#include <KSharedConfig>
#include <KStandardDirs>
#include <KTemporaryFile>

#include "configloader.h"

#include "version.h"

namespace Plasma
{

Service::Service(QObject *parent)
    : QObject(parent),
      d(new ServicePrivate(this))
{
}

Service::Service(QObject *parent, const QVariantList &args)
    : QObject(parent),
      d(new ServicePrivate(this))
{
    Q_UNUSED(args)
}

Service::~Service()
{
    delete d;
}

Service *Service::load(const QString &name, QObject *parent)
{
    //TODO: scripting API support
    if (name.isEmpty()) {
        return new NullService(QString(), parent);
    }

    QString constraint = QString("[X-KDE-PluginInfo-Name] == '%1'").arg(name);
    KService::List offers = KServiceTypeTrader::self()->query("Plasma/Service", constraint);

    if (offers.isEmpty()) {
        kDebug() << "offers is empty for " << name;
        return new NullService(name, parent);
    }

    KService::Ptr offer = offers.first();
    QString error;
    QVariantList args;
    //args << name;
    Service *service = 0;

    if (Plasma::isPluginVersionCompatible(KPluginLoader(*offer).pluginVersion())) {
        service = offer->createInstance<Plasma::Service>(parent, args, &error);
    }

    if (!service) {
        kDebug() << "Couldn't load Service \"" << name << "\"! reason given: " << error;
        return new NullService(name, parent);
    }

    if (service->name().isEmpty()) {
        service->setName(name);
    }

    return service;
}

void Service::setDestination(const QString &destination)
{
    d->destination = destination;
}

QString Service::destination() const
{
    return d->destination;
}

QStringList Service::operationNames() const
{
    if (!d->config) {
        kDebug() << "No valid operations scheme has been registered";
        return QStringList();
    }

    return d->config->groupList();
}

KConfigGroup Service::operationDescription(const QString &operationName)
{
    if (!d->config) {
        kDebug() << "No valid operations scheme has been registered";
        return KConfigGroup();
    }

    d->config->writeConfig();
    KConfigGroup params(d->config->config(), operationName);
    //kDebug() << "operation" << operationName
    //         << "requested, has keys" << params.keyList() << "from"
    //         << d->config->config()->name();
    return params;
}

ServiceJob *Service::startOperationCall(const KConfigGroup &description, QObject *parent)
{
    // TODO: nested groups?
    ServiceJob *job = 0;
    QString op = description.name();
    if (!d->config) {
        kDebug() << "No valid operations scheme has been registered";
    } else if (d->config->hasGroup(op)) {
        if (d->disabledOperations.contains(op)) {
            kDebug() << "Operation" << op << "is disabled";
        } else {
            QMap<QString, QVariant> params;
            foreach (const QString &key, description.keyList()) {
                KConfigSkeletonItem *item = d->config->findItem(op, key);
                if (item) {
                    params.insert(key, description.readEntry(key, item->property()));
                }
            }

            job = createJob(description.name(), params);
        }
    } else {
        kDebug() << "Not a valid group!";
    }

    if (!job) {
        job = new NullServiceJob(destination(), op, this);
    }

    job->setParent(parent ? parent : this);
    connect(job, SIGNAL(finished(KJob*)), this, SLOT(jobFinished(KJob*)));
    QTimer::singleShot(0, job, SLOT(slotStart()));
    return job;
}

void Service::associateWidget(QWidget *widget, const QString &operation)
{
    disassociateWidget(widget);
    d->associatedWidgets.insert(widget, operation);
    connect(widget, SIGNAL(destroyed(QObject*)), this, SLOT(associatedWidgetDestroyed(QObject*)));

    widget->setEnabled(!d->disabledOperations.contains(operation));
}

void Service::disassociateWidget(QWidget *widget)
{
    disconnect(widget, SIGNAL(destroyed(QObject*)),
               this, SLOT(associatedWidgetDestroyed(QObject*)));
    d->associatedWidgets.remove(widget);
}

void Service::associateWidget(QGraphicsWidget *widget, const QString &operation)
{
    disassociateWidget(widget);
    d->associatedGraphicsWidgets.insert(widget, operation);
    connect(widget, SIGNAL(destroyed(QObject*)),
            this, SLOT(associatedGraphicsWidgetDestroyed(QObject*)));

    widget->setEnabled(!d->disabledOperations.contains(operation));
}

void Service::disassociateWidget(QGraphicsWidget *widget)
{
    disconnect(widget, SIGNAL(destroyed(QObject*)),
               this, SLOT(associatedGraphicsWidgetDestroyed(QObject*)));
    d->associatedGraphicsWidgets.remove(widget);
}

QString Service::name() const
{
    return d->name;
}

void Service::setName(const QString &name)
{
    d->name = name;

    // now reset the config, which may be based on our name
    delete d->config;
    d->config = 0;

    delete d->tempFile;
    d->tempFile = 0;

    registerOperationsScheme();
}

void Service::setOperationEnabled(const QString &operation, bool enable)
{
    if (!d->config || !d->config->hasGroup(operation)) {
        return;
    }

    if (enable) {
        d->disabledOperations.remove(operation);
    } else if (!d->disabledOperations.contains(operation)) {
        d->disabledOperations.insert(operation);
    }

    {
        QHashIterator<QWidget *, QString> it(d->associatedWidgets);
        while (it.hasNext()) {
            it.next();
            if (it.value() == operation) {
                it.key()->setEnabled(enable);
            }
        }
    }

    {
        QHashIterator<QGraphicsWidget *, QString> it(d->associatedGraphicsWidgets);
        while (it.hasNext()) {
            it.next();
            if (it.value() == operation) {
                it.key()->setEnabled(enable);
            }
        }
    }
}

bool Service::isOperationEnabled(const QString &operation) const
{
    return d->config && d->config->hasGroup(operation) && !d->disabledOperations.contains(operation);
}

void Service::setOperationsScheme(QIODevice *xml)
{
    delete d->config;
    delete d->tempFile;

    //FIXME: make KSharedConfig and KConfigSkeleton not braindamaged in 4.2 and then get rid of the
    //       temp file object here
    d->tempFile = new KTemporaryFile;
    d->tempFile->open();

    KSharedConfigPtr c = KSharedConfig::openConfig(d->tempFile->fileName(), KConfig::NoGlobals);
    d->config = new ConfigLoader(c, xml, this);

    emit operationsChanged();

    {
        QHashIterator<QWidget *, QString> it(d->associatedWidgets);
        while (it.hasNext()) {
            it.next();
            it.key()->setEnabled(d->config->hasGroup(it.value()));
        }
    }

    {
        QHashIterator<QGraphicsWidget *, QString> it(d->associatedGraphicsWidgets);
        while (it.hasNext()) {
            it.next();
            it.key()->setEnabled(d->config->hasGroup(it.value()));
        }
    }
}

void Service::registerOperationsScheme()
{
    if (d->config) {
        // we've already done our job. let's go home.
        return;
    }

    if (d->name.isEmpty()) {
        kDebug() << "No name found";
        return;
    }

    QString path = KStandardDirs::locate("data", "plasma/services/" + d->name + ".operations");

    if (path.isEmpty()) {
        kDebug() << "Cannot find operations description";
        return;
    }

    QFile file(path);
    setOperationsScheme(&file);
}

} // namespace Plasma

#include "service.moc"

