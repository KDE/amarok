/****************************************************************************************
 * Copyright (c) 2008-2010 Soren Harward <stharward@gmail.com>                          *
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

#ifndef APG_PRESET
#define APG_PRESET

#include "core/meta/forward_declarations.h"

#include "AmarokSharedPointer.h"

#include <QDomElement>
#include <QObject>
#include <QSharedData>
#include <ThreadWeaver/Job>

namespace ThreadWeaver {
    class Job;
}

class ConstraintNode;

namespace APG {
    class Preset : public QObject, public QSharedData {
        Q_OBJECT

        public:
            static AmarokSharedPointer<Preset> createFromXml( QDomElement& );
            static AmarokSharedPointer<Preset> createNew();
            ~Preset();

            QString title() const { return m_title; }
            void setTitle( const QString& t ) { m_title = t; }

            ConstraintNode* constraintTreeRoot() const { return m_constraintTreeRoot; }
            void setConstraintTreeRoot( ConstraintNode* c ) { m_constraintTreeRoot = c; }

            QDomElement* toXml( QDomDocument& ) const;

        public Q_SLOTS:
            void generate( int );

        Q_SIGNALS:
            void lock( bool );

        private Q_SLOTS:
            void queueSolver();
            void solverFinished( ThreadWeaver::JobPointer );

        private:
            Preset( const QString&, QDomElement& );
            Preset( const QString& );

            QString m_title;

            ConstraintNode* m_constraintTreeRoot;
    };
    typedef AmarokSharedPointer<Preset> PresetPtr;
}
#endif
