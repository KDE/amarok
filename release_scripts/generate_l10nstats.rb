#!/usr/bin/env ruby
#
# Generate statistics of Amarok's localizations
#
# Copyright (C) 2007 Harald Sitter <sitter.harald@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

require 'fileutils'

if $*.empty?()
    puts "generate_l10nstats.rb [amarokversion]"
    exit 1
end

@version  = $*[0]
@clang    = -2
@cfuzzy   = 0
@cuntrans = 0
@cnotshow = 0
@cper     = 0
@file     = "../amarok-#{@version}.html"

def setmaindir()
    @maindir  = Dir.pwd
end

def calcpercentage( per )
    unless @cper == "0"
        @cper = ((@cper + per) / 2)
    else
        @cper = per
    end
end

# write HTML header part
def header()
    `echo '
        <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"  "http://www.w3.org/TR/html4/loose.dtd"> 
        <html>
        <head>
          <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
          <title>Statistics of Amarok #{@version} translations</title>
        </head>
        <body>
        <a name="__top">
        <p align="center">
        <a name="statistics of amarok #{@version} translations">
        <h1>Statistics of Amarok #{@version} translations</h1><br>
        <table border="1" cellspacing="0"dir="ltr">
        <tr>
          <td align="left" valign="middle" width="60" height="12">
            <font color="#196aff"><i><b>Language</b></i></font>
          </td>
          <td align="center" valign="middle" width="142" height="12">
            <font color="#196aff"><i><b>Fuzzy Strings</b></i></font>
          </td>
          <td align="center" valign="middle" width="168" height="12">
            <font color="#196aff"><i><b>Untranslated Strings</b></i></font>
          </td>
          <td align="center" valign="middle" width="163" height="12">
            <font color="#196aff"><i><b>All Not Shown Strings</b></i></font>
          </td>
          <td align="center" valign="middle" width="163" height="12">
            <font color="#196aff"><i><b>Translated %</b></i></font>
          </td>
        </tr>' > #{@file}`
end

# write HTML footer part
def footer()
    `echo "
        <tr>
        <td align="left" valign="middle" width="60" height="12">
        <u><i><b>#{@clang}</b></i></u>
        </td>
        <td align="center" valign="middle" width="142" height="12">
        <u><i><b>
        #{if @cfuzzy == "0" then
            "0"
          else
            @cfuzzy
          end}
        </b></i></u>
        </td>
        <td align="center" valign="middle" width="168" height="12">
        <u><i><b>
        #{if @cuntrans == "0" then
            "0"
          else
            @cuntrans
          end}
        </b></i></u>
        </td>
        <td align="center" valign="middle" width="163" height="12">
            <u><i><b>
                #{if @cnotshow == "0" then
                    "0"
                else
                    @cnotshow
                end}
            </b></i></u>
        </td>
        <td align="center" valign="middle" width="163" height="12">
            <u><i><b>
                #{if @cper == "0" then
                    "0"
                else
                    @cper.to_s + " %"
                end}
            </b></i></u>
        </td>
        </tr>" >> #{@file}`
end

def stats( lang )
  values = nil

  if lang != "." and lang != ".." then
        Dir.chdir("po/#{lang}")

        # grab statistics data
        `msgfmt --statistics amarok.po 2> tmp.txt`
        term = `cat tmp.txt`
        File.delete("tmp.txt")

        # rape the data and create some proper variables
        values  = term.scan(/[\d]+/)
        notshow = values[1].to_i + values[2].to_i
        all     = values[0].to_i + values[1].to_i + values[2].to_i
        show    = all - notshow
        per     = ((100.0 * show.to_f) / all.to_f)

        # assign font colors according to translation status
        # TODO: replace with case -> point out how to do with relational operators
        if per == 100 then
            fcolor = "#00B015" #green
        elsif per >= 95 then
            fcolor = "#FF9900" #orange
        elsif per >= 75 then
            fcolor = "#6600FF" #blue
        elsif per >= 50 then
            fcolor = "#000000" #black
        else
            fcolor = "#FF0000" #red
        end

        Dir.chdir(@maindir)

    `echo "
        <tr>
            <td align="left" valign="middle" width="60" height="12">
                <font color="#{fcolor}">
                #{lang}
                </font>
            </td>
            <td align="center" valign="middle" width="142" height="12">
                <font color="#{fcolor}">
                #{if values[1] == nil then
                    "0"
                else
                    values[1]
                end}
                </font>
            </td>
            <td align="center" valign="middle" width="168" height="12">
                <font color="#{fcolor}">
                #{if values[2] == nil then
                    "0"
                else
                    values[2]
                end}
                </font>
            </td>
            <td align="center" valign="middle" width="163" height="12">
                <font color="#{fcolor}">
                #{if notshow == nil then
                    "0"
                else
                    notshow
                end}
                </font>
            </td>
            <td align="center" valign="middle" width="163" height="12">
                <font color="#{fcolor}">
                #{if per == 0 then
                    "0 %"
                else
                    per.to_i.to_s + " %"
                end}
                </font>
            </td>
        </tr>" >> #{@file}`

    # update countin variables
    @cfuzzy   += values[1].to_i
    @cuntrans += values[2].to_i
    @cnotshow += notshow.to_i
    @clang    += 1
    calcpercentage( per.to_i )

  end
end

puts "Untaring the tarball..."
    `tar -xf amarok-#{@version}.tar.bz2`
        Dir.chdir("amarok-#{@version}")
        File.delete("po/Makefile.am", "po/Makefile.in")
        setmaindir()

puts "Writing the header..."
    header()

puts "Writing the statistics..."
    langs = Dir.entries("po")
    Dir.foreach("po") {|lang|
        stats( lang )
    }

puts "Writing the footer..."
    footer()

puts "Cleaning up..."
    Dir.chdir( "../" )
    FileUtils.rm_r( "amarok-#{@version}" )

puts "Generation finished..."

exit 0