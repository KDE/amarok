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

#ifndef PLASMA_RUNNERSCRIPT_H
#define PLASMA_RUNNERSCRIPT_H

#include <KDE/KGenericFactory>

#include <plasma/plasma_export.h>
#include <plasma/scripting/scriptengine.h>

namespace Plasma
{

class AbstractRunner;
class RunnerContext;
class QueryMatch;
class RunnerScriptPrivate;

/**
 * @class RunnerScript plasma/scripting/runnerscript.h <Plasma/Scripting/RunnerScript>
 *
 * @short Provides a restricted interface for scripting a runner.
 */
class PLASMA_EXPORT RunnerScript : public ScriptEngine
{
    Q_OBJECT

public:
    /**
     * Default constructor for a RunnerScript.
     * Subclasses should not attempt to access the Plasma::AbstractRunner
     * associated with this RunnerScript in the constructor. All
     * such set up that requires the AbstractRunner itself should be done
     * in the init() method.
     */
    explicit RunnerScript(QObject *parent = 0);
    ~RunnerScript();

    /**
     * Sets the Plasma::AbstractRunner associated with this RunnerScript
     */
    void setRunner(AbstractRunner *runner);

    /**
     * Returns the Plasma::AbstractRunner associated with this script component
     */
    AbstractRunner *runner() const;

    /**
     * Called when the script should create QueryMatch instances through
     * RunnerContext::addInformationalMatch, RunnerContext::addExactMatch, and
     * RunnerContext::addPossibleMatch.
     */
    virtual void match(Plasma::RunnerContext &search);

    /**
     * Called whenever an exact or possible match associated with this
     * runner is triggered.
     */
    virtual void run(const Plasma::RunnerContext &search, const Plasma::QueryMatch &action);

protected:
    /**
     * @return absolute path to the main script file for this plasmoid
     */
    QString mainScript() const;

    /**
     * @return the Package associated with this plasmoid which can
     *         be used to request resources, such as images and
     *         interface files.
     */
    const Package *package() const;

private:
    RunnerScriptPrivate *const d;
};

#define K_EXPORT_PLASMA_RUNNERSCRIPTENGINE(libname, classname) \
K_PLUGIN_FACTORY(factory, registerPlugin<classname>();) \
K_EXPORT_PLUGIN(factory("plasma_runnerscriptengine_" #libname))

} //Plasma namespace

#endif
