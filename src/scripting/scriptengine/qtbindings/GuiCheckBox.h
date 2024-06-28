/*
 * Replacement fot QT Bindings that were removed from QT5
 * Copyright (C) 2020  Pedro de Carvalho Gomes <pedrogomes81@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GUICHECKBOX_H
#define GUICHECKBOX_H

#include "QtBinding.h"

#include <QCheckBox>

namespace QtBindings
{
    namespace Gui
    {
        class CheckBox : public QCheckBox, public QtBindings::Base<CheckBox>
        {
            Q_OBJECT
        public:
            Q_INVOKABLE CheckBox(QWidget *parent = Q_NULLPTR);
            Q_INVOKABLE CheckBox(const CheckBox &other);
            Q_INVOKABLE CheckBox(const QString &text, QWidget *parent = Q_NULLPTR);
            Q_INVOKABLE ~CheckBox();
            CheckBox &operator=(const CheckBox &other);
        public Q_SLOTS:
            Qt::CheckState checkState() const;
            bool isTristate() const;
            void setCheckState(Qt::CheckState state);
            void setTristate(bool y = true);
            void setEnabled(bool enabled);
            virtual QSize minimumSizeHint() const override;
            virtual QSize sizeHint() const override;
        };
    }
}
Q_DECLARE_METATYPE(QtBindings::Gui::CheckBox)
#endif //GUICHECKBOX_H
