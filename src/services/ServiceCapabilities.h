/****************************************************************************************
 * Copyright (c) 2010 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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


#ifndef SERVICECAPABILITIES_H
#define SERVICECAPABILITIES_H


#include "amarok_export.h"
#include "amarokurls/AmarokUrl.h"

#include "meta/capabilities/BookmarkThisCapability.h"
#include "meta/capabilities/CurrentTrackActionsCapability.h"
#include "meta/capabilities/CustomActionsCapability.h"
#include "meta/capabilities/FindInSourceCapability.h"
#include "meta/capabilities/SourceInfoCapability.h"


class BookmarkThisProvider;
class CurrentTrackActionsProvider;
class CustomActionsProvider;
class SourceInfoProvider;

namespace Meta
{
    class ServiceTrack;
}


/**
A service specific implementation of the BookmarkThisCapability

    @author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class AMAROK_EXPORT ServiceBookmarkThisCapability : public Meta::BookmarkThisCapability {
public:
    ServiceBookmarkThisCapability( BookmarkThisProvider * provider );

    ~ServiceBookmarkThisCapability();

    virtual bool isBookmarkable();
    virtual QString browserName();
    virtual QString collectionName();
    virtual bool simpleFiltering();
    virtual QAction * bookmarkAction();

private:

    BookmarkThisProvider * m_provider;
};



class AMAROK_EXPORT ServiceCurrentTrackActionsCapability : public Meta::CurrentTrackActionsCapability {
    Q_OBJECT
    public:
        ServiceCurrentTrackActionsCapability( CurrentTrackActionsProvider * currentTrackActionsProvider  );
        virtual ~ServiceCurrentTrackActionsCapability();
        virtual QList< QAction * > customActions() const;

    private:
        CurrentTrackActionsProvider * m_currentTrackActionsProvider;
};



class AMAROK_EXPORT ServiceCustomActionsCapability : public Meta::CustomActionsCapability
{
    Q_OBJECT

    public:
        ServiceCustomActionsCapability( CustomActionsProvider * customActionsProvider  );
        virtual ~ServiceCustomActionsCapability();
        virtual QList< QAction * > customActions() const;

    private:
        CustomActionsProvider * m_customActionsProvider;
};



class AMAROK_EXPORT ServiceSourceInfoCapability : public Meta::SourceInfoCapability
{
public:
    ServiceSourceInfoCapability( SourceInfoProvider * sourceInfoProvider );

    ~ServiceSourceInfoCapability();

    QString sourceName();
    QString sourceDescription();
    QPixmap emblem();
    QString scalableEmblem();

private:
    SourceInfoProvider * m_sourceInfoProvider;

};



class AMAROK_EXPORT ServiceFindInSourceCapability : public Meta::FindInSourceCapability
{
    Q_OBJECT
    public:
        ServiceFindInSourceCapability( Meta::ServiceTrack *track );
        virtual void findInSource();

    private:
        Meta::ServiceTrack * m_track;
};



#endif // SERVICECAPABILITIES_H
