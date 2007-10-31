#!/usr/bin/env ruby

# = kdialog.rb
#
# KDE kdialog wrapper class.
#
# Author:: Ed Hames
# Documentation:: Ed Hames
# License::
#   This software is distributed under the terms of the GPL.
# Revision:: kdialog.rb 0.3
#
# See KDialog for Documentation.
#
# For further information, please refer to
# http://developer.kde.org/documentation/tutorials/kdialog/x85.html

#
# == Description
#
# KDialog is a wrapper class for KDE kdialog application.
# kdialog is a simple (easy to use) program which lets you build GUIs for
# your apps and scripts.
# The KDialog class tries to bring that simplicity into your Ruby programs.
#
# == Synopsis
#  require 'kdialog'
#
#  dlg = Kdialog.new "KDialog Test"
#  dlg.yesno 'Do you like this class?'
#  if dlg.selection
#    dlg.msgbox 'You picked YES'
#  else
#    dlg.msgbox 'You picked NO'
#  end
#
#
#  An example by Gabriele Renzi
#
#  dlg = Kdialog.new "KDialog Test"
#  dlg.yesnocancel 'Save and Quit?'
#  if dlg.selection
#    save and quit
#  else
#    resume_main_loop if dlg.selection.nil?
#  end
#
#
# == Todo
# * Handle older versions of kdialog (< 1.0). Might need to handle exceptions. 
# * Complete documentation
#

class KDialog

  YES    = 0 
  NO     = 256
  CANCEL = 512

  @@exit_status = { YES => true, NO => false, CANCEL => nil}

  attr_reader :selection
  attr_writer :miniicon

  def initialize(title='', icon='')
    @dialog = 'kdialog '
    self.backtitle = title
    self.miniicon = icon
    self
  end

  def backtitle=(title)
    @backtitle = title.gsub(/\s/, '\ ')
  end

  def yesno(text)
    exit = perform('--yesno', text)[1]
    @selection = @@exit_status[exit]
  end

  def yesnocancel(text)
    exit = perform('--yesnocancel', text)[1]
    @selection = @@exit_status[exit]
  end

  def warningcontinuecancel(text)
    exit = perform('--warningcontinuecancel', text)[1]
    @selection = @@exit_status[exit]
  end

  def warningyesno(text)
    exit = perform('--warningyesno', text)[1]
    @selection = @@exit_status[exit]
  end

  def warningyesnocancel(text)
    exit = perform('--warningyesnocancel', text)[1]
    @selection = @@exit_status[exit]
  end

  def sorry(text)
    exit = perform('--sorry', text)[1]
    @selection = @@exit_status[exit]
  end

  def error(text)
    exit = perform('--error', text)[1]
    @selection = @@exit_status[exit]
  end

  def msgbox(text)
    exit = perform('--msgbox', text)[1]
    @selection = @@exit_status[exit]
  end

  def inputbox(text, init = '')
    (selection, exit) = perform('--inputbox', text, init.gsub(/\s/, '\ '))
    @selection = @@exit_status[exit] ? selection.chomp : init
  end

  def password(text)
    (selection, exit) = perform('--password', text)
    @selection = @@exit_status[exit] ? selection.chomp : ''
  end

  def textbox(file, width=80, height=25)
    exit = perform('--textbox', '', "#{file} #{width} #{height}")[1]
    @selection = @@exit_status[exit]
  end

  def combobox(text, list)
    args = list.sort.join(' ')
    (selection, exit) = perform('--combobox', text, args)
    @selection = @@exit_status[exit] ? selection : ''
  end

  def menu(text, list)
    args = ''
    list.sort.each{|l| args += " #{l} #{l}"}
    (selection, exit) = perform('--menu', text, args)
    @selection = @@exit_status[exit] ? selection : ''
  end

  def checklist(text, on_list, off_list)
    hash = Hash.new
    on_list.each{|l| hash[l] = " #{l} #{l} on"}
    off_list.each{|l| hash[l] = " #{l} #{l} off"}

    args = ''
    hash.keys.sort.each{|k| args += hash[k]}
    (selection, exit) = perform('--checklist', text, args)
    @selection = @@exit_status[exit] ? selection.delete('"').split(/\s/) : []
  end

  def radiolist(text, list, selected)
    args = ''
    list.sort.each{|l|
      args += " #{l} #{l} " + ((l == selected) ? "on" : "off")
    }
    (selection, exit) = perform('--radiolist', text, args)
    @selection = @@exit_status[exit] ? selection : ''
  end

  def passivepopup(text, timeout)
    exit = perform('--passivepopup', text, timeout)[1]
    @selection = @@exit_status[exit]
  end

  def getopenfilename(dir, filter)
    args = "#{dir} #{filter}"
    (selection, exit) = perform('--getopenfilename', '', args)
    @selection = @@exit_status[exit] ? selection : nil
  end

  def getsavefilename(dir, filter='')
    args = "#{dir} #{filter}"
    (selection, exit) = perform('--getsavefilename', '', args)
    @selection = @@exit_status[exit] ? selection : nil
  end

  def getexistingdirectory(dir)
    (selection, exit) = perform('--getexistingdirectory', dir)
    @selection = @@exit_status[exit] ? selection : nil
  end

  def getopenurl(dir, filter)
    args = "#{dir} #{filter}"
    (selection, exit) = perform('--getopenurl', '', args)
    @selection = @@exit_status[exit] ? selection : nil
  end

  def getsaveurl(dir, filter)
    args = "#{dir} #{filter}"
    (selection, exit) = perform('--getsaveurl', '', args)
    @selection = @@exit_status[exit] ? selection : nil
  end

  def progressbar(text, steps, cancel=false)
    dcop_ref = perform('--progressbar', text, steps)[0]
    bar = ProgressBar.new(dcop_ref, cancel)
  end

  def geticon(group, context)
    args = "#{group} #{context}"
    (selection, exit) = perform('--geticon', '', args)
    @selection = @@exit_status[exit] ? selection : ''
  end

  def perform(cmd, cmd_text, args='')
    dlg_cmd = @dialog
    dlg_cmd += " --title #{@backtitle} " unless @backtitle.empty?
    dlg_cmd += " --miniicon #{@miniicon} " unless @miniicon.empty?
    dlg_cmd += " #{cmd} #{cmd_text.gsub(/\s/, '\ ')} #{args}"
#     puts "Executing: '#{dlg_cmd}'"
    selection = `#{dlg_cmd}`
    [selection, $?.to_i]
  end
  private :perform
end

class ProgressBar
  def initialize(dcop_ref, cancel)
    @dcop_ref = dcop_ref.gsub!(/\(/, '\(').gsub!(/\)/, '\)').chop!
    self.cancel = cancel
  end

  def progress=(value)
    `dcop #{@dcop_ref} setProgress #{value}`
  end

  def label=(label)
    `dcop #{@dcop_ref} setLabel #{label.gsub(/\s/, '\ ')}`
  end

  def cancelled?
    `dcop #{@dcop_ref} wasCancelled`.chop == 'true'
  end

  def cancel=(bool)
    `dcop #{@dcop_ref} showCancelButton #{bool}`
  end

  def close
    `dcop #{@dcop_ref} close`
  end
end

class IconGroup
  Desktop     = "Desktop"
  Toolbar     = "Toolbar"
  MainToolbar = "MainToolbar"
  Small       = "Small"
  Panel       = "Panel"
  User        = "User"
end

class IconContext
  Devices      = "Devices"
  MimeTypes    = "MimeTypes"
  FileSystems  = "FileSystems"
  Applications = "Applications"
  Actions      = "Actions"
end
