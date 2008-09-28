#!/usr/bin/env ruby
#
# Copyright (C) 2008 Harald Sitter <harald@getamarok.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License or (at your option) version 3 or any later version
# accepted by the membership of KDE e.V. (or its successor approved
# by the membership of KDE e.V.), which shall act as a proxy
# defined in Section 14 of version 3 of the license.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

require 'Qt4'

class MyLineEdit < Qt::LineEdit
    slots 'setStatus(int)'
    def initialize(parent = nil)
        super()
        self.setDisabled(true)
    end

    def setStatus(int)
        if int == 0
            self.setDisabled(true)
        else
            self.setDisabled(false)
        end
    end
end

class MyCheckBox < Qt::CheckBox
    slots 'setStatusApp(int)', 'setStatusOption(int)'
    def initialize(parent = nil)
        super()
    end

    def setStatusApp(int)
        if int == 1
            self.setChecked(false)
            self.setDisabled(true)
        else
            self.setDisabled(false)
        end
    end

    def setStatusOption(int)
        if int == 0
            self.setChecked(false)
            self.setDisabled(true)
        elsif int == 2
            self.setChecked(true)
            self.setDisabled(false)
        end
    end
end

class MyWidget < Qt::Widget
      slots 'invoke()', 'progressing()'
    def initialize()
        super()

        grid = Qt::GridLayout.new()

        infoFrame = Qt::Frame.new()
        infoFrame.setFrameShape(Qt::Frame::StyledPanel)
        infoFrame.setFrameShadow(Qt::Frame::Raised)
        infoGrid = Qt::GridLayout.new(infoFrame)

        header = Qt::Label.new('Please Release:')
        header.setFont(Qt::Font.new('Sans', 18, Qt::Font::Bold))
        infoGrid.addWidget(header,0,0,1,2,Qt::AlignRight)

        @appBox = Qt::ComboBox.new()
        @appBox.addItem('Amarok')
        @appBox.addItem('Digikam')
        infoGrid.addWidget(@appBox,0,2,1,-1)

        @coLabel = Qt::Label.new('Checkout Location')
        @coBox = Qt::ComboBox.new()
        @coBox.addItem('trunk')
        @coBox.addItem('stable')
        @coBox.addItem('tag')
        infoGrid.addWidget(@coLabel,1,0)
        infoGrid.addWidget(@coBox,1,1)

        @versionLabel = Qt::Label.new('Version Number')
        @versionText = Qt::LineEdit.new()
        infoGrid.addWidget(@versionLabel,2,0)
        infoGrid.addWidget(@versionText,2,1)

        @accessLabel = Qt::Label.new('SVN Access')
        @accessBox = Qt::ComboBox.new()
        @accessBox.addItem('anonsvn')
        @accessBox.addItem('https')
        @accessBox.addItem('svn+ssh')
        infoGrid.addWidget(@accessLabel,1,3)
        infoGrid.addWidget(@accessBox,1,4)

        @userLabel = Qt::Label.new('User')
        @userText = MyLineEdit.new()
        infoGrid.addWidget(@userLabel,2,3)
        infoGrid.addWidget(@userText,2,4)

        optionGroup = Qt::GroupBox.new('Stuff to do:')
        optionGrid = Qt::GridLayout.new(optionGroup)

        @sourceCheck = Qt::CheckBox.new('Get Source')
        @sourceCheck.setChecked(true)
        @sourceCheck.setDisabled(true)
        optionGrid.addWidget(@sourceCheck,0,0)

        @transCheck = Qt::CheckBox.new('Get Translations')
        @transCheck.setChecked(true)
        optionGrid.addWidget(@transCheck,0,1)

        @docCheck = MyCheckBox.new
        @docCheck.setText('Get Documentation')
        @docCheck.setChecked(false)
        optionGrid.addWidget(@docCheck,1,0)

        line = Qt::Frame.new()
        line.setFrameShape(Qt::Frame::HLine)
        line.setFrameShadow(Qt::Frame::Sunken)
        optionGrid.addWidget(line,2,0,1,2)

        @transStatCheck = MyCheckBox.new()
        @transStatCheck.setText('Create Translation Statistics')
        @transStatCheck.setChecked(true)
        optionGrid.addWidget(@transStatCheck,3,0)

        @tagCheck = Qt::CheckBox.new('Create Tag')
        @tagCheck.setChecked(true)
        optionGrid.addWidget(@tagCheck,3,1)

        @specCheck = Qt::CheckBox.new('Apply Specific Changes')
        @specCheck.setChecked(true)
        optionGrid.addWidget(@specCheck,4,0)

        @tarCheck = Qt::CheckBox.new('Create Tarball')
        @tarCheck.setChecked(true)
        optionGrid.addWidget(@tarCheck,4,1)

        connect(@appBox, SIGNAL('currentIndexChanged(int)'), @docCheck, SLOT('setStatusApp(int)'))
        connect(@transCheck, SIGNAL('stateChanged(int)'), @transStatCheck, SLOT('setStatusOption(int)'))
        connect(@accessBox, SIGNAL('currentIndexChanged(int)'), @userText, SLOT('setStatus(int)'))

