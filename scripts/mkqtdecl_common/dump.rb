#!/usr/bin/env ruby

# 
# Copyright (C) 2006-2024 Matthias Koefferlein
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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

$:.push(File.dirname($0))

require 'oj'
require 'cpp_classes.rb'
require 'reader_ext.rb'

input_file = "all.db"
conf_file = "mkqtdecl.conf"
cls_list = nil
$gen_dir = "generated"

while ARGV.size > 0
  o = ARGV.shift
  if o == "-i"
    input_file = ARGV.shift
  else
    raise("Invalid option #{o} - usage is 'dump.rb -i all.db'")
  end
end

File.open(input_file, "r") do |file|

  json = file.read

  @root = Oj.load(json)
  puts "Reading done."

  puts @root.dump 

end

