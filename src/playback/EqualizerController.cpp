/****************************************************************************************
 * Copyright (c) 2004 Frederik Holljen <fh@ez.no>                                       *
 * Copyright (c) 2004,2005 Max Howell <max.howell@methylblue.com>                       *
 * Copyright (c) 2004-2013 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2006,2008 Ian Monroe <ian@monroe.nu>                                   *
 * Copyright (c) 2008 Jason A. Donenfeld <Jason@zx2c4.com>                              *
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Artur Szymiec <artur.szymiec@gmail.com>                           *
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

#define DEBUG_PREFIX "EqualizerController"

#include "playback/EqualizerController.h"

#include "amarokconfig.h"
#include "core/support/Debug.h"
#include "equalizer/EqualizerPresets.h"

#include <KLocalizedString>
#include <QRegularExpression>

#include <phonon/BackendCapabilities>
#include <phonon/EffectParameter>

EqualizerController::EqualizerController( QObject *object )
: QObject( object )
{}

EqualizerController::~EqualizerController()
{}

//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

void
EqualizerController::initialize( const Phonon::Path &path )
{
    DEBUG_BLOCK
    m_path = path;
    delete m_equalizer.data();

    using namespace Phonon;

    // Add an equalizer effect if available
    const QList<EffectDescription> effects = BackendCapabilities::availableAudioEffects();
    QRegularExpression equalizerRegExp( QStringLiteral( "equalizer.*%1.*bands" ).arg( s_equalizerBandsNum ),
                             QRegularExpression::CaseInsensitiveOption );
    for( auto const &description : effects )
    {
        if( !description.name().contains( equalizerRegExp ) )
            continue;

        QScopedPointer<Effect> equalizer( new Effect( description, this ) );
        int parameterCount = equalizer->parameters().count();
        if( parameterCount == s_equalizerBandsNum || parameterCount == s_equalizerBandsNum + 1 )
        {
            debug() << "Established Phonon equalizer effect with" << parameterCount
                    << "parameters.";
            m_equalizer = equalizer.take(); // accept the effect
            eqUpdate();
            break;
        }
        else
        {
            QStringList paramNames;
            for( const EffectParameter &param : equalizer->parameters() )
                paramNames << param.name();
            warning() << "Phonon equalizer effect" << description.name() << "with description"
                      << description.description() << "has" << parameterCount << "parameters ("
                      << paramNames << ") - which is unexpected. Trying other effects.";
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC
//////////////////////////////////////////////////////////////////////////////////////////

bool
EqualizerController::isEqSupported() const
{
    // If effect was created it means we have equalizer support
    return m_equalizer;
}

double
EqualizerController::eqMaxGain() const
{
   if( !m_equalizer )
       return 100;
   QList<Phonon::EffectParameter> equalizerParameters = m_equalizer->parameters();
   if( equalizerParameters.isEmpty() )
       return 100.0;
   double mScale;
   mScale = ( qAbs(equalizerParameters.at(0).maximumValue().toDouble() )
              + qAbs( equalizerParameters.at(0).minimumValue().toDouble() ) );
   mScale /= 2.0;
   return mScale;
}

QStringList
EqualizerController::eqBandsFreq() const
{
    // This will extract the bands frequency values from effect parameter name
    // as long as they follow the rules:
    // eq-preamp parameter will contain 'pre-amp' string
    // bands parameters are described using schema 'xxxHz'
    QStringList bandFrequencies;
    if( !m_equalizer )
        return bandFrequencies;
    QList<Phonon::EffectParameter> equalizerParameters = m_equalizer->parameters();
    if( equalizerParameters.isEmpty() )
        return bandFrequencies;
    QRegularExpression rx( QStringLiteral("\\d+(?=Hz)") );
    for( const Phonon::EffectParameter &mParam : equalizerParameters )
    {
        if( mParam.name().contains( rx ) )
        {
            QRegularExpressionMatch rmatch = rx.match( mParam.name() );
            if( rmatch.captured( 0 ).toInt() < 1000 )
                bandFrequencies << i18n( "%0\nHz" ).arg( rmatch.captured( 0 ) );
            else
                bandFrequencies << i18n( "%0\nkHz" ).arg( QString::number( rmatch.captured( 0 ).toInt()/1000 ) );
        }
        else
            bandFrequencies << mParam.name();
    }
    return bandFrequencies;
}

void
EqualizerController::eqUpdate()
{
    DEBUG_BLOCK
    // if equalizer not present simply return
    if( !m_equalizer )
        return;
    // check if equalizer should be disabled ??
    QList<int> equalizerParametersCfg;
    if( AmarokConfig::equalizerMode() <= 0 )
    {
        // Remove effect from path
        if( m_path.effects().indexOf( m_equalizer.data() ) != -1 )
            m_path.removeEffect( m_equalizer.data() );
    }
    else
    {
        // Set equalizer parameter according to the gains from settings
        QList<Phonon::EffectParameter> equalizerParameters = m_equalizer->parameters();
        equalizerParametersCfg = AmarokConfig::equalizerGains();

        QListIterator<int> equalizerParametersIt( equalizerParametersCfg );
        double scaledVal; // Scaled value to set from universal -100 - 100 range to plugin scale
        // Checking if preamp is present in equalizer parameters
        if( equalizerParameters.size() == s_equalizerBandsNum )
        {
            // If pre-amp is not present then skip the first element of equalizer gain
            if( equalizerParametersIt.hasNext() )
                equalizerParametersIt.next();
        }
        for( const Phonon::EffectParameter &mParam : equalizerParameters )
        {
            scaledVal = equalizerParametersIt.hasNext() ? equalizerParametersIt.next() : 0;
            scaledVal *= qAbs(mParam.maximumValue().toDouble() )
                         + qAbs( mParam.minimumValue().toDouble() );
            scaledVal /= 200.0;
            m_equalizer->setParameterValue( mParam, scaledVal );
        }
        // Insert effect into path if needed
        if( m_path.effects().indexOf( m_equalizer.data() ) == -1 )
        {
            if( !m_path.effects().isEmpty() )
            {
                m_path.insertEffect( m_equalizer.data(), m_path.effects().first() );
            }
            else
            {
                m_path.insertEffect( m_equalizer.data() );
            }
        }
    }
    Q_EMIT gainsChanged( equalizerParametersCfg );
}

QString
EqualizerController::equalizerPreset() const
{
    int index = AmarokConfig::equalizerMode() - 1;
    if( index > 0 )
        return EqualizerPresets::eqGlobalList()[index];
    else
        return QString();
}

void
EqualizerController::applyEqualizerPresetByIndex( int index )
{
    if( index > -1 )
    {
        AmarokConfig::setEqualizerMode( index + 1 );
        AmarokConfig::setEqualizerGains( EqualizerPresets::eqCfgGetPresetVal( EqualizerPresets::eqGlobalTranslatedList().value( index ) ) );
    }
    else
        AmarokConfig::setEqualizerMode( 0 );

    eqUpdate();
    Q_EMIT presetApplied( index );
}

void
EqualizerController::applyEqualizerPresetByName( const QString &name )
{
    DEBUG_BLOCK
    const int index = EqualizerPresets::eqGlobalTranslatedList().indexOf( name );
    applyEqualizerPresetByIndex( index > 0 ? index : 0 );
}

void
EqualizerController::savePreset( const QString &name, const QList<int> &gains )
{
    EqualizerPresets::eqCfgSetPresetVal( name, gains );
    Q_EMIT presetsChanged( name );
}

bool
EqualizerController::deletePreset( const QString &name )
{
    if( !EqualizerPresets::eqCfgDeletePreset( name ) )
        return false;

    Q_EMIT presetsChanged( name );
    return true;
}

void
EqualizerController::setGains( const QList<int> &gains )
{
    AmarokConfig::setEqualizerGains( gains );
    eqUpdate();
}

QList<int>
EqualizerController::gains() const
{
    return AmarokConfig::equalizerGains();
}

bool
EqualizerController::enabled()
{
    return AmarokConfig::equalizerMode() > 0;
}
