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

#ifndef AMAROK_XMLLOADER_P_H
#define AMAROK_XMLLOADER_P_H

#include <qapplication.h>
#include <qthread.h>

class MetaBundle::XmlLoader::ThreadedLoader: public QObject, public QThread
{
    Q_OBJECT
    QObject *m_target;
    QXmlInputSource *m_source;

    private slots:
        void bundleLoaded( const MetaBundle &bundle, const XmlAttributeList &attributes )
        {
            QApplication::postEvent( m_target, new BundleLoadedEvent( bundle, attributes ) );
        }

    public:
        ThreadedLoader( QXmlInputSource *source, QObject *target ): m_target( target ), m_source( source ) { }

    protected:
        virtual void run()
        {
            {
                XmlLoader loader;
                connect( &loader, SIGNAL( newBundle( const MetaBundle&, const XmlAttributeList& ) ),
                         this,  SLOT( bundleLoaded( const MetaBundle&, const XmlAttributeList& ) ) );
                bool success = loader.load( m_source );
                if( !success )
                    QApplication::postEvent( m_target, new BundleLoadedEvent( loader.m_lastError ) );
            }

            delete this;
        }
};

class MetaBundle::XmlLoader::SimpleLoader: public QObject
{
    Q_OBJECT

public:
    BundleList bundles;

    SimpleLoader( QXmlInputSource *source, bool *ok )
    {
        XmlLoader loader;
        connect( &loader, SIGNAL( newBundle( const MetaBundle&, const XmlAttributeList& ) ),
                 this,  SLOT( bundleLoaded( const MetaBundle&, const XmlAttributeList& ) ) );
        const bool success = loader.load( source );
        if( ok )
            (*ok) = success;
    }

private slots:
    void bundleLoaded( const MetaBundle &bundle, const XmlAttributeList& )
    {
        bundles << bundle;
    }
};

#endif
