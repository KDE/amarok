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

#ifndef IPODTRANSCODECAPABILITY_H
#define IPODTRANSCODECAPABILITY_H

#include "IpodCollection.h"
#include "core/capabilities/TranscodeCapability.h"


namespace Capabilities
{

    class IpodTranscodeCapability : public TranscodeCapability
    {
        Q_OBJECT

        public:
            /**
             * @param coll collection
             * @param deviceDirPath path to .../iPod_Control/Device directory
             */
            IpodTranscodeCapability( IpodCollection *coll, const QString &deviceDirPath );
            virtual ~IpodTranscodeCapability();

            virtual QStringList playableFileTypes();
            virtual Transcoding::Configuration savedConfiguration();
            virtual void setSavedConfiguration( const Transcoding::Configuration &configuration );

        private:
            QPointer<IpodCollection> m_coll;
            QString m_configFilePath; // must be absolute
    };

} // namespace capabilities

#endif // IPODTRANSCODECAPABILITY_H
