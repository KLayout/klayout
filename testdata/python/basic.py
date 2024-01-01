# KLayout Layout Viewer
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


import pya
import unittest
import os
import sys
import gc
import copy

# Set this to True to disable some tests involving exceptions
leak_check = "TEST_LEAK_CHECK" in os.environ

class ObjectWithStr:
  def __init__(self, s):
    self.s = s
  def __str__(self):
    return self.s

# see test_21
class AEXT(pya.A):
  def __init__(self):
    self.offset = None
  def s(self, o):
    self.offset = o
  def g(self):
    return self.offset
  def m(self):
    return self.offset+self.get_n()
  # Note: there are no protected methods, but this emulates the respective test for RBA
  def call_a10_prot(self, f):
    a10_prot(f)
  def __repr__(self):
    return str(self.offset)

def repr_of_a(self):
  return "a1=" + str(self.get_n())

pya.A.__repr__ = repr_of_a

class XEdge(pya.Edge):
  def __init__(self):
    super(XEdge, self).__init__(pya.Point(1,2), pya.Point(3,4))

class EEXT(pya.E):
  def func(self, x):
    self.m = x.n
  def xfunc(self, x):
    return len(x)
  def __init__(self):
    self.m = None
    self.n = 42

class X(object):
  nothing = False

class C_IMP1(pya.C):
  def f(self, s):
    return 615

class C_IMP2(pya.C):
  def f(self, s):
    return len(s)

class C_IMP3(pya.C):
  anything = None

class C_IMP4(pya.C):
  def __init__(self):
    self.x = None
    self.xx = None
  def vfunc(self, cd):
    self.x = cd.x()
    self.xx = cd.xx()

class Z_IMP1(pya.Z):
  def f(self, x):
    return x.cls_name()

class Z_IMP2(pya.Z):
  def f(self, x):
    return type(x).__name__

class Z_IMP3(pya.Z):
  def f(self, x):
    return super(Z_IMP3, self).f(x) + "*"

def ph(x):
  # prepares a string with the proper integera values (1L for Python2, 1 for Python3)
  if sys.version_info < (3, 0):
    return x.replace("X", "L")
  else:
    return x.replace("X", "")
    
def map2str(dict):
  # A helper function to product a "canonical" (i.e. sorted-keys) string
  # representation of a dict
  keys = list(dict)

  for k in keys:
    if type(k) is str:
      strKeys = []
      strDict = {}
      for x in keys:
        strKeys.append(str(x))
        strDict[str(x)] = dict[x]
      strings = []
      for x in sorted(strKeys):
        strings.append(str(x) + ": " + str(strDict[x]))
      return "{" + ", ".join(strings) + "}"

  strings = []
  for x in sorted(keys):
    strings.append(str(x) + ": " + str(dict[x]))
  return "{" + ", ".join(strings) + "}"
  
class PyGObject(pya.GObject):
  z = -1
  def __init__(self, z):
    super(PyGObject, self).__init__()
    self.z = z
  # reimplementation of "virtual int g()"
  def g(self):
    return self.z*2

class PyGFactory(pya.GFactory):
  def __init__(self):
    super(PyGFactory, self).__init__()
  # reimplementation of "virtual GObject *f(int)"
  def f(self, z):
    return PyGObject(z)

