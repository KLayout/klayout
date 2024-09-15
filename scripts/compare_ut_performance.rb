#!/bin/ruby 

require "nokogiri"

# Collect files from command line

files = []
time_class = :wall
sort_key = :name

ARGV.each do |arg|
  if arg =~ /^--help|-h/
    puts <<"END"
#{$0} [options] <file1> <file2> ...

The files are XML files produced by "ut_runner" with the -a option.

Options are:

  -w     Use wall time (default)
  -u     Use user time
  -s     Sort by average time, lowest first
  +s     Sort by average time, largest first

The script reads these files are compares performance (user and wall times)
of the different tests.
END
    exit(0)
  elsif arg == "-w"
    time_class = :wall
  elsif arg == "-u"
    time_class = :user
  elsif arg == "-s"
    sort_key = :time_up
  elsif arg == "+s"
    sort_key = :time_down
  elsif arg =~ /^-/
    puts("*** ERROR: unknown option #{arg}. Use -h for help.")
    exit(1)
  else
    files << arg
  end
end


# A class representing the data from one test

class TestData

  def initialize(file)

    @file = file
    @data = {}

    File.open(file) do |f|

      doc = Nokogiri::XML(f)

      doc.xpath("//testsuite").each do |testsuite|
        ts_name = testsuite.at_xpath("@name").content
        testsuite.xpath("testcase").each do |testcase|
          tc_name = testcase.at_xpath("@name").content
          times = testcase.at_xpath("x-testcase-times")
          if times
            wall_time = times.at_xpath("@wall").content.to_f
            user_time = times.at_xpath("@user").content.to_f
            @data[ [ts_name, tc_name] ] = [ wall_time, user_time ]
          end
        end
      end

    end

  end

  def file
    @file
  end

  def keys
    @data.keys
  end

  def times(key)
    @data[key]
  end

end 


# Read the tests

tests = []
files.each do |f|
  puts("Reading test file #{f} ..")
  tests << TestData::new(f)
end

puts "Reading done."
puts ""


# Build the comparison table

all_tests = {}

tests.each_with_index do |test,index|
  test.keys.each do |k|
    all_tests[k] ||= [nil] * tests.size
    all_tests[k][index] = test.times(k)
  end
end


# print the result

tests.each_with_index do |test,index|
  puts "(#{index + 1}) #{test.file}"
end

puts ""

time_index = 0
if time_class == :wall
  puts "Wall times"
elsif time_class == :user
  time_index = 1
  puts "User times"
end

puts ""

l1 = all_tests.keys.collect { |k| k[0].size }.max
l2 = all_tests.keys.collect { |k| k[1].size }.max

fmt = "%-#{l1}s  %-#{l2}s  " + (["%15s"] * tests.size).join(" ") + " %15s %15s %10s"

title = fmt % ([ "Testsuite", "Test", ] + tests.each_with_index.collect { |t,i| "(#{i + 1})" } + [ "Min", "Max", "Delta" ])
puts title
puts "-" * title.size

total = [0.0] * tests.size

lines = []

all_tests.keys.sort { |a,b| a <=> b }.each do |k|

  times = all_tests[k].collect { |t| t && t[time_index] }

  min = max = delta = nil
  if ! times.index(nil)
    times.each_with_index do |t,i|
      total[i] += t
    end
    min = times.min
    max = times.max
    if times.size > 1 && (max + min).abs > 1.0
      delta = (max - min) / 0.5 / (max + min) 
    end
  end

  line = fmt % (k + times.collect { |t| t ? ("%.6f" % t) : "" } + [ min ? "%.6f" % min : "", max ? "%.6f" % max : "", delta ? "%.2f%%" % (delta * 100) : ""])

  if sort_key == :time_up
    lines << [ min && max ? min + max : 0.0, line ]
  elsif sort_key == :time_down
    lines << [ min && max ? -(min + max) : 0.0, line ]
  else
    lines << [ k, line ]
  end

end  

lines.sort { |a,b| a[0] <=> b[0] }.each do |k,line|
  puts line
end


# Add total row

min = total.min
max = total.max
delta = nil
if total.size > 1 && (max + min).abs > 1.0
  delta = (max - min) / 0.5 / (max + min) 
end

puts ""
puts fmt % ([ "Total" , "" ] + total.collect { |t| t ? ("%.6f" % t) : "" } + [ min ? "%.6f" % min : "", max ? "%.6f" % max : "", delta ? "%.2f%%" % (delta * 100) : ""])


