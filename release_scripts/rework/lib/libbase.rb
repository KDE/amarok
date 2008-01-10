#!/usr/bin/env ruby
#
# Generic ruby library for KDE extragear/playground releases
#
# Copyright (C) 2007-2008 Harald Sitter <harald@getamarok.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

require './lib/libkdialog.rb'
require 'fileutils'

@dlg = KDialog.new("#{NAME} release script","cookie")

def BaseDir()
  Dir.chdir(BASEPATH + "/" + @folder)
end

def InformationQuery()
  @version  = "2.0.0" #DEBUG
  @protocol = "anonsvn" #DEBUG
  #   CheckoutLocation()
  #   ReleaseVersion()
  #   SvnProtcol()
  SvnUsername()
end


def CheckoutLocation()
  location = @dlg.combobox("Select checkout\\'s place:", "Trunk Stable Tag")
  puts location #DEBUG
  if location == "Stable"
    @useStable = true
  elsif location == "Tag"
    @tag = @dlg.inputbox("Enter the tag name:")
    puts @tag #DEBUG
  end
end

def ReleaseVersion()
  if @tag and not @tag.empty?()
    @version = @tag
  else
    @version = @dlg.inputbox("Enter the release version:")
  end
  puts @version #DEBUG
end

def SvnProtcol()
  @protocol = @dlg.radiolist("Do you use svn+ssh, https or anonsvn :",["svn+ssh","https","anonsvn"],1)
  puts @protocol #DEBUG
end

def SvnUsername()
  if @protocol == "anonsvn"
    @protocol = "svn"
    @user = "anonsvn"
  else
    @user = @dlg.inputbox("Your SVN user:")
    @user += "@svn"
  end
  puts @user #DEBUG
end
