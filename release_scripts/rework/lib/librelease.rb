#!/usr/bin/env ruby
#
# Generic ruby library for KDE extragear/playground releases
#
# Copyright (C) 2007 Harald Sitter <harald@getamarok.com>
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

def InformationQuery()
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

  @version  = "2.0.0" #DEBUG
  @protocol = "anonsvn" #DEBUG
  #   CheckoutLocation()
  #   ReleaseVersion()
  #   SvnProtcol()
  SvnUsername()
end


def FetchSource()
  bar  = @dlg.progressbar("fetching source code",1)
  FileUtils.rm_rf( @folder )
  FileUtils.rm_rf( "#{@folder}.tar.bz2" )

  branch = "trunk"

  if @useStable
    branch = "branches/stable"
  elsif @tag and not @tag.empty?()
    branch = "tags/#{NAME}/#{@tag}"
  end

  @repo = "#{@protocol}://#{@user}.kde.org/home/kde/#{branch}"
  puts @repo #DEBUG

  puts "Fetching source from #{branch}...\n\n"
  # TODO: ruby-svn
  `svn co #{@repo}/#{COMPONENT}/#{SECTION}/#{NAME} #{@folder}`

  bar.progress = 1
  bar.close
end


def CreateTar()
  bar  = @dlg.progressbar("creating tarball",4)
  `find -name ".svn" | xargs rm -rf`
  bar.progress = 1
  `tar -cf #{@folder}.tar #{@folder}`
  bar.progress = 2
  `bzip2 #{@folder}.tar`
  bar.progress = 3
  FileUtils.rm_rf(@folder)
  bar.close
end
