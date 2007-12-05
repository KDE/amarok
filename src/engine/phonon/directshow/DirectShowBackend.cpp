/***************************************************************************
 *   Copyright (C) 2007 Shane King <kde@dontletsstart.com>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "phonon-directshow"

#include "DirectShow.h"

#include "DirectShowBackend.h"
#include "DirectShowMediaObject.h"
#include "DirectShowAudioOutput.h"

#include <kpluginfactory.h>


K_PLUGIN_FACTORY(DirectShowBackendFactory, registerPlugin<DirectShowBackend>();)
K_EXPORT_PLUGIN(DirectShowBackendFactory("amarokdirectshowbackend"))


DirectShowBackend::DirectShowBackend(QObject *parent, const QVariantList &args)
    : QObject(parent)
{
}


DirectShowBackend::~DirectShowBackend()
{
}


QObject *
DirectShowBackend::createObject(BackendInterface::Class objectClass, QObject *parent, const QList<QVariant> &args)
{
    switch( objectClass )
    {
        case BackendInterface::MediaObjectClass:
            return new DirectShowMediaObject(parent);

        case BackendInterface::AudioOutputClass:
            return new DirectShowAudioOutput(parent);
    }
    return 0;
}


QList<int> 
DirectShowBackend::objectDescriptionIndexes(Phonon::ObjectDescriptionType type) const
{
    return QList<int>();
}


QHash<QByteArray, QVariant> 
DirectShowBackend::objectDescriptionProperties(Phonon::ObjectDescriptionType type, int index) const
{
    return QHash<QByteArray, QVariant>();
}


bool 
DirectShowBackend::startConnectionChange(QSet<QObject *>)
{
    return true;
}


bool 
DirectShowBackend::connectNodes(QObject *, QObject *)
{
    return false;
}


bool 
DirectShowBackend::disconnectNodes(QObject *, QObject *)
{
    return false;
}


bool 
DirectShowBackend::endConnectionChange(QSet<QObject *>)
{
    return true;
}


QStringList 
DirectShowBackend::availableMimeTypes() const
{
    return QStringList();
}
