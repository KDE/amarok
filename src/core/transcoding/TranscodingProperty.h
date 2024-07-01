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

#include "core/amarokcore_export.h"

#include <QStringList>
#include <QVariant>

namespace Transcoding
{

/**
 * This class defines a single option that modifies the behavior of an encoder, as
 * defined by a Transcoding::Format subclass.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class AMAROKCORE_EXPORT Property
{
public:
    enum Type
    {
        TRADEOFF
    };

    static Property Tradeoff( const QByteArray &name,
                              const QString &prettyName,
                              const QString &description,
                              const QString &leftText,
                              const QString &rightText,
                              int min,
                              int max,
                              int defaultValue );

    static Property Tradeoff( const QByteArray &name,
                              const QString &prettyName,
                              const QString &description,
                              const QString &leftText,
                              const QString &rightText,
                              const QStringList &valueLabels,
                              int defaultValue );

    QByteArray name() const { return m_name; }

    const QString & prettyName() const { return m_prettyName; }

    const QString & description() const { return m_description; }

    Type type() const { return m_type; }

    QVariant::Type variantType() const;

    int min() const { return m_min; }

    int max() const { return m_max; }

    QVariant defaultValue() const { return m_defaultValue; }

    const QStringList & valueLabels() const { return m_valueLabels; }

    const QStringList & endLabels() const { return m_endLabels; }

private:
    Property( const QByteArray &name,
              const QString &prettyName,
              const QString &description,
              Type type,
              const QVariant &defaultValue,
              int min,
              int max,
              const QStringList &valueLabels,
              const QStringList &endLabels );

    QByteArray m_name;
    QString m_prettyName;
    QString m_description;
    Type m_type;
    QVariant m_defaultValue;
    int m_min;
    int m_max;
    QStringList m_valueLabels;
    QStringList m_endLabels;
};

typedef QList< Property > PropertyList;

}

#endif //TRANSCODING_PROPERTY_H
