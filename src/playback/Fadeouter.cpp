/****************************************************************************************
 * Copyright (c) 2013 Matěj Laitl <matej@laitl.cz>                                      *
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
 * **************************************************************************************/

#include "Fadeouter.h"

#include <Phonon/MediaObject>
#include <Phonon/VolumeFaderEffect>

#include <QTimer>

static const int safetyDelay = 300; // in ms

Fadeouter::Fadeouter( const QWeakPointer<Phonon::MediaObject> &media,
                      const QWeakPointer<Phonon::VolumeFaderEffect> &fader,
                      int fadeOutLength )
    : QObject( fader.data() )
    , m_fader( fader )
{
    Q_ASSERT( media );
    Q_ASSERT( fader );
    Q_ASSERT( fadeOutLength > 0 );

    m_fader.data()->fadeOut( fadeOutLength );
    // add a bit of a second so that the effect is not cut even if there are some delays
    QTimer::singleShot( fadeOutLength + safetyDelay, this, SLOT(slotFinalizeFadeout()) );

    // in case a new track starts playing before the fadeout ends, we skip
    // slotFinalizeFadeout() and go directly to destructor, which resets fader volume
    connect( media.data(), SIGNAL(currentSourceChanged(Phonon::MediaSource)), SLOT(deleteLater()) );

    // no point in having dangling Fadeouters
    connect( media.data(), SIGNAL(destroyed(QObject*)), SLOT(deleteLater()) );
}

Fadeouter::~Fadeouter()
{
    if( m_fader )
        // warning: phonon-gstreamer bug 313551 (still present in 4.6.2) prevents
        // following call to succeed if a fade is still in progress
        m_fader.data()->fadeIn( safetyDelay ); // fade-in, just in case, be nice to ears
}

void
Fadeouter::slotFinalizeFadeout()
{
    emit fadeoutFinished();
    deleteLater();
}
