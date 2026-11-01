#!/bin/env ruby

input=ARGV[0]
inc=ARGV[1]
mod=ARGV[2]
output=ARGV[3]

files = Dir.glob(File.join(inc, "Q*")).collect { |f| File.basename(f) }.sort { |a,b| a.upcase <=> b.upcase }

ex_files = {}

File.open(input, "r") do |file|
  file.each_line do |line|
    if line =~ /#include <(.*?)\/(.*)>/
      ex_files[$2] = (line =~ /^\s*\/\// ? :comment : :normal)
    end 
  end
end

File.open(output, "w") do |file|
  files.each do |f|
    st = ex_files[f]
    if !st
      file.puts("#include <#{mod}/#{f}>")
    else
      file.puts((st == :comment ? "//" : "") + "#include <#{mod}/#{f}>")
    end 
  end
end

