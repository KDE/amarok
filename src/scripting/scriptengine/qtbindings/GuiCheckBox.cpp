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

#include "GuiCheckBox.h"

using namespace QtBindings::Gui;

CheckBox::CheckBox(QWidget *parent) : QCheckBox(parent)
{
}

CheckBox::CheckBox(const CheckBox &other) : QCheckBox(qobject_cast<QWidget*>( other.parent() ))
{
    *this = other;
}

CheckBox::CheckBox(const QString &text, QWidget *parent) : QCheckBox(
        text, parent)
{
}

CheckBox::~CheckBox()
{
}

Qt::CheckState CheckBox::checkState() const
{
    return QCheckBox::checkState();
}

bool CheckBox::isTristate() const
{
    return QCheckBox::isTristate();
}

void CheckBox::setCheckState(Qt::CheckState state)
{
    QCheckBox::setCheckState(state);
}

void CheckBox::setTristate(bool y)
{
    QCheckBox::setTristate(y);
}

void CheckBox::setEnabled(bool enabled)
{
    QCheckBox::setEnabled(enabled);
}

QSize CheckBox::minimumSizeHint() const
{
    return QCheckBox::minimumSizeHint();
}

QSize CheckBox::sizeHint() const
{
    return QCheckBox::sizeHint();
}

CheckBox &CheckBox::operator=(const CheckBox &other)
{
    if (this != &other) {
       this->setCheckState( other.checkState() );
       this->setTristate( other.isTristate() );
       this->setEnabled( other.isEnabled() );
    }
    return *this;
}