#         @overallprogress = Qt::ProgressBar.new()
#         @overallprogress.hide()
# 
#       oProgressHeader = Qt::Label.new('Overall Progress')
#       oProgressHeader.setAlignment(Qt::AlignHCenter)
#       oProgressHeader.hide()

        @button = Qt::PushButton.new('Start')
#         connect(button, SIGNAL('clicked()'), button, SLOT('hide()'))
        connect(@button, SIGNAL('clicked()'), self, SLOT('invoke()'))
#       connect(button, SIGNAL('clicked()'), @overallprogress, SLOT('show()'))
#       connect(button, SIGNAL('clicked()'), oProgressHeader, SLOT('show()'))
#         connect(button, SIGNAL('clicked()'), self, SLOT('progressing()'))

        grid.addWidget(infoFrame)
        grid.addWidget(optionGroup)
        grid.addWidget(@button)
#         grid.addWidget(oProgressHeader)
#         grid.addWidget(@overallprogress)
#         layout = Qt::VBoxLayout.new()
#         layout.addLayout(infoGrid)
# 	layout.addWidget(button)
#         layout.addWidget(oProgressHeader)
# 	layout.addWidget(@overallprogress)
        setLayout(grid)
    end

    def progressing()
        count = 1
        while count <= 100
            @overallprogress.setValue(count)
            count += 1
        end
    end

    def invoke()
        window.hide()

        case @appBox.currentText()
            when "Amarok" then
                require 'release_amarok2.rb'
            when "Digikam" then
                require 'digikam.rb'
        end

        informationQuery(@coBox.currentText, @versionText.text, @accessBox.currentText, @userText.text)

        @folder = "#{NAME}-#{@version}"

        if @sourceCheck.checkState() == 2
            fetchSource()
        end

        if @transCheck.checkState() == 2
            fetchTranslations()
        end

        if @docCheck.checkState() == 2
            fetchDocumentation()
        end

        if @transStatCheck.checkState() == 2
            createTranslationStats()
        end

        if @tagCheck.checkState() == 2
            createTag()
        end

        if @specCheck.checkState() == 2
                case @appBox.currentText()
                when "Amarok" then
                    amarok()
                when "Digikam" then
                    digikam()
            end
        end

        if @tarCheck.checkState() == 2
            createTar()
            createCheckSums()
        end

        quit = Qt::PushButton.new('Quit')
        quit.resize(400, 30)
        quit.setWindowIcon(Qt::Icon.new('icons/16.png'))
        quit.setWindowTitle('....<<<<~~| RELEASE |~~>>>>....')
        connect(quit, SIGNAL('clicked()'), $qApp, SLOT('quit()'))
        quit.show()
    end
end

app = Qt::Application.new(ARGV)

window = MyWidget.new()
window.setWindowIcon(Qt::Icon.new('icons/16.png'))
window.setWindowTitle('....<<<<~~| RELEASE |~~>>>>....')

window.show()
app.exec()
