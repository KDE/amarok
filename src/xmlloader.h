/*
  Copyright (c) 2006 Gábor Lehel <illissius@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/


#ifndef AMAROK_XML_LOADER_H
#define AMAROK_XML_LOADER_H

#include <qevent.h>
#include <qobject.h>
#include <qxml.h>
#include "metabundle.h"

/**
 * Used for loading XML of the format outputted by MetaBundle::save(),
 * back into MetaBundle form.
 * There are four ways of using it:
 * - the simplest is to use MetaBundle::XmlLoader::loadBundles(), which just
 *   returns a BundleList of the loaded MetaBundles, to load all the bundles
 *   synchronously in a single shot
 * - you can create a MetaBundle::XmlLoader object and ask it to load(), and
 *   connect to the newBundle() signal to receive the loaded bundles, to load
 *   the bundles synchronously, but one-by-one
 * - you can use MetaBundle::XmlLoader::loadInThread(), and the loaded bundles
 *   will get posted as BundleLoadedEvents to the object you specify; this way
 *   you can load asynchronously
 * - or you can derive from MetaBundle::XmlLoader, reimplement the relevant
 *   functions, and do whatever you want
 */

/** The type used for extra XML attributes not recognized. */
typedef QValueList< QPair<QString, QString> > XmlAttributeList;


class MetaBundle::XmlLoader: public QObject, public QXmlDefaultHandler
{
    Q_OBJECT
    public:
        /** Posted when a MetaBundle has been loaded. */
        class BundleLoadedEvent: public QCustomEvent
        {
            public:
                /** The type() of BundleLoadedEvents. */
                static const int Type = QEvent::User + 127;

                /** Whether an error occurred. If yes, both bundle and extraAttributes are empty. */
                bool error;

                /** A description of the error, if there was one. */
                QString errorMessage;

                /** The loaded bundle. */
                MetaBundle bundle;

                /** Any extra attributes not recognized. */
                QValueList< QPair<QString, QString> > extraAttributes;

            public:
                BundleLoadedEvent( const MetaBundle &b, const XmlAttributeList &a )
                    : QCustomEvent( Type ), error( false ), bundle( b ), extraAttributes( a ) { }
                BundleLoadedEvent( const QString &error )
                    : QCustomEvent( Type ), error( true ), errorMessage( error ) { }
        };

    public:
        /** Construct a MetaBundle::XmlLoader. */
        XmlLoader();

        /** Destruct. */
        virtual ~XmlLoader();

        /**
         * Load bundles from \p source. The loaded bundles will be emitted as
         * newBundle() signals, and if you provide a \p target, also sent as
         * BundleLoadedEvents to \p target. In case of a fatal error,
         * processing will stop and false will be returned, and an empty
         * BundleLoadedEvent with the error flag set will be sent to \p target,
         * if one is provided.
         * @param source the source to load from
         * @param target the target to send events to; if none is provided,
         *               none will be sent
         * @return whether a fatal error occurred
         * @see newBundle
         * @see BundleLoadedEvent
         */
        bool load( QXmlInputSource *source, QObject *target = 0 );

        /** Aborts loading. */
        void abort();

        /** Returns the last error encountered; empty if there hasn't been an error. */
        QString lastError() const;

        /**
         * Load bundles from \p source. If a fatal error occurs, processing
         * will stop and the list of complete bundles at that point will be
         * returned, and \p ok will be set to true, if provided.
         * @param source the source to load from
         * @param ok if provided, will be set to false if a fatal error occurs,
                     and to true otherwise
         * @return the list of loaded bundles
         */
        static BundleList loadBundles( QXmlInputSource *source, bool *ok = 0 );

        /**
         * Load bundles from \p source in a separate thread. The loaded bundles
         * will be posted as BundleLoadedEvents to \p target. If an error
         * occurs, processing will stop and an empty BundleLoadedEvent will be
         * posted with the error flag set to true.
         * This function returns immediately after being called.
         * @param source the source to load from
         * @param target the object to post BundleLoadedEvents to
         * @see BundleLoadedEvent
         */
        static void loadInThread( QXmlInputSource *source, QObject *target );

    signals:
        /**
         * Emitted by load() whenever a MetaBundle has been loaded.
         * @param bundle the loaded MetaBundle
         * @param extraAttributes any extra attributes in the XML not recognized
         */
        void newBundle( const MetaBundle &bundle, const XmlAttributeList &extraAttributes );

        /** Emitted when an error occurs. */
        void error( const QString &errorMessage );

    protected:
        virtual void newAttribute( const QString &key, const QString &value );
        virtual void newTag( const QString &name, const QString &value );
        virtual void bundleLoaded();
        virtual void errorEncountered( const QString &message, int line, int column );

    protected:
        /** The bundle currently being loaded. */
        MetaBundle m_bundle;

        /** Unrecognized attributes in the XML for the currently loading bundle. */
        XmlAttributeList m_attributes;

        /** The message from the last error encountered, empty if there hasn't been an error. */
        QString m_lastError;

        /** Whether we have been abort()ed. */
        bool m_aborted;

    private:
        QXmlSimpleReader m_reader;
        QString m_currentElement;
        QObject *m_target;

    protected:
        virtual bool startElement( const QString&, const QString&, const QString &, const QXmlAttributes& );
        virtual bool endElement( const QString &namespaceURI, const QString &localName, const QString &qName );
        virtual bool characters( const QString &ch );
        virtual bool endDocument();
        virtual bool fatalError( const QXmlParseException &exception );

    public: //fucking moc, these should be private
        class ThreadedLoader;
        class SimpleLoader;
};


#endif
