/***************************************************************************
 file                 : artsengine.cpp - aRts audio interface
 begin                : Dec 31 2003
 copyright            : (C) 2003 Mark Kretschmann <markey@web.de>
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#if 0

class ArtsEffects : public Engine::Effects
{
    public:
        QStringList                              availableEffects() const;
        std::vector<long>                        activeEffects() const;
        QString                                  effectNameForId( long id ) const;
        bool                                     effectConfigurable( long id ) const;
        long                                     createEffect( const QString& name );
        void                                     removeEffect( long id );
        void                                     configureEffect( long id );
};


class ArtsConfigWidget : public QWidget
{
    public:
                                            ArtsConfigWidget( Arts::Object object );
                                            ~ArtsConfigWidget();
    private:
        Arts::Widget                     m_gui;
        KArtsWidget                      *m_pArtsWidget;
};

struct EffectContainer
{
    Arts::StereoEffect*                  effect;
    QGuardedPtr<ArtsConfigWidget>        widget;
};


#endif
