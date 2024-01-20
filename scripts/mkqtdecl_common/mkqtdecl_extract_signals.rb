
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

# A helper script to extract the signals from the Qt declarations
# It can be used to write the .events configuration file which will 
# declare the signals of objects.

# run with
# klayout -r mkqtdecl_extract_signals.rb -z >mkqtdecl.events

classes = {}
RBA::Class::each_class do |cls|
  classes[cls.name] = true
end

output = $output ? File.open($output, "w") : stdout

RBA::Class::each_class do |cls|

  if cls.name =~ /^Q/ && (cls.name =~ /_Native$/ || !classes[cls.name + "_Native"])

    b = cls
    while b && b.name != "QObject_Native"
      b = b.base
    end

    if b

      mo = eval("RBA::#{cls.name}.staticMetaObject")

      c = cls.name.sub(/_Native/, "")

      valid_sig = {} 
      (0..(mo.methodCount-1)).each do |i|
        mm = mo.method(i)
        if mm.methodType == RBA::QMetaMethod::Signal 
          n = mm.methodSignature.sub(/\(.*/, "")
          s = mm.parameterTypes.join(", ")
          valid_sig[n] ||= []
          found = false
          valid_sig[n] = valid_sig[n].select do |vs| 
            ret = true
            if s.index(vs) == 0
              ret = false
            elsif vs.index(s) == 0
              found = true  
            end
            ret
          end
          if !found
            valid_sig[n].push(s)
          end
        end
      end

      valid_sig.each do |n,ss|
        ss.each do |s|
          match = "::#{n}\\s*\\("
          renamed = nil
          aliased = nil
          if c == "QTabWidget" 
            if n == "currentChanged" && s =~ /QWidget/
              match += ".*QWidget"
              renamed = "currentChanged_qw"
            elsif n == "currentChanged" && s =~ /int/
              match += ".*int"
            end
          elsif c == "QSignalMapper" 
            if n == "mapped" && s =~ /QWidget/
              renamed = "mapped_qw"
              match += ".*QWidget"
            elsif n == "mapped" && s =~ /int/
              match += ".*int"
            elsif n == "mapped" && s =~ /QString/
              renamed = "mapped_qs"
              match += ".*QString"
            elsif n == "mapped" && s =~ /QObject/
              renamed = "mapped_qo"
              match += ".*QObject"
            end
          elsif c == "QComboBox" || c == "QFontComboBox"
            if n == "activated" && s =~ /QString/
              renamed = "activated_qs"
              match += ".*QString"
            elsif n == "activated" && s =~ /int/
              match += ".*int"
            elsif n == "currentIndexChanged" && s =~ /QString/
              renamed = "currentIndexChanged_qs"
              match += ".*QString"
            elsif n == "currentIndexChanged" && s =~ /int/
              match += ".*int"
            elsif n == "highlighted" && s =~ /QString/
              renamed = "highlighted_qs"
              match += ".*QString"
            elsif n == "highlighted" && s =~ /int/
              match += ".*int"
            end
          elsif c == "QCompleter" 
            if n == "activated" && s =~ /QString/
              renamed = "activated_qs"
              match += ".*QString"
            elsif n == "activated" && s =~ /QModelIndex/
              match += ".*QModelIndex"
            elsif n == "highlighted" && s =~ /QString/
              renamed = "highlighted_qs"
              match += ".*QString"
            elsif n == "highlighted" && s =~ /QModelIndex/
              match += ".*QModelIndex"
            end
          elsif c == "QTextBrowser" 
            if n == "highlighted" && s =~ /QString/
              renamed = "highlighted_qs"
              match += ".*QString"
            elsif n == "highlighted" && s =~ /QUrl/
              match += ".*QUrl"
            end
          elsif c == "QDoubleSpinBox" 
            if n == "valueChanged" && s =~ /QString/
              renamed = "valueChanged_qs"
              match += ".*QString"
            elsif n == "valueChanged" && s =~ /double/
              match += ".*double"
            end
          elsif c == "QSpinBox" 
            if n == "valueChanged" && s =~ /QString/
              renamed = "valueChanged_qs"
              match += ".*QString"
            elsif n == "valueChanged" && s =~ /int/
              match += ".*int"
            end
          elsif c == "QButtonGroup" 
            if n == "buttonClicked" && s =~ /QAbstractButton/
              renamed = "buttonClicked_qab"
              match += ".*QAbstractButton"
            elsif n == "buttonClicked" && s =~ /int/
              match += ".*int"
            elsif n == "buttonPressed" && s =~ /QAbstractButton/
              renamed = "buttonPressed_qab"
              match += ".*QAbstractButton"
            elsif n == "buttonPressed" && s =~ /int/
              match += ".*int"
            elsif n == "buttonReleased" && s =~ /QAbstractButton/
              renamed = "buttonReleased_qab"
              match += ".*QAbstractButton"
            elsif n == "buttonReleased" && s =~ /int/
              match += ".*int"
            end
          end
          output.puts "event(\"#{c}\", /#{match}/, \"#{s}\")"
          if renamed
            output.puts "rename(\"#{c}\", /#{match}/, \"#{renamed}\")"
          end
        end
      end
    end

  end

end

if $output
  output.close
end
