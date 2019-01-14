/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#ifndef METABOOKMARKTHISCAPABILITY_H
#define METABOOKMARKTHISCAPABILITY_H

#include "core/amarokcore_export.h"
#include "core/capabilities/Capability.h"

#include <QAction>

namespace Capabilities {

/**
  This capability determines whether a meta item in a collection can be directly bookmarked. Not all collections/services supports bookmarks on all levels, and some might not support Item level bookmarks at all as they have no query field and some might only support simple queries.

    @author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class AMAROKCORE_EXPORT BookmarkThisCapability : public Capability {
    Q_OBJECT
public:
    explicit BookmarkThisCapability( QAction* action );
    virtual ~BookmarkThisCapability();

    virtual bool isBookmarkable() { return true; }
    virtual QString browserName() { return QStringLiteral("collections"); }
    virtual QString collectionName() { return QString(); }
    virtual bool simpleFiltering() { return false; }

    /**
       The caller must free actions that have no parent after use.
       Actions with a parent are freed by the parent (obviously)
       @return the bookmarkAction itself (or 0).
    */
    virtual QAction * bookmarkAction() const { return m_action; }

    /**
     * Get the capabilityInterfaceType of this capability
     * @return The capabilityInterfaceType ( always Capabilities::Capability::BookmarkThis; )
    */
    static Type capabilityInterfaceType() { return Capabilities::Capability::BookmarkThis; }

protected:
    QAction* m_action;

};

}

#endif
