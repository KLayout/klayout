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


import pya
import unittest
import sys

def astr(a):
  astr = []
  for i in a:
    astr.append(str(i))
  return "[" + ", ".join(astr) + "]"

class TLTest(unittest.TestCase):

  def test_1(self):

    ctx = pya.ExpressionContext()
    self.assertEqual(ctx.eval("1+2"), 3)
    ctx.var("a", 21)
    self.assertEqual(ctx.eval("2*a"), 42)

    expr = pya.Expression()
    res = expr.eval()
    self.assertEqual(str(type(res)).replace("class", "type"), "<type 'NoneType'>")
    self.assertEqual(repr(res), "None")

    expr = pya.Expression.eval("1+2")
    self.assertEqual(str(type(expr)).replace("class", "type"), "<type 'float'>")
    self.assertEqual(repr(expr), "3.0")

    expr = pya.Expression()
    expr.text = "1+2"
    res = expr.eval()
    self.assertEqual(str(type(res)).replace("class", "type"), "<type 'float'>")
    self.assertEqual(str(res), "3.0")

    expr = pya.Expression()
    expr.var("a", 5)
    expr.text = "a+to_i(2)"
    res = expr.eval()
    self.assertEqual(str(type(res)).replace("class", "type").replace("int", "long"), "<type 'long'>")
    self.assertEqual(str(res), "7")
    expr.var("a", 7)
    res = expr.eval()
    self.assertEqual(str(res), "9")

    pya.Expression.global_var("xxx", 17.5)
    expr = pya.Expression("xxx+1")
    res = expr.eval()
    self.assertEqual(str(type(res)).replace("class", "type"), "<type 'float'>")
    self.assertEqual(str(res), "18.5")

    expr = pya.Expression("a+b*2", { "a": 18, "b": 2.5 })
    res = expr.eval()
    self.assertEqual(str(type(res)).replace("class", "type"), "<type 'float'>")
    self.assertEqual(str(res), "23.0")

    expr = pya.Expression("[a[1],a[2],a[0],a[4],a[3]]", { "a": [ 17, "a", None, [ 2, 7 ], { 8: "x", "u": 42 } ] })
    res = expr.eval()
    self.assertEqual(str(type(res)).replace("class", "type"), "<type 'list'>")
    self.assertEqual(str(res) == "['a', None, 17L, {8L: 'x', 'u': 42L}, [2L, 7L]]" or str(res) == "['a', None, 17, {8: 'x', 'u': 42}, [2, 7]]", True)

    expr = pya.Expression("a[1]", { "a": [ 17, "a", None, [ 2, 7 ], { 8: "x", "u": 42 } ] })
    res = expr.eval()
    self.assertEqual(str(type(res)).replace("class", "type"), "<type 'str'>")
    self.assertEqual(str(res), "a")

    expr = pya.Expression("a[4]", { "a": [ 17, "a", None, [ 2, 7 ], { 8: "x", "u": 42 } ] })
    res = expr.eval()
    self.assertEqual(str(type(res)).replace("class", "type"), "<type 'dict'>")
    self.assertEqual(str(res) == "{8L: 'x', 'u': 42L}" or str(res) == "{8: 'x', 'u': 42}", True)

  # Advanced expressions 
  def test_2_Expression(self):

    box1 = pya.Box(0, 100, 200, 300)
    box2 = pya.Box(50, 150, 250, 350)
    expr = pya.Expression("a", { "a": box1, "b": box2 })
    res = expr.eval()

    self.assertEqual(str(res), "(0,100;200,300)")

    # boxes are non-managed objects -> passing the object through the expression does not persist their ID
    self.assertNotEqual(id(res), id(box1))
    self.assertNotEqual(id(res), id(box2))

    # -------------------------------------------------

    box1 = pya.Box(0, 100, 200, 300)
    box2 = pya.Box(50, 150, 250, 350)
    expr = pya.Expression("a&b", { "a": box1, "b": box2 })
    res = expr.eval()

    self.assertEqual(str(res), "(50,150;200,300)")

    # computed objects are entirely new ones
    self.assertNotEqual(id(res), id(box1))
    self.assertNotEqual(id(res), id(box2))

    # -------------------------------------------------

    box1 = pya.Box(0, 100, 200, 300)
    box2 = pya.Box(50, 150, 250, 350)
    expr = pya.Expression("x=a&b; y=x; z=y; [x,y,z]", { "a": box1, "b": box2, "x": None, "y": None, "z": None })
    res = expr.eval()

    self.assertEqual(astr(res), "[(50,150;200,300), (50,150;200,300), (50,150;200,300)]")

    # all objects are individual copies
    self.assertNotEqual(id(res[0]), id(box1))
    self.assertNotEqual(id(res[0]), id(box2))
    self.assertNotEqual(id(res[1]), id(res[0]))
    self.assertNotEqual(id(res[2]), id(res[0]))

    # -------------------------------------------------

    box1 = pya.Box(0, 100, 200, 300)
    box2 = pya.Box(50, 150, 250, 350)
    expr = pya.Expression("var x=a&b; var y=x; var z=y; [x,y,z]", { "a": box1, "b": box2 })
    res = expr.eval()

    self.assertEqual(astr(res), "[(50,150;200,300), (50,150;200,300), (50,150;200,300)]")

    # all objects are individual copies
    self.assertNotEqual(id(res[0]), id(box1))
    self.assertNotEqual(id(res[0]), id(box2))
    self.assertNotEqual(id(res[1]), id(res[0]))
    self.assertNotEqual(id(res[2]), id(res[0]))

    # destruction of the expression's object space does not matter since we have copies
    expr._destroy()
    self.assertEqual(astr(res), "[(50,150;200,300), (50,150;200,300), (50,150;200,300)]")

    # -------------------------------------------------

    region1 = pya.Region()
    region1 |= pya.Box(0, 100, 200, 300)
    region2 = pya.Region()
    region2 |= pya.Box(50, 150, 250, 350)
    expr = pya.Expression("a", { "a": region1, "b": region2 })
    res = expr.eval()

    # regions are managed objects -> passing the object through the expression persists it's object ID
    self.assertEqual(id(res), id(region1))
    self.assertNotEqual(id(res), id(region2))

    # -------------------------------------------------

    region1 = pya.Region()
    region1 |= pya.Box(0, 100, 200, 300)
    region2 = pya.Region()
    region2 |= pya.Box(50, 150, 250, 350)
    expr = pya.Expression("a&b", { "a": region1, "b": region2, "x": None, "y": None, "z": None })
    res = expr.eval()

    self.assertEqual(str(res), "(50,150;50,300;200,300;200,150)")

    # The returned object (as a new one) is an entirely fresh one
    self.assertNotEqual(id(res), id(region1))
    self.assertNotEqual(id(res), id(region2))

    # -------------------------------------------------

    region1 = pya.Region()
    region1 |= pya.Box(0, 100, 200, 300)
    region2 = pya.Region()
    region2 |= pya.Box(50, 150, 250, 350)
    expr = pya.Expression("x=a&b; y=x; z=y; [x,y,z]", { "a": region1, "b": region2, "x": None, "y": None, "z": None })
    res = expr.eval()

    self.assertEqual(astr(res), "[(50,150;50,300;200,300;200,150), (50,150;50,300;200,300;200,150), (50,150;50,300;200,300;200,150)]")

    # regions are managed objects -> passing the object through the expression persists it's object ID
    self.assertNotEqual(id(res[0]), id(region1))
    self.assertNotEqual(id(res[0]), id(region2))
    self.assertEqual(id(res[1]), id(res[0]))
    self.assertEqual(id(res[2]), id(res[0]))

    # -------------------------------------------------

    region1 = pya.Region()
    region1 |= pya.Box(0, 100, 200, 300)
    region2 = pya.Region()
    region2 |= pya.Box(50, 150, 250, 350)
    expr = pya.Expression("var x=a&b; var y=x; var z=y; [x,y,z]", { "a": region1, "b": region2 })
    res = expr.eval()

    self.assertEqual(astr(res), "[(50,150;50,300;200,300;200,150), (50,150;50,300;200,300;200,150), (50,150;50,300;200,300;200,150)]")

    # regions are managed objects -> passing the object through the expression persists it's object ID
    self.assertNotEqual(id(res[0]), id(region1))
    self.assertNotEqual(id(res[0]), id(region2))
    self.assertEqual(id(res[1]), id(res[0]))
    self.assertEqual(id(res[2]), id(res[0]))

    # the result objects live in the expression object space and are destroyed with the expression
    expr._destroy()

    self.assertEqual(len(res), 3)
    self.assertEqual(res[0].destroyed(), True)
    self.assertEqual(res[1].destroyed(), True)
    self.assertEqual(res[2].destroyed(), True)

# run unit tests
if __name__ == '__main__':
  suite = unittest.TestLoader().loadTestsFromTestCase(TLTest)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)

