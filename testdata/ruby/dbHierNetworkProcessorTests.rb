# encoding: UTF-8

# KLayout Layout Viewer
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

if !$:.member?(File::dirname($0))
  $:.push(File::dirname($0))
end

load("test_prologue.rb")

class DBHierNetworkProcessor_TestClass < TestBase

  # Connectivity
  def test_1_Connectivity

    conn = RBA::Connectivity::new

    assert_equal(conn.to_s, "")

    conn.connect(1)
    assert_equal(conn.to_s, "1:1")

    conn.connect(0, 1)
    assert_equal(conn.to_s, "0:1\n1:0,1")

    conn.soft_connect(0, 2)
    assert_equal(conn.to_s, "0:1,2-S\n1:0,1\n2:0+S")

    gid1 = conn.connect_global(0, "GLOBAL1")
    assert_equal(gid1, 0)
    assert_equal(conn.global_net_name(gid1), "GLOBAL1")
    assert_equal(conn.global_net_id("GLOBAL1"), 0)
    assert_equal(conn.to_s, "0:1,2-S\nG0:0\n1:0,1\n2:0+S")

    conn.soft_connect_global(1, "GLOBAL1")
    assert_equal(conn.to_s, "0:1,2-S\nG0:0\n1:0,1\nG1:0-S\n2:0+S")

  end

end

load("test_epilogue.rb")
