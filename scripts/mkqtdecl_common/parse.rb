#!/usr/bin/env ruby

# 
# Copyright (C) 2006-2025 Matthias Koefferlein
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

# Usage
#   parse.rb -i prep.cpp -o dump.json
#
# prep.cpp: preprocessed .cpp file
# dump.json: output dump

$:.push(File.dirname($0))

require 'oj'
require 'treetop'

Treetop.load File.join(File.dirname($0), "c++")

input="all.e"
output="all.db"

while ARGV.size > 0
  o = ARGV.shift
  if o == "-i"
    input = ARGV.shift
  elsif o == "-o"
    output = ARGV.shift
  else
    raise("Invalid option #{o} - usage is 'parse.rb -i prep.cpp -o dump.json'")
  end
end

input || raise("No input given (use -i)")
output || raise("No output given (use -o)")

text = nil
File.open(input, "r") do |file|
  text = file.read
end

parser = CPPParser.new
parser.root = :module
p = parser.parse(text)

if p
  puts "Input #{input} successfully read."
  File.open(output, "w") do |file|
    file.puts(Oj.dump(p.cpp, :mode => :object))
  end
  puts "Output file #{output} written."
else
  puts "Failure in line: "+parser.failure_line.to_s
  puts parser.failure_reason
end

