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

#ifndef TRANSCODECAPABILITY_H
#define TRANSCODECAPABILITY_H

#include "core/amarokcore_export.h"
#include "core/capabilities/Capability.h"
#include "core/transcoding/TranscodingConfiguration.h"

#include <QStringList>

namespace Capabilities
{

    /**
     * Collections whose CollectionLocation supports transcoding (e.g. it doesn't ignore
     * Transcoding::Configuration configuration parameter in copyUrlsToCollection())
     * can and should provide this capability so that core CollectionLocation methods can
     * ask user whether she wants to just copy/move or transcode tracks when
     * copying/moving/dragging them to destination collection.
     *
     * If your collection doesn't support transcoding (not implemented or just
     * temporarily), you should not (temporarily) provide this capability.
     *
     * @author Matěj Laitl <matej@laitl.cz>
     */
    class AMAROK_CORE_EXPORT TranscodeCapability : public Capability
    {
        Q_OBJECT

        public:
            virtual ~TranscodeCapability();

            /**
             * Return a list of file types (should be compatible with Meta::Track::type())
             * that your collection is able to play. This is used to disable transcoding
             * to formats that wouldn't be playable; if your collection is a portable player
             * that can only play ogg vorbis and flac, you would return
             * QStringList() << "ogg" << "flac";
             *
             * In order not to suck users, "plain copy" option is always available
             * regardless of what this method returns.
             *
             * Return value of empty QStringList() is special and means that there should
             * be no restriction on enabled transcoders. Default implementation returns
             * this value.
             */
            virtual QStringList playableFileTypes() { return QStringList(); }

            /**
             * Return configuration previously saved using setSavedConfiguration() or invalid
             * configuration if there is no configuration saved.
             */
            virtual Transcoding::Configuration savedConfiguration() = 0;

            /**
             * Set saved configuration to @p configuration. An invalid configuration
             * should be interpreted as an action to unset saved configuration.
             *
             * @param configuration the transcoding configuration
             */
            virtual void setSavedConfiguration( const Transcoding::Configuration &configuration ) = 0;

            /**
             * Type of this capability
             */
            static Type capabilityInterfaceType() { return Capability::Transcode; }
    };

} // napespace Capabilities

#endif // TRANSCODECAPABILITY_H
