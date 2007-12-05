/***************************************************************************
 *   Copyright (C) 2007 Shane King <kde@dontletsstart.com>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_DIRECTSHOWBACKEND_H
#define AMAROK_DIRECTSHOWBACKEND_H

#include <phonon/backendinterface.h>

class DirectShowBackend : public QObject, public Phonon::BackendInterface
{
    Q_OBJECT
    Q_INTERFACES(Phonon::BackendInterface)

    public:
        DirectShowBackend(QObject *parent, const QVariantList &args);
        ~DirectShowBackend();

        QObject *createObject(BackendInterface::Class, QObject *parent, const QList<QVariant> &args);
        QList<int> objectDescriptionIndexes(Phonon::ObjectDescriptionType type) const;
        QHash<QByteArray, QVariant> objectDescriptionProperties(Phonon::ObjectDescriptionType type, int index) const;
        bool startConnectionChange(QSet<QObject *>);
        bool connectNodes(QObject *, QObject *);
        bool disconnectNodes(QObject *, QObject *);
        bool endConnectionChange(QSet<QObject *>);
        QStringList availableMimeTypes() const;
};

#endif // AMAROK_DIRECTSHOWBACKEND_H
