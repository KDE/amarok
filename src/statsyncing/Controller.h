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

#ifndef STATSYNCING_CONTROLLER_H
#define STATSYNCING_CONTROLLER_H

#include "amarok_export.h"

#include <QWeakPointer>

namespace StatSyncing
{
    class Process;
    class Provider;
    typedef QSet<QSharedPointer<Provider> > ProviderPtrSet;

    /**
     * A singleton class that controls statistics synchronization and related tasks.
     */
    class AMAROK_EXPORT Controller : public QObject
    {
        Q_OBJECT

        public:
            explicit Controller( QObject *parent = 0 );
            ~Controller();

        public slots:
            /**
             * Start the whole synchronization machinery. This call returns quickly,
             * way before the synchronization is finished.
             */
            void synchronize();

        private slots:
            void saveSettings( const ProviderPtrSet &checkedProviders,
                               const ProviderPtrSet &unCheckedProviders,
                               qint64 checkedFields );

        private:
            Q_DISABLE_COPY(Controller)

            QWeakPointer<Process> m_currentProcess;
    };

} // namespace StatSyncing

#endif // STATSYNCING_CONTROLLER_H