class BasicTest(unittest.TestCase):

  def test_00(self):

    # all references of PA are released now:
    ic0 = pya.A.instance_count()

    a = pya.A.new_a(100)
    self.assertEqual( pya.A.instance_count(), ic0 + 1 )

    a = pya.A()
    self.assertEqual(a.get_n(), 17)
    a.assign(pya.A(110))
    self.assertEqual(a.get_n(), 110)

    a = None
    self.assertEqual( pya.A.instance_count(), ic0 )

    a = pya.A()
    self.assertEqual( pya.A.instance_count(), ic0 )  # delayed instantiation of detached objects - A is actually created if it is used first
    a.a2()   # just check, if it can be called
    self.assertEqual( pya.A.instance_count(), ic0 + 1 )

    # open question: with ruby 1.8, aa is not deleted if the self.assertEqual is missing. Why?
    # maybe the GC does not like to be called that frequently?
    aa = a.dup()
    self.assertEqual( pya.A.instance_count(), ic0 + 2 )

    aa = None
    self.assertEqual( pya.A.instance_count(), ic0 + 1 )

    a = None
    self.assertEqual( pya.A.instance_count(), ic0 )

    a = pya.A()
    self.assertEqual( pya.A.instance_count(), ic0 )  # delayed instantiation of detached objects - A is actually created if it is used first
    a.a2()   # just check, if it can be called
    self.assertEqual( pya.A.instance_count(), ic0 + 1 )

    # static and non-static methods can be mixed, but they will be made non-ambiguous too
    self.assertEqual( pya.A.aa(), "static_a" )
    self.assertEqual( a.aa(), "a" )

    self.assertEqual( a.get_n(), 17 )
    a.a5(-5)
    self.assertEqual( a.get_n(), -5 )
    a.a5(0x7fffffff)
    self.assertEqual( a.get_n(), 0x7fffffff )
    a.a5(-0x80000000)
    self.assertEqual( a.get_n(), -0x80000000 )

    self.assertEqual( a.a3("a"), 1 )
    self.assertEqual( a.a3(ObjectWithStr("abcde")), 5 )   # implicitly using to_s for string conversion
    self.assertEqual( a.a3("ab"), 2 )
    self.assertEqual( a.a3("µ"), 2 )  # two UTF8 bytes
    if "a3_qstr" in a.__dict__:
      self.assertEqual( a.a3_qstr("a"), 1 )
      self.assertEqual( a.a3_qstr("ab"), 2 )
      self.assertEqual( a.a3_qstr("µ"), 1 )  # one UTF character
      self.assertEqual( a.a3_qstrref("a"), 1 )
      self.assertEqual( a.a3_qstrref("ab"), 2 )
      self.assertEqual( a.a3_qstrref("µ"), 1 )  # one UTF character
      self.assertEqual( a.a3_qba("a"), 1 )
      self.assertEqual( a.a3_qba("ab"), 2 )
      self.assertEqual( a.a3_qba("µ"), 2 )  # two UTF8 bytes
    self.assertEqual( a.a3(""), 0 )

    self.assertEqual( a.a4([1]), 1.0 )
    t = (1,)
    self.assertEqual( a.a4(t), 1.0 )
    self.assertEqual( a.a4([1, 125e-3]), 0.125 )
    t = (5, 1, -1.25)
    self.assertEqual( a.a4(t), -1.25 )

    arr = []
    for d in a.a6():
      arr.append(d)
    self.assertEqual(arr, [5, 1, -1.25])

    arr = []
    for d in a.a7():
      arr.append(d)
    self.assertEqual(arr, [5, 1, -1.25])

    arr = []
    for d in a.a8():
      arr.append(d)
    self.assertEqual(arr, [5, 1, -1.25])

    a.destroy()
    self.assertEqual( pya.A.instance_count(), ic0 )

    if not leak_check:

      error_caught = False
      try: 
        a.a2()  # object has been destroyed already
      except:
        error_caught = True
      self.assertEqual( error_caught, True )

      error_caught = False
      try: 
        a.destroy()  # object has been destroyed already
      except:
        error_caught = True
      self.assertEqual( error_caught, True )

    self.assertEqual( pya.A.instance_count(), ic0 )
    a = pya.A.new_a( 55 )
    self.assertEqual( pya.A.instance_count(), ic0 + 1 )
    self.assertEqual( a.get_n(), 55 )
    self.assertEqual( a.a_vp1( a.a_vp2() ), "abc" )
    a.destroy()
    self.assertEqual( pya.A.instance_count(), ic0 )

    a = pya.A.new_a(0)
    self.assertEqual( str(a.a9a(5)), "True" )
    self.assertEqual( str(a.a9a(4)), "False" )
    self.assertEqual( str(a.a9b(True)), "5" )
    self.assertEqual( str(a.a9b(0)), "-5" )
    self.assertEqual( str(a.a9b(1)), "5" )
    self.assertEqual( str(a.a9b(False)), "-5" )
    self.assertEqual( str(a.a9b(None)), "-5" )

    self.assertEqual( str(a.get_e()), "#0" )
    a.set_e( pya.Enum.a )
    self.assertEqual( str(a.get_e()), "a" )
    a.set_e( pya.Enum.b )
    self.assertEqual( str(a.get_e()), "b" )
    a.set_eptr( None )
    self.assertEqual( str(a.get_e()), "#0" )
    a.set_eptr( pya.Enum.c )
    self.assertEqual( str(a.get_e()), "c" )
    a.set_ecptr( None )
    self.assertEqual( str(a.get_e()), "#0" )
    a.set_ecptr( pya.Enum.b )
    self.assertEqual( str(a.get_e()), "b" )
    a.set_ecref( pya.Enum.a )
    self.assertEqual( str(a.get_e()), "a" )
    a.set_eref( pya.Enum.c )
    self.assertEqual( str(a.get_e()), "c" )
    self.assertEqual( str(a.get_eptr()), "c" )
    self.assertEqual( str(a.get_eref()), "c" )
    self.assertEqual( str(a.get_ecptr()), "c" )
    self.assertEqual( str(a.get_ecref()), "c" )
    a.set_ecptr( None )
    self.assertEqual( a.get_ecptr(), None )
    self.assertEqual( str(a.get_ecref()), "#0" )
    self.assertEqual( a.get_eptr(), None )
    self.assertEqual( str(a.get_eref()), "#0" )

    ea = pya.Enum.a
    eb = pya.Enum.b
    ei = pya.Enum(17)
    e0 = pya.Enum()
    self.assertEqual( str(ea), "a" )
    self.assertEqual( str(pya.Enum(str(ea))), "a" )
    self.assertEqual( str(eb), "b" )
    self.assertEqual( str(pya.Enum(str(eb))), "b" )
    self.assertEqual( str(ei), "#17" )
    self.assertEqual( str(pya.Enum(str(ei))), "#17" )
    self.assertEqual( str(e0), "#0" )
    self.assertEqual( str(pya.Enum(str(e0))), "#0" )
    self.assertEqual( repr(e0), "(not a valid enum value)" )
    self.assertEqual( repr(ea), "a (1)" )
    self.assertEqual( repr(eb), "b (2)" )
    self.assertEqual( eb == ea, False )
    self.assertEqual( eb == eb, True )
    self.assertEqual( eb != ea, True )
    self.assertEqual( eb != eb, False )
    self.assertEqual( eb < ea, False )
    self.assertEqual( eb < eb, False )
    self.assertEqual( ea < eb, True )

    if "Enums" in pya.__dict__:

      eea = pya.Enums()
      eei = pya.Enums(3)
      eeb = pya.Enums(eb)
      self.assertEqual( str(eea), "" )
      self.assertEqual( repr(eea), " (0)" )
      self.assertEqual( repr(pya.Enums(str(eea))), " (0)" )
      self.assertEqual( repr(eei), "a|b (3)" )
      self.assertEqual( repr(pya.Enums(str(eei))), "a|b (3)" )
      self.assertEqual( repr(eeb), "b (2)" )
      self.assertEqual( repr(pya.Enums(str(eeb))), "b (2)" )
      eeab1 = ea | eb
      eeab2 = ea | pya.Enums(eb)
      eeab3 = pya.Enums(ea) | eb
      eeab4 = pya.Enums(ea) | pya.Enums(eb)
      self.assertEqual( repr(eeab1), "a|b (3)" )
      self.assertEqual( repr(eeab2), "a|b (3)" )
      self.assertEqual( repr(eeab3), "a|b (3)" )
      self.assertEqual( repr(eeab4), "a|b (3)" )
      # Note: unsigned enum's will produce the long int, signed enums will produce the short one
      self.assertEqual( repr(~eeab4) == " (-4)" or repr(~eeab4) == " (4294967292)", True )
      self.assertEqual( repr(eeab4 & ea), "a (1)" )
      self.assertEqual( repr(eeab4 & eeb), "b (2)" )
      self.assertEqual( repr(eeab4 ^ eeb), "a (1)" )
      self.assertEqual( repr(eeab4 ^ eb), "a (1)" )
      self.assertEqual( repr(eeab4), "a|b (3)" )
      eeab4 ^= ea
      self.assertEqual( repr(eeab4), "b (2)" )

      ee = pya.Enum()
      self.assertEqual( str(ee), "#0" )
      a.mod_eref( ee, pya.Enum.c )
      self.assertEqual( str(ee), "c" )
      a.mod_eptr( ee, pya.Enum.a )
      self.assertEqual( str(ee), "a" )

      self.assertEqual( repr(a.ev()), "[]" )
      a.push_ev( pya.Enum.a )
      a.push_ev( pya.Enum() )
      a.push_ev( pya.Enum.b )
      self.assertEqual( repr(a.ev()), "[a (1), (not a valid enum value), b (2)]" )

      self.assertEqual( repr(a.get_ef()), " (0)" )
      a.set_ef( pya.Enum.a )
      self.assertEqual( str(a.get_ef()), "a" )
      a.set_ef( pya.Enums(pya.Enum.b) )
      self.assertEqual( str(a.get_ef()), "b" )
      a.set_efptr( None )
      self.assertEqual( repr(a.get_ef()), " (0)" )
      a.set_efptr( pya.Enums(pya.Enum.c) )
      self.assertEqual( str(a.get_ef()), "a|b|c" )
      a.set_efcptr( None )
      self.assertEqual( repr(a.get_ef()), " (0)" )
      a.set_efcptr( pya.Enums(pya.Enum.b) )
      self.assertEqual( str(a.get_ef()), "b" )
      a.set_efcptr( pya.Enum.c )
      self.assertEqual( str(a.get_ef()), "a|b|c" )
      a.set_efcref( pya.Enum.b )
      self.assertEqual( str(a.get_ef()), "b" )
      a.set_efcref( pya.Enums(pya.Enum.a) )
      self.assertEqual( str(a.get_ef()), "a" )
      a.set_efref( pya.Enums(pya.Enum.c) )
      self.assertEqual( str(a.get_ef()), "a|b|c" )
      self.assertEqual( str(a.get_efptr()), "a|b|c" )
      self.assertEqual( str(a.get_efref()), "a|b|c" )
      self.assertEqual( str(a.get_efcptr()), "a|b|c" )
      self.assertEqual( str(a.get_efcref()), "a|b|c" )
      a.set_efcptr( None )
      self.assertEqual( a.get_efcptr(), None )
      self.assertEqual( repr(a.get_efcref()), " (0)" )
      self.assertEqual( a.get_efptr(), None )
      self.assertEqual( repr(a.get_efref()), " (0)" )

      ee = pya.Enums()
      self.assertEqual( repr(ee), " (0)" )
      a.mod_efref( ee, pya.Enum.b )
      self.assertEqual( str(ee), "b" )
      a.mod_efptr( ee, pya.Enum.a )
      self.assertEqual( str(ee), "a|b" )

  def test_10(self):

    # all references of A are released now:
    ic0 = pya.A.instance_count()
    self.assertEqual(ic0, 0)

    a = pya.A.new_a_by_variant()
    self.assertEqual(pya.A.instance_count(), ic0 + 1)

    self.assertEqual(a.get_n(), 17)
    a.a5(-15)
    self.assertEqual(a.get_n(), -15)

    a = None
    self.assertEqual(pya.A.instance_count(), ic0)

    ic0 = pya.B.instance_count()
    self.assertEqual(ic0, 0)

    b = pya.B.new_b_by_variant()
    self.assertEqual(pya.B.instance_count(), ic0 + 1)

    b.set_str_combine("x", "y")
    self.assertEqual(b.str(), "xy")

    b._destroy()
    self.assertEqual(pya.B.instance_count(), ic0)

  def test_11(self):

    # implicitly converting tuples/lists to objects by calling the constructor

    b = pya.B()
    b.av_cptr = [ pya.A(17), [1,2], [4,6,0.5] ]

    arr = []
    for a in b.each_a():
      arr.append(a.get_n_const())
    self.assertEqual(arr, [17, 3, 5])
    
    b = pya.B()
    # NOTE: this gives an error (printed only) that tuples can't be modified as out parameters
    b.av_ref = ( (1,2), (6,2,0.25), [42] )

    arr = []
    for a in b.each_a():
      arr.append(a.get_n_const())
    self.assertEqual(arr, [3, 2, 42])
    
    b = pya.B()
    aa = [ (1,2), (6,2,0.25), [42] ]
    b.av_ptr = aa

    arr = []
    for a in b.each_a():
      arr.append(a.get_n_const())
    self.assertEqual(arr, [3, 2, 42])
    
    # NOTE: as we used aa in "av_ptr", it got modified as out parameter and
    # now holds A object references
    arr = []
    for a in aa:
      arr.append(a.get_n_const())
    self.assertEqual(arr, [3, 2, 42])
    
    b.av = ()

    arr = []
    for a in b.each_a():
      arr.append(a.get_n_const())
    self.assertEqual(arr, [])
    
    b.push_a_ref( (1, 7) )

    arr = []
    for a in b.each_a():
      arr.append(a.get_n_const())
    self.assertEqual(arr, [8])
    
    b.push_a_ptr( (1, 7, 0.25) )

    arr = []
    for a in b.each_a():
      arr.append(a.get_n_const())
    self.assertEqual(arr, [8, 2])
    
    b.push_a_cref( [42] )

    arr = []
    for a in b.each_a():
      arr.append(a.get_n_const())
    self.assertEqual(arr, [8, 2, 42])
    
    b.push_a_cptr( (1, 16) )

    arr = []
    for a in b.each_a():
      arr.append(a.get_n_const())
    self.assertEqual(arr, [8, 2, 42, 17])
    
    b.push_a( (4, 6, 0.5) )

    arr = []
    for a in b.each_a():
      arr.append(a.get_n_const())
    self.assertEqual(arr, [8, 2, 42, 17, 5])
    
  def test_12(self):

    a1 = pya.A()
    a1.a5( -15 )
    a2 = a1
    a3 = a2.dup()

    self.assertEqual( a1.get_n(), -15 )
    self.assertEqual( a2.get_n(), -15 )
    self.assertEqual( a3.get_n(), -15 )

    a1.a5( 11 )
    a3.a5( -11 )
    
    self.assertEqual( a1.get_n(), 11 )
    self.assertEqual( a2.get_n(), 11 )
    self.assertEqual( a3.get_n(), -11 )

    self.assertEqual( a1.a10_d(5.2), "5.2" )
    self.assertEqual( a1.a10_s(0x70000000), "0" )
    self.assertEqual( a1.a10_s(0x7fffffff), "-1" )
    self.assertEqual( a1.a10_us(0x70000000), "0" )
    self.assertEqual( a1.a10_us(0x7fffffff), "65535" )
    self.assertEqual( a1.a10_i(-0x80000000), "-2147483648" )
    self.assertEqual( a1.a10_l(-0x80000000), "-2147483648" )
    self.assertEqual( a1.a10_ll(-0x80000000), "-2147483648" )
    self.assertEqual( a1.a10_ui(0xffffffff), "4294967295" )
    self.assertEqual( a1.a10_ul(0xffffffff), "4294967295" )
    self.assertEqual( a1.a10_ull(0xffffffff), "4294967295" )
    self.assertEqual( a1.a11_s(0x70000000), 0 )
    self.assertEqual( a1.a11_s(0x7fffffff), -1 )
    self.assertEqual( a1.a11_us(0x70000000), 0 )
    self.assertEqual( a1.a11_us(0x7fffffff), 65535 )
    self.assertEqual( a1.a11_i(-0x80000000), -2147483648 )
    self.assertEqual( a1.a11_l(-0x80000000), -2147483648 )
    self.assertEqual( a1.a11_ll(-0x80000000), -2147483648 )
    self.assertEqual( a1.a11_ui(0xffffffff), 4294967295 )
    self.assertEqual( a1.a11_ul(0xffffffff), 4294967295 )
    self.assertEqual( a1.a11_ull(0xffffffff), 4294967295 )
    if "a10_d_qstr" in a1.__dict__:
      self.assertEqual( a1.a10_d_qstr(5.25), "5.25" )
      self.assertEqual( a1.a10_d_qstrref(5.2), "5.2" )
      self.assertEqual( a1.a10_d_qba(5.1), "5.1" )
    self.assertEqual( a1.a10_f(5.7), "5.7" )
    x = pya.Value(1.5)
    self.assertEqual( str(x.value), "1.5" )
    self.assertEqual( a1.a10_fptr(x), "6.5" )
    self.assertEqual( str(x.value), "6.5" )
    self.assertEqual( a1.a10_fptr(1), "6" )
    self.assertEqual( a1.a10_fptr(None), "nil" )
    self.assertEqual( a1.a10_fptr(pya.Value()), "nil" )
    self.assertEqual( str(x), "6.5" )
    self.assertEqual( str(x.value), "6.5" )
    x = pya.Value(2.5)
    self.assertEqual( a1.a10_dptr(x), "8.5" )
    self.assertEqual( a1.a10_dptr(2), "8" )
    self.assertEqual( a1.a10_dptr(None), "nil" )
    self.assertEqual( a1.a10_dptr(pya.Value()), "nil" )
    self.assertEqual( x.to_s(), "8.5" )
    self.assertEqual( str(x.value), "8.5" )
    x = pya.Value(2)
    self.assertEqual( a1.a10_iptr(x), "9" )
    self.assertEqual( a1.a10_iptr(3), "10" )
    self.assertEqual( a1.a10_iptr(None), "nil" )
    self.assertEqual( a1.a10_iptr(pya.Value()), "nil" )
    self.assertEqual( str(x), "9" )
    self.assertEqual( str(x.value), "9" )
    x = pya.Value(False)
    self.assertEqual( a1.a10_bptr(x), "true" )
    self.assertEqual( a1.a10_bptr(False), "true" )
    self.assertEqual( a1.a10_bptr(None), "nil" )
    self.assertEqual( a1.a10_bptr(pya.Value()), "nil" )
    self.assertEqual( str(x), "true" )
    self.assertEqual( str(x.value), "True" )
    x = pya.Value(10)
    self.assertEqual( a1.a10_uiptr(x), "20" )
    self.assertEqual( a1.a10_uiptr(11), "21" )
    self.assertEqual( a1.a10_uiptr(None), "nil" )
    self.assertEqual( a1.a10_uiptr(pya.Value()), "nil" )
    self.assertEqual( str(x), "20" )
    self.assertEqual( str(x.value), "20" )
    x = pya.Value(10)
    self.assertEqual( a1.a10_ulptr(x), "21" )
    self.assertEqual( a1.a10_ulptr(12), "23" )
    self.assertEqual( a1.a10_ulptr(None), "nil" )
    self.assertEqual( a1.a10_ulptr(pya.Value()), "nil" )
    self.assertEqual( str(x), "21" )
    self.assertEqual( str(x.value), "21" )
    x = pya.Value(10)
    self.assertEqual( a1.a10_lptr(x), "22" )
    self.assertEqual( a1.a10_lptr(11), "23" )
    self.assertEqual( a1.a10_lptr(None), "nil" )
    self.assertEqual( a1.a10_lptr(pya.Value()), "nil" )
    self.assertEqual( str(x), "22" )
    self.assertEqual( str(x.value), "22" )
    x = pya.Value(10)
    self.assertEqual( a1.a10_llptr(x), "23" )
    self.assertEqual( a1.a10_llptr(11), "24" )
    self.assertEqual( a1.a10_llptr(None), "nil" )
    self.assertEqual( a1.a10_llptr(pya.Value()), "nil" )
    self.assertEqual( str(x), "23" )
    self.assertEqual( str(x.value), "23" )
    x = pya.Value(10)
    self.assertEqual( a1.a10_ullptr(x), "24" )
    self.assertEqual( a1.a10_ullptr(12), "26" )
    self.assertEqual( a1.a10_ullptr(None), "nil" )
    self.assertEqual( a1.a10_ullptr(pya.Value()), "nil" )
    self.assertEqual( x.to_s(), "24" )
    self.assertEqual( x.value, 24 )
    x = pya.Value("z")
    self.assertEqual( a1.a10_sptr(x), "zx" )
    self.assertEqual( a1.a10_sptr("a"), "ax" )
    self.assertEqual( a1.a10_sptr(None), "nil" )
    self.assertEqual( a1.a10_sptr(pya.Value()), "nil" )
    self.assertEqual( x.to_s(), "zx" )
    self.assertEqual( x.value, "zx" )
    
    # String modification is not possible in CPython API, hence strings cannot be used
    # directly as out parameters (but as boxed values they can)
    x = "z"
    self.assertEqual( a1.a10_sptr(x), "zx" )
    self.assertEqual( x, "z" )

    try:
      # passing other objects than StringValue and a string fails
      self.assertEqual( a1.a10_sptr([]), "nil" )
      err = False
    except: 
      err = True
    self.assertEqual( err, True )

    self.assertEqual( a1.a10_cfptr(6.5), "6.5" )
    self.assertEqual( a1.a10_cfptr(None), "nil" )
    self.assertEqual( a1.a10_cdptr(8.5), "8.5" )
    self.assertEqual( a1.a10_cdptr(None), "nil" )
    self.assertEqual( a1.a10_ciptr(9), "9" )
    self.assertEqual( a1.a10_ciptr(None), "nil" )
    self.assertEqual( a1.a10_cbptr(True), "true" )
    self.assertEqual( a1.a10_cbptr(None), "nil" )
    self.assertEqual( a1.a10_cuiptr(20), "20" )
    self.assertEqual( a1.a10_cuiptr(None), "nil" )
    self.assertEqual( a1.a10_culptr(21), "21" )
    self.assertEqual( a1.a10_culptr(None), "nil" )
    self.assertEqual( a1.a10_clptr(22), "22" )
    self.assertEqual( a1.a10_clptr(None), "nil" )
    self.assertEqual( a1.a10_cllptr(23), "23" )
    self.assertEqual( a1.a10_cllptr(None), "nil" )
    self.assertEqual( a1.a10_cullptr(24), "24" )
    self.assertEqual( a1.a10_cullptr(None), "nil" )
    self.assertEqual( a1.a10_csptr(None), "nil" )
    self.assertEqual( a1.a10_csptr("x"), "x" )

    x = pya.Value(1.5)
    self.assertEqual( a1.a10_fref(x), "11.5" )
    try:
      self.assertEqual( a1.a10_fref(None), "nil" )
      err = False
    except: 
      err = True
    self.assertEqual( err, True )
    try:
      self.assertEqual( a1.a10_fref(pya.Value()), "nil" )
      err = False
    except: 
      err = True
    self.assertEqual( err, True )
    self.assertEqual( x.value, 11.5 )
    x = pya.Value(2.5)
    self.assertEqual( a1.a10_dref(x), "13.5" )
    self.assertEqual( a1.a10_dref(2), "13" )
    self.assertEqual( x.value, 13.5 )
    x = pya.Value(2)
    self.assertEqual( a1.a10_iref(x), "14" )
    self.assertEqual( a1.a10_iref(1), "13" )
    self.assertEqual( x.value, 14 )
    x = pya.Value(False)
    self.assertEqual( a1.a10_bref(x), "true" )
    self.assertEqual( a1.a10_bref(False), "true" )
    self.assertEqual( x.value, True )
    x = pya.Value(10)
    self.assertEqual( a1.a10_uiref(x), "24" )
    self.assertEqual( a1.a10_uiref(11), "25" )
    self.assertEqual( x.value, 24 )
    x = pya.Value(10)
    self.assertEqual( a1.a10_ulref(x), "25" )
    self.assertEqual( a1.a10_ulref(12), "27" )
    self.assertEqual( x.value, 25 )
    x = pya.Value(10)
    self.assertEqual( a1.a10_lref(x), "26" )
    self.assertEqual( a1.a10_lref(13), "29" )
    self.assertEqual( x.value, 26 )
    x = pya.Value(10)
    self.assertEqual( a1.a10_llref(x), "27" )
    self.assertEqual( a1.a10_llref(14), "31" )
    self.assertEqual( x.value, 27 )
    x = pya.Value(10)
    self.assertEqual( a1.a10_ullref(x), "28" )
    self.assertEqual( a1.a10_ullref(11), "29" )
    self.assertEqual( x.value, 28 )
    x = pya.Value("x")
    self.assertEqual( a1.a10_sref(x), "xy" )
    self.assertEqual( x.value, "xy" )
    self.assertEqual( a1.a10_sref("p"), "py" )

    self.assertEqual( a1.a10_cfref(6.5), "6.5" )
    try:
      self.assertEqual( a1.a10_cfref(None), "nil" )
      err = False
    except: 
      err = True
    self.assertEqual( err, True )
    self.assertEqual( a1.a10_cdref(8.5), "8.5" )
    self.assertEqual( a1.a10_ciref(9), "9" )
    self.assertEqual( a1.a10_cbref(True), "true" )
    self.assertEqual( a1.a10_cuiref(20), "20" )
    self.assertEqual( a1.a10_culref(21), "21" )
    self.assertEqual( a1.a10_clref(22), "22" )
    self.assertEqual( a1.a10_cllref(23), "23" )
    self.assertEqual( a1.a10_cullref(24), "24" )
    self.assertEqual( a1.a10_csref("x"), "x" )

  def test_13(self):

    b = pya.B()

    if not leak_check:

      err_caught = False
      try:
        b.amember_cptr().get_n() # cannot call non-const method on const reference
      except: 
        err_caught = True
      self.assertEqual( err_caught, True )

    b.amember_cptr().a2()

    self.assertEqual(b.always_5(), 5)
    self.assertEqual(b.str(), "")
    b.set_str("xyz")
    self.assertEqual(b.str(), "xyz")
    self.assertEqual(b.str_ccptr(), "xyz")
    b.set_str_combine("yx", "zz")
    self.assertEqual(b.str(), "yxzz")
    self.assertEqual(b.str_ccptr(), "yxzz")

    arr = []

    err_caught = False

    if not leak_check:

      try:
        for a in b.b10():
          arr.append(a.get_n())  # b10 is a const iterator - cannot call a1 on it
      except: 
        err_caught = True
      self.assertEqual( err_caught, True )
      self.assertEqual(arr, [])

    err_caught = False

    if not leak_check:

      try:
        for a in b.b10p():
          arr.append(a.get_n())  # b10p is a const iterator - cannot call a1 on it
      except: 
        err_caught = True
      self.assertEqual( err_caught, True )
      self.assertEqual(arr, [])

    arr = []
    for a in b.b10():
      arr.append(a.dup().get_n()) 
    self.assertEqual(arr, [100, 121, 144])

    arr = []
    # NOTE: the following will crash: 
    #   for a in b.dup().b10():
    #     ...
    #   because the clone created by dup() will be
    #   destroyed too early.
    bdup = b.dup()
    for a in bdup.b10():
      arr.append(a.dup().get_n()) 
    self.assertEqual(arr, [100, 121, 144])
    return

    arr = []
    for a in b.b10():
      arr.append(a.get_n_const()) 
    self.assertEqual(arr, [100, 121, 144])

    arr = []
    for a in b.b10p():
      arr.append(a.dup().get_n())
    self.assertEqual(arr, [100, 121, 144])

    arr = []
    # Ticket #811:
    for a in b.dup().b10p():
      arr.append(a.dup().get_n())
    self.assertEqual(arr, [100, 121, 144])

    arr = []
    bdup = b.dup()
    for a in bdup.b10p():
      arr.append(a.dup().get_n())
    self.assertEqual(arr, [100, 121, 144])

    arr = []
    for a in b.b10p():
      arr.append(a.get_n_const())
    self.assertEqual(arr, [100, 121, 144])

    arr = []
    for a in b.b11():
      arr.append(a.get_n())
    self.assertEqual(arr, [100, 121, 144])

    arr = []
    bdup = b.dup()
    for a in bdup.b11():
      arr.append(a.get_n())
    self.assertEqual(arr, [100, 121, 144])

    arr = []
    for a in b.b12():
      arr.append(a.get_n())
    self.assertEqual(arr, [7100, 7121, 7144, 7169])

    arr = []
    bdup = b.dup()
    for a in bdup.b12():
      arr.append(a.get_n())
    self.assertEqual(arr, [7100, 7121, 7144, 7169])

    aarr = b.b16a()
    arr = []
    for a in aarr:
      arr.append(a.get_n())
    self.assertEqual(arr, [100, 121, 144])

    aarr = b.b16b()
    arr = []
    for a in aarr:
      arr.append(a.get_n())
    self.assertEqual(arr, [100, 121, 144])

    aarr = b.b16c()
    arr = []
    for a in aarr:
      arr.append(a.get_n())
    self.assertEqual(arr, [100, 121, 144])

    b.b17a( [ pya.A.new_a( 101 ), pya.A.new_a( -122 ) ] )
    arr = []
    for a in b.b11():
      arr.append(a.get_n())
    self.assertEqual(arr, [101, -122])

    b.b17a( [] )
    arr = []
    for a in b.b11():
      arr.append(a.get_n())
    self.assertEqual(arr, [])

    b.b17b( [ pya.A.new_a( 102 ), pya.A.new_a( -123 ) ] )
    arr = []
    for a in b.b11():
      arr.append(a.get_n())
    self.assertEqual(arr, [102, -123])

    b.b17c( [ pya.A.new_a( 100 ), pya.A.new_a( 121 ), pya.A.new_a( 144 ) ] )
    arr = []
    for a in b.b11():
      arr.append(a.get_n())
    self.assertEqual(arr, [100, 121, 144])

    if not leak_check: 

      arr = []
      try:
        for a in b.b13():
          arr.append(a.get_n())
      except: 
        err_caught = True
      self.assertEqual( err_caught, True )
      self.assertEqual(arr, [])

    arr = []
    for a in b.b13():
      arr.append(a.get_n_const())
    self.assertEqual(arr, [-3100, -3121])

    arr = []
    bdup = b.dup()
    for a in bdup.b13():
      arr.append(a.get_n_const())
    self.assertEqual(arr, [-3100, -3121])

    arr = []
    for a in b.each_a():
      arr.append(a.get_n_const())
    self.assertEqual(arr, [100, 121, 144])

    arr = []
    for a in b.each_a():
      arr.append(a.get_n())
    self.assertEqual(arr, [100, 121, 144])

    arr = []
    for a in b.each_a_ref():
      arr.append(a.get_n_const())
    self.assertEqual(arr, [100, 121, 144])

    arr = []
    # even though b18b returns a "const A &", calling a non-const method does not work
    # since A is a managed object and is not turned into a copy.
    err_caught = False
    try:
      for a in b.each_a_ref():
        arr.append(a.get_n())
    except:
      err_caught = True
    end
    self.assertEqual(arr, [])
    self.assertEqual(err_caught, True)

    arr = []
    for a in b.each_a_ptr():
      arr.append(a.get_n_const())
    self.assertEqual(arr, [100, 121, 144])

    arr = []
    # this does not work since each_a_ptr delivers a "const *" which cannot be used to call a non-const
    # method on
    err_caught = False
    try: 
      for a in b.each_a_ptr():
        arr.append(a.get_n())
    except:
      err_caught = True
    end
    self.assertEqual(arr, [])
    self.assertEqual(err_caught, True)

  def test_13b(self):

    b = pya.B()

    bb = pya.B()
    bb.set_str("a")
    b.push_b(bb)

    bb = pya.B()
    bb.set_str("y")
    b.push_b(bb)

    bb = pya.B()
    bb.set_str("uu")
    b.push_b(bb)

    arr = []
    for bb in b.each_b_copy():
      arr.append(bb.str())
    self.assertEqual(arr, ["a", "y", "uu"])

    arr = []
    for bb in b.each_b_copy():
      bb.set_str(bb.str() + "x")
      arr.append(bb.str())
    self.assertEqual(arr, ["ax", "yx", "uux"])

    arr = []
    for bb in b.each_b_copy():
      arr.append(bb.str())
    self.assertEqual(arr, ["a", "y", "uu"])

    arr = []
    for bb in b.each_b_cref():
      arr.append(bb.str())
    self.assertEqual(arr, ["a", "y", "uu"])

    arr = []
    # this works, since the "const B &" will be converted to a copy
    for bb in b.each_b_cref():
      bb.set_str(bb.str() + "x")
      arr.append(bb.str())
    self.assertEqual(arr, ["ax", "yx", "uux"])

    arr = []
    for bb in b.each_b_cref():
      arr.append(bb.str())
    self.assertEqual(arr, ["a", "y", "uu"])

    arr = []
    for bb in b.each_b_cptr():
      arr.append(bb.str())
    self.assertEqual(arr, ["a", "y", "uu"])

    arr = []
    # const references cannot be modified
    err_caught = False
    try:
      for bb in b.each_b_cptr():
        bb.set_str(bb.str() + "x")
        arr.append(bb.str())
    except: 
      err_caught = True
    self.assertEqual(err_caught, True)
    self.assertEqual(arr, [])

    arr = []
    for bb in b.each_b_cptr():
      arr.append(bb.str())
    self.assertEqual(arr, ["a", "y", "uu"])

    arr = []
    for bb in b.each_b_ref():
      arr.append(bb.str())
    self.assertEqual(arr, ["a", "y", "uu"])

    arr = []
    for bb in b.each_b_ref():
      bb.set_str(bb.str() + "x")
      arr.append(bb.str())
    self.assertEqual(arr, ["ax", "yx", "uux"])

    arr = []
    for bb in b.each_b_ref():
      arr.append(bb.str())
    self.assertEqual(arr, ["ax", "yx", "uux"])

    arr = []
    for bb in b.each_b_ptr():
      arr.append(bb.str())
    self.assertEqual(arr, ["ax", "yx", "uux"])

    arr = []
    for bb in b.each_b_ptr():
      bb.set_str(bb.str() + "x")
      arr.append(bb.str())
    self.assertEqual(arr, ["axx", "yxx", "uuxx"])

    arr = []
    for bb in b.each_b_ptr():
      arr.append(bb.str())
    self.assertEqual(arr, ["axx", "yxx", "uuxx"])

  def test_14(self):

    a = pya.A()
    a.a5( 22 )

    b = pya.B()
    self.assertEqual( b.b3( a ), 22 )
    self.assertEqual( b.b4( a ), "b4_result: 22" )
    a.a5( -6 )
    self.assertEqual( b.b3( a ), -6 )
    self.assertEqual( b.b4( a ), "b4_result: -6" )
    self.assertEqual( b.b4( pya.A() ), "b4_result: 17" )

  def test_15(self):

    a = pya.A_NC()
    self.assertEqual( True, isinstance(a, pya.A) )
    a.a5( 22 )

    b = pya.B()
    self.assertEqual( b.b3( a ), 22 )
    self.assertEqual( b.b4( a ), "b4_result: 22" )
    a.a5( -6 )
    self.assertEqual( b.b3( a ), -6 )
    self.assertEqual( b.b4( a ), "b4_result: -6" )
    self.assertEqual( b.b4( pya.A_NC() ), "b4_result: 17" )

  def test_16(self):

    if leak_check:
      return

    # Test, if this throws an error (object of class X passed to A argument):
    err = ""
    try:
      b = pya.B()
      self.assertEqual( b.b4( pya.X() ), "b4_result: -6" )
      self.assertEqual( False, True )  # this must never hit
    except Exception as e:
      err = str(e)
    self.assertEqual( err, "Unexpected object type (expected argument of class A, got X) for argument #1 in B.b4" );
  
    # Test, if this throws an error (object of class X passed to A argument):
    err = ""
    try:
      b = pya.B()
      bb = pya.B()
      self.assertEqual( b.b4( bb ), "b4_result: -6" )
      self.assertEqual( False, True )  # this must never hit
    except Exception as e:
      err = str(e)
    self.assertEqual( err, "Unexpected object type (expected argument of class A, got B) for argument #1 in B.b4" );
  
  def test_17(self):

    # test copies of objects being returned

    b = pya.B()
    b._create()  # force constructor of B to allocate some A instances internally

    a_count = pya.A.instance_count()
    a = b.make_a( 1971 );
    self.assertEqual( pya.A.instance_count(), a_count + 1 )
    self.assertEqual( a.get_n(), 1971 );
    self.assertEqual( b.an( a ), 1971 );

    aa = b.make_a( -61 );
    self.assertEqual( pya.A.instance_count(), a_count + 2 )
    self.assertEqual( b.an_cref( aa ), -61 );
    self.assertEqual( a.get_n(), 1971 );
    self.assertEqual( b.an( a ), 1971 );
    self.assertEqual( aa.get_n(), -61 );
    self.assertEqual( b.an( aa ), -61 );

    aa.a5(98);
    a.a5(100);
    
    self.assertEqual( a.get_n(), 100 );
    self.assertEqual( b.an( a ), 100 );
    self.assertEqual( aa.get_n(), 98 );
    self.assertEqual( b.an( aa ), 98 );

    a._destroy()
    aa = None
    self.assertEqual( pya.A.instance_count(), a_count )

  def test_18(self):

    # Test references to objects (returned by b.amember_cptr)

    b = pya.B()
    b.set_an( 77 )
    self.assertEqual( b.amember_cptr().get_n_const(), 77 );

    b.set_an_cref( 79 )
    self.assertEqual( b.amember_cptr().get_n_const(), 79 );

    aref = b.amember_cptr()
    err_caught = False

    if not leak_check:

      try:
        x = aref.get_n() # cannot call non-const method on const reference (as delivered by amember_cptr)
      except:
        err_caught = True
      self.assertEqual( err_caught, True )
      self.assertEqual( aref.get_n_const(), 79 );

    b.set_an( -1 )
    self.assertEqual( aref.get_n_const(), -1 );

  def test_19(self):

    c0 = pya.C()
    self.assertEqual( c0.g("x"), 1977 );

    c1 = C_IMP1()
    self.assertEqual( c1.g("x"), 615 );

    c2 = C_IMP2()
    self.assertEqual( c2.g("x"), 1 );
    self.assertEqual( c2.g(""), 0 );
    self.assertEqual( c2.g("abc"), 3 );
    self.assertEqual( c1.g("x"), 615 );

    c3 = C_IMP3()
    self.assertEqual( c3.g("x"), 1977 );

    self.assertEqual( pya.C.s1(), 4451 );
    pya.C.s2clr()
    pya.C.s2( 7.0 )
    self.assertEqual( pya.C.s3( 5.5 ), "5.500" );

    arr = []
    for i in pya.C.each():
      arr.append(i) 
    self.assertEqual( arr, [ 0, 1, 2, 3, 4, 5, 6 ] )

    self.assertEqual( C_IMP1.s1(), 4451 );
    C_IMP1.s2( 1.0 )
    self.assertEqual( C_IMP1.s3( 1.5 ), "1.500" );

    arr = []
    for i in C_IMP1.each():
      arr.append(i)
    self.assertEqual( arr, [ 0, 1, 2, 3, 4, 5, 6, 0 ] )

    self.assertEqual( C_IMP2.s1(), 4451 );
    C_IMP2.s2( 2.0 )
    self.assertEqual( C_IMP2.s3( -1.5 ), "-1.500" );

    arr = []
    for i in C_IMP2.each():
      arr.append(i)
    self.assertEqual( arr, [ 0, 1, 2, 3, 4, 5, 6, 0, 0, 1 ] )

    c4 = C_IMP4()
    c4.call_vfunc(pya.CopyDetector(17))
    self.assertEqual(c4.x, 17)
    self.assertEqual(c4.xx, 17)

  def test_20(self):

    b = pya.B()

    a1 = b.amember_or_nil_alt( True )
    a2 = b.amember_ptr_alt()
    self.assertEqual( a1.get_n(), 17 )
    self.assertEqual( a2.get_n(), 17 )
    a1.a5( 761 )
    self.assertEqual( a1.get_n(), 761 )
    self.assertEqual( a2.get_n(), 761 )

    a1 = b.amember_or_nil( False )
    self.assertEqual( a1, None )
    
    self.assertEqual( b.b15( b.amember_ptr() ), True )
    self.assertEqual( b.b15( b.amember_or_nil( False ) ), False )
    self.assertEqual( b.b15( None ), False )

  def test_21(self):

    # Python does not allow extending built-in types - the following test
    # is taken from the Ruby binding. I don't know how to implement it for 
    # Python however.
    return

    # test client data binding to C++ objects 

    b = pya.B()
    
    # amember_ptr() returns a pya.A object, but it cannot be extended dynamically by a method s ...
    b.amember_ptr().s( 117 )
    self.assertEqual( b.amember_ptr().g(), 117 )

    n = 0

    def handler(a):
      a.s(n)
      n += 1

    b.b10_nc( lambda a: handler(a) ) 

    arr = []
    b.b10( lambda a: arr.append( a.g ) )
    self.assertEqual( arr, [ 0, 1, 2 ] )

    arr = []
    b.b10p( lambda a: arr.append( a.g ) )
    self.assertEqual( arr, [ 0, 1, 2 ] )

  def test_22(self):

    # test client data binding to C++ objects 
    
    b = pya.B()

    longint = 10000000000000000
    longint_as_int = (sys.maxsize > 5000000000 or (str(longint) == "10000000000000000" and type(longint) is int))
    
    self.assertEqual( b.b20a( 5.0 ), False )
    self.assertEqual( b.b20a( None ), True )
    self.assertEqual( b.b20a( 1 ), False )
    self.assertEqual( b.b20a( "hallo" ), False )
    self.assertEqual( b.b20a( False ), False )
    self.assertEqual( b.b20a( True ), False )
    self.assertEqual( b.b20a( longint ), False )
    self.assertEqual( b.b20b( 5.0 ), True )
    self.assertEqual( b.b20b( None ), False )
    self.assertEqual( b.b20b( 1 ), False )
    self.assertEqual( b.b20b( "hallo" ), False )
    self.assertEqual( b.b20b( False ), False )
    self.assertEqual( b.b20b( True ), False )
    if longint_as_int:
      # this fits into a long value, therefore this test returns false:
      self.assertEqual( b.b20b( longint ), False )
    else:
      # otherwise it is converted to a double:
      self.assertEqual( b.b20b( longint ), True )
    self.assertEqual( b.b20c( 5.0 ), False )
    self.assertEqual( b.b20c( None ), False )
    if longint_as_int:
      # this fits into a long value, therefore this test returns false:
      self.assertEqual( b.b20c( longint ), True )
    else:
      # otherwise it is converted to a double and the test returns false:
      self.assertEqual( b.b20c( longint ), False )
    self.assertEqual( b.b20c( "hallo" ), False )
    self.assertEqual( b.b20c( False ), False )
    self.assertEqual( b.b20c( True ), False )
    self.assertEqual( b.b20d( 5.0 ), False )
    self.assertEqual( b.b20d( None ), False )
    self.assertEqual( b.b20d( 1 ), False )
    self.assertEqual( b.b20d( "hallo" ), True )
    self.assertEqual( b.b20d( False ), False )
    self.assertEqual( b.b20d( True ), False )
    self.assertEqual( b.b20d( longint ), False )
    self.assertEqual( b.b20e( 5.0 ), False )
    self.assertEqual( b.b20e( None ), False )
    self.assertEqual( b.b20e( 1 ), False )
    self.assertEqual( b.b20e( "hallo" ), False )
    self.assertEqual( b.b20e( False ), True )
    self.assertEqual( b.b20e( True ), True )
    self.assertEqual( b.b20e( longint ), False )

    self.assertEqual( b.b21a( 50 ), "50" )
    self.assertEqual( b.b21a( True ), "true" )
    self.assertEqual( b.b21a( False ), "false" )
    self.assertEqual( b.b21a( "hallo" ), "hallo" )
    self.assertEqual( b.b21a( 5.5 ), "5.5" )
    self.assertEqual( b.b21a( None ), "nil" )

    self.assertEqual( b.b21b( 50 ), 50.0 )
    self.assertEqual( b.b21b( True ), 1.0 )
    self.assertEqual( b.b21b( False ), 0.0 )
    self.assertEqual( b.b21b( 5.5 ), 5.5 )
    self.assertEqual( b.b21b( None ), 0.0 )

    self.assertEqual( b.b21c( 50 ), 50 )
    self.assertEqual( b.b21c( True ), 1 )
    self.assertEqual( b.b21c( False ), 0 )
    self.assertEqual( b.b21c( 5.5 ), 5 )
    self.assertEqual( b.b21c( None ), 0 )

    self.assertEqual( b.b22a( [ 1, "hallo", 5.5 ] ), 3 ) 
    self.assertEqual( b.b23a(), [ 1, "hallo", 5.5 ] ) 
    a = [] 
    for x in b.b24():
      a.append(x) 
    self.assertEqual( a, [ 1, "hallo", 5.5 ] ) 
    self.assertEqual( b.b22c(), 5.5 )
    self.assertEqual( b.b22d(), 5.5 )
    self.assertEqual( b.b22a( [ 1, "hallo" ] ), 2 ) 
    self.assertEqual( b.b23b(), [ 1, "hallo" ] ) 
    self.assertEqual( b.b23d(), [ 1, "hallo" ] ) 
    self.assertEqual( b.b23e(), [ 1, "hallo" ] ) 
    self.assertEqual( b.b23e_null(), None ) 
    self.assertEqual( b.b23f(), [ 1, "hallo" ] ) 
    self.assertEqual( b.b23f_null(), None ) 
    self.assertEqual( b.b22c(), "hallo" )
    self.assertEqual( b.b22d(), "hallo" )
    self.assertEqual( b.b22a( [ ] ), 0 ) 
    self.assertEqual( b.b23c(), [ ] ) 
    a = [] 
    for x in b.b24():
      a.append(x) 
    self.assertEqual( a, [ ] ) 
    self.assertEqual( b.b22b(), None )
    self.assertEqual( b.b22c(), None )
    self.assertEqual( b.b22d(), None )
    self.assertEqual( b.b22a( [ [ 1, "hallo" ], [ 10, 17, 20 ] ] ), 2 ) 
    self.assertEqual( b.b23a(), [ [ 1, "hallo" ], [ 10, 17, 20 ] ] ) 
    a = [] 
    for x in b.b24():
      a.append(x) 
    self.assertEqual( a, [ [ 1, "hallo" ], [ 10, 17, 20 ] ] ) 

    # ability to pass complex objects over tl::Variant:
    self.assertEqual( b.b22a( [ pya.Box(pya.Point(0, 0), pya.Point(10, 20)) ] ), 1 ) 
    self.assertEqual( str(b.b22c()), "(0,0;10,20)" )
    self.assertEqual( type(b.b22c()).__name__, "Box" )

    # ability to pass complex objects over tl::Variant:
    self.assertEqual( b.b22a( [ pya.DBox(pya.DPoint(0, 0), pya.DPoint(10, 20)) ] ), 1 ) 
    self.assertEqual( str(b.b22c()), "(0,0;10,20)" )
    self.assertEqual( type(b.b22c()).__name__, "DBox" )

    # ability to pass complex objects over tl::Variant:
    self.assertEqual( b.b22a( [ pya.LayerInfo("hallo") ] ), 1 ) 
    self.assertEqual( str(b.b22c()), "hallo" )
    self.assertEqual( type(b.b22c()).__name__, "LayerInfo" )

    # byte arrays through Variants
    if sys.version_info >= (3, 0):
      self.assertEqual( b.b22a( [ bytes('abc', 'utf-8') ] ), 1 )
      self.assertEqual( str(b.b22c()), "b'abc'" )
      self.assertEqual( str(b.b22d()), "b'abc'" )
      self.assertEqual( str(b.var()), "b'abc'" )

  def test_23(self):

    b = pya.B()
    a = pya.A()

    self.assertEqual( b.bx(), 17 )
    self.assertEqual( b.b30(), 17 )
    self.assertEqual( b.bx(5), "xz" )
    self.assertEqual( b.by(5), "xz" )
    self.assertEqual( b.b31(6), "xz" )
    self.assertEqual( b.b33(a), "aref" )
    self.assertEqual( b.bx(a), "aref" )
    self.assertEqual( b.bx("a", 15), 20.5 )
    self.assertEqual( b.b32("b", 25), 20.5 )

    na = pya.A.instance_count()    # instance count
    self.assertEqual(b.bx(a, 15), "aref+i")
    self.assertEqual(pya.A.instance_count(), na)
    err_caught = False
    try: 
      # cannot cast second argument to int
      self.assertEqual(b.b34(a, "X"), "aref+i")
    except:
      err_caught = True
    self.assertEqual(err_caught, True)
    # the exception thrown before must not leave an instance on the call stack:
    self.assertEqual(pya.A.instance_count(), na)

    err_caught = False
    try: 
      # invalid number of arguments
      self.assertEqual(b.by(), "xz")
    except:
      err_caught = True
    self.assertEqual(err_caught, True)

    err_caught = False
    try: 
      # invalid number of arguments
      self.assertEqual(b.bx( 1, 5, 7 ), "xz")
    except:
      err_caught = True
    self.assertEqual(err_caught, True)
    
    b.destroy()
    a.destroy()

  def test_24(self):

    n = [ 0, 0 , "" ]

    # Events
    e = EEXT()
    e.m = 100

    e.s1() # no event installed
    self.assertEqual( 0, n[0] )
    e.s2()
    self.assertEqual( 0, n[1] )
    e.s3()
    self.assertEqual( "", n[2] )
  
    self.assertEqual( 100, e.m )

    # This is not allowed: e.e0( lambda: n0 = n0 + 1 ), hence we use a function:
    def f0():
      n[0] = n[0] + 1
    e.e0(f0)

    # This is not allowed: e.e1( lambda x: n1 = n1 + x.m ), hence we use a function:
    def f1(x):
      n[1] = n[1] + x.m
    e.e1(f1)

    # This is not allowed: e.e2( lambda i, s: n2 = n2 + str(i) + s), hence we use a function:
    def f2(i, s):
      n[2] = n[2] + str(i) + s
    e.e2(f2)

    e.s1()
    self.assertEqual( 1, n[0] )
    e.s1()
    self.assertEqual( 2, n[0] )

    # This is not allowed: p = lambda: n0 = n0 + 2, hence we use a function:
    def f4():
      n[0] = n[0] + 2

    e.e0(f4)
    e.s1()
    self.assertEqual( 4, n[0] )

    e.s2()
    self.assertEqual( 100, n[1] )
    e.m = 1
    e.s2()
    self.assertEqual( 101, n[1] )

    e.s3()
    self.assertEqual( "18hallo", n[2] )
    e.s3()
    self.assertEqual( "18hallo18hallo", n[2] )

    # NOTE: currently, exceptions are not propagated over
    # signals because of undefined behaviour
    if False:

      def raise_excpt():
        raise Exception("X")

      e.e0( lambda: raise_excpt() )
      error_caught = False
      try: 
        e.s1()
      except:
        error_caught = True
      self.assertEqual( error_caught, True )

    # Signals with return values are not supported currently
    if False:

      e.e0r( lambda x: 5 )
      self.assertEqual( e.s1r("x"), 5 )
      e.e0r( lambda s: len(s) + 2 )
      self.assertEqual( e.s1r("x"), 3 )
      self.assertEqual( e.s1r("abcxyz"), 8 )

      # Event bound to itself
      e.e0r(e.xfunc)
      self.assertEqual( e.s1r("x"), 1 )
      self.assertEqual( e.s1r("xxx"), 3 )

  def test_25(self):

    # destruction of an instance via c++
    pya.A.a20(None) 
    ic0 = pya.A.instance_count()
    a = pya.A()
    a.create()
    self.assertEqual(a.destroyed(), False)
    self.assertEqual(pya.A.instance_count(), ic0 + 1)
    pya.A.a20(a)    # install static instance of A
    self.assertEqual(a.destroyed(), False)
    pya.A.a20(None) 
    self.assertEqual(a.destroyed(), True)
    self.assertEqual(pya.A.instance_count(), ic0)

    a = pya.A()
    a.create()
    self.assertEqual(a.destroyed(), False)
    self.assertEqual(pya.A.instance_count(), ic0 + 1)
    pya.A.a20(a)    # install static instance of A
    self.assertEqual(a.destroyed(), False)
    self.assertEqual(pya.A.instance_count(), ic0 + 1)
    pya.A.a20(a)    # re-install static instance of A
    self.assertEqual(a.destroyed(), False)
    self.assertEqual(pya.A.instance_count(), ic0 + 1)
    
    # install another instance
    aa = pya.A()
    aa.create()
    self.assertEqual(aa.destroyed(), False)
    self.assertEqual(pya.A.instance_count(), ic0 + 2)
    pya.A.a20(aa)    # install static instance of A

    # original one is destroyed now, only new instance remains
    self.assertEqual(a.destroyed(), True)
    self.assertEqual(aa.destroyed(), False)
    self.assertEqual(pya.A.instance_count(), ic0 + 1)
    pya.A.a20(None)    # discard installed instance
    self.assertEqual(aa.destroyed(), True)
    self.assertEqual(pya.A.instance_count(), ic0)

    # the same without create .. should work too, but not create an instance because of late 
    # instantiation in default ctor
    a = pya.A()
    self.assertEqual(a.destroyed(), False)
    self.assertEqual(pya.A.instance_count(), ic0)
    pya.A.a20(a)    # install static instance of A
    self.assertEqual(a.destroyed(), False)
    pya.A.a20(None) 
    self.assertEqual(pya.A.instance_count(), ic0)
    self.assertEqual(a.destroyed(), True)

  def test_26(self):

    # cyclic references - event bound to itself

    base_count = EEXT.inst_count() 

    e = EEXT()
    e.e1(e.func)
    e.m = -17
    self.assertEqual(EEXT.inst_count(), base_count + 1)

    self.assertEqual(e.m, -17)
    # s2 will call e.func(self,x) with x=self and func will put x.n (which is 42) into x.m
    e.s2()
    self.assertEqual(e.m, 42)
    self.assertEqual(EEXT.inst_count(), base_count + 1)

    e = None
    self.assertEqual(EEXT.inst_count(), base_count)

    # the same, but with +=

    e = EEXT()
    e.e1 += e.func
    e.m = -17
    self.assertEqual(EEXT.inst_count(), base_count + 1)

    self.assertEqual(e.m, -17)
    # s2 will call e.func(self,x) with x=self and func will put x.n (which is 42) into x.m
    e.s2()
    self.assertEqual(e.m, 42)
    self.assertEqual(EEXT.inst_count(), base_count + 1)

    # now detach the event with -=
    e.e1 -= e.func
    e.m = -17
    e.s2()
    self.assertEqual(e.m, -17)
    self.assertEqual(EEXT.inst_count(), base_count + 1)

    e = None
    self.assertEqual(EEXT.inst_count(), base_count)

  def test_27(self):

    # destruction of an instance via c++
    pya.A.a20(None)
    ic0 = pya.A.instance_count()

    a = pya.A()
    a._create()
    self.assertEqual(a._destroyed(), False)
    self.assertEqual(pya.A.instance_count(), ic0 + 1)
    pya.A.a20(a)    
    self.assertEqual(pya.A.a20_get() == None, False)
    # release A instance -> will delete it
    a = None
    self.assertEqual(pya.A.instance_count(), ic0)
    self.assertEqual(pya.A.a20_get() == None, True)

    a = pya.A()
    a._create()
    self.assertEqual(a.destroyed(), False)
    self.assertEqual(pya.A.instance_count(), ic0 + 1)
    pya.A.a20(a)    # install static instance of A
    self.assertEqual(pya.A.a20_get() == None, False)
    a._unmanage()
    # release A instance -> won't delete it since it is unmanaged
    a = None
    self.assertEqual(pya.A.instance_count(), ic0 + 1)
    self.assertEqual(pya.A.a20_get() == None, False)

    a = pya.A.a20_get()
    a._manage()
    # release A instance -> will be deleted since now it's managed again
    a = None
    self.assertEqual(pya.A.instance_count(), ic0)
    self.assertEqual(pya.A.a20_get() == None, True)

  def test_28(self):

    self.assertEqual(pya.B.inst() == None, True)
    self.assertEqual(pya.B.has_inst(), False)

    b = pya.B()
    pya.B.set_inst(b)
    self.assertEqual(pya.B.has_inst(), True)
    self.assertEqual(pya.B.inst() == b, False)
    self.assertEqual(pya.B.inst().addr(), b.addr())

    # new B instance -> will delete the old one
    b = None
    self.assertEqual(pya.B.has_inst(), False)

    b = pya.B()
    pya.B.set_inst(b)
    b._unmanage()
    ba = b.addr()
    self.assertEqual(pya.B.has_inst(), True)
    self.assertEqual(pya.B.inst() == b, False)
    self.assertEqual(pya.B.inst().addr(), b.addr())

    # new B instance -> will not delete the old one (since we made it unmanaged)
    b = None
    self.assertEqual(pya.B.has_inst(), True)
    self.assertEqual(pya.B.inst().addr(), ba)

    # Make it managed again
    pya.B.inst()._manage()

    # new B instance -> will delete the old one (since we made it managed again)
    b = None
    self.assertEqual(pya.B.has_inst(), False)

  def test_29(self):
    
    # copy/ref semantics on return

    c = pya.C()
    
    cd = pya.CopyDetector(42)

    cd2 = c.pass_cd_direct(cd)
    self.assertEqual(cd2.x(), 42)
    # two copies: one for return statement and then one for the new object
    self.assertEqual(cd2.xx(), 44)
    
    cd2 = c.pass_cd_cref(cd)
    self.assertEqual(cd2.x(), 42)
    self.assertEqual(cd2.xx(), 43)
    
    cd2 = c.pass_cd_cref_as_copy(cd)
    self.assertEqual(cd2.x(), 42)
    self.assertEqual(cd2.xx(), 43)
    
    cd2 = c.pass_cd_cref_as_ref(cd)
    self.assertEqual(cd2.x(), 42)
    self.assertEqual(cd2.xx(), 42)
    
    cd2 = c.pass_cd_cptr(cd)
    self.assertEqual(cd2.x(), 42)
    self.assertEqual(cd2.xx(), 42)
    
    cd2 = c.pass_cd_cptr_as_copy(cd)
    self.assertEqual(cd2.x(), 42)
    self.assertEqual(cd2.xx(), 43)
    
    cd2 = c.pass_cd_cptr_as_ref(cd)
    self.assertEqual(cd2.x(), 42)
    self.assertEqual(cd2.xx(), 42)
    
    cd2 = c.pass_cd_ptr(cd)
    self.assertEqual(cd2.x(), 42)
    self.assertEqual(cd2.xx(), 42)
    
    cd2 = c.pass_cd_ptr_as_copy(cd)
    self.assertEqual(cd2.x(), 42)
    self.assertEqual(cd2.xx(), 43)
    
    cd2 = c.pass_cd_ptr_as_ref(cd)
    self.assertEqual(cd2.x(), 42)
    self.assertEqual(cd2.xx(), 42)
    
    cd2 = c.pass_cd_ref(cd)
    self.assertEqual(cd2.x(), 42)
    self.assertEqual(cd2.xx(), 42)
    
    cd2 = c.pass_cd_ref_as_copy(cd)
    self.assertEqual(cd2.x(), 42)
    self.assertEqual(cd2.xx(), 43)
    
    cd2 = c.pass_cd_ref_as_ref(cd)
    self.assertEqual(cd2.x(), 42)
    self.assertEqual(cd2.xx(), 42)

  def test_30(self):

    # some basic tests for the *Value boxing classes

    val = pya.Value()
    self.assertEqual(val.to_s(), "nil")
    self.assertEqual(val.value, None)
    val.value = 17.5
    self.assertEqual(val.value, 17.5)
    self.assertEqual(val.to_s(), "17.5")
    val.value += 1
    self.assertEqual(val.to_s(), "18.5")
    val = pya.Value(5)
    self.assertEqual(val.value, 5)
    val.value = None
    self.assertEqual(val.value, None)
    self.assertEqual(val.to_s(), "nil")

    val = pya.Value()
    self.assertEqual(val.to_s(), "nil")
    self.assertEqual(val.value, None)
    val.value = 17.5
    self.assertEqual(val.value, 17.5)
    self.assertEqual(val.to_s(), "17.5")
    val.value += 1
    self.assertEqual(val.to_s(), "18.5")
    val = pya.Value(5)
    self.assertEqual(val.value, 5)
    val.value = None
    self.assertEqual(val.value, None)
    self.assertEqual(val.to_s(), "nil")

    val = pya.Value()
    self.assertEqual(val.to_s(), "nil")
    self.assertEqual(val.value, None)
    val.value = True
    self.assertEqual(val.value, True)
    self.assertEqual(val.to_s(), "true")
    val = pya.Value(True)
    self.assertEqual(val.value, True)
    val.value = None
    self.assertEqual(val.value, None)
    self.assertEqual(val.to_s(), "nil")

    val = pya.Value()
    self.assertEqual(val.to_s(), "nil")
    self.assertEqual(val.value, None)
    val.value = 17
    self.assertEqual(val.value, 17)
    self.assertEqual(val.to_s(), "17")
    val.value += 1
    self.assertEqual(val.to_s(), "18")
    val = pya.Value(5)
    self.assertEqual(val.value, 5)
    val.value = None
    self.assertEqual(val.value, None)
    self.assertEqual(val.to_s(), "nil")

    val = pya.Value()
    self.assertEqual(val.to_s(), "nil")
    self.assertEqual(val.value, None)
    val.value = 17
    self.assertEqual(val.value, 17)
    self.assertEqual(val.to_s(), "17")
    val.value += 1
    self.assertEqual(val.to_s(), "18")
    val = pya.Value(5)
    self.assertEqual(val.value, 5)
    val.value = None
    self.assertEqual(val.value, None)
    self.assertEqual(val.to_s(), "nil")

    val = pya.Value()
    self.assertEqual(val.to_s(), "nil")
    self.assertEqual(val.value, None)
    val.value = 17
    self.assertEqual(val.value, 17)
    self.assertEqual(val.to_s(), "17")
    val.value += 1
    self.assertEqual(val.to_s(), "18")
    val = pya.Value(5)
    self.assertEqual(val.value, 5)
    val.value = None
    self.assertEqual(val.value, None)
    self.assertEqual(val.to_s(), "nil")

    val = pya.Value()
    self.assertEqual(val.to_s(), "nil")
    self.assertEqual(val.value, None)

    if sys.version_info < (3, 0):
      # Version 2.x has int instead of long
      val.value = 270000000
      self.assertEqual(val.value, 270000000)
      self.assertEqual(val.to_s(), "270000000")
      val.value += 1
      self.assertEqual(val.to_s(), "270000001")
    else:
      val.value = 2700000000
      self.assertEqual(val.value, 2700000000)
      self.assertEqual(val.to_s(), "2700000000")
      val.value += 1
      self.assertEqual(val.to_s(), "2700000001")

    val = pya.Value(500000000)
    self.assertEqual(val.value, 500000000)
    val.value = None
    self.assertEqual(val.value, None)
    self.assertEqual(val.to_s(), "nil")

    val = pya.Value()
    self.assertEqual(val.to_s(), "nil")
    self.assertEqual(val.value, None)

    if sys.version_info < (3, 0):
      # Version 2.x has int instead of long
      val.value = 1700000000
      self.assertEqual(val.to_s(), "1700000000")
      self.assertEqual(val.value, 1700000000)
      val.value += 1
      self.assertEqual(val.to_s(), "1700000001")
      val = pya.Value(500000000)
      self.assertEqual(val.value, 500000000)
    else:
      val.value = 170000000000
      self.assertEqual(val.to_s(), "170000000000")
      self.assertEqual(val.value, 170000000000)
      val.value += 1
      self.assertEqual(val.to_s(), "170000000001")
      val = pya.Value(50000000000)
      self.assertEqual(val.value, 50000000000)

    val.value = None
    self.assertEqual(val.value, None)
    self.assertEqual(val.to_s(), "nil")

    val = pya.Value()
    self.assertEqual(val.to_s(), "nil")
    self.assertEqual(val.value, None)

    if sys.version_info < (3, 0):
      # Version 2.x has int instead of long
      val.value = 1700000000
      self.assertEqual(val.value, 1700000000)
      self.assertEqual(val.to_s(), "1700000000")
      val.value += 1
      self.assertEqual(val.to_s(), "1700000001")
      val = pya.Value(500000000)
      self.assertEqual(val.value, 500000000)
    else:
      val.value = 170000000000
      self.assertEqual(val.value, 170000000000)
      self.assertEqual(val.to_s(), "170000000000")
      val.value += 1
      self.assertEqual(val.to_s(), "170000000001")
      val = pya.Value(50000000000)
      self.assertEqual(val.value, 50000000000)

    val.value = None
    self.assertEqual(val.value, None)
    self.assertEqual(val.to_s(), "nil")

    val = pya.Value()
    self.assertEqual(val.value, None)
    self.assertEqual(val.to_s(), "nil")
    val.value = "abc"
    self.assertEqual(val.value, "abc")
    self.assertEqual(val.to_s(), "abc")
    val.value += "x"
    self.assertEqual(val.to_s(), "abcx")
    val = pya.Value("uv")
    self.assertEqual(val.value, "uv")
    val.value = None
    self.assertEqual(val.value, None)
    self.assertEqual(val.to_s(), "nil")

  def test_31(self):

    # some basic tests with derived and base classes

    pya.X.init()
    pya.Y.init()
    x = pya.X("hallo")
    self.assertEqual(True, isinstance(x, pya.X))
    self.assertEqual(False, isinstance(x, pya.A))
    self.assertEqual(False, isinstance(x, pya.Y))
    self.assertEqual("hallo", x.s)
    self.assertEqual("X", x.cls_name())
    cxp = pya.X.x_cptr()
    self.assertEqual("X::a", cxp.s)
    try: 
      cxp.s = "x"
      error_caught = False
    except:
      error_caught = True
    self.assertEqual(True, error_caught)
    xp = pya.X.x_ptr()
    self.assertEqual("X::a", xp.s)

    s_old = xp.s
    xp.s = "x"
    self.assertEqual("x", xp.s)
    xp.s = 41
    self.assertEqual("42", xp.s)
    xp.s = s_old

    y = pya.Y("hallo")
    self.assertEqual(True, isinstance(y, pya.X))
    self.assertEqual(False, isinstance(y, pya.A))
    self.assertEqual(True, isinstance(y, pya.Y))
    self.assertEqual("hallo", y.s)
    self.assertEqual("Y", y.cls_name())
    self.assertEqual(5, y.i())
    cyp = pya.Y.y_cptr()
    self.assertEqual("Y::a", cyp.s)
    self.assertEqual("Y", cyp.cls_name())
    try: 
      cyp.s = "y"
      error_caught = False
    except:
      error_caught = True
    self.assertEqual(True, error_caught)
    yp = pya.Y.y_ptr()
    self.assertEqual("Y", yp.cls_name())
    self.assertEqual("Y::a", yp.s)
    yp.s = "y"
    self.assertEqual("y", yp.s)
    self.assertEqual(1, yp.i())
    yp.s = "abc"
    self.assertEqual(3, yp.i())
    self.assertEqual("Y", type(yp).__name__)

  def test_32(self):

    # run test only if we have Qt bindings 
    if not "QStringPair" in pya.__dict__:
      return

    # QPair<String, String>
    p = pya.QStringPair()
    p.first = "a"
    p.second = "b"
    self.assertEqual("a", p.first)
    self.assertEqual("b", p.second)
    pp = p.dup()
    self.assertEqual("a", pp.first)
    self.assertEqual("b", pp.second)
    pp.first = "u"
    self.assertEqual("a", p.first)
    self.assertEqual("b", p.second)
    self.assertEqual("u", pp.first)
    self.assertEqual("b", pp.second)
    self.assertEqual(pp == p, False)
    self.assertEqual(pp != p, True)
    pp = pya.QStringPair("a", "b")
    self.assertEqual("a", pp.first)
    self.assertEqual("b", pp.second)
    self.assertEqual(pp == p, True)
    self.assertEqual(pp != p, False)

  def test_33(self):
  
    def str_from_bytearray(ba):
      if (sys.version_info > (3, 0)):
        return ba
      else:
        return ba.decode("utf-8")  

    # run test only if we have Qt bindings 
    if not "QByteArrayPair" in pya.__dict__:
      return

    # QPair<QByteArray, QByteArray>
    p = pya.QByteArrayPair()
    p.first = "a"
    p.second = "b"
    self.assertEqual("a", str_from_bytearray(p.first))
    self.assertEqual("b", str_from_bytearray(p.second))
    pp = p.dup()
    self.assertEqual("a", str_from_bytearray(pp.first))
    self.assertEqual("b", str_from_bytearray(pp.second))
    pp.first = "u"
    self.assertEqual("a", str_from_bytearray(p.first))
    self.assertEqual("b", str_from_bytearray(p.second))
    self.assertEqual("u", str_from_bytearray(pp.first))
    self.assertEqual("b", str_from_bytearray(pp.second))
    self.assertEqual(pp == p, False)
    self.assertEqual(pp != p, True)
    pp = pya.QByteArrayPair("a", "b")
    self.assertEqual("a", str_from_bytearray(pp.first))
    self.assertEqual("b", str_from_bytearray(pp.second))
    self.assertEqual(pp == p, True)
    self.assertEqual(pp != p, False)

  def test_34(self):

    # run test only if we have Qt bindings 
    if not "QDialog" in pya.__dict__:
      return

    # QDialog and QWidget
    # Hint: QApplication creates some leaks (FT, GTK). Hence it must not be used in the leak_check case ..
    if not leak_check:
      a = pya.QCoreApplication.instance()
      self.assertEqual("<class 'pya.Application'>", str(type(a)))
      qd = pya.QDialog()
      pya.QApplication.setActiveWindow(qd)
      self.assertEqual(repr(pya.QApplication.activeWindow), repr(qd))
      self.assertEqual("<class 'pya.QDialog'>", str(type(pya.QApplication.activeWindow)))
      qd._destroy()
      self.assertEqual(repr(pya.QApplication.activeWindow), "None")

  def test_35(self):

    # vectors of pointers

    pya.X.init()
    pya.Y.init()

    vx_cptr = pya.X.vx_cptr()
    self.assertEqual(2, len(vx_cptr))
    try: 
      vx_cptr[0].s = "y"
      error_caught = False
    except:
      error_caught = True
    self.assertEqual(True, error_caught)

    vx = pya.X.vx()
    self.assertEqual(2, len(vx))
    self.assertEqual("X::a", vx[0].s)
    self.assertEqual("X::b", vx[1].s)

    vx_ptr = pya.X.vx_ptr()
    self.assertEqual(2, len(vx_ptr))
    self.assertEqual("X", type(vx_ptr[0]).__name__)
    self.assertEqual("X", type(vx_ptr[1]).__name__)

    vx_ptr[0].s = "u"
    self.assertEqual("u", vx_cptr[0].s)
    self.assertEqual("X::a", vx[0].s)
    self.assertEqual("X::b", vx[1].s)

    vy_cptr = pya.Y.vy_cptr()
    self.assertEqual(2, len(vy_cptr))

    try: 
      vy_cptr[0].s = "y"
      error_caught = False
    except:
      error_caught = True
    self.assertEqual(True, error_caught)

    vy_cptr = pya.Y.vyasx_cptr()
    self.assertEqual(2, len(vy_cptr))

    try: 
      vy_cptr[0].s = "y"
      error_caught = False
    except:
      error_caught = True
    self.assertEqual(True, error_caught)

    vy0_ptr = pya.Y.vy0_ptr()
    self.assertEqual(1, len(vy0_ptr))
    self.assertEqual("None", str(vy0_ptr[0]))

    vy_ptr = pya.Y.vy_ptr()
    self.assertEqual(2, len(vy_ptr))
    self.assertEqual("Y", type(vy_ptr[0]).__name__)
    self.assertEqual("Y", type(vy_ptr[1]).__name__)

    vy_ptr[0].s = "uvw"
    self.assertEqual("uvw", vy_cptr[0].s)
    self.assertEqual(3, vy_cptr[0].i())

    vy_ptr = pya.Y.vyasx_ptr()
    self.assertEqual(2, len(vy_ptr))
    self.assertEqual("Y", type(vy_ptr[0]).__name__)
    self.assertEqual("Y", type(vy_ptr[1]).__name__)

    vy_ptr[0].s = "uvw"
    self.assertEqual("uvw", vy_cptr[0].s)
    self.assertEqual(3, vy_cptr[0].i())

    y = pya.Y("")
    yc = y.vx_dyn_count()
    y.vx_dyn_make()
    self.assertEqual(yc + 1, y.vx_dyn_count())
    y.vx_dyn_destroy()
    self.assertEqual(yc, y.vx_dyn_count())

    y.vx_dyn_make()
    self.assertEqual(yc + 1, y.vx_dyn_count())
    yy = y.vx_dyn()
    self.assertEqual(1, len(yy))
    self.assertEqual("Y", type(yy[0]).__name__)
    self.assertEqual(True, yy[0] != None)
    yy[0].destroy()
    self.assertEqual(True, yy[0].destroyed())
    self.assertEqual(yc, y.vx_dyn_count())

    y.vx_dyn_make()
    self.assertEqual(yc + 1, y.vx_dyn_count())
    yy = y.vx_dyn()
    self.assertEqual(1, len(yy))
    self.assertEqual("Y", type(yy[0]).__name__)
    self.assertEqual(True, yy[0] != None)
    y.vx_dyn_destroy()
    self.assertEqual(True, yy[0].destroyed())
    self.assertEqual(yc, y.vx_dyn_count())

  def test_36(self):

    x = XEdge()
    self.assertEqual("XEdge", type(x).__name__)
    self.assertEqual("(1,2;3,4)", str(x))

  def test_37(self):

    # This test is taken from the Ruby binding, but
    # Python does not have protected methods:
    return

    # protected methods
    ok = False
    a = pya.A()
    e = ""
    try:
      a.a10_prot() # cannot be called - is protected
      ok = True
    except Exception as ex:
      e = str(ex)
    self.assertEqual(e == "protected method `a10_prot' called", 0)
    self.assertEqual(ok, False)
    self.assertEqual(a.call_a10_prot(1.25), "1.25")

  def test_38(self):

    # mixed const / non-const reference and events
    ec = pya.E.ic()
    self.assertEqual(ec.is_const_object(), True)
    enc = pya.E.inc()
    # Now, ec has turned into a non-const reference as well!
    # This is strange but is a consequence of the unique C++/Python binding and there can 
    # only be a non-const or const ruby object!
    self.assertEqual(ec.is_const_object(), False)
    self.assertEqual(enc.is_const_object(), False)

    # the "True reference" is a not copy since E is derived from ObjectBase
    ec.x = 15
    self.assertEqual(ec.x, 15);
    ec2 = pya.E.ic()
    self.assertEqual(ec2.x, 15);
    ec2 = pya.E.icref()
    self.assertEqual(ec2.x, 15);
    ec2.x = 17
    self.assertEqual(ec2.x, 17);
    self.assertEqual(ec.x, 17);
    self.assertEqual(ec2.is_const_object(), False) # because it's a copy

    # the "True reference" is a not copy since E is derived from ObjectBase
    enc2 = pya.E.incref()
    self.assertEqual(enc2.x, 17);
    enc2.x = 19
    self.assertEqual(enc2.x, 19);
    self.assertEqual(ec.x, 19);  # because the non-const reference by incref is not a copy

  def test_39(self):

    # mixed const / non-const reference and events
    fc = pya.F.ic()
    self.assertEqual(fc.is_const_object(), True)
    fnc = pya.F.inc()
    # In contrase to E, the fc reference is not touched because F is not derived
    # from ObjectBase
    self.assertEqual(fc.is_const_object(), True)
    self.assertEqual(fnc.is_const_object(), False)

    # the "True reference" is a copy
    fnc.x = 15
    self.assertEqual(fc.x, 15);
    fc2 = pya.F.ic()
    self.assertEqual(fc2.x, 15);
    fc2 = pya.F.icref()
    self.assertEqual(fc2.x, 15);
    fc2.x = 17
    self.assertEqual(fc2.x, 17);
    self.assertEqual(fc.x, 15);
    self.assertEqual(fc2.is_const_object(), False) # because it's a copy

    # the "True reference" is a copy
    fnc2 = pya.F.incref()
    self.assertEqual(fnc2.x, 15);
    fnc2.x = 19
    self.assertEqual(fnc2.x, 19);
    self.assertEqual(fc.x, 19);  # because the non-const reference by incref is not a copy

  def test_40(self):

    # optional arguments
    g = pya.G()

    self.assertEqual(g.iv(), 0)
    g.set_iva(2)
    self.assertEqual(g.iv(), 2)
    g.set_ivb(3)
    self.assertEqual(g.iv(), 3)
    g.set_ivb()
    self.assertEqual(g.iv(), 1)
    g.set_sv1a("hello")
    self.assertEqual(g.sv(), "hello")

    failed = False
    try:
      g.set_sv1a()
    except: 
      failed = True
    self.assertEqual(failed, True)

    g.set_sv1b("world")
    self.assertEqual(g.sv(), "world")
    g.set_sv1b()
    self.assertEqual(g.sv(), "value")
    g.set_sv2a("hello")
    self.assertEqual(g.sv(), "hello")

    failed = False
    try:
      g.set_sv2a()
    except: 
      failed = True
    self.assertEqual(failed, True)

    g.set_sv2b("world")
    self.assertEqual(g.sv(), "world")
    g.set_sv2b()
    self.assertEqual(g.sv(), "value")

    g.set_vva(17, "c")
    self.assertEqual(g.iv(), 17)
    self.assertEqual(g.sv(), "c")

    failed = False
    try:
      g.set_svva()
    except: 
      failed = True
    self.assertEqual(failed, True)

    failed = False
    try:
      g.set_svva(11)
    except: 
      failed = True
    self.assertEqual(failed, True)

    g.set_vvb(11)
    self.assertEqual(g.iv(), 11)
    self.assertEqual(g.sv(), "value")
    g.set_vvb(10, "None")
    self.assertEqual(g.iv(), 10)
    self.assertEqual(g.sv(), "None")

    failed = False
    try:
      g.set_svvb()
    except: 
      failed = True
    self.assertEqual(failed, True)

    g.set_vvc(11)
    self.assertEqual(g.iv(), 11)
    self.assertEqual(g.sv(), "value")
    g.set_vvc()
    self.assertEqual(g.iv(), 1)
    self.assertEqual(g.sv(), "value")
    g.set_vvc(17, "None")
    self.assertEqual(g.iv(), 17)
    self.assertEqual(g.sv(), "None")

  def test_41(self):

    # maps 
    b = pya.B()

    b.insert_map1(1, "hello")
    self.assertEqual(map2str(b.map1), "{1: hello}")

    b.map1 = {}
    b.insert_map1(2, "hello")
    self.assertEqual(map2str(b.map1_cref()), "{2: hello}")

    b.map1 = {}
    b.insert_map1(3, "hello")
    self.assertEqual(map2str(b.map1_cptr()), "{3: hello}")

    b.map1 = {}
    b.insert_map1(4, "hello")
    self.assertEqual(map2str(b.map1_ref()), "{4: hello}")

    b.map1 = {}
    b.insert_map1(5, "hello")
    self.assertEqual(map2str(b.map1_ptr()), "{5: hello}")

    self.assertEqual(b.map1_cptr_null() == None, True);
    self.assertEqual(b.map1_ptr_null() == None, True);

    b.map1 = { 42: 1, -17: True }
    self.assertEqual(map2str(b.map1), "{-17: True, 42: 1}")

    b.map1 = { 42: "1", -17: "True" }
    self.assertEqual(map2str(b.map1), "{-17: True, 42: 1}")

    b.set_map1_cref({ 42: "2", -17: "True" })
    self.assertEqual(map2str(b.map1), "{-17: True, 42: 2}")

    b.set_map1_cptr({ 42: "3", -17: "True" })
    self.assertEqual(map2str(b.map1), "{-17: True, 42: 3}")

    b.set_map1_cptr(None)
    self.assertEqual(map2str(b.map1), "{-17: True, 42: 3}")

    b.set_map1_ref({ 42: "4", -17: "True" })
    self.assertEqual(map2str(b.map1), "{-17: True, 42: 4}")

    b.set_map1_ptr({ 42: "5", -17: "True" })
    self.assertEqual(map2str(b.map1), "{-17: True, 42: 5}")

    b.set_map1_ptr(None)
    self.assertEqual(map2str(b.map1), "{-17: True, 42: 5}")

    b.map2 = { 'xy': 1, -17: True }
    self.assertEqual(map2str(b.map2), "{-17: True, xy: 1}")

    self.assertEqual(b.map2_null(), None)

  def test_42(self):

    # virtual functions and sub-classes 
    z = pya.Z()
    self.assertEqual(z.f(None), "(nil)")
    self.assertEqual(z.f(pya.X("hello")), "hello")

    z1 = Z_IMP1()
    self.assertEqual(z1.f(pya.X("a")), "X")
    self.assertEqual(z1.f(pya.Y("b")), "Y")
    self.assertEqual(z1.f_with_x("a"), "X")
    self.assertEqual(z1.f_with_y("b"), "Y")
    self.assertEqual(z1.f_with_yy("b"), "YY")

    z2 = Z_IMP2()
    self.assertEqual(z2.f(pya.X("1")), "X")
    self.assertEqual(z2.f(pya.Y("2")), "Y")
    self.assertEqual(z2.f_with_x("1"), "X")
    self.assertEqual(z2.f_with_y("2"), "Y")
    self.assertEqual(z2.f_with_yy("3"), "Y")

    z1 = Z_IMP3()
    self.assertEqual(z1.f(pya.X("x")), "x*")
    self.assertEqual(z1.f(pya.Y("y")), "y*")
    self.assertEqual(z1.f_with_x("x"), "x*")
    self.assertEqual(z1.f_with_y("y"), "y*")
    self.assertEqual(z1.f_with_yy("yy"), "yy*")

  def test_50(self):
  
    # advanced containers and out parameters

    b = pya.B()
    a1 = AEXT()
    a1.s(42)
    a1.a5(11)
    a2 = AEXT()
    a2.s(17)
    a2.a5(22)
    a3 = AEXT()
    a3.s(33)
    a3.a5(33)

    b.set_map_iaptr( { 1: a1, 2: a2 } )
    self.assertEqual(repr(b.map_iaptr()), ph("{1X: 42, 2X: 17}"))
    self.assertEqual(repr(b.map_iaptr_cref()), ph("{1X: 42, 2X: 17}"))
    self.assertEqual(repr(b.map_iaptr_ref()), ph("{1X: 42, 2X: 17}"))
    self.assertEqual(repr(b.map_iaptr_cptr()), ph("{1X: 42, 2X: 17}"))
    self.assertEqual(repr(b.map_iaptr_ptr()), ph("{1X: 42, 2X: 17}"))
    b.set_map_iaptr_cptr(None)
    self.assertEqual(repr(b.map_iaptr()), "{}")
    b.set_map_iaptr_cptr( { 17: a2, 42: a1 } )
    self.assertEqual(repr(b.map_iaptr()), ph("{17X: 17, 42X: 42}"))
    b.set_map_iaptr_ptr( { 18: a2, 43: a1 } )
    self.assertEqual(repr(b.map_iaptr()), ph("{18X: 17, 43X: 42}"))
    b.set_map_iaptr_ref( { 1: a2, 3: a1 } )
    self.assertEqual(repr(b.map_iaptr()), ph("{1X: 17, 3X: 42}"))
    b.set_map_iaptr_cref( { 2: a2, 4: a1 } )
    self.assertEqual(repr(b.map_iaptr()), ph("{2X: 17, 4X: 42}"))
    b.set_map_iaptr_ptr( { } )
    self.assertEqual(repr(b.map_iaptr()), "{}")
    b.set_map_iaptr_cref( { 2: a2, 4: a1 } )
    self.assertEqual(repr(b.map_iaptr()), ph("{2X: 17, 4X: 42}"))
    b.set_map_iaptr_ptr(None)
    self.assertEqual(repr(b.map_iaptr()), "{}")

    m = { 2: a1, 4: a2 }
    # map as an "out" parameter:
    pya.B.insert_map_iaptr(m, 3, a3)
    self.assertEqual(repr(m), ph("{2X: 42, 3X: 33, 4X: 17}"))

    b.set_map_iacptr( { 1: a1, 2: a2 } )
    self.assertEqual(repr(b.map_iacptr()), ph("{1X: 42, 2X: 17}"))
    m = { 2: a1, 4: a2 }
    # map as an "out" parameter:
    pya.B.insert_map_iacptr(m, 5, a3)
    self.assertEqual(repr(m), ph("{2X: 42, 4X: 17, 5X: 33}"))

    b.set_map_ia( { 1: a1, 2: a2 } )
    # because we have raw copies, the Ruby-add-on is lost and
    # only a1 (built-in) remains as an attribute:
    self.assertEqual(repr(b.map_ia()), ph("{1X: a1=11, 2X: a1=22}"))
    m = { 2: a1, 4: a2 }
    # map as an "out" parameter:
    pya.B.insert_map_ia(m, 5, a3)
    self.assertEqual(repr(m), ph("{2X: a1=11, 4X: a1=22, 5X: a1=33}"))

    b.set_map_iav( { 1: [ a1, a2 ], 2: [] } )
    # because we have raw copies, the Ruby-add-on is lost and
    # only a1 (built-in) remains as an attribute:
    self.assertEqual(repr(b.map_iav()), ph("{1X: [a1=11, a1=22], 2X: []}"))
    m = { 1: [ a1, a2 ], 2: [] }
    # map as an "out" parameter:
    pya.B.push_map_iav(m, 2, a3)
    self.assertEqual(repr(m), ph("{1X: [a1=11, a1=22], 2X: [a1=33]}"))
    pya.B.insert_map_iav(m, 5, [ a1, a3 ])
    self.assertEqual(repr(m), ph("{1X: [a1=11, a1=22], 2X: [a1=33], 5X: [a1=11, a1=33]}"))

    v = [ [ "a", "aa" ], [] ]
    pya.B.push_vvs( v, [ "1", "2" ] )
    self.assertEqual(v, [["a", "aa"], [], ["1", "2"]])
    b.set_vvs( [ [ "1" ], [ "2", "3" ] ] )
    self.assertEqual(b.vvs(), [["1"], ["2", "3"]])
    self.assertEqual(b.vvs_ref(), [["1"], ["2", "3"]])
    self.assertEqual(b.vvs_cref(), [["1"], ["2", "3"]])
    self.assertEqual(b.vvs_ptr(), [["1"], ["2", "3"]])
    self.assertEqual(b.vvs_cptr(), [["1"], ["2", "3"]])
    b.set_vvs_ref( [ [ "1" ], [ "2", "3" ] ] )
    self.assertEqual(b.vvs(), [["1"], ["2", "3"]])
    b.set_vvs_cref( [ [ "2" ], [ "1", "3" ] ] )
    self.assertEqual(b.vvs(), [["2"], ["1", "3"]])
    b.set_vvs_ptr( [ [ "1" ], [ "3", "2" ] ] )
    self.assertEqual(b.vvs(), [["1"], ["3", "2"]])
    b.set_vvs_ptr(None)
    self.assertEqual(b.vvs(), [])
    b.set_vvs_cptr( [ [ "0" ], [ "3", "2" ] ] )
    self.assertEqual(b.vvs(), [["0"], ["3", "2"]])
    b.set_vvs_cptr(None)
    self.assertEqual(b.vvs(), [])

    v = [ "a", "b" ]
    pya.B.push_ls(v, "x")
    self.assertEqual(v, [ "a", "b", "x" ])
    b.set_ls([ "1" ])
    self.assertEqual(b.ls(), [ "1" ])
    b.set_ls([])
    self.assertEqual(b.ls(), [])

    # Tuples cannot be modified - this will not work:
    # v = ( "a", "b" )
    # pya.B.push_ls(v, "x")
    # self.assertEqual(v, ( "a", "b", "x" ))
    b.set_ls(( "1", "2" ))
    self.assertEqual(b.ls(), [ "1", "2" ])
    b.set_ls(())
    self.assertEqual(b.ls(), [])

    v = [ "a", "b" ]
    pya.B.push_ss(v, "x")
    self.assertEqual(v, [ "a", "b", "x" ])
    b.set_ss([ "1" ])
    self.assertEqual(b.ss(), [ "1" ])
    b.set_ss([])
    self.assertEqual(b.ss(), [])

    if "set_qls" in b.__dict__:

      v = [ "a", "b" ]
      pya.B.push_qls(v, "x")
      self.assertEqual(v, [ "a", "b", "x" ])
      b.set_qls([ "1" ])
      self.assertEqual(b.qls(), [ "1" ])
      b.set_qls([])
      self.assertEqual(b.qls(), [])

      v = [ "a", 1 ]
      pya.B.push_qlv(v, 2.5)
      self.assertEqual(v, [ "a", 1, 2.5 ])
      b.set_qlv([ 17, "1" ])
      self.assertEqual(b.qlv(), [ 17, "1" ])
      b.set_qlv([])
      self.assertEqual(b.qlv(), [])

      v = [ "a", "b" ]
      pya.B.push_qsl(v, "x")
      self.assertEqual(v, [ "a", "b", "x" ])
      b.set_qsl([ "1" ])
      self.assertEqual(b.qsl(), [ "1" ])
      b.set_qsl([])
      self.assertEqual(b.qsl(), [])

      v = [ "a", "b" ]
      pya.B.push_qvs(v, "x")
      self.assertEqual(v, [ "a", "b", "x" ])
      b.set_qvs([ "1" ])
      self.assertEqual(b.qvs(), [ "1" ])
      b.set_qvs([])
      self.assertEqual(b.qvs(), [])

      v = [ "a", "b" ]
      pya.B.push_qss(v, "x")
      v_sorted = v
      v_sorted.sort()
      self.assertEqual(v_sorted, [ "a", "b", "x" ])
      b.set_qss([ "1" ])
      self.assertEqual(b.qss(), [ "1" ])
      b.set_qss([])
      self.assertEqual(b.qss(), [])

      v = { 1: "a", 17: "b" }
      pya.B.insert_qmap_is(v, 2, "x")
      self.assertEqual(v, { 1: "a", 17: "b", 2: "x" })
      b.set_qmap_is({ 1: "t", 17: "b" })
      self.assertEqual(b.qmap_is(), { 1: "t", 17: "b" })
      b.set_qmap_is({})
      self.assertEqual(b.qmap_is(), {})

      v = { 1: "a", 17: "b" }
      pya.B.insert_qhash_is(v, 2, "x")
      self.assertEqual(v, { 1: "a", 17: "b", 2: "x" })
      b.set_qhash_is({ 1: "t", 17: "b" })
      self.assertEqual(b.qhash_is(), { 1: "t", 17: "b" })
      b.set_qhash_is({})
      self.assertEqual(b.qhash_is(), {})

  def test_51(self):
  
    # new subclass and child class declarations
    y2 = pya.Y2()
    self.assertEqual(y2.x1(), 2)
    self.assertEqual(y2.x2(), 42)
    self.assertEqual(isinstance(y2, pya.X), True)

    y3 = pya.Z.Y3()
    self.assertEqual(y3.x1(), 3)
    self.assertEqual(y3.x2(), 42)
    self.assertEqual(isinstance(y3, pya.X), True)

    y4 = pya.Z.Y4()
    self.assertEqual(y4.x1(), 4)
    self.assertEqual(isinstance(y4, pya.X), False)

  def test_60(self):

    if not "SQ" in pya.__dict__:
      return

    class SignalCollector(object):

      got_s0 = False
      got_s1 = None
      got_s2_1 = None
      got_s2_2 = None

      def f_s0(self):
        self.got_s0 = True

      def f_s1(self, iv):
        self.got_s1 = iv

      def f_s2(self, s, obj):
        self.got_s2_1 = s
        self.got_s2_2 = obj

    sc = SignalCollector()
  
    sq = pya.SQ()

    sq.s0 = sc.f_s0
    sq.trigger_s0()
    self.assertEqual(sc.got_s0, True)

    sq.s1 = sc.f_s1
    sq.trigger_s1(17)
    self.assertEqual(sc.got_s1, 17)
    sq.trigger_s1(42)
    self.assertEqual(sc.got_s1, 42)

    sq.tag = 999

    sq.s2 = sc.f_s2
    sq.trigger_s2("foo")
    self.assertEqual(sc.got_s2_1, "foo")
    self.assertEqual(sc.got_s2_2.tag, 999)
    sq.tag = 111
    sc.got_s2_2 = None 
    sq.trigger_s2("bar")
    self.assertEqual(sc.got_s2_1, "bar")
    self.assertEqual(sc.got_s2_2.tag, 111)

    # clear handler
    sq.s2 = None

    sq.tag = 0
    sc.got_s2_1 = None 
    sc.got_s2_2 = None 
    sq.trigger_s2("x")
    self.assertEqual(sc.got_s2_1, None)
    self.assertEqual(sc.got_s2_2, None)

    # attach again with set
    sq.s2.set(sc.f_s2)
    sq.tag = 2
    sc.got_s2_1 = None 
    sc.got_s2_2 = None 
    sq.trigger_s2("y")
    self.assertEqual(sc.got_s2_1, "y")
    self.assertEqual(sc.got_s2_2.tag, 2)

    # clear handler (with clear)
    sq.s2.clear()

    sq.tag = 0
    sc.got_s2_1 = None 
    sc.got_s2_2 = None 
    sq.trigger_s2("z")
    self.assertEqual(sc.got_s2_1, None)
    self.assertEqual(sc.got_s2_2, None)

    # attach again with add
    sq.s2.set(sc.f_s2)
    sq.tag = 2222
    sc.got_s2_1 = None 
    sc.got_s2_2 = None 
    sq.trigger_s2("u")
    self.assertEqual(sc.got_s2_1, "u")
    self.assertEqual(sc.got_s2_2.tag, 2222)

    # clear handler (with remove)
    sq.s2.remove(sc.f_s2)

    sq.tag = 0
    sc.got_s2_1 = None 
    sc.got_s2_2 = None 
    sq.trigger_s2("v")
    self.assertEqual(sc.got_s2_1, None)
    self.assertEqual(sc.got_s2_2, None)

  def test_61(self):
  
    if not "SQ" in pya.__dict__:
      return

    class SignalCollector(object):

      got_s0a = 0
      got_s0b = 0

      def p1(self):
        self.got_s0a += 1

      def p1b(self):
        self.got_s0a += 1

      def p2(self):
        self.got_s0b += 1

    sc = SignalCollector()
  
    sq = pya.SQ()

    sq.s0 = sc.p1
    sq.trigger_s0()
    self.assertEqual(sc.got_s0a, 1)
    self.assertEqual(sc.got_s0b, 0)

    sc.got_s0a = 0
    sc.got_s0b = 0
    sq.s0 = sc.p2
    sq.trigger_s0()
    self.assertEqual(sc.got_s0a, 0)
    self.assertEqual(sc.got_s0b, 1)

    sc.got_s0a = 0
    sc.got_s0b = 0
    sq.s0 += sc.p1
    sq.trigger_s0()
    self.assertEqual(sc.got_s0a, 1)
    self.assertEqual(sc.got_s0b, 1)

    sc.got_s0a = 0
    sc.got_s0b = 0 
    # same proc is not added again
    sq.s0 += sc.p1
    sq.trigger_s0()
    self.assertEqual(sc.got_s0a, 1)
    self.assertEqual(sc.got_s0b, 1)

    sc.got_s0a = 0
    sc.got_s0b = 0
    # second proc p1 with same effect
    sq.s0 += sc.p1b
    sq.trigger_s0()
    self.assertEqual(sc.got_s0a, 2)
    self.assertEqual(sc.got_s0b, 1)

    sc.got_s0a = 0
    sc.got_s0b = 0
    sq.s0 -= sc.p1
    sq.trigger_s0()
    self.assertEqual(sc.got_s0a, 1)
    self.assertEqual(sc.got_s0b, 1)

    sc.got_s0a = 0
    sc.got_s0b = 0
    sq.s0 -= sc.p1b
    sq.trigger_s0()
    self.assertEqual(sc.got_s0a, 0)
    self.assertEqual(sc.got_s0b, 1)

    sc.got_s0a = 0
    sc.got_s0b = 0
    sq.s0 -= sc.p1
    sq.trigger_s0()
    self.assertEqual(sc.got_s0a, 0)
    self.assertEqual(sc.got_s0b, 1)

    sc.got_s0a = 0
    sc.got_s0b = 0
    sq.s0 -= sc.p2
    sq.trigger_s0()
    self.assertEqual(sc.got_s0a, 0)
    self.assertEqual(sc.got_s0b, 0)

  def test_70(self):
  
    class SignalCollector(object):

      got_s0 = False
      got_s1 = None
      got_s2_1 = None
      got_s2_2 = None

      def f_s0(self):
        self.got_s0 = True

      def f_s1(self, iv):
        self.got_s1 = iv

      def f_s2(self, s, obj):
        self.got_s2_1 = s
        self.got_s2_2 = obj

    sc = SignalCollector()
  
    se = pya.SE()

    se.s0 = sc.f_s0
    se.trigger_s0()
    self.assertEqual(sc.got_s0, True)

    sc.got_s1 = None
    se.s1 = sc.f_s1
    se.trigger_s1(17)
    self.assertEqual(sc.got_s1, 17)
    se.trigger_s1(42)
    self.assertEqual(sc.got_s1, 42)

    sc.got_s2_1 = None
    sc.got_s2_2 = None
    se.tag = 999

    se.s2 = sc.f_s2
    se.trigger_s2("foo")
    self.assertEqual(sc.got_s2_1, "foo")
    self.assertEqual(sc.got_s2_2.tag, 999)
    se.tag = 111
    sc.got_s2_2 = None 
    se.trigger_s2("bar")
    self.assertEqual(sc.got_s2_1, "bar")
    self.assertEqual(sc.got_s2_2.tag, 111)

  def test_71(self):
  
    class SignalCollector(object):

      got_s0a = 0
      got_s0b = 0

      def p1(self):
        self.got_s0a += 1

      def p1b(self):
        self.got_s0a += 1

      def p2(self):
        self.got_s0b += 1

    sc = SignalCollector()
  
    se = pya.SE()

    se.s0 = sc.p1
    se.trigger_s0()
    self.assertEqual(sc.got_s0a, 1)
    self.assertEqual(sc.got_s0b, 0)

    sc.got_s0a = 0
    sc.got_s0b = 0
    se.s0 = sc.p2
    se.trigger_s0()
    self.assertEqual(sc.got_s0a, 0)
    self.assertEqual(sc.got_s0b, 1)

    sc.got_s0a = 0
    sc.got_s0b = 0
    se.s0 += sc.p1
    se.trigger_s0()
    self.assertEqual(sc.got_s0a, 1)
    self.assertEqual(sc.got_s0b, 1)

    sc.got_s0a = 0
    sc.got_s0b = 0 
    # same proc is not added again
    se.s0 += sc.p1
    se.trigger_s0()
    self.assertEqual(sc.got_s0a, 1)
    self.assertEqual(sc.got_s0b, 1)

    sc.got_s0a = 0
    sc.got_s0b = 0
    # second proc p1 with same effect
    se.s0 += sc.p1b
    se.trigger_s0()
    self.assertEqual(sc.got_s0a, 2)
    self.assertEqual(sc.got_s0b, 1)

    sc.got_s0a = 0
    sc.got_s0b = 0
    se.s0 -= sc.p1
    se.trigger_s0()
    self.assertEqual(sc.got_s0a, 1)
    self.assertEqual(sc.got_s0b, 1)

    sc.got_s0a = 0
    sc.got_s0b = 0
    se.s0 -= sc.p1b
    se.trigger_s0()
    self.assertEqual(sc.got_s0a, 0)
    self.assertEqual(sc.got_s0b, 1)

    sc.got_s0a = 0
    sc.got_s0b = 0
    se.s0 -= sc.p1
    se.trigger_s0()
    self.assertEqual(sc.got_s0a, 0)
    self.assertEqual(sc.got_s0b, 1)

    sc.got_s0a = 0
    sc.got_s0b = 0
    se.s0 -= sc.p2
    se.trigger_s0()
    self.assertEqual(sc.got_s0a, 0)
    self.assertEqual(sc.got_s0b, 0)

  def test_QByteArray(self):

    # QByteArray

    if "ia_cref_to_qba" in pya.A.__dict__:

      qba = pya.A.ia_cref_to_qba([ 16, 42, 0, 8 ])
      if sys.version_info < (3, 0):
        self.assertEqual(repr(qba), "bytearray(b'\\x10*\\x00\\x08')")
      else:
        self.assertEqual(repr(qba), "b'\\x10*\\x00\\x08'")

      self.assertEqual(pya.A.qba_to_ia(qba), [ 16, 42, 0, 8 ])
      self.assertEqual(pya.A.qba_cref_to_ia(qba), [ 16, 42, 0, 8 ])
      self.assertEqual(pya.A.qba_cptr_to_ia(qba), [ 16, 42, 0, 8 ])
      self.assertEqual(pya.A.qba_ref_to_ia(qba), [ 16, 42, 0, 8 ])
      self.assertEqual(pya.A.qba_ptr_to_ia(qba), [ 16, 42, 0, 8 ])
  
      qba = pya.A.ia_cref_to_qba_cref([ 17, 42, 0, 8 ])
      if sys.version_info < (3, 0):
        self.assertEqual(repr(qba), "bytearray(b'\\x11*\\x00\\x08')")
      else:
        self.assertEqual(repr(qba), "b'\\x11*\\x00\\x08'")

      qba = pya.A.ia_cref_to_qba_ref([ 18, 42, 0, 8 ])
      if sys.version_info < (3, 0):
        self.assertEqual(repr(qba), "bytearray(b'\\x12*\\x00\\x08')")
      else:
        self.assertEqual(repr(qba), "b'\\x12*\\x00\\x08'")

      qba = pya.A.ia_cref_to_qba_cptr([ 19, 42, 0, 8 ])
      if sys.version_info < (3, 0):
        self.assertEqual(repr(qba), "bytearray(b'\\x13*\\x00\\x08')")
      else:
        self.assertEqual(repr(qba), "b'\\x13*\\x00\\x08'")

      qba = pya.A.ia_cref_to_qba_ptr([ 20, 42, 0, 8 ])
      if sys.version_info < (3, 0):
        self.assertEqual(repr(qba), "bytearray(b'\\x14*\\x00\\x08')")
      else:
        self.assertEqual(repr(qba), "b'\\x14*\\x00\\x08'")

      self.assertEqual(pya.A.qba_to_ia(b'\x00\x01\x02'), [ 0, 1, 2 ])

  def test_QByteArrayView(self):

    # QByteArrayView

    if "ia_cref_to_qbav" in pya.A.__dict__:

      qbav = pya.A.ia_cref_to_qbav([ 16, 42, 0, 8 ])
      if sys.version_info < (3, 0):
        self.assertEqual(repr(qbav), "bytearray(b'\\x10*\\x00\\x08')")
      else:
        self.assertEqual(repr(qbav), "b'\\x10*\\x00\\x08'")

      self.assertEqual(pya.A.qbav_to_ia(qbav), [ 16, 42, 0, 8 ])
      self.assertEqual(pya.A.qbav_cref_to_ia(qbav), [ 16, 42, 0, 8 ])
      self.assertEqual(pya.A.qbav_cptr_to_ia(qbav), [ 16, 42, 0, 8 ])
      self.assertEqual(pya.A.qbav_ref_to_ia(qbav), [ 16, 42, 0, 8 ])
      self.assertEqual(pya.A.qbav_ptr_to_ia(qbav), [ 16, 42, 0, 8 ])
  
      qbav = pya.A.ia_cref_to_qbav_cref([ 17, 42, 0, 8 ])
      if sys.version_info < (3, 0):
        self.assertEqual(repr(qbav), "bytearray(b'\\x11*\\x00\\x08')")
      else:
        self.assertEqual(repr(qbav), "b'\\x11*\\x00\\x08'")

      qbav = pya.A.ia_cref_to_qbav_ref([ 18, 42, 0, 8 ])
      if sys.version_info < (3, 0):
        self.assertEqual(repr(qbav), "bytearray(b'\\x12*\\x00\\x08')")
      else:
        self.assertEqual(repr(qbav), "b'\\x12*\\x00\\x08'")

      qbav = pya.A.ia_cref_to_qbav_cptr([ 19, 42, 0, 8 ])
      if sys.version_info < (3, 0):
        self.assertEqual(repr(qbav), "bytearray(b'\\x13*\\x00\\x08')")
      else:
        self.assertEqual(repr(qbav), "b'\\x13*\\x00\\x08'")

      qbav = pya.A.ia_cref_to_qbav_ptr([ 20, 42, 0, 8 ])
      if sys.version_info < (3, 0):
        self.assertEqual(repr(qbav), "bytearray(b'\\x14*\\x00\\x08')")
      else:
        self.assertEqual(repr(qbav), "b'\\x14*\\x00\\x08'")

      self.assertEqual(pya.A.qbav_to_ia(b'\x00\x01\x02'), [ 0, 1, 2 ])

  def test_QString(self):

    # QString

    if "ia_cref_to_qs" in pya.A.__dict__:

      qs = pya.A.ia_cref_to_qs([ 16, 42, 0, 8 ])
      self.assertEqual(repr(qs), "'\\x10*\\x00\\x08'")

      self.assertEqual(pya.A.qs_to_ia(qs), [ 16, 42, 0, 8 ])
      self.assertEqual(pya.A.qs_cref_to_ia(qs), [ 16, 42, 0, 8 ])
      self.assertEqual(pya.A.qs_cptr_to_ia(qs), [ 16, 42, 0, 8 ])
      self.assertEqual(pya.A.qs_ref_to_ia(qs), [ 16, 42, 0, 8 ])
      self.assertEqual(pya.A.qs_ptr_to_ia(qs), [ 16, 42, 0, 8 ])
  
      qs = pya.A.ia_cref_to_qs_cref([ 17, 42, 0, 8 ])
      self.assertEqual(repr(qs), "'\\x11*\\x00\\x08'")

      qs = pya.A.ia_cref_to_qs_ref([ 18, 42, 0, 8 ])
      self.assertEqual(repr(qs), "'\\x12*\\x00\\x08'")

      qs = pya.A.ia_cref_to_qs_cptr([ 19, 42, 0, 8 ])
      self.assertEqual(repr(qs), "'\\x13*\\x00\\x08'")

      qs = pya.A.ia_cref_to_qs_ptr([ 20, 42, 0, 8 ])
      self.assertEqual(repr(qs), "'\\x14*\\x00\\x08'")

      self.assertEqual(pya.A.qs_to_ia('\x00\x01\x02'), [ 0, 1, 2 ])

  def test_QStringView(self):

    # QStringView

    if "ia_cref_to_qsv" in pya.A.__dict__:

      qsv = pya.A.ia_cref_to_qsv([ 16, 42, 0, 8 ])
      self.assertEqual(repr(qsv), "'\\x10*\\x00\\x08'")

      self.assertEqual(pya.A.qsv_to_ia(qsv), [ 16, 42, 0, 8 ])
      self.assertEqual(pya.A.qsv_cref_to_ia(qsv), [ 16, 42, 0, 8 ])
      self.assertEqual(pya.A.qsv_cptr_to_ia(qsv), [ 16, 42, 0, 8 ])
      self.assertEqual(pya.A.qsv_ref_to_ia(qsv), [ 16, 42, 0, 8 ])
      self.assertEqual(pya.A.qsv_ptr_to_ia(qsv), [ 16, 42, 0, 8 ])
  
      qsv = pya.A.ia_cref_to_qsv_cref([ 17, 42, 0, 8 ])
      self.assertEqual(repr(qsv), "'\\x11*\\x00\\x08'")

      qsv = pya.A.ia_cref_to_qsv_ref([ 18, 42, 0, 8 ])
      self.assertEqual(repr(qsv), "'\\x12*\\x00\\x08'")

      qsv = pya.A.ia_cref_to_qsv_cptr([ 19, 42, 0, 8 ])
      self.assertEqual(repr(qsv), "'\\x13*\\x00\\x08'")

      qsv = pya.A.ia_cref_to_qsv_ptr([ 20, 42, 0, 8 ])
      self.assertEqual(repr(qsv), "'\\x14*\\x00\\x08'")

      self.assertEqual(pya.A.qsv_to_ia('\x00\x01\x02'), [ 0, 1, 2 ])

  def test_QLatin1String(self):

    # QLatin1String

    if "ia_cref_to_ql1s" in pya.A.__dict__:

      ql1s = pya.A.ia_cref_to_ql1s([ 16, 42, 0, 8 ])
      self.assertEqual(repr(ql1s), "'\\x10*\\x00\\x08'")

      self.assertEqual(pya.A.ql1s_to_ia(ql1s), [ 16, 42, 0, 8 ])
      self.assertEqual(pya.A.ql1s_cref_to_ia(ql1s), [ 16, 42, 0, 8 ])
      self.assertEqual(pya.A.ql1s_cptr_to_ia(ql1s), [ 16, 42, 0, 8 ])
      self.assertEqual(pya.A.ql1s_ref_to_ia(ql1s), [ 16, 42, 0, 8 ])
      self.assertEqual(pya.A.ql1s_ptr_to_ia(ql1s), [ 16, 42, 0, 8 ])
  
      ql1s = pya.A.ia_cref_to_ql1s_cref([ 17, 42, 0, 8 ])
      self.assertEqual(repr(ql1s), "'\\x11*\\x00\\x08'")

      ql1s = pya.A.ia_cref_to_ql1s_ref([ 18, 42, 0, 8 ])
      self.assertEqual(repr(ql1s), "'\\x12*\\x00\\x08'")

      ql1s = pya.A.ia_cref_to_ql1s_cptr([ 19, 42, 0, 8 ])
      self.assertEqual(repr(ql1s), "'\\x13*\\x00\\x08'")

      ql1s = pya.A.ia_cref_to_ql1s_ptr([ 20, 42, 0, 8 ])
      self.assertEqual(repr(ql1s), "'\\x14*\\x00\\x08'")

      self.assertEqual(pya.A.ql1s_to_ia('\x00\x01\x02'), [ 0, 1, 2 ])

  def test_binaryStrings(self):

    # binary strings (non-Qt)

    ba = pya.A.ia_cref_to_ba([ 17, 42, 0, 8 ])
    if sys.version_info < (3, 0):
      self.assertEqual(repr(ba), "bytearray(b'\\x11*\\x00\\x08')")
    else:
      self.assertEqual(repr(ba), "b'\\x11*\\x00\\x08'")
    self.assertEqual(pya.A.ba_to_ia(ba), [ 17, 42, 0, 8 ])
    self.assertEqual(pya.A.ba_cref_to_ia(ba), [ 17, 42, 0, 8 ])
    self.assertEqual(pya.A.ba_cptr_to_ia(ba), [ 17, 42, 0, 8 ])
    self.assertEqual(pya.A.ba_ref_to_ia(ba), [ 17, 42, 0, 8 ])
    self.assertEqual(pya.A.ba_ptr_to_ia(ba), [ 17, 42, 0, 8 ])

    ba = pya.A.ia_cref_to_ba_cref([ 17, 42, 0, 8 ])
    if sys.version_info < (3, 0):
      self.assertEqual(repr(ba), "bytearray(b'\\x11*\\x00\\x08')")
    else:
      self.assertEqual(repr(ba), "b'\\x11*\\x00\\x08'")

    self.assertEqual(pya.A.ba_to_ia(b'\x00\x01\x02'), [ 0, 1, 2 ])

  # Tests multi-base mixins (only constants and enums available)
  def test_multiBaseMixins(self):
    
    bb = pya.BB()  # base classes B1,B2,B3
    bb.set1(17)                                          # B1
    self.assertEqual(bb.get1(), 17)                      # B1
    bb.set1(21)                                          # B1

    self.assertEqual(bb.get1(), 21)                      # B1
    self.assertEqual(pya.BB.C2, 17)                      # B2
    self.assertEqual(pya.BB.C3, -1)                      # B3
    self.assertEqual(pya.BB.E.E3B.to_i(), 101)           # B3
    self.assertEqual(bb.d3(pya.BB.E.E3C, pya.BB.E.E3A), -2)  # BB with B3 enums
    self.assertEqual(bb.d3(pya.BB.E.E3A, pya.BB.E.E3C), 2)   # BB with B3 enums

  # Custom factory implemented in Python

  def test_80(self):

    gc = pya.GObject.g_inst_count()
    gf = PyGFactory()
    go = pya.GFactory.create_f(gf, 17)
    self.assertEqual(go.g_virtual(), 34)
    self.assertEqual(go.g_org(), 0)
    self.assertEqual(pya.GObject.g_inst_count(), gc + 1)
    go = None
    self.assertEqual(pya.GObject.g_inst_count(), gc)

  # fallback to __rmul__ for not implemented __mul__

  def test_90(self):

    # skip this test for Python 2
    if sys.version_info < (3, 0):
      return

    class RMulObject:
      def __init__(self, factor):
        self.factor = factor
      def __rmul__(self, point):
        return point * self.factor
      def __radd__(self, point):
        return point + pya.Vector(1,1) * self.factor

    p = pya.Point(1, 0)
    fac2 = RMulObject(2)
    self.assertEqual(p * 2, p * fac2)  # p.__mul__(fac2) should return NotImplemented, which will call fac2.__rmul__(p)
    self.assertEqual(pya.Point(3,2), p + fac2)

  # copy and deepcopy

  def test_91(self):

    p = pya.Point(1, 0)
    pc = copy.copy(p)
    pdc = copy.deepcopy(p)

    pdc.x = 4
    pc.x = 3
    p.x = 2
    self.assertEqual(p.x, 2)
    self.assertEqual(pc.x, 3)
    self.assertEqual(pdc.x, 4)

# run unit tests
if __name__ == '__main__':
  suite = unittest.TestSuite()
  # NOTE: Use this instead of loadTestsfromTestCase to select a specific test:
  #   suite.addTest(BasicTest("test_26"))
  suite = unittest.TestLoader().loadTestsFromTestCase(BasicTest)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)

