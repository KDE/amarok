/****************************************************************************************
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef WEEKLY_TOP_BIAS_H
#define WEEKLY_TOP_BIAS_H

#include "CustomBiasEntry.h"

class QByteArray;
class QDate;
class QDateTimeEdit;
class QVBoxLayout;
class QNetworkReply;

/**
 *  This is a bias which allows the user to select a range of dates, and then
 *  adds to the playlist tracks/artists/albums that were on their last.fm top
 *  tracks during that time
 *
 */

namespace Amarok
{
class Collection;
}

namespace Dynamic
{

class WeeklyTopBiasFactory : public CustomBiasFactory
{
    public:
        WeeklyTopBiasFactory();
        ~WeeklyTopBiasFactory();

        virtual QString name() const;
        virtual QString pluginName() const;
        virtual CustomBiasEntry* newCustomBias( double weight );
        virtual CustomBiasEntry* newCustomBias( QDomElement e, double weight );
};

class WeeklyTopBias : public CustomBiasEntry
{
    Q_OBJECT
public:
    WeeklyTopBias( double weight );
    ~WeeklyTopBias();


    // reimplemented from CustomBiasEntry
    virtual QString pluginName() const;
    virtual QWidget* configWidget ( QWidget* parent );

    virtual bool trackSatisfies ( const Meta::TrackPtr track );
    virtual double numTracksThatSatisfy ( const Meta::TrackList& tracks );

    virtual QDomElement xml( QDomDocument doc ) const;

    virtual bool hasCollectionFilterCapability();
    virtual CollectionFilterCapability* collectionFilterCapability();

private Q_SLOTS:
    void fromDateChanged( const QDateTime& );
    void toDateChanged( const QDateTime& );

    void rangeJobFinished();
    void dataJobFinished();

    // querymaker
    void updateReady( QString, QStringList );
    
private:
    void getPossibleRange();
    void update();

    QSet< QByteArray > m_trackList;
    
    QueryMaker* m_qm;

    QVBoxLayout* m_layout;
    QDateTimeEdit* m_fromEdit;
    QDateTimeEdit* m_toEdit;
    
    uint m_fromDate;
    uint m_toDate;

    // be able to warn the user
    uint m_earliestDate;
    
    QNetworkReply* m_rangeJob;
    QNetworkReply* m_dataJob;
    Amarok::Collection* m_collection;

    friend class WeeklyTopBiasCollectionFilterCapability; // so it can report the property and weight
};


class WeeklyTopBiasCollectionFilterCapability : public Dynamic::CollectionFilterCapability
{
public:
    WeeklyTopBiasCollectionFilterCapability ( WeeklyTopBias* bias ) : m_bias ( bias ) {}

    // re-implemented
    virtual const QSet< QByteArray >& propertySet();
    virtual double weight() const;


private:
    WeeklyTopBias* m_bias;

};

}

#endif
