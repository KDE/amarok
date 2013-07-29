/****************************************************************************************
 * Copyright (c) 2013 Anmol Ahuja <darthcodus@gmail.com>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "ScriptingDefines.h"

#include "core/support/Debug.h"

using namespace AmarokScript;

AmarokScriptEngine::AmarokScriptEngine( QObject *parent )
: QScriptEngine(parent)
, internalObject( "UndocumentedAmarokScriptingInternals" )
{
    QScriptValue scriptObject = newQObject( this, QtOwnership,
                                            ExcludeChildObjects | ExcludeSuperClassContents );
    globalObject().setProperty( internalObject, scriptObject, QScriptValue::ReadOnly );
}

void
AmarokScriptEngine::setDeprecatedProperty( const QString &parent, const QString &name, const QScriptValue &property )
{
    const QString objName = QString( "%1%2" ).arg( name ).arg( qrand() );
    globalObject().property( internalObject ). setProperty( objName, property, QScriptValue::ReadOnly | QScriptValue::SkipInEnumeration );
    const QString command = "Object.defineProperty( " + parent +", \"" + name
                            + "\", {get : function(){ var iobj=" + internalObject+"; iobj.slotDeprecatedCall(\""
                            + parent + "." + name +"\"); return iobj." + objName + "; },\
                                                                        enumerable : false,\
                                                                        configurable : false});";
    evaluate( command );
}

void
AmarokScriptEngine::slotDeprecatedCall( const QString &call )
{
    warning() << "Deprecated function " + call;
    emit deprecatedCall( call );
}

AmarokScriptEngine::~AmarokScriptEngine()
{}

#include "ScriptingDefines.moc"
