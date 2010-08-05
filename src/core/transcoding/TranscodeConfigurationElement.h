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
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef TRANSCODECONFIGURATIONELEMENT_H
#define TRANSCODECONFIGURATIONELEMENT_H

#include "shared/amarok_export.h"

#include <QPointer>
#include <QStringList>
#include <QWidget>

class AMAROK_EXPORT TranscodeConfigurationElement
{
public:
    enum Type
    {
        NUMERIC = 0,
        TEXT,
        LIST
    };

    static TranscodeConfigurationElement Numeric( const QString &label, long defaultValue, long min, long max );
    static TranscodeConfigurationElement String( const QString &label, const QString &defaultText );
    static TranscodeConfigurationElement List( const QString &label, const QStringList &values, int defaultValueIndex );

    QWidget *createConfigurationWidget( QWidget *parent = 0 );

    /**
     * Returns the configuration widget for the current configuration element if the widget
     * exists, otherwise null.
     * @return a pointer to the widget
     */
    QWidget *widget() const;
private:
    TranscodeConfigurationElement( const QString &label,
                                   Type type,
                                   long defaultNumber,
                                   long min,
                                   long max,
                                   const QStringList &values,
                                   const QString &defaultText );
    const QString m_label;
    const Type m_type;
    const long m_defaultNumber;
    const long m_min;
    const long m_max;
    const QStringList m_values;
    const QString m_defaultString;
    QPointer< QWidget > m_widget;
};

#endif // TRANSCODECONFIGURATIONELEMENT_H
