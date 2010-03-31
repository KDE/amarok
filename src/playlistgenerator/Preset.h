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

#include "core/meta/Meta.h"

#include <KSharedPtr>

#include <QDomElement>
#include <QObject>
#include <QSharedData>

namespace ThreadWeaver {
    class Job;
}

class ConstraintNode;

namespace APG {
    class Preset : public QObject, public QSharedData {
        Q_OBJECT

        public:
            static KSharedPtr<Preset> createFromXml( QDomElement& );
            static KSharedPtr<Preset> createNew();
            ~Preset();

            QString title() const { return m_title; }
            void setTitle( const QString& t ) { m_title = t; }

            ConstraintNode* constraintTreeRoot() const { return m_constraintTreeRoot; }
            void setConstraintTreeRoot( ConstraintNode* c ) { m_constraintTreeRoot = c; }

            QDomElement* toXml( QDomDocument& ) const;

        public slots:
            void generate( int );

        signals:
            void lock( bool );

        private slots:
            void queueSolver();
            void solverFinished( ThreadWeaver::Job* );

        private:
            Preset( const QString&, QDomElement& );
            Preset( const QString& );

            QString m_title;

            ConstraintNode* m_constraintTreeRoot;
    };
    typedef KSharedPtr<Preset> PresetPtr;
}
#endif
