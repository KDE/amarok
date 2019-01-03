/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#ifndef TRANSCODINGSELECTCONFIGWIDGET_H
#define TRANSCODINGSELECTCONFIGWIDGET_H

#include "amarok_transcoding_export.h"
#include "core/transcoding/TranscodingConfiguration.h"

#include <QComboBox>

namespace Transcoding
{

    /**
     * Convenience QComboBox subclass that can be used in collection configuration dialogs
     * to let user change/unset current transcoding preference for transferring tracks.
     */
    class AMAROK_TRANSCODING_EXPORT SelectConfigWidget : public QComboBox
    {
        Q_OBJECT

        public:
            explicit SelectConfigWidget( QWidget *parent = nullptr );

            /**
             * Fills the combobox widget with appropriate transcoding configurations.
             * "Just copy" and "Ask every time" options are always present.
             *
             * @param savedConfiguration current saved configuration or invalid one if no
             * saved config exists.
             */
            void fillInChoices( const Configuration &savedConfiguration );

            /**
             * Get current choice. Will return invalid configuration if called before
             * @see fillInChoices()
             */
            Configuration currentChoice() const;

            /**
             * Return true if currently selected choice is different from one that was
             * passed to fillInChoices()
             */
            bool hasChanged() const;

        private:
            enum Choice {
                TranscodeAll,
                TranscodeUnlessSameType,
                TranscodeOnlyIfNeeded,
                JustCopy,
                Invalid
            };

            Q_DISABLE_COPY(SelectConfigWidget)
            Configuration m_passedChoice;
    };

} // namespace Transcoding

#endif // TRANSCODINGSELECTCONFIGWIDGET_H
