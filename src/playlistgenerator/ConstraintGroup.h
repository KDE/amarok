/****************************************************************************************
 * Copyright (c) 2008-2012 Soren Harward <stharward@gmail.com>                          *
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

#include "core/meta/forward_declarations.h"

#include <QDomElement>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QtGlobal>
#include <cmath>

namespace Collections {
    class QueryMaker;
}

class ConstraintGroup : public ConstraintNode {
    Q_OBJECT
    public:
        enum MatchType { MatchAny, MatchAll };

        static ConstraintGroup* createFromXml( QDomElement&, ConstraintNode* );
        static ConstraintGroup* createNew( ConstraintNode* );

        QString getName() const override;
        int getNodeType() const override { return ConstraintNode::ConstraintGroupType; }
        QWidget* editWidget() const override;
        void toXml( QDomDocument&, QDomElement& ) const override;

        Collections::QueryMaker* initQueryMaker( Collections::QueryMaker* ) const override;
        double satisfaction( const Meta::TrackList& ) const override;
        quint32 suggestPlaylistSize() const override;

    private Q_SLOTS:
        void setMatchAny();
        void setMatchAll();

    private:
        ConstraintGroup(QDomElement&, ConstraintNode*);
        explicit ConstraintGroup(ConstraintNode*);

        // parameters
        MatchType m_matchtype;

        // internal mathematical functions
        double combineInterdependentConstraints( const Meta::TrackList&, const double, const QMultiHash<int,int>& ) const;
};

class ConstraintGroupEditWidget : public QWidget {
    Q_OBJECT

    public:
        explicit ConstraintGroupEditWidget( const ConstraintGroup::MatchType );

    Q_SIGNALS:
        void updated();
        void clickedMatchAll();
        void clickedMatchAny();

    private Q_SLOTS:
        void on_radioButton_MatchAll_clicked( bool );
        void on_radioButton_MatchAny_clicked( bool );

    private:
        Ui::ConstraintGroupEditWidget ui;
};

#endif
