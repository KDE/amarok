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

#include "AmarokEqualizerScript.h"

#include "amarokconfig.h"
#include "EngineController.h"
#include "equalizer/EqualizerPresets.h"

#include <QScriptEngine>

using namespace AmarokScript;

AmarokEqualizerScript::AmarokEqualizerScript( QScriptEngine *scriptEngine )
: QObject( scriptEngine )
{
    QScriptValue scriptObject = scriptEngine->newQObject( this, QScriptEngine::AutoOwnership
                                                        , QScriptEngine::ExcludeSuperClassContents );
    scriptEngine->globalObject().property( QStringLiteral("Amarok") ).property( QStringLiteral("Engine") ).setProperty( QStringLiteral("Equalizer"), scriptObject );

    EqualizerController *equalizer  = The::engineController()->equalizerController();
    connect( equalizer, &EqualizerController::gainsChanged, this, &AmarokEqualizerScript::gainsChanged );
    connect( equalizer, &EqualizerController::presetsChanged, this, &AmarokEqualizerScript::presetsChanged );
    connect( equalizer, &EqualizerController::presetApplied, this, &AmarokEqualizerScript::equalizerPresetApplied );
}

// script invokable
bool
AmarokEqualizerScript::deletePreset( const QString &presetName )
{
    return The::engineController()->equalizerController()->deletePreset( presetName );
}

void
AmarokEqualizerScript::savePreset( const QString &name, const QList<int> &presetGains )
{
    The::engineController()->equalizerController()->savePreset( name, presetGains );
}


//private

bool
AmarokEqualizerScript::enabled()
{
    return The::engineController()->equalizerController()->enabled();
}

QStringList
AmarokEqualizerScript::bandFrequencies() const
{
    return The::engineController()->equalizerController()->eqBandsFreq();
}

QStringList
AmarokEqualizerScript::defaultPresetList() const
{
    return EqualizerPresets::eqDefaultPresetsList();
}

void
AmarokEqualizerScript::equalizerPresetApplied( int index )
{
    emit presetApplied( EqualizerPresets::eqGlobalList().value( index ) );
}

QList<int>
AmarokEqualizerScript::gains() const
{
    return The::engineController()->equalizerController()->gains();
}

QList<int>
AmarokEqualizerScript::getPresetGains( const QString &presetName )
{
    return EqualizerPresets::eqCfgGetPresetVal( presetName );
}

QStringList
AmarokEqualizerScript::globalPresetList() const
{
    return EqualizerPresets::eqGlobalList();
}

bool
AmarokEqualizerScript::isSupported() const
{
    return The::engineController()->equalizerController()->isEqSupported();
}

int
AmarokEqualizerScript::maxGain() const
{
    return The::engineController()->equalizerController()->eqMaxGain();
}

QString
AmarokEqualizerScript::selectedPreset() const
{
    return The::engineController()->equalizerController()->equalizerPreset();
}

void
AmarokEqualizerScript::setEqualizerPreset( const QString &name ) const
{
    The::engineController()->equalizerController()->applyEqualizerPresetByName( name );
}

QStringList
AmarokEqualizerScript::translatedDefaultPresetList() const
{
    return EqualizerPresets::eqDefaultTranslatedPresetsList();
}

QStringList
AmarokEqualizerScript::translatedGlobalPresetList() const
{
    return EqualizerPresets::eqGlobalTranslatedList();
}

QStringList
AmarokEqualizerScript::userPresets() const
{
    return EqualizerPresets::eqUserList();
}

void
AmarokEqualizerScript::setGains( QList<int> gains )
{
    The::engineController()->equalizerController()->setGains( gains );
}
