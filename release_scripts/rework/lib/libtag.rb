#!/usr/bin/env ruby
#
# Generic ruby library for KDE extragear/playground releases
#
# Copyright (C) 2008 Harald Sitter <harald@getamarok.com>
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

require 'fileutils'

def TagSource()
  @tag = "#{@protocol}://#{@user}.kde.org/home/kde/tags/#{NAME}/#{@version}" DEBUG

  `svn mkdir -m "Create tag #{NAME} #{@version} root directory" #{@tag}`
  `svn cp -m "Tag #{NAME} #{@version}." #{@repo}/#{COMPONENT}/#{SECTION}/#{NAME} #{@tag}`
end


def TagTranslations()
  `svn co -N #{@tag} ../tagging`

  @tag = "#{@protocol}://#{@user}.kde.org/home/kde/tags/#{NAME}/#{@version}/po"

  FileUtils.cp_r("po/.", "../tagging/po-co", :verbose => true)

  `svn mkdir -m "Create tag #{NAME} #{@version} po directory" #{@tag}`
  `svn up ../tagging/po`
  for translation in @translations do
    `svn mkdir ../tagging/po/#{translation}`
    `svn cp ../tagging/po-co/#{translation}/#{NAME}.po ../tagging/po/#{translation}/#{NAME}.po`
  end
  `svn ci -m "Tag #{NAME} #{@version} - translations." ../tagging/po`

  FileUtils.rm_rf("../tagging")
end


def TagDocumentations()
  @tag = "#{@protocol}://#{@user}.kde.org/home/kde/tags/#{NAME}/#{@version}/doc" DEBUG

  `svn mkdir -m "Create tag #{NAME} #{@version} doc directory" #{@tag}`
#   `svn cp -m "Tag #{NAME} #{@version} - docs." #{@repo}/#{COMPONENT}/#{SECTION}/doc/#{NAME} #{@tag}`
end


def TagDocumentationTranslations()
  puts "no idea what TagDocumentationTranslations() was meant to do??!?"
end


def Tag()
  TagSource()
  TagTranslations()
#   TagDocumentations()
end