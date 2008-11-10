/*
 *   Copyright 2007 by Aaron Seigo <aseigo@kde.org>
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

#include "plasma/scripting/runnerscript.h"

#include "plasma/abstractrunner.h"
#include "plasma/package.h"

namespace Plasma
{

class RunnerScriptPrivate
{
public:
    AbstractRunner *runner;
};

RunnerScript::RunnerScript(QObject *parent)
    : ScriptEngine(parent),
      d(new RunnerScriptPrivate)
{
}

RunnerScript::~RunnerScript()
{
//    delete d;
}

void RunnerScript::setRunner(AbstractRunner *runner)
{
    d->runner = runner;
}

AbstractRunner *RunnerScript::runner() const
{
    return d->runner;
}

void RunnerScript::match(Plasma::RunnerContext &search)
{
    Q_UNUSED(search);
}

void RunnerScript::run(const Plasma::RunnerContext &search, const Plasma::QueryMatch &action)
{
    Q_UNUSED(search);
    Q_UNUSED(action);
}

const Package *RunnerScript::package() const
{
    return d->runner ? d->runner->package() : 0;
}

QString RunnerScript::mainScript() const
{
    if (!package()) {
        return QString();
    } else {
        return package()->filePath("mainscript");
    }
}

} // Plasma namespace

#include "runnerscript.moc"
