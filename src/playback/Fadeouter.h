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

#ifndef FADEOUTER_H
#define FADEOUTER_H

#include <amarok_export.h>

#include <QObject>
#include <QPointer>

/**
 * A RAII approach to fadeout on track stop. Once created, it automatically
 * initiates fadeout. Fadeouter ensures that fadeoutFinished() is signalled when
 * fadeout is done. It also handles well situations where different track is played
 * while fadeout is still active.
 *
 * In each case, Fadeouter auto-deletes itself.
 */
class AMAROK_EXPORT Fadeouter : public QObject
{
    Q_OBJECT

    public:
        Fadeouter( int fadeOutLength );

        /**
         * Destructor ensures that fader volume is set back to normal
         */
        ~Fadeouter() override;

    Q_SIGNALS:
        /**
         * This signal is emitted when the fade-out is done. This signal may not
         * be emitted at all when Fadeouter is interrupted by new track playing.
         * Yuu should connect your track-finished-playing logic to this.
         */
        void fadeoutFinished();

    private Q_SLOTS:
        /**
         * Emits fadeoutFinished() and commits Fadeouter suicide.
         */
        void slotFinalizeFadeout();

    private:
};

#endif // FADEOUTER_H
