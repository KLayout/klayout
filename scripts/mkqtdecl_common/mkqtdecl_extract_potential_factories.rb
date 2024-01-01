
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

# A helper script to detect potential factory methods

# run with
# klayout -r mkqtdecl_extract_potential_factories.rb -z s

events = {}

RBA::Class::each_class do |cls|

  if cls.name =~ /_Native$/
    cls.each_method do |meth|
      if meth.ret_type.is_ptr?
        if meth.name =~ /create/i || meth.name =~ /make/i || meth.name =~ /build/i
          puts "#{cls.name}::#{meth.name} -> #{meth.ret_type.to_s}"
        end
      end
    end
  end

end

