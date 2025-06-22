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

EqualizerController::EqualizerController( QObject *object )
: QObject( object )
, m_equalizer( nullptr )
{} // initialization done during gst pipeline inititialization

EqualizerController::~EqualizerController()
{}

void
EqualizerController::setEqElement( GstElement *eqElement)
{
    m_equalizer = eqElement;
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
   double mScale;
   mScale = 12; // gstreamer-specific
   return mScale;
}

QStringList
EqualizerController::eqBandsFreq() const
{
    QStringList bandFrequencies;
    if( !m_equalizer )
        return bandFrequencies;
    // frequencies of the gstreamer equalizer-10bands effect
    QList<int> freqs;
    freqs << 29 << 59 << 119 << 237 << 474 << 947 << 1889 << 3770 << 7523 << 15011;
    for( const auto &freq: freqs )
    {
        if( freq < 1000 )
            bandFrequencies << i18nc( "label for equalizer band frequency", "%1\nHz", freq );
        else
            bandFrequencies << i18nc( "label for equalizer band frequency", "%1\nkHz", QString::number( freq/1000 ) );
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
        for(int i=0; i < 10; i++)
        {
            g_object_set( m_equalizer, QString(QStringLiteral("band") + QString::number( i )).toLocal8Bit().data(), 0, NULL);
        }
    }
    else
    {
        // Set equalizer parameter according to the gains from settings
        equalizerParametersCfg = AmarokConfig::equalizerGains();

        QListIterator<int> equalizerParametersIt( equalizerParametersCfg );
        double scaledVal; // Scaled value to set from universal -100 - 100 range to plugin scale
        for(int i=0; i < 10; i++)
        {
            scaledVal = equalizerParametersIt.hasNext() ? equalizerParametersIt.next() : 0;
            scaledVal *= 24;
            scaledVal /= 200.0;
            g_object_set( m_equalizer, QString(QStringLiteral("band") + QString::number( i )).toLocal8Bit().data(), scaledVal, NULL);
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
