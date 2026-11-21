# KLayout Layout Viewer
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

from klayout.dbcore import Layout, TilingProcessor
from klayout.rdbcore import ReportDatabase

layout_a = Layout()
cell_a = layout_a.create_cell("a")

layout_b = Layout()
cell_b = layout_b.create_cell("b")

rdb = ReportDatabase("XOR")
rdb_cell = rdb.create_cell("a_xor_b")
layer_0_cat = rdb.create_category("layer_0")

tp = TilingProcessor()
tp.input("a", layout_a, cell_a.cell_index(), 0)
tp.input("b", layout_b, cell_b.cell_index(), 0)

# this failed
tp.output("xor", rdb, rdb_cell.rdb_id(), layer_0_cat.rdb_id())

# makes the unit test happy
print("OK")

