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

#ifndef PLASMA_SERVICEJOB_H
#define PLASMA_SERVICEJOB_H

#include <QtCore/QVariant>
#include <plasma/plasma_export.h>
#include <KDE/KJob>
#include <KDE/KService>

namespace Plasma
{

class ServiceJobPrivate;

/**
 * @class ServiceJob plasma/servicejob.h <Plasma/ServiceJob>
 *
 * @short This class provides jobs for use with Plasma::Service
 *
 * Unlike KJob, you can do the work in start(), since Plasma::Service already
 * delays the call to start() until the event loop is reached.
 *
 * If the job is quick enough that it is not worth reporting the progress,
 * you just need to implement start() to do the task, then call emitResult()
 * at the end of it.  If the task does not complete successfully, you should
 * set a non-zero error code with setError(int) and an error message with
 * setErrorText(QString).
 *
 * If the job is longer (involving network access, for instance), you should
 * report the progress at regular intervals.  See the KJob documentation for
 * information on how to do this.
 */
class PLASMA_EXPORT ServiceJob : public KJob
{
    Q_OBJECT

public:
    /**
     * Default constructor
     *
     * @arg destination the subject that the job is acting on
     * @arg operation   the action that the job is performing on the @p destination
     * @arg parameters  the parameters of the @p action
     * @arg parent      the parent object for this service
     */
    ServiceJob(const QString &destination, const QString &operation,
               const QMap<QString, QVariant> &parameters, QObject *parent = 0);

    /**
     * Destructor
     */
    ~ServiceJob();

    /**
     * @return the subject that the job is acting on
     */
    QString destination() const;

    /**
     * @return the operation the job is performing on the destination
     */
    QString operationName() const;

    /**
     * @return the parameters for the operation
     */
    QMap<QString, QVariant> parameters() const;

    /**
     * Returns the result of the operation
     *
     * The result will be invalid if the job has not completed yet, or
     * if the job does not have a meaningful result.
     *
     * Note that this should not be used to find out whether the operation
     * was successful.  Instead, you should check the value of error().
     *
     * @return the result of the operation
     */
    QVariant result() const;

    /**
     * Default implementation of start, which simply sets the results to false.
     * This makes it easy to create a "failure" job.
     */
    virtual void start();

protected:
    /**
     * Sets the result for an operation.
     */
    void setResult(const QVariant &result);

private:
    Q_PRIVATE_SLOT(d, void slotStart())

    ServiceJobPrivate * const d;
};

} // namespace Plasma

#endif // multiple inclusion guard

