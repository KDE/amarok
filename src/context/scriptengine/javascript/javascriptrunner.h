// -*- c++ -*-

/*
 *   Copyright 2008 Richard J. Moore <rich@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
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

#ifndef RUNNERSCRIPTQSCRIPT_H
#define RUNNERSCRIPTQSCRIPT_H

#include <QScriptValue>

#include <Plasma/RunnerScript>

class QScriptEngine;

class JavaScriptRunner : public Plasma::RunnerScript
{
    Q_OBJECT

public:
    JavaScriptRunner(QObject *parent, const QVariantList &args);
    ~JavaScriptRunner();

    bool init();

    /** Reimplemented to add Q_INVOKABLE. */
    Q_INVOKABLE Plasma::AbstractRunner* runner() const;

    /** Reimplemented to forward to script. */
    void match(Plasma::RunnerContext *search);

    /** Reimplemented to forward to script. */
    void exec(const Plasma::RunnerContext *search, const Plasma::QueryMatch *action);

protected:
    void setupObjects();
    void importExtensions();
    void reportError();

private:
    QScriptEngine *m_engine;
    QScriptValue m_self;
};

K_EXPORT_PLASMA_RUNNERSCRIPTENGINE(qscriptrunner, JavaScriptRunner)

#endif // RUNNERSCRIPTQSCRIPT_H
