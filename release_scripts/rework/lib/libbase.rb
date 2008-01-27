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

require 'lib/libkdialog'
require 'fileutils'

@dlg = KDialog.new("#{NAME} release script","cookie")

# This will return you to the default execution directory, which is by default:
def BaseDir()
  Dir.chdir(BASEPATH)
end

# This will return you to the default src directory, which is by default:
#  execution dir + / + src folder name (NAME-VERSION)
def SrcDir()
  Dir.chdir(BASEPATH + "/" + @folder)
end


# Queries the executer for all sorts of so called information.
# By default it will query for:
#  - SVN location to checkout (trunk, stable, tag)
#  - Release version
#  - SVN protcol to use (ssh, https, anonsvn)
#  - If protocl is not 'anonsvn' it will also ask for a user name
#
# You can override the query by providing these information when calling the method.
# For example:
#    InformationQuery("trunk","1.0.0","https","sitter")
def InformationQuery(location=nil, version=nil, protocol=nil, user=nil)
#     @version  = "2.0.0" #DEBUG.
#     @protocol = "anonsvn" #DEBUG

  unless location
    CheckoutLocation()
  else
    CheckoutLocationDef(location)
  end

  unless version
    ReleaseVersion()
  else
    @version = version
  end

  unless protocol
    SvnProtcol()
  else
    @protocol = protocol
  end

  unless user
    SvnUsername()
  else
    @user = user + "@svn"
  end
end

private
def CheckoutLocation()
  location = @dlg.combobox("Select checkout\\'s place:", "Trunk Stable Tag")
  puts location #DEBUG
  CheckoutLocationDef(location)
end

def CheckoutLocationDef(location)
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
