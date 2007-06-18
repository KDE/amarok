#!/usr/bin/env ruby
#
#
# Form implementation generated from reading ui file 'selector.ui'
#
# Created: Fri Dec 2 23:40:46 2005
#      by: The QtRuby User Interface Compiler (rbuic)
#
# WARNING! All changes made in this file will be lost!
#
# Ruby script for generic amarok database scripts
# (c) 2005 Seb Ruiz <me@sebruiz.net>
# Released under the GPL v2 license

begin
    require 'Korundum'
rescue LoadError
    error = 'Korundum (KDE bindings for ruby) from kdebindings v3.4 is required for this script.'
    system("dcop", "amarok", "playlist", "popupMessage", "DatabaseScripts: #{error}")
    exit
end

class DatabaseScriptChooser < Qt::Dialog

    attr_reader :m_optionCombo
    attr_reader :m_saveText
    attr_reader :m_saveDir
    attr_reader :m_okayButton

    slots 'optionChanged(int)', 'textChanged(const QString &)', 'accept()', 'cancel()'

    def initialize(parent = nil, name = nil, modal = false, fl = 0)
        super

        if name.nil?
            setName("Database Script Chooser")
        end

        @Form1Layout = Qt::GridLayout.new(self, 1, 1, 2, 2, 'Form1Layout')

        @layout3 = Qt::VBoxLayout.new(nil, 0, 2, 'layout3')

        @m_optionCombo = Qt::ComboBox.new(false, self, "m_optionCombo")
        @layout3.addWidget(@m_optionCombo)

        @layout1 = Qt::HBoxLayout.new(nil, 0, 2, 'layout1')

        @m_saveText = Qt::Label.new(self, "m_saveText")
        @layout1.addWidget(@m_saveText)

        @m_saveDir = KDE::URLRequester.new(self, "m_saveDir")
        @m_saveDir.setMode( KDE::File::Directory | KDE::File::ExistingOnly );
        @m_saveDir.setURL( ENV["HOME"] )

        @layout1.addWidget(@m_saveDir)
        @layout3.addLayout(@layout1)
        @spacer1 = Qt::SpacerItem.new(20, 21, Qt::SizePolicy::Minimum, Qt::SizePolicy::Expanding)
        @layout3.addItem(@spacer1)

        @layout2 = Qt::HBoxLayout.new(nil, 0, 2, 'layout2')

        @m_cancelButton = Qt::PushButton.new(self, "@m_cancelButton")
        @layout2.addWidget(@m_cancelButton)

        @spacer2 = Qt::SpacerItem.new(61, 20, Qt::SizePolicy::Expanding, Qt::SizePolicy::Minimum)
        @layout2.addItem(@spacer2)

        @m_okayButton = Qt::PushButton.new(self, "m_okayButton")
        @layout2.addWidget(@m_okayButton)
        @layout3.addLayout(@layout2)

        connect( @m_optionCombo,  SIGNAL( "activated(int)" ), self, SLOT( "optionChanged(int)" ) );
        connect( @m_okayButton,   SIGNAL( "clicked()" ),      self, SLOT( "accept()" ) )
        connect( @m_cancelButton, SIGNAL( "clicked()" ),      self, SLOT( "cancel()" ) )

        connect( @m_saveDir, SIGNAL( "textChanged(const QString &)" ),
                 self,       SLOT( "textChanged(const QString &)" ) );

        @Form1Layout.addLayout(@layout3, 0, 0)
        languageChange()

        resize( Qt::Size.new(356, 137).expandedTo(minimumSizeHint()) )
        clearWState( WState_Polished )
    end

    def optionChanged( i )
        @m_saveDir.setEnabled( i == 0 )
        @m_saveText.setEnabled( i == 0 )
    end

    def textChanged(s)
        @m_okayButton.setEnabled( !s.empty?() )
    end

    def accept()
        arg = ""
        case @m_optionCombo.currentItem()
            when 0 # Backup
                filename = File.dirname( File.expand_path( __FILE__ ) ) + "/backupDatabase.rb"
                arg      = @m_saveDir.url()

            when 1 # Optimise
                filename = File.dirname( File.expand_path( __FILE__ ) ) + "/staleStatistics.rb"
        end

        system("ruby", filename, arg)

        done( 0 )
    end

    def cancel()
        done( 0 )
    end


    #
    #  Sets the strings of the subwidgets using the current
    #  language.
    #
    def languageChange()
        setCaption( trUtf8("Database Scripts") )
        @m_optionCombo.clear()

        # add combo box items
        @m_optionCombo.insertItem( trUtf8("Backup Database") )
        @m_optionCombo.insertItem( trUtf8("Optimise Database") )

        @m_saveText.setText( trUtf8("Save location:") )
        @m_cancelButton.setText( trUtf8("Cancel") )
        @m_okayButton.setText( trUtf8("Go!") )
    end
    protected :languageChange


end

if $0 == __FILE__
    about = KDE::AboutData.new("databaseScriptChooser", "DatabaseScriptChooser", "0.1")
    KDE::CmdLineArgs.init(ARGV, about)
    a = KDE::Application.new
    w = DatabaseScriptChooser.new
    a.mainWidget = w
    w.show
    a.exec
end
