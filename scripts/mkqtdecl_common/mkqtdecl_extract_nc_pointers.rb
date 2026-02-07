
# 
# Copyright (C) 2006-2026 Matthias Koefferlein
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

# A helper script to detect non-const pointers 
# (those indicate ownership issues).

# run with
# klayout -r mkqtdecl_extract_nc_pointers.rb -z s

events = {}

classes = {}
RBA::Class::each_class do |cls|
  classes[cls.name] = true
end

RBA::Class::each_class do |cls|

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
      has_ptr = false
      has_int = false
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
          has_ptr = true
          csig += "p"
        end
        if a.type == RBA::ArgType::t_vector
          csig += "a"
          c = uc[a.inner.type].to_s
          csig += c
        elsif a.type == RBA::ArgType::t_object || a.type == RBA::ArgType::t_object_new
          csig += "x"
          csig += a.cls.name
        else
          c = uc[a.type]
          if c == "i" || c == "ui" || c == "l" || c == "ul"
            has_int = true
          end
          csig += c
        end
      end
      if has_ptr && has_int
        puts "#{cls.name}::#{meth.name}_#{csig}"
      end
    end

  end

end

