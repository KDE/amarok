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

#ifndef GUIDIALOGBUTTONBOX_H
#define GUIDIALOGBUTTONBOX_H

#include "QtBinding.h"

#include <QDialogButtonBox>

namespace QtBindings
{
    namespace Gui
    {
        class DialogButtonBox : public QDialogButtonBox, public QtBindings::Base<DialogButtonBox>
        {
            Q_OBJECT
        public:
            enum ButtonRole {
                InvalidRole = -1,
                AcceptRole,
                RejectRole,
                DestructiveRole,
                ActionRole,
                HelpRole,
                YesRole,
                NoRole,
                ResetRole,
                ApplyRole,
                NRoles
            };
            Q_ENUM(ButtonRole)

            enum StandardButton {
                NoButton           = 0x00000000,
                Ok                 = 0x00000400,
                Save               = 0x00000800,
                SaveAll            = 0x00001000,
                Open               = 0x00002000,
                Yes                = 0x00004000,
                YesToAll           = 0x00008000,
                No                 = 0x00010000,
                NoToAll            = 0x00020000,
                Abort              = 0x00040000,
                Retry              = 0x00080000,
                Ignore             = 0x00100000,
                Close              = 0x00200000,
                Cancel             = 0x00400000,
                Discard            = 0x00800000,
                Help               = 0x01000000,
                Apply              = 0x02000000,
                Reset              = 0x04000000,
                RestoreDefaults    = 0x08000000,
            };
            Q_FLAG(StandardButtons)

            enum ButtonLayout {
                WinLayout,
                MacLayout,
                KdeLayout,
                GnomeLayout
            };
            Q_ENUM(ButtonLayout)

            Q_INVOKABLE DialogButtonBox(QWidget *parent = Q_NULLPTR);
            Q_INVOKABLE DialogButtonBox(const DialogButtonBox &other);
            Q_INVOKABLE ~DialogButtonBox();
            Q_INVOKABLE DialogButtonBox &operator=(const DialogButtonBox &other);
        };
    }
}
Q_DECLARE_METATYPE(QtBindings::Gui::DialogButtonBox)
#endif //GUIDIALOGBUTTONBOX_H
