/****************************************************************************************
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_BIAS_FACTORY_H
#define AMAROK_BIAS_FACTORY_H

#include "amarok_export.h"
#include "dynamic/Bias.h"

#include <QObject>
#include <KLocalizedString>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace Dynamic
{
    /** A bias that will be used when a "real" bias could not be found.
        This bias will listen to the BiasFactory and present a "stand in" bias
        until a new factory with it's name becomes available.
        This will allow services with a bias to be switched off without their
        bias getting removed or otherwise corrupted.
    */
    class ReplacementBias : public RandomBias
    {
        Q_OBJECT

        public:
            explicit ReplacementBias( const QString &n );
            ReplacementBias( const QString &n, QXmlStreamReader *reader );

            void toXml( QXmlStreamWriter *writer ) const override;

            static QString sName();
            QString name() const override;
            QString toString() const override;

            QWidget* widget( QWidget* parent = nullptr ) override;

        protected Q_SLOTS:
                void factoryChanged();

        private:
                QString m_name;
                QByteArray m_html;

                Q_DISABLE_COPY(ReplacementBias)
    };

    /**
     * The factory which creates bias entries on demand. As the user can create any number
     * of biases from from the bias addition widget, new custom biass types need to be able to be
     * generated on command and at runtime.
     **/
    class AMAROK_EXPORT AbstractBiasFactory
    {
        public:
            AbstractBiasFactory() {}
            virtual ~AbstractBiasFactory() {}

            /** Returns the translated name of the type of bias.
                This one is used in the combo boxes when selecting the bias.
                It could be eg. "Last.fm Similar Artists"
             */
            virtual QString i18nName() const = 0;

            /** Returns an internal non-translatable name for this custom bias type.
                This name must be unique over all biases and will also be used
                when reading and writing a bias to xml.
             */
            virtual QString name() const = 0;

            /** Returns the translated description of the bias */
            virtual QString i18nDescription() const = 0;

            /** Create the custom bias. The caller takes owner of the pointer
             */
            virtual BiasPtr createBias() = 0;

            /** Creates a new custom bias from xml data
             */
            virtual BiasPtr createFromXml( QXmlStreamReader *reader );
    };

    class AMAROK_EXPORT BiasFactory : public QObject
    {
        Q_OBJECT

    public:
        static BiasFactory* instance();

        /** Add a new CustomBiasEntry to the registry.
             It will show up for users when then select the type of bias they want.
         */
        static void registerNewBiasFactory( AbstractBiasFactory* factory );

        /** Remove CustomBiasEntry from the list of bias types that the user can select.
         */
        static void removeBiasFactory( AbstractBiasFactory* factory );

        /** Helper function to get a bias from an xml tag */
        static BiasPtr fromXml( QXmlStreamReader *reader );

        /** Helper function to get a bias from an name */
        static BiasPtr fromName( const QString &name );

        /**
         * Returns all the current registered factories for this CustomBias
         */
        static QList<AbstractBiasFactory*> factories();

    Q_SIGNALS:
        /** Emitted when the list of bias factories was changed. */
        void changed();

    private:
        BiasFactory( QObject *parent = nullptr );
        ~BiasFactory() override;

        void emitChanged();

        static BiasFactory* s_instance;
        static QList<Dynamic::AbstractBiasFactory*> s_biasFactories;
    };
}

#endif
