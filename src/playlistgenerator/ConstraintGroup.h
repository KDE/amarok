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

#ifndef APG_CONSTRAINTGROUP
#define APG_CONSTRAINTGROUP

#include "ui_ConstraintGroupEditWidget.h"

#include "ConstraintNode.h"

#include "core/meta/Meta.h"

#include <QDomElement>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QtGlobal>
#include <math.h>

namespace Collections {
    class QueryMaker;
}

class ConstraintGroup : public ConstraintNode {
    Q_OBJECT
    public:
        enum MatchType { MatchAny, MatchAll };

        static ConstraintGroup* createFromXml(QDomElement&, ConstraintNode*); // Potential Factory restriction
        static ConstraintGroup* createNew(ConstraintNode*); // Potential Factory restriction

        virtual QString getName() const;
        virtual int getNodeType() const { return ConstraintNode::ConstraintGroupType; }
        virtual QWidget* editWidget() const;
        virtual void toXml(QDomDocument&, QDomElement&) const;

        virtual Collections::QueryMaker* initQueryMaker(Collections::QueryMaker*) const;
        virtual double satisfaction(const Meta::TrackList&);
        virtual double deltaS_insert(const Meta::TrackList&, const Meta::TrackPtr, const int) const;
        virtual double deltaS_replace(const Meta::TrackList&, const Meta::TrackPtr, const int) const;
        virtual double deltaS_delete(const Meta::TrackList&, const int) const;
        virtual double deltaS_swap(const Meta::TrackList&, const int, const int) const;
        virtual void insertTrack(const Meta::TrackList&, const Meta::TrackPtr, const int);
        virtual void replaceTrack(const Meta::TrackList&, const Meta::TrackPtr, const int);
        virtual void deleteTrack(const Meta::TrackList&, const int);
        virtual void swapTracks(const Meta::TrackList&, const int, const int);
        virtual int suggestInitialPlaylistSize() const;
        virtual ConstraintNode::Vote* vote( const Meta::TrackList&, const Meta::TrackList& ) const;

#ifndef KDE_NO_DEBUG_OUTPUT
        virtual void audit(const Meta::TrackList&) const;
#endif

    private slots:
        void setMatchAny();
        void setMatchAll();

    private:
        ConstraintGroup(QDomElement&, ConstraintNode*);
        ConstraintGroup(ConstraintNode*);

        // parameters
        MatchType m_matchtype;

        // internal mathematical functions
        double combineNonIndependentConstraints(const Meta::TrackList&, const double) const;

        // internal state variables
        double m_satisfaction;
        QList<double> m_childSatisfactions;
        QHash<int, int> m_constraintMatchTypes;
};

class ConstraintGroupEditWidget : public QWidget {
    Q_OBJECT

    public:
        ConstraintGroupEditWidget( const ConstraintGroup::MatchType );

    signals:
        void updated();
        void clickedMatchAll();
        void clickedMatchAny();

    private slots:
        void on_radioButton_MatchAll_clicked( bool );
        void on_radioButton_MatchAny_clicked( bool );

    private:
        Ui::ConstraintGroupEditWidget ui;
};

#endif
