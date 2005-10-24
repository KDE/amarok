//Copyright: (C) 2005 Robert Gogolok
//License:   See COPYING


#ifndef NMMCONFIGDIALOG_H
#define NMMCONFIGDIALOG_H

#include "nmm_configdialogbase.h"
#include "plugin/pluginconfig.h"

#include "qobject.h"

class QListBox;

class NmmConfigDialog : public amaroK::PluginConfig
{
Q_OBJECT
    public:
        NmmConfigDialog();
        ~NmmConfigDialog();
        
        QWidget* view() { return m_view; }

        bool hasChanged() const;

        bool isDefault() const;
        
    public slots:
        void save();
        void addAudioHost();
        void removeAudioHost();
        void addVideoHost();
        void removeVideoHost();

        void setCheckedAudioList( bool );
        void setCheckedVideoList( bool );

    private:
        /**
         * Returns hosts in the audio/video host list.
         */
        QStringList hostlist( QListBox* listBox ) const;

        void addHost( QListBox *listBox );
        
        /**
         * Removes current item from QListBox.
         */
        void removeCurrentHost( QListBox* listBox );



        NmmConfigDialogBase* m_view;
};

#endif
