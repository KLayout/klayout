
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

# A helper script to create or update the .properties configuration file.
# This file will specify the methods that will be turned into properties
# with a getter/setter pair or single getter/setter function.

# run with
# klayout -r mkqtdecl_extract_props.rb -b >mkqtdecl.properties

classes = {}
RBA::Class::each_class do |cls|
  classes[cls.name] = true
end

output = $output ? File.open($output, "w") : stdout

output.puts "# Properties from Qt meta objects:"

setters_sig = {}
getters_sig = {}

RBA::Class::each_class do |cls|

  if cls.name =~ /^Q/ && (cls.name =~ /_Native$/ || !classes[cls.name + "_Native"])

    b = cls.base
    while b && b.name != "QObject_Native"
      b = b.base
    end

    if b

      mo = eval("RBA::#{cls.name}.staticMetaObject")

      c = cls.name.sub(/_Native$/, "")

      signal_names = {} 
      (0..(mo.methodCount-1)).each do |i|
        mm = mo.method(i)
        if mm.methodType == RBA::QMetaMethod::Signal 
          signal_names[mm.methodSignature.sub(/\(.*/, "")] = true
        end
      end

      valid_sig = {} 
      (0..(mo.propertyCount-1)).each do |i|
        pr = mo.property(i)
        if signal_names[pr.name]
          # ignore properties that clash with signal names (e.g. QCamera::flashReady)
          next
        end
        ucname = pr.name[0..0].upcase + pr.name[1..-1]
        if pr.isReadable
          output.puts "property_reader(\"#{c}\", /::(#{pr.name}|is#{ucname}|has#{ucname})\\s*\\(/, \"#{pr.name}\")"
          getters_sig["#{cls.name}##{pr.name}"] = true
          getters_sig["#{cls.name}#is#{ucname}"] = true
          getters_sig["#{cls.name}#has#{ucname}"] = true
        end
        if pr.isWritable
          output.puts "property_writer(\"#{c}\", /::set#{ucname}\\s*\\(/, \"#{pr.name}\")"
          setters_sig["#{cls.name}#set#{ucname}"] = true
        end
      end

    end

  end

end

output.puts ""
output.puts "# Synthetic properties"

# strip const and references from types
def normalize_type(s)
  if s =~ /^const\s+(.*)$/
    s = $1
  end
  if s =~ /^(.*?)\s*&/
    s = $1
  end
  s
end

RBA::Class::each_class do |cls|

  if cls.name =~ /^Q/ && (cls.name =~ /_Native$/ || !classes[cls.name + "_Native"])

    #  make all methods into properties with the following signatures
    #    setX(x) + x()     -> "setX|x=" + ":x"
    #    setX(x) + isX()   -> "setX|x=" + "isX|:x"
    #    setX(x) + hasX()  -> "setX|x=" + "hasX|:x"
    #
    
    getters = {}
    setters = {}

    c = cls
    while c != nil

      c.each_method do |m| 

        is_setter = false
        is_getter = false
        m.each_overload do |ov|
          is_setter ||= setters_sig["#{c.name}##{ov.name}"]
          is_getter ||= getters_sig["#{c.name}##{ov.name}"]
        end

        if ! is_setter && m.accepts_num_args(1) && m.ret_type.to_s == "void"
          m.each_overload do |ov|
            if ov.name =~ /^set([A-Z])(\w*)$/
              pn = $1.downcase + $2
              setters[pn] ||= [ ov, c, m ]
            end
          end
        end

        if ! is_getter && m.accepts_num_args(0)
          m.each_overload do |ov|
            if ov.name =~ /^is([A-Z])(\w*)$/ || ov.name =~ /^has([A-Z])(\w*)$/ || ov.name =~ /^([a-z])(\w*)$/
              pn = $1.downcase + $2
              getters[pn] ||= [ ov, c, m ]
            end
          end
        end

      end

      c = c.base

    end

    setters.keys.sort.each do |pn|

      s = setters[pn]
      g = getters[pn]

      if g && (s[1] == cls || g[1] == cls)

        a = nil
        s[2].each_argument { |aa| a ||= aa }
        setter_type = normalize_type(a.to_s)
        getter_type = normalize_type(g[2].ret_type.to_s)

        if setter_type == getter_type
          output.puts "# Property #{pn} (#{setter_type})"
          gc = g[1].name.sub(/_Native$/, "")
          sc = s[1].name.sub(/_Native$/, "")
          output.puts "property_reader(\"#{gc}\", /::#{g[0].name}\\s*\\(/, \"#{pn}\")"
          output.puts "property_writer(\"#{sc}\", /::#{s[0].name}\\s*\\(/, \"#{pn}\")"
        end

      end
    end

  end

end

if $output
  output.close
end

