/****************************************************************************************
 * Copyright (c) 2010 Téo Mrnjavac <teo@kde.org>                                        *
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
 * You should have received a copy of the GNU General Public License aint with          *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef TRANSCODING_PROPERTY_H
#define TRANSCODING_PROPERTY_H

#include "shared/amarok_export.h"

#include <QPointer>
#include <QStringList>
#include <QVariant>
#include <QWidget>

namespace Transcoding
{

/**
 * This class defines a single option that modifies the behavior of an encoder, as
 * defined by a Transcoding::Format subclass.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class AMAROK_EXPORT Property
{
public:
    enum Type
    {
        NUMERIC = 0,
        TEXT,
        LIST
    };

    static Property Numeric( const QByteArray name,
                                         const QString &prettyName,
                                         int min,
                                         int max,
                                         int defaultValue );

    static Property String(  const QByteArray name,
                                         const QString &prettyName,
                                         const QString &defaultText );

    static Property List(    const QByteArray name,
                                         const QString &prettyName,
                                         const QStringList &valuesList,
                                         int defaultIndex );

    QByteArray name() const { return m_name; }

    const QString & prettyName() const { return m_prettyName; }

    Type type() const { return m_type; }

    QVariant::Type variantType() const
    { return ( m_type == NUMERIC ) ? QVariant::Int : QVariant::String; }

    int min() const { return m_min; }

    int max() const { return m_max; }

    int defaultValue() const { return m_defaultNumber; }

    const QString & defaultText() const { return m_defaultString; }

    const QStringList & values() const { return m_values; }

    int defaultIndex() const { return m_defaultNumber; }

private:
    Property( const QByteArray name,
                          const QString &prettyName,
                          Type type,
                          int defaultNumber,
                          int min,
                          int max,
                          const QStringList &values,
                          const QString &defaultText );
    QByteArray m_name;
    QString m_prettyName;
    Type m_type;
    int m_defaultNumber;
    int m_min;
    int m_max;
    QStringList m_values;
    QString m_defaultString;
    QPointer< QWidget > m_widget;
};

typedef QList< Property > PropertyList;

}

#endif //TRANSCODING_PROPERTY_H
