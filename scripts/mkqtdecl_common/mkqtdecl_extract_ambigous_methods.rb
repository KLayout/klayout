
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

# A helper script to detect ambiguous methods

# run with
# klayout -r mkqtdecl_extract_ambigous_methods.rb -z s

events = {}

classes = {}
RBA::Class::each_class do |cls|
  classes[cls.name] = true
end

RBA::Class::each_class do |cls|

  tc = {
    RBA::ArgType.t_void => "",
    RBA::ArgType.t_bool => "b",
    RBA::ArgType.t_char => "i",
    RBA::ArgType.t_schar => "i",
    RBA::ArgType.t_uchar => "i",
    RBA::ArgType.t_short => "i",
    RBA::ArgType.t_ushort => "i",
    RBA::ArgType.t_int => "i",
    RBA::ArgType.t_uint => "i",
    RBA::ArgType.t_long => "i",
    RBA::ArgType.t_ulong => "i",
    RBA::ArgType.t_longlong => "i",
    RBA::ArgType.t_ulonglong => "i",
    RBA::ArgType.t_double => "d",
    RBA::ArgType.t_float => "d",
    RBA::ArgType.t_var => "v",
    RBA::ArgType.t_string => "s",
    RBA::ArgType.t_qstring => "s",
    RBA::ArgType.t_string_ccptr => "s",
  }

  uc = {
    RBA::ArgType.t_void => "",
    RBA::ArgType.t_bool => "b",
    RBA::ArgType.t_char => "c",
    RBA::ArgType.t_schar => "c",
    RBA::ArgType.t_uchar => "uc",
    RBA::ArgType.t_short => "s",
    RBA::ArgType.t_ushort => "us",
    RBA::ArgType.t_int => "i",
    RBA::ArgType.t_uint => "ui",
    RBA::ArgType.t_long => "l",
    RBA::ArgType.t_ulong => "ul",
    RBA::ArgType.t_longlong => "w",
    RBA::ArgType.t_ulonglong => "uw",
    RBA::ArgType.t_double => "d",
    RBA::ArgType.t_float => "f",
    RBA::ArgType.t_var => "v",
    RBA::ArgType.t_string => "s",
    RBA::ArgType.t_qstring => "qs",
    RBA::ArgType.t_string_ccptr => "t",
  }

  if cls.name =~ /_Native$/ || !classes[cls.name + "_Native"]

    m = {}
    cls.each_method do |meth|
      m[meth.name] ||= {}
      csig = ""
      if meth.is_static?
        csig += "s"
      end
      if meth.is_const?
        csig += "c"
      end
      if csig != ""
        csig += "#"
      end
      meth.each_argument do |a|
        if a.is_ptr? || a.is_cptr? 
          csig += "p"
        end
        if a.type == RBA::ArgType::t_vector
          csig += "a"
          csig += tc[a.inner.type].to_s
        elsif a.type == RBA::ArgType::t_object || a.type == RBA::ArgType::t_object_new
          csig += "x"
          csig += a.cls.name
        else
          csig += tc[a.type]
        end
      end
      ((m[meth.name] ||= {})[csig] ||= []).push(meth)
    end

    m.each do |n,cs|
      cs.each do |csig,m|
        if m.size > 1
          puts "Ambigous: #{cls.name}::#{n}_#{csig}"
          usigs = []
          m.each do |mm|
            usig = ""
            mm.each_argument do |a|
              if a.is_ptr? || a.is_cptr? 
                usig += "p"
              end
              if a.type == RBA::ArgType::t_vector
                usig += "a"
                usig += uc[a.inner.type].to_s
              elsif a.type == RBA::ArgType::t_object || a.type == RBA::ArgType::t_object_new
                usig += "x"
                usig += a.cls.name
              else
                usig += uc[a.type]
              end
            end
            usigs.push(usig)
          end
          puts "-> #{usigs.join(',')}"
        end
      end
    end

  end

end

