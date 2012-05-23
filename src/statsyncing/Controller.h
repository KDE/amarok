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

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "statsyncing/amarok_statsyncing_export.h"

#include <QObject>

namespace StatSyncing
{
    /**
     * A singleton class that controls statistics synchronization and related tasks.
     */
    class AMAROK_STATSYNCING_EXPORT Controller : public QObject
    {
        Q_OBJECT

        public:
            explicit Controller( QObject *parent );
            ~Controller();

        public slots:
            /**
             * Start the whole synchronization machinery. This call returns quickly,
             * way before the synchronization is finished.
             */
            void synchronize();

        private:
            Q_DISABLE_COPY(Controller)
    };

} // namespace StatSyncing

#endif // CONTROLLER_H
