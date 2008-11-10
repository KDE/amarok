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

#ifndef PLASMA_SCRIPTENGINE_H
#define PLASMA_SCRIPTENGINE_H

#include <plasma/plasma_export.h>
#include <plasma/packagestructure.h>
#include <plasma/plasma.h>

#include <QtCore/QObject>
#include <QtCore/QRect>
#include <QtCore/QSizeF>

class QPainter;
class QStyleOptionGraphicsItem;

namespace Plasma
{

class AbstractRunner;
class Applet;
class AppletScript;
class DataEngine;
class DataEngineScript;
class RunnerScript;
class Package;
class ScriptEnginePrivate;

/**
 * @class ScriptEngine plasma/scripting/scriptengine.h <Plasma/Scripting/ScriptEngine>
 *
 * @short The base class for scripting interfaces to be used in loading
 *        plasmoids of a given language.
 *
 * All ScriptEngines should export as consistent an interface as possible
 * so that the learning curve is limited. In particular, the following
 * API should be made available in the script environment:
 *
 * TODO: define the actual scripting APIas ...
 * PlasmaApplet - the applet of this plasmoid
 * LoadUserInterface(String uiFile) - loads and returns a given UI file
 * LoadImage - loads an image resource out of the plasmoid's package
 * PlasmaSvg - creates and returns an Svg file
 **/

class PLASMA_EXPORT ScriptEngine : public QObject
{
    Q_OBJECT

public:
    ~ScriptEngine();

    /**
     * Called when it is safe to initialize the internal state of the engine
     */
    virtual bool init();

protected:
    explicit ScriptEngine(QObject *parent = 0);

    /**
     * @return absolute path to the main script file for this plasmoid
     */
    virtual QString mainScript() const;

    /**
     * @return the Package associated with this plasmoid which can
     *         be used to request resources, such as images and
     *         interface files.
     */
    virtual const Package *package() const;

private:
    ScriptEnginePrivate *const d;
};

/**
 * @arg types a set of ComponentTypes flags for which to look up the
 *            language support for
 * @return a list of all supported languages for the given type(s).
 **/
PLASMA_EXPORT QStringList knownLanguages(ComponentTypes types);

/**
 * Loads an Applet script engine for the given language.
 *
 * @param language the language to load for
 * @param applet the Plasma::Applet for this script
 * @return pointer to the AppletScript or 0 on failure; the caller is responsible
 *         for the return object which will be parented to the Applet
 **/
PLASMA_EXPORT AppletScript *loadScriptEngine(const QString &language, Applet *applet);

/**
 * Loads an DataEngine script engine for the given language.
 *
 * @param language the language to load for
 * @param dataEngine the Plasma::DataEngine for this script;
 * @return pointer to the DataEngineScript or 0 on failure; the caller is responsible
 *         for the return object which will be parented to the DataEngine
 **/
PLASMA_EXPORT DataEngineScript *loadScriptEngine(const QString &language, DataEngine *dataEngine);

/**
 * Loads an Applet script engine for the given language.
 *
 * @param language the language to load for
 * @param runner the Plasma::AbstractRunner for this script
 * @return pointer to the RunnerScript or 0 on failure; the caller is responsible
 *         for the return object which will be parented to the AbstractRunner
 **/
PLASMA_EXPORT RunnerScript *loadScriptEngine(const QString &language, AbstractRunner *runner);

/**
 * Loads an appropriate PackageStructure for the given language and type
 *
 * @param langauge the language to load the PackageStructure for
 * @param type the component type
 * @return a guarded PackageStructure pointer
 */
PLASMA_EXPORT PackageStructure::Ptr packageStructure(const QString &language, ComponentType type);

} // namespace Plasma

#endif

