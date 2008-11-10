/*
 *   Copyright 2006-2007 Aaron Seigo <aseigo@kde.org>
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

#include "runnercontext.h"

#include <QReadWriteLock>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSharedData>

#include <KCompletion>
#include <KDebug>
#include <KMimeType>
#include <KShell>
#include <KStandardDirs>
#include <KUrl>

#include "abstractrunner.h"
#include "querymatch.h"

//#define LOCK_FOR_READ(context) if (context->d->policy == Shared) { context->d->lock.lockForRead(); }
//#define LOCK_FOR_WRITE(context) if (context->d->policy == Shared) { context->d->lock.lockForWrite(); }
//#define UNLOCK(context) if (context->d->policy == Shared) { context->d->lock.unlock(); }

#define LOCK_FOR_READ(context) context->d->lock.lockForRead();
#define LOCK_FOR_WRITE(context) context->d->lock.lockForWrite();
#define UNLOCK(context) context->d->lock.unlock();

namespace Plasma
{

class RunnerContextPrivate : public QSharedData
{
    public:
        RunnerContextPrivate(RunnerContext *context)
            : QSharedData(),
              type(RunnerContext::UnknownType),
              q(context)
        {
        }

        RunnerContextPrivate(const RunnerContextPrivate &p)
            : QSharedData(),
              type(RunnerContext::None),
              q(p.q)
        {
            //kDebug() << "¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿boo yeah" << type;
        }

        ~RunnerContextPrivate()
        {
        }

        /**
         * Determines type of query
         */
        void determineType()
        {
            // NOTE! this method must NEVER be called from
            // code that may be running in multiple threads
            // with the same data.
            type = RunnerContext::UnknownType;
            QString path = QDir::cleanPath(KShell::tildeExpand(term));

            int space = term.indexOf(' ');
            if (space > 0) {
                if (!KStandardDirs::findExe(path.left(space)).isEmpty()) {
                    type = RunnerContext::ShellCommand;
                }
            } else if (!KStandardDirs::findExe(path.left(space)).isEmpty()) {
                type = RunnerContext::Executable;
            } else {
                KUrl url(term);
                if (!url.protocol().isEmpty() && !url.isLocalFile()) {
                    type = RunnerContext::NetworkLocation;
                } else if (QFile::exists(path)) {
                    QFileInfo info(path);
                    if (info.isSymLink()) {
                        path = info.canonicalFilePath();
                        info = QFileInfo(path);
                    }
                    if (info.isDir()) {
                        type = RunnerContext::Directory;
                        mimeType = "inode/folder";
                    } else if (info.isFile()) {
                        type = RunnerContext::File;
                        KMimeType::Ptr mimeTypePtr = KMimeType::findByPath(path);
                        if (mimeTypePtr) {
                            mimeType = mimeTypePtr->name();
                        }
                    }
                }
            }
        }

        QReadWriteLock lock;
        QList<QueryMatch> matches;
        QMap<QString, const QueryMatch*> matchesById;
        QString term;
        QString mimeType;
        RunnerContext::Type type;
        RunnerContext * q;
};

RunnerContext::RunnerContext(QObject *parent)
    : QObject(parent),
      d(new RunnerContextPrivate(this))
{
}

//copy ctor
RunnerContext::RunnerContext(RunnerContext &other, QObject *parent)
    : QObject(parent)
{
    LOCK_FOR_READ((&other))
    d = other.d;
    UNLOCK((&other))
}

RunnerContext::~RunnerContext()
{
}

void RunnerContext::reset()
{
    // We will detach if we are a copy of someone. But we will reset
    // if we are the 'main' context others copied from. Resetting
    // one RunnerContext makes all the copies oneobsolete.
    d.detach();

    // we still have to remove all the matches, since if the
    // ref count was 1 (e.g. only the RunnerContext is using
    // the dptr) then we won't get a copy made
    if (!d->matches.isEmpty()) {
        d->matchesById.clear();
        d->matches.clear();
        emit d->q->matchesChanged();
    }

    d->term.clear();
    d->mimeType.clear();
    d->type = UnknownType;
    //kDebug() << "match count" << d->matches.count();
}

void RunnerContext::setQuery(const QString &term)
{
    reset();

    if (term.isEmpty()) {
        return;
    }

    d->term = term;
    d->determineType();
}

QString RunnerContext::query() const
{
    // the query term should never be set after
    // a search starts. in fact, reset() ensures this
    // and setQuery(QString) calls reset()
    return d->term;
}

RunnerContext::Type RunnerContext::type() const
{
    return d->type;
}

QString RunnerContext::mimeType() const
{
    return d->mimeType;
}

bool RunnerContext::addMatches(const QString &term, const QList<QueryMatch> &matches)
{
    Q_UNUSED(term)

    if (matches.isEmpty()) {
        return false;
    }

    LOCK_FOR_WRITE(this)
    foreach (const QueryMatch &match, matches) {
        d->matches.append(match);
#ifndef NDEBUG
        if (d->matchesById.contains(match.id())) {
                kDebug() << "Duplicate match id " << match.id() << "from" << match.runner()->name();
        }
#endif
        d->matchesById.insert(match.id(), &d->matches.at(d->matches.size() - 1));
    }
    UNLOCK(this);
    //kDebug()<< "add matches";
    // A copied searchContext may share the d pointer,
    // we always want to sent the signal of the object that created
    // the d pointer
    emit d->q->matchesChanged();
    return true;
}

bool RunnerContext::addMatch(const QString &term, const QueryMatch &match)
{
    Q_UNUSED(term)

    LOCK_FOR_WRITE(this)
    d->matches.append(match);
    d->matchesById.insert(match.id(), &d->matches.at(d->matches.size() - 1));
    UNLOCK(this);
    //kDebug()<< "added match" << match->text();
    emit d->q->matchesChanged();

    return true;
}

QList<QueryMatch> RunnerContext::matches() const
{
    LOCK_FOR_READ(this)
    QList<QueryMatch> matches = d->matches;
    UNLOCK(this);
    return matches;
}

QueryMatch RunnerContext::match(const QString &id) const
{
    LOCK_FOR_READ(this)
    if (d->matchesById.contains(id)) {
        const QueryMatch *match = d->matchesById.value(id);
        UNLOCK(this)
        return *match;
    }
    UNLOCK(this)

    return QueryMatch(0);
}

} // Plasma namespace

#include "runnercontext.moc"
