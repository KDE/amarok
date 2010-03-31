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

#ifndef APG_CONSTRAINT_FACTORY
#define APG_CONSTRAINT_FACTORY

#include <QDomElement>
#include <QPair>
#include <QString>
#include <QStringList>
#include <limits.h>

class Constraint;
class ConstraintNode;

class ConstraintFactoryEntry {
	public:
        friend class ConstraintFactory;
		ConstraintFactoryEntry(const QString&, const QString&, Constraint* (*xmlf)(QDomElement&, ConstraintNode*), Constraint* (*nf)(ConstraintNode*));

	private:
		const QString m_name;
		const QString m_description;
        Constraint* (*m_createFromXmlFunc)(QDomElement&, ConstraintNode*);
        Constraint* (*m_createNewFunc)(ConstraintNode*);
};

class ConstraintFactory {
    public:
        static ConstraintFactory* instance();
        static void destroy();
        ~ConstraintFactory();

        // row is set to INT_MAX so that children are appended by default
        ConstraintNode* createConstraint(QDomElement&, ConstraintNode*, int row=INT_MAX) const;
        ConstraintNode* createConstraint(const QString&, ConstraintNode*, int row=INT_MAX) const;
        ConstraintNode* createConstraint(const int, ConstraintNode*, int row=INT_MAX) const;

        ConstraintNode* createGroup(QDomElement&, ConstraintNode*, int row=INT_MAX) const;
        ConstraintNode* createGroup(ConstraintNode*, int row=INT_MAX) const;

        const QStringList names() const;
        QList< QPair<int, QString> > registeredConstraints() const;
        int getTypeId(const QString&) const;

    private:
        ConstraintFactory();

        static ConstraintFactory* s_self;
        QHash<int, ConstraintFactoryEntry*> m_registryIds;
        QHash<QString, ConstraintFactoryEntry*> m_registryNames;
};

#endif // APG_CONSTRAINT_FACTORY
