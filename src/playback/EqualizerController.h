/****************************************************************************************
 * Copyright (c) 2004 Frederik Holljen <fh@ez.no>                                       *
 * Copyright (c) 2004,2005 Max Howell <max.howell@methylblue.com>                       *
 * Copyright (c) 2004-2013 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2008 Jason A. Donenfeld <Jason@zx2c4.com>                              *
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

#ifndef AMAROK_EQUALIZERCONTROLLER_H
#define AMAROK_EQUALIZERCONTROLLER_H

#include "amarok_export.h"

#include <QPointer>

#include <phonon/Path>
#include <phonon/Effect>

static const int s_equalizerBandsNum = 10; // Number of equalizer parameters excluding Preamp

class AMAROK_EXPORT EqualizerController : public QObject
{
    Q_OBJECT

public:

    EqualizerController( QObject *object );
    ~EqualizerController();

    void initialize( const Phonon::Path &path );

    /**
     * Phonon equalizer support is required for Amarok to enable equalizer
     * this method return whatever phonon support equalizer effect.
     *
     * @return @c true if the phonon support equalizer effect, @c false otherwise
     */
    bool isEqSupported() const;

    /**
     * Equalizer implementation for different backends may have different
     * gain scale. To properly display it we need to get a scale from effect
     *
     * @return maximum gain value for equalizer parameters.
     */
    double eqMaxGain() const;

    /**
     * Equalizer implementation for different backends may have different
     * frequency bands. For proper display this will try to extract frequency values
     * from effect parameters info.
     *
     * @return QStringList with band labels (form xxx Hz or xxx kHz).
     */
    QStringList eqBandsFreq() const;

    /**
     * @return the name of the equalizer preset being currently used.
     */
    QString equalizerPreset() const;

    /**
     * Changes equaliser preset to preset @param name if it exists.
     */
    void applyEqualizerPresetByName( const QString &name );

    QList<int> gains() const;
    void setGains( const QList<int> &gains );
    void savePreset( const QString &name, const QList<int> &gains );
    bool deletePreset( const QString &name );
    bool enabled();

public Q_SLOTS:

    /**
     * Update equalizer status - enabled,disabled,set values
     */
    void eqUpdate();

    /**
     * Change equalizer to preset with index @param index in the global equalizer list.
     * Pass -1 to disable.
     */
    void applyEqualizerPresetByIndex( int index );

Q_SIGNALS:

    /**
     * Emitted when preset with index @param idnex is applied or the equalizer is disabled.
     * index is <0 when disabled.
     */
    void presetApplied( int index );

    /**
     * Emitted when the current gains are changed.
     */
    void gainsChanged( QList<int> gains );

    /**
     * Emitted when preset @param name is added, removed or modified.
     */
    void presetsChanged( QString name );

private:
    QPointer<Phonon::Effect>            m_equalizer;
    Phonon::Path                            m_path;
};

#endif
