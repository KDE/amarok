/****************************************************************************************
 * Copyright (c) 2013 Anmol Ahuja <darthcodus@gmail.com>                                *
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

#ifndef BIAS_EXPORTER_H
#define BIAS_EXPORTER_H

#include "dynamic/Bias.h"
#include "dynamic/BiasFactory.h"

#include <QObject>
#include <QPointer>
#include <QJSValue>
#include <QString>
#include <QXmlStreamReader>

class QJSEngine;

namespace AmarokScript
{
    /**
     * Wraps ScriptableBiasFactory Ctors
     */
    class ScriptableBiasFactoryWrapper : public QObject
    {
        Q_OBJECT
    public:
        ScriptableBiasFactoryWrapper(QJSEngine *engine);
        Q_INVOKABLE QJSValue groupBiasCtor();
        Q_INVOKABLE QJSValue biasCtor();

    private:
        QJSEngine *m_engine;
    };

    // SCRIPTDOX BiasFactory
    class ScriptableBiasFactory : public QObject, public Dynamic::AbstractBiasFactory
    {
        Q_OBJECT

        /**
         * Set whether this bias is enabled and visible in the dynamic playlist bias menu.
         */
        Q_PROPERTY( bool enabled READ enabled WRITE setEnabled )

        /**
         * A user visible name for this bias
         */
        Q_PROPERTY( QString name READ i18nName WRITE setI18nName ) // corresponds to i18name

        /**
         * A unique identifier for this bias.
         */
        Q_PROPERTY( QString identifier READ name WRITE setName )

        /**
         * A user visible description for this bias.
         */
        Q_PROPERTY( QString description READ i18nDescription WRITE setI18nDescription )

        /**
         * Set a function returning widget appropriate for editing the bias, if needed.
         * var obj = new ScriptableBias(); obj.widget = function( a ) { return new QLabel(\"dfdsf\"); }"
         */
        Q_PROPERTY( QJSValue widget READ widgetFunction WRITE setWidgetFunction )

        Q_PROPERTY( QJSValue fromXml READ fromXmlFunction WRITE setFromXmlFunction )

        Q_PROPERTY( QJSValue toXml READ toXmlFunction WRITE setToXmlFunction )

        /**
         * Set this to a function of signature:
         * TrackSet ( const Meta::TrackList &playlist, int contextCount, int finalCount, QStringList universeUids )
         */
        Q_PROPERTY( QJSValue matchingTracks READ matchingTracksFunction WRITE setMatchingTracksFunction )

        /**
         * bool trackMatches( int position, TrackList playlist, int contextCount )
         */
        Q_PROPERTY( QJSValue trackMatches READ trackMatchesFunction WRITE setTrackMatchesFunction )

        Q_PROPERTY( QJSValue toStringFunction READ toStringFunction WRITE setToStringFunction )

        Q_PROPERTY( QJSValue init READ initFunction WRITE setInitFunction )

        public:
            static void init( QJSEngine *engine );

            QString i18nName() const override;
            QString name() const override;
            QString i18nDescription() const override;
            QJSValue initFunction() const;
            QJSValue fromXmlFunction() const;
            QJSValue matchingTracksFunction() const;
            QJSValue toXmlFunction() const;
            QJSValue toStringFunction() const;
            QJSValue trackMatchesFunction() const;
            QJSValue widgetFunction() const;
            QJSEngine *engine() const;

        public Q_SLOTS:
            Dynamic::BiasPtr createBias() override;

        private:
            friend QJSValue ScriptableBiasFactoryWrapper::groupBiasCtor();
            friend QJSValue ScriptableBiasFactoryWrapper::biasCtor();
            ScriptableBiasFactory( QJSEngine *engine = nullptr, bool groupBias = false );
            ~ScriptableBiasFactory() override;

            bool enabled() const;
            void setEnabled( bool enabled );
            void setName( const QString &name );
            void setI18nName( const QString &i18nName );
            void setI18nDescription( const QString &description );
            void setInitFunction( const QJSValue &value );
            void setWidgetFunction( const QJSValue &value );
            void setFromXmlFunction( const QJSValue &value );
            void setToXmlFunction( const QJSValue &value );
            void setTrackMatchesFunction( const QJSValue &value );
            void setMatchingTracksFunction( const QJSValue &value );
            void setToStringFunction( const QJSValue &value );

            QJSValue m_initFunction;
            QString m_name;
            QString m_i18nName;
            QString m_description;
            QJSValue m_widgetFunction;
            QJSValue m_fromXmlFunction;
            QJSValue m_toXmlFunction;
            QJSValue m_matchingTracksFunction;
            QJSValue m_trackMatchesFunction;
            QJSValue m_toStringFunction;
            bool m_groupBias;
            QJSEngine *m_engine;
            bool m_enabled;
    };

    class ScriptableBias : public Dynamic::AbstractBias
    {
        Q_OBJECT

        public:
            explicit ScriptableBias( ScriptableBiasFactory *biasProto );
            ~ScriptableBias() override;
            QJSValue scriptObject() { return m_biasObject; }

        public:
            void fromXml( QXmlStreamReader *reader ) override;
            void toXml(QXmlStreamWriter *writer) const override;
            Dynamic::TrackSet matchingTracks( const Meta::TrackList &playlist, int contextCount,
                                              int finalCount, const Dynamic::TrackCollectionPtr &universe ) const override;
            bool trackMatches( int position, const Meta::TrackList &playlist, int contextCount ) const override;
            QString toString() const override;
            QString name() const override;
            void paintOperator( QPainter *painter, const QRect &rect, AbstractBias *bias ) override;
            QWidget* widget( QWidget *parent = nullptr ) override;

        private Q_SLOTS:
            Dynamic::TrackSet slotMatchingTracks( const Meta::TrackList &playlist, int contextCount,
                                              int finalCount, const Dynamic::TrackCollectionPtr &universe ) const;
            void removeBias();

        public Q_SLOTS:

            /** This slot is called when the bias should discard cached results.
            * This will be done in case a new playlist is requested for an updated
            * collection.
            */
            void invalidate() override;

            /** Call this function when this bias should be replaced by a new one.
            *                @param newBias The bias that replaces this bias. If you give
            *                an empty BiasPrt as argument the bias will be removed.
            */
            void replace( const Dynamic::BiasPtr &newBias ) override;

            /**
             * Call after an outstanding result is completed
             */
            void ready( const Dynamic::TrackSet &trackSet );

        private:
            QPointer<ScriptableBiasFactory> m_scriptBias;
            QJSEngine *m_engine;
            QJSValue m_biasObject;
    };

    /**
     * Wraps TrackSetExporter Ctors
     */
    class TrackSetExporterWrapper : public QObject
    {
        Q_OBJECT
        public:
            TrackSetExporterWrapper(QJSEngine *engine);
            Q_INVOKABLE QJSValue trackSetConstructor( QJSValue arg0, QJSValue arg1 = QJSValue(QJSValue::UndefinedValue) );

        private:
            QJSEngine *m_engine;
    };

    /**
    * A representation of a set of tracks, relative to a given universe set.
    * Intersecting TrackSets from different universes is not a good idea.
    */
    class TrackSetExporter : public QObject, public Dynamic::TrackSet
    {
        Q_OBJECT

        /**
         * The number of songs contained in this trackSet.
         */
        Q_PROPERTY( int count READ trackCount )

        /**
         * True if all of the tracks are included in the set.
         */
        Q_PROPERTY( bool isFull READ isFull )

        /**
         * Returns true if the results of this track set are not yet available
         */
        Q_PROPERTY( bool isOutstanding READ isOutstanding )

        /**
         * True if none of the tracks are included in the set.
         */
        Q_PROPERTY( bool isEmpty READ isEmpty )

        public:
            static QJSValue toScriptValue( QJSEngine *engine, Dynamic::TrackSet const &trackSet );
            static void fromScriptValue( const QJSValue &obj, Dynamic::TrackSet &trackSet );

            /**
             * Includes or excludes all tracks in the set.
             * @param value If true set is set to "full". Else to "empty".
             */
            Q_INVOKABLE void reset( bool value );

            /**
             * Returns true if the @param uid is included in the set
             */
            Q_INVOKABLE bool containsUid( const QString& uid ) const;

            Q_INVOKABLE bool containsTrack( const Meta::TrackPtr &track ) const;

            /**
             * Returns the uids of a random track contains in this set
             */
            Q_INVOKABLE Meta::TrackPtr getRandomTrack() const;

            /**
             * Add the track @param track to the trackset.
             */
            Q_INVOKABLE void uniteTrack( const Meta::TrackPtr &track );

            /**
             * Add the track set @p trackSet to the trackset.
             *
             * @param trackSet the track set to be added.
             */
            Q_INVOKABLE void uniteTrackSet( const Dynamic::TrackSet &trackSet );

            /**
             * Add the uids @p uids to the trackset.
             *
             * @param uids string list of the uids.
             */
            Q_INVOKABLE void uniteUids( const QStringList &uids );

            /**
             * Perform an intersection of the trackset with the set @param trackSet
             */
            Q_INVOKABLE void intersectTrackSet( const Dynamic::TrackSet &trackSet );

            /**
             * Perform an intersection on this trackset with the trackset represented by @param uids
             */
            Q_INVOKABLE void intersectUids( const QStringList &uids );

            /**
             * Subtract the track @param track from this trackset.
             */
            Q_INVOKABLE void subtractTrack( const Meta::TrackPtr &track );

            /**
             * Subtract the trackset @param trackSet from this trackset
             */
            Q_INVOKABLE void subtractTrackSet( const Dynamic::TrackSet &trackSet );

            /**
             * Subtract the set represented by @param uids from this trackset
             */
            Q_INVOKABLE void subtractUids( const QStringList &uids );

        private:
            static void init( QJSEngine *engine );
            explicit TrackSetExporter( const Dynamic::TrackSet &trackSet );
            friend QJSValue TrackSetExporterWrapper::trackSetConstructor( QJSValue arg0, QJSValue arg1 );

            friend class ScriptableBiasFactory;
    };
}

Q_DECLARE_METATYPE( QXmlStreamReader* )
Q_DECLARE_METATYPE( QXmlStreamWriter* )
Q_DECLARE_METATYPE( AmarokScript::ScriptableBias* )

#endif
