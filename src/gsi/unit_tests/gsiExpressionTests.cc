
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/


#include "gsiExpression.h"
#include "gsiDecl.h"

#include "tlUnitTest.h"

#include <stdlib.h>
#include <math.h>

// ----------------------------------------------------------------------
//  Tests

// basics
TEST(1) 
{
  tl::Eval e;
  tl::Variant v;

  v = e.parse ("A.instance_count").execute ();
  int base_insts = v.to_int ();
  EXPECT_EQ (base_insts, 0);

  v = e.parse ("A.new(35).to_s").execute ();
  EXPECT_EQ (v.to_string (), std::string ("A: 35"));

  EXPECT_EQ (e.parse ("A.instance_count").execute ().to_int (), 0);

  // mapping of to_string to to_s method
  v = e.parse ("A.new(35)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("A: 35"));

  // static and non-static methods can have the same name:
  v = e.parse ("A.new.aa").execute ();
  EXPECT_EQ (v.to_string (), std::string ("a"));
  v = e.parse ("A.aa").execute ();
  EXPECT_EQ (v.to_string (), std::string ("static_a"));

  v = e.parse ("A.new.get_n").execute ();
  EXPECT_EQ (v.to_string (), std::string ("17"));
  v = e.parse ("var a=A.new").execute ();
  v = e.parse ("a.a5(-5); a.get_n").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-5"));

  //  mapping of property assignment to method
  v = e.parse ("a.n = -177; a.get_n").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-177"));
  bool error = false;
  try {
    v = e.parse ("a.unknown").execute ();
  } catch (...) {
    //  invalid method
    error = true;
  }
  EXPECT_EQ (error, true);

  error = false;
  try {
    v = e.parse ("a.a5").execute ();
    EXPECT_EQ (false, true);
  } catch (...) {
    //  invalid number of arguments
    error = true;
  }
  EXPECT_EQ (error, true);

  v = e.parse ("a.a3('a')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("a.a3('ab')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("a.a3('')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0"));
  v = e.parse ("a.a4([1])").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("a.a4([1, 125e-3])").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0.125"));
  v = e.parse ("a.a4([5, 1, -1.25])").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-1.25"));
  v = e.parse ("A.instance_count").execute ();
  EXPECT_EQ (v.to_int (), base_insts + 1);   //  one instance more
  v = e.parse ("a=1; A.instance_count").execute ();
  EXPECT_EQ (v.to_int (), base_insts);   //  remaining instances 
  v = e.parse ("A.instance_count").execute ();
  EXPECT_EQ (v.to_int (), base_insts);   //  remaining instances 

  v = e.parse ("var a1=A.new; a1.a5(-15); var a2=a1.dup; a2.a5(107); a1.get_n").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-15"));
  v = e.parse ("var a1=A.new; a1.a5(-15); var a2=a1.dup; a2.a5(107); a2.get_n").execute ();
  EXPECT_EQ (v.to_string (), std::string ("107"));

  v = e.parse ("var a=A.new; a.get_e.to_s").execute ();
  EXPECT_EQ (v.to_string (), std::string ("#0"));
  v = e.parse ("var a=A.new; a.set_e(Enum.a); a.get_e.to_s").execute ();
  EXPECT_EQ (v.to_string (), std::string ("a"));
  v = e.parse ("var a=A.new; a.set_e(Enum.b); a.get_e.to_s").execute ();
  EXPECT_EQ (v.to_string (), std::string ("b"));
  v = e.parse ("var a=A.new; a.set_eptr(nil); a.get_e.to_s").execute ();
  EXPECT_EQ (v.to_string (), std::string ("#0"));
  v = e.parse ("var a=A.new; a.set_eptr(Enum.c); a.get_e.to_s").execute ();
  EXPECT_EQ (v.to_string (), std::string ("c"));
  v = e.parse ("var a=A.new; a.set_ecptr(nil); a.get_e.to_s").execute ();
  EXPECT_EQ (v.to_string (), std::string ("#0"));
  v = e.parse ("var a=A.new; a.set_ecptr(Enum.b); a.get_e.to_s").execute ();
  EXPECT_EQ (v.to_string (), std::string ("b"));
  v = e.parse ("var a=A.new; a.set_ecref(Enum.a); a.get_e.to_s").execute ();
  EXPECT_EQ (v.to_string (), std::string ("a"));
  v = e.parse ("var a=A.new; a.set_eref(Enum.c); a.get_e.to_s").execute ();
  EXPECT_EQ (v.to_string (), std::string ("c"));
  v = e.parse ("var a=A.new; a.set_eref(Enum.a); a.get_eptr.to_s").execute ();
  EXPECT_EQ (v.to_string (), std::string ("a"));
  v = e.parse ("var a=A.new; a.set_eref(Enum.c); a.get_eref.to_s").execute ();
  EXPECT_EQ (v.to_string (), std::string ("c"));
  v = e.parse ("var a=A.new; a.set_eref(Enum.a); a.get_ecptr.to_s").execute ();
  EXPECT_EQ (v.to_string (), std::string ("a"));
  v = e.parse ("var a=A.new; a.set_eref(Enum.c); a.get_ecref.to_s").execute ();
  EXPECT_EQ (v.to_string (), std::string ("c"));
  v = e.parse ("var a=A.new; a.set_ecptr(nil); a.get_ecptr").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil"));
  v = e.parse ("var a=A.new; a.set_ecptr(nil); a.get_ecref").execute ();
  EXPECT_EQ (v.to_string (), std::string ("#0"));
  v = e.parse ("var a=A.new; a.set_ecptr(nil); a.get_eptr").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil"));
  v = e.parse ("var a=A.new; a.set_ecptr(nil); a.get_eref").execute ();
  EXPECT_EQ (v.to_string (), std::string ("#0"));
  v = e.parse ("var a=A.new; var ee=Enum.new; ee").execute ();
  EXPECT_EQ (v.to_string (), std::string ("#0"));
#if 0
  // No "out" parameters currently:
  v = e.parse ("var a=A.new; var ee=Enum.new; a.mod_eref(ee, Enum.a); ee").execute ();
  EXPECT_EQ (v.to_string (), std::string ("a"));
  v = e.parse ("var a=A.new; var ee=Enum.new; a.mod_eptr(ee, Enum.a); ee").execute ();
  EXPECT_EQ (v.to_string (), std::string ("a"));
#endif
  v = e.parse ("var a=A.new; a.ev").execute ();
  EXPECT_EQ (v.to_string (), std::string (""));
  v = e.parse ("var a=A.new; a.push_ev(Enum.a); a.push_ev(Enum.new); a.push_ev(Enum.b); a.ev").execute ();
  EXPECT_EQ (v.to_string (), std::string ("a,#0,b"));

#if defined(HAVE_QT)
  v = e.parse ("var a=A.new; a.get_ef").execute ();
  EXPECT_EQ (v.to_string (), std::string (""));
  v = e.parse ("var a=A.new; a.set_ef(Enum.a); a.get_ef").execute ();
  EXPECT_EQ (v.to_string (), std::string ("a"));
  v = e.parse ("var a=A.new; a.set_ef(Enums.new(Enum.b)); a.get_ef").execute ();
  EXPECT_EQ (v.to_string (), std::string ("b"));
  v = e.parse ("var a=A.new; a.set_efptr(nil); a.get_ef").execute ();
  EXPECT_EQ (v.to_string (), std::string (""));
  v = e.parse ("var a=A.new; a.set_efptr(Enums.new(Enum.c)); a.get_ef").execute ();
  EXPECT_EQ (v.to_string (), std::string ("a|b|c"));
  v = e.parse ("var a=A.new; a.set_efcptr(nil); a.get_ef").execute ();
  EXPECT_EQ (v.to_string (), std::string (""));
  v = e.parse ("var a=A.new; a.set_efcptr(Enums.new(Enum.b)); a.get_ef").execute ();
  EXPECT_EQ (v.to_string (), std::string ("b"));
  v = e.parse ("var a=A.new; a.set_efcptr(Enum.c); a.get_ef").execute ();
  EXPECT_EQ (v.to_string (), std::string ("a|b|c"));
  v = e.parse ("var a=A.new; a.set_efcref(Enum.b); a.get_ef").execute ();
  EXPECT_EQ (v.to_string (), std::string ("b"));
  v = e.parse ("var a=A.new; a.set_efcref(Enums.new(Enum.a)); a.get_ef").execute ();
  EXPECT_EQ (v.to_string (), std::string ("a"));
  v = e.parse ("var a=A.new; a.set_efref(Enums.new(Enum.c)); a.get_ef").execute ();
  EXPECT_EQ (v.to_string (), std::string ("a|b|c"));
  v = e.parse ("var a=A.new; a.set_efref(Enums.new(Enum.c)); a.get_efptr").execute ();
  EXPECT_EQ (v.to_string (), std::string ("a|b|c"));
  v = e.parse ("var a=A.new; a.set_efref(Enums.new(Enum.c)); a.get_efref").execute ();
  EXPECT_EQ (v.to_string (), std::string ("a|b|c"));
  v = e.parse ("var a=A.new; a.set_efref(Enums.new(Enum.c)); a.get_efcptr").execute ();
  EXPECT_EQ (v.to_string (), std::string ("a|b|c"));
  v = e.parse ("var a=A.new; a.set_efref(Enums.new(Enum.c)); a.get_efcref").execute ();
  EXPECT_EQ (v.to_string (), std::string ("a|b|c"));
  v = e.parse ("var a=A.new; a.set_efcptr(nil); a.get_efcptr").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil"));
  v = e.parse ("var a=A.new; a.set_efcptr(nil); a.get_efcref").execute ();
  EXPECT_EQ (v.to_string (), std::string (""));
  v = e.parse ("var a=A.new; a.set_efcptr(nil); a.get_efptr").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil"));
  v = e.parse ("var a=A.new; a.set_efcptr(nil); a.get_efref").execute ();
  EXPECT_EQ (v.to_string (), std::string (""));
#if 0
  //  No "out" parameters currently
  v = e.parse ("var a=A.new; var ef=Enums.new(); ef").execute ();
  EXPECT_EQ (v.to_string (), std::string (""));
  v = e.parse ("var a=A.new; var ef=Enums.new(); a.mod_efref(ef, Enum.b); ef").execute ();
  EXPECT_EQ (v.to_string (), std::string ("b"));
  v = e.parse ("var a=A.new; var ef=Enums.new(); a.mod_efref(ef, Enum.b); a.mod_efptr(ef, Enum.a); ef").execute ();
  EXPECT_EQ (v.to_string (), std::string ("a|b"));
#endif
#endif

}

TEST(2) 
{
  tl::Eval e;
  tl::Variant v;

  v = e.parse ("var b=B.new; b.always_5").execute ();
  EXPECT_EQ (v.to_string (), std::string ("5"));
  v = e.parse ("var b=B.new; var a1=A.new(-17); var a2=A.new(42); b.av = [ a1, a2 ]; to_s(b.av)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("A: -17,A: 42"));
  v = e.parse ("var b=B.new; var a1=A.new(-17); var a2=A.new(42); b.av = []; b.push_a(a1); b.push_a(a2); to_s(b.av)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("A: -17,A: 42"));
  v = e.parse ("var b=B.new; var a1=A.new(-17); var a2=A.new(42); b.av = []; b.push_a_cref(a1); b.push_a_cptr(a2); to_s(b.av)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("A: -17,A: 42"));
  v = e.parse ("var b=B.new; var a1=A.new(-17); var a2=A.new(42); b.av = []; b.push_a_ref(a1); b.push_a_ptr(a2); to_s(b.av)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("A: -17,A: 42"));
  v = e.parse ("var b=B.new; var a1=A.new(-17); var a2=A.new(1); b.av_cref = [ a1, a2 ]; to_s(b.av_cref)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("A: -17,A: 1"));
  v = e.parse ("var b=B.new; b.av_cptr = [ A.new(-13) ]; to_s(b.av)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("A: -13"));
  v = e.parse ("var b=B.new; b.av_ptr = [ A.new(13) ]; to_s(b.av)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("A: 13"));
  v = e.parse ("var b=B.new; b.av = [ A.new(-13) ]; b.av_cptr = nil; to_s(b.av)").execute ();
  EXPECT_EQ (v.to_string (), std::string (""));
  v = e.parse ("var b=B.new; b.av = [ A.new(13) ]; b.av_ptr = nil; to_s(b.av)").execute ();
  EXPECT_EQ (v.to_string (), std::string (""));
  v = e.parse ("var b=B.new; var a1=A.new(17); b.av_ref = [ a1 ]; to_s(b.av_ref)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("A: 17"));
  v = e.parse ("var b=B.new; b.arg_is_not_nil(nil)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("false"));
  v = e.parse ("var b=B.new; b.arg_is_not_nil(A.new)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("true"));
  v = e.parse ("var b=B.new; b.bx").execute ();
  EXPECT_EQ (v.to_string (), std::string ("17"));
  v = e.parse ("var b=B.new; b.bx(-1)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("xz"));

  //  List to constructor call
  v = e.parse ("var b=B.new; b.av = [ [5, 6], [4, 6, 0.5], [42] ]; to_s(b.av)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("A: 11,A: 5,A: 42"));
  v = e.parse ("var b=B.new; b.av = []; b.push_a([ 1, 2 ]); b.push_a([ 17 ]); to_s(b.av)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("A: 3,A: 17"));
  v = e.parse ("var b=B.new; b.av = []; b.push_a([ 1, 2 ]); b.push_a_cref([ 17 ]); to_s(b.av)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("A: 3,A: 17"));
  v = e.parse ("var b=B.new; b.av = []; b.push_a([ 1, 2 ]); b.push_a_cptr([ 17 ]); to_s(b.av)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("A: 3,A: 17"));
  v = e.parse ("var b=B.new; b.av = []; b.push_a([ 1, 2 ]); b.push_a_ref([ 17 ]); to_s(b.av)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("A: 3,A: 17"));
  v = e.parse ("var b=B.new; b.av = []; b.push_a([ 1, 2 ]); b.push_a_ptr([ 17 ]); to_s(b.av)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("A: 3,A: 17"));

  /*
  TODO: No detailed type analysis for ambiguity resolution so far:
  v = e.parse ("var b=B.new; b.bx('hello', 1)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("20.5"));
  */
  v = e.parse ("var b=B.new; var a=A.new; b.bx(a)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("aref"));
  v = e.parse ("var b=B.new; b.var_is_nil(1)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("false"));
  v = e.parse ("var b=B.new; b.var_is_nil(nil)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("true"));
  v = e.parse ("var b=B.new; b.set_vars([])").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0"));
  v = e.parse ("var b=B.new; b.set_vars([]); b.vars").execute ();
  EXPECT_EQ (v.to_string (), std::string (""));
  v = e.parse ("var b=B.new; b.set_vars([true, 'hello']); b.vars").execute ();
  EXPECT_EQ (v.to_string (), std::string ("true,hello"));
  v = e.parse ("var b=B.new; b.set_vars([1, 'hello']); b.vars_ref").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1,hello"));
  v = e.parse ("var b=B.new; b.set_vars([17, 1]); b.vars_cref").execute ();
  EXPECT_EQ (v.to_string (), std::string ("17,1"));
  v = e.parse ("var b=B.new; b.set_vars([nil,nil]); b.vars_cptr").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil,nil"));
  v = e.parse ("var b=B.new; b.set_vars([1,2,3]); b.vars_cptr_null").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil"));
  v = e.parse ("var b=B.new; b.set_vars([27, 1]); b.vars_ref").execute ();
  EXPECT_EQ (v.to_string (), std::string ("27,1"));
  v = e.parse ("var b=B.new; b.set_vars([1.5]); b.vars_ptr").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1.5"));
  v = e.parse ("var b=B.new; b.set_vars([-1.5]); b.vars_ptr_null").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil"));
  v = e.parse ("var b=B.new; b.set_vars([nil])").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("var b=B.new; b.set_vars([17, 21])").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("var b=B.new; b.set_vars([17, 21]); b.var").execute ();
  EXPECT_EQ (v.to_string (), std::string ("21"));
  v = e.parse ("var b=B.new; b.set_vars([-2]); b.var_cref").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-2"));
  v = e.parse ("var b=B.new; b.set_vars([17, 22]); b.var_cptr").execute ();
  EXPECT_EQ (v.to_string (), std::string ("22"));
  v = e.parse ("var b=B.new; b.set_vars([]); b.var_cptr").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil"));
  v = e.parse ("var b=B.new; b.set_vars(['hello']); b.var_ref").execute ();
  EXPECT_EQ (v.to_string (), std::string ("hello"));
  v = e.parse ("var b=B.new; b.set_vars([27]); b.var_ptr").execute ();
  EXPECT_EQ (v.to_string (), std::string ("27"));
  v = e.parse ("var b=B.new; b.set_vars([]); b.var_ptr").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil"));
  v = e.parse ("var a=A.new; a.a5(22); var b=B.new; b.aptr_to_n(a)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("22"));
  v = e.parse ("var a=A.new; a.a5(22); var b=B.new; b.aref_to_s(a)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("b4_result: 22"));
  v = e.parse ("var a=A.new; a.a5(22); var b=B.new; a.a5(-6); b.aptr_to_n(a)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-6"));
  v = e.parse ("var a=A.new; a.a5(22); var b=B.new; a.a5(-6); b.aref_to_s(a)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("b4_result: -6"));
  v = e.parse ("var b=B.new; b.aref_to_s(A.new)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("b4_result: 17"));

  v = e.parse ("b.amember_ref.a5(177)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil"));

  v = e.parse ("b.amember_or_nil(true)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("A: 177"));

  bool error = false;
  try {
    tl::Expression ex;
    e.parse(ex, "b.amember_cref.a5(177)");
    v = ex.execute ();
  } catch (...) {
    //  can't call non-const method on const ref
    error = true;
  }
  EXPECT_EQ (error, true);

  //  references
  v = e.parse ("b.amember_or_nil(true)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("A: 177"));
  v = e.parse ("b.amember_or_nil(false)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil"));
  v = e.parse ("b.amember_ptr.a5(177); b.amember_ref.get_n").execute ();
  EXPECT_EQ (v.to_string (), std::string ("177"));
  v = e.parse ("b.amember_ref.get_n_const").execute ();
  EXPECT_EQ (v.to_string (), std::string ("177"));
  v = e.parse ("b.amember_cref.get_n_const").execute ();
  EXPECT_EQ (v.to_string (), std::string ("177"));
  error = false;
  try {
    v = e.parse ("b.amember_cref.get_n").execute ();
  } catch (...) {
    //  can't call non-const method on const ref
    error = true;
  }
  EXPECT_EQ (error, true);

  //  references: storage in variables
  v = e.parse ("var aref = b.amember_ptr").execute ();
  v = e.parse ("aref.n = 178").execute ();
  v = e.parse ("aref.get_n").execute ();
  EXPECT_EQ (v.to_string (), std::string ("178"));
  v = e.parse ("aref.get_n == 178").execute ();
  EXPECT_EQ (v.to_string (), std::string ("true"));
  v = e.parse ("b.amember_ref.get_n").execute ();
  EXPECT_EQ (v.to_string (), std::string ("178"));

  //  references: storage in variables
  v = e.parse ("var aref = b.amember_cptr").execute ();
  v = e.parse ("aref.get_n_const").execute ();
  EXPECT_EQ (v.to_string (), std::string ("178"));
  error = false;
  try {
    v = e.parse ("aref.n = 179").execute ();
  } catch (...) {
    //  can't call non-const method on const ref
    error = true;
  }
  EXPECT_EQ (error, true);
}

TEST(3) 
{
  tl::Eval e;
  tl::Variant v;

  v = e.parse ("var c=C.new; c.g('hallo')").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1977"));
  //  Currently there is no way to override virtual methods in expressions so we can't check much else.
}

TEST(4) 
{
  tl::Eval e;
  tl::Variant v;
  bool error;

  v = e.parse ("var g=G.new; g.iv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("0"));
  v = e.parse ("var g=G.new; g.set_iva(2); g.iv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
  v = e.parse ("var g=G.new; g.set_ivb(3); g.iv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3"));
  v = e.parse ("var g=G.new; g.set_ivb; g.iv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("var g=G.new; g.set_sv1a('hallo'); g.sv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("hallo"));
  error = false;
  try {
    v = e.parse ("var g=G.new; g.set_sv1a; g.sv").execute ();
  } catch (...) {
    error = true;
  }
  EXPECT_EQ (error, true);
  v = e.parse ("var g=G.new; g.set_sv1b('world'); g.sv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("world"));
  v = e.parse ("var g=G.new; g.set_sv1b; g.sv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("value"));
  v = e.parse ("var g=G.new; g.set_sv2a('hallo'); g.sv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("hallo"));
  error = false;
  try {
    v = e.parse ("var g=G.new; g.set_sv2a; g.sv").execute ();
  } catch (...) {
    error = true;
  }
  EXPECT_EQ (error, true);
  v = e.parse ("var g=G.new; g.set_sv2b('world'); g.sv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("world"));
  v = e.parse ("var g=G.new; g.set_sv2b; g.sv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("value"));
  v = e.parse ("var g=G.new; g.set_vva(17, 'c'); g.iv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("17"));
  v = e.parse ("var g=G.new; g.set_vva(17, 'c'); g.sv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("c"));
  v = e.parse ("var g=G.new; g.set_vvb(11); g.iv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("11"));
  v = e.parse ("var g=G.new; g.set_vvb(11); g.sv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("value"));
  v = e.parse ("var g=G.new; g.set_vvb(11, 'nix'); g.iv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("11"));
  v = e.parse ("var g=G.new; g.set_vvb(11, 'nix'); g.sv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nix"));
  v = e.parse ("var g=G.new; g.set_vvc(11); g.iv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("11"));
  v = e.parse ("var g=G.new; g.set_vvc; g.iv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("var g=G.new; g.set_vvc(17, 'nix'); g.iv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("17"));
  v = e.parse ("var g=G.new; g.set_vvc(11); g.sv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("value"));
  v = e.parse ("var g=G.new; g.set_vvc; g.sv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("value"));
  v = e.parse ("var g=G.new; g.set_vvc(17, 'nix'); g.sv").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nix"));
}

TEST(5) 
{
  tl::Eval e;
  tl::Variant v;

  //  derived classes
  v = e.parse ("var o=X.new; o.x1").execute ();
  EXPECT_EQ (v.to_string (), std::string ("17"));
  v = e.parse ("var o=X.new; o.x2").execute ();
  EXPECT_EQ (v.to_string (), std::string ("42"));
  v = e.parse ("var o=Y.new; o.x1").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1"));
  v = e.parse ("var o=Y.new; o.x2").execute ();
  EXPECT_EQ (v.to_string (), std::string ("42"));
  v = e.parse ("var o=Y.new; o.y1").execute ();
  EXPECT_EQ (v.to_string (), std::string ("117"));
}

TEST(6) 
{
  tl::Eval e;
  tl::Variant v;

  v = e.parse ("var o=Box.new(1, 2, 11, 12); var p=Box.new(5, 6, 15, 16); (o*p).to_s").execute ();
  EXPECT_EQ (v.to_string (), std::string ("(6,8;26,28)"));
  v = e.parse ("var o=Box.new(1, 2, 11, 12); var p=Box.new(5, 6, 15, 16); (o&p).to_s").execute ();
  EXPECT_EQ (v.to_string (), std::string ("(5,6;11,12)"));
  v = e.parse ("var o=Box.new(1, 2, 11, 12); var p=Box.new(5, 6, 15, 16); (o+p).to_s").execute ();
  EXPECT_EQ (v.to_string (), std::string ("(1,2;15,16)"));
  v = e.parse ("var o=Box.new(1, 2, 11, 12); var p=Box.new(5, 6, 15, 16); o<p").execute ();
  EXPECT_EQ (v.to_string (), std::string ("true"));
  v = e.parse ("var o=Box.new(1, 2, 11, 12); var p=Box.new(5, 6, 15, 16); p<o").execute ();
  EXPECT_EQ (v.to_string (), std::string ("false"));
  v = e.parse ("var o=Box.new(1, 2, 11, 12); var p=Box.new(5, 6, 15, 16); o<o").execute ();
  EXPECT_EQ (v.to_string (), std::string ("false"));
  v = e.parse ("var o=Box.new(1, 2, 11, 12); var p=Box.new(5, 6, 15, 16); p==o").execute ();
  EXPECT_EQ (v.to_string (), std::string ("false"));
  v = e.parse ("var o=Box.new(1, 2, 11, 12); var p=Box.new(5, 6, 15, 16); o==o").execute ();
  EXPECT_EQ (v.to_string (), std::string ("true"));
  v = e.parse ("var o=Box.new(1, 2, 11, 12); var p=Box.new(5, 6, 15, 16); p!=o").execute ();
  EXPECT_EQ (v.to_string (), std::string ("true"));
  v = e.parse ("var o=Box.new(1, 2, 11, 12); var p=Box.new(5, 6, 15, 16); o!=o").execute ();
  EXPECT_EQ (v.to_string (), std::string ("false"));
}

TEST(7) 
{
  tl::Eval e;
  tl::Variant v;

  v = e.parse ("var o=B.new(); to_s(o.map1_cptr_null)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil"));
  v = e.parse ("var o=B.new(); to_s(o.map1_ptr_null)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil"));
  v = e.parse ("var o=B.new(); o.insert_map1(1, 'hello'); to_s(o.map1)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("1=>hello"));
  v = e.parse ("var o=B.new(); o.insert_map1(2, 'hello'); to_s(o.map1_cref)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2=>hello"));
  v = e.parse ("var o=B.new(); o.insert_map1(3, 'hello'); to_s(o.map1_cptr)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3=>hello"));
  v = e.parse ("var o=B.new(); o.insert_map1(2, 'hello'); to_s(o.map1_ref)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2=>hello"));
  v = e.parse ("var o=B.new(); o.insert_map1(3, 'hello'); to_s(o.map1_ptr)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("3=>hello"));
  v = e.parse ("var o=B.new(); o.map1 = { 42 => 1, -17 => true }; to_s(o.map1)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-17=>true,42=>1"));
  v = e.parse ("var o=B.new(); o.set_map1({ 42 => 1, -17 => true }); to_s(o.map1)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-17=>true,42=>1"));
  v = e.parse ("var o=B.new(); o.set_map1_cref({ 42 => 1, -17 => true }); to_s(o.map1)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-17=>true,42=>1"));
  v = e.parse ("var o=B.new(); o.set_map1_cptr({ 42 => 1, -17 => true }); to_s(o.map1)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-17=>true,42=>1"));
  v = e.parse ("var o=B.new(); o.set_map1_cptr(nil); to_s(o.map1)").execute ();
  EXPECT_EQ (v.to_string (), std::string (""));
  v = e.parse ("var o=B.new(); o.set_map1_ref({ 42 => 1, -17 => true }); to_s(o.map1)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-17=>true,42=>1"));
  v = e.parse ("var o=B.new(); o.set_map1_ptr({ 42 => 1, -17 => true }); to_s(o.map1)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-17=>true,42=>1"));
  v = e.parse ("var o=B.new(); o.set_map1_ptr(nil); to_s(o.map1)").execute ();
  EXPECT_EQ (v.to_string (), std::string (""));
  v = e.parse ("var o=B.new(); o.map2 = { 'xy' => 1, -17 => true }; to_s(o.map2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-17=>true,xy=>1"));
  v = e.parse ("var o=B.new(); to_s(o.map2_null)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("nil"));
}

TEST(8)
{
  tl::Eval e;
  tl::Variant v;

  v = e.parse ("var l = Layout.new(); l.create_cell('TOP'); l.top_cell.name").execute ();
  EXPECT_EQ (v.to_string (), std::string ("TOP"));
  v = e.parse ("var l = Layout.new(); l.create_cell('TOP'); l.top_cell.name = 'X'; l.top_cell.name").execute ();
  EXPECT_EQ (v.to_string (), std::string ("X"));
  v = e.parse ("var l = Layout.new(); l.create_cell('TOP'); var c = l.top_cell; c.name = 'X'; l.top_cell.name").execute ();
  EXPECT_EQ (v.to_string (), std::string ("X"));
  v = e.parse ("var l = Layout.new(); l.create_cell('TOP'); var c = l.top_cell; l._destroy; c._destroyed").execute ();
  EXPECT_EQ (v.to_string (), std::string ("true"));
}

namespace
{

class CollectFunction
  : public tl::EvalFunction
{
public:
  virtual void execute (const tl::ExpressionParserContext & /*context*/, tl::Variant &out, const std::vector<tl::Variant> &args, const std::map<std::string, tl::Variant> * /*kwargs*/) const
  {
    out = tl::Variant ();
    if (args.size () > 0) {
      values.push_back (args.front ().to_double ());
    }
  }

  mutable std::vector<double> values;
};

}

TEST(9)
{
  tl::Eval e;
  CollectFunction *collect_func = new CollectFunction ();
  e.define_function ("put", collect_func);

  tl::Variant v;
  v = e.parse ("var x=Region.new(Box.new(0,0,100,100)); put(x.area); x=x.sized(10); put(x.area); x=x.sized(10); put(x.area);").execute ();
  EXPECT_EQ (collect_func->values.size (), size_t (3));
  EXPECT_EQ (collect_func->values[0], 10000);
  EXPECT_EQ (collect_func->values[1], 14400);
  EXPECT_EQ (collect_func->values[2], 19600);
}

TEST(10)
{
  tl::Eval e;
  tl::Variant v;

  v = e.parse ("var b3 = B3.new(); b3.E.E3B").execute ();
  EXPECT_EQ (v.to_string (), std::string ("E3B"));
  v = e.parse ("B3.E.E3B").execute ();
  EXPECT_EQ (v.to_string (), std::string ("E3B"));
  v = e.parse ("var bb = BB.new(); bb.C1").execute ();
  EXPECT_EQ (v.to_string (), std::string ("42"));
  v = e.parse ("var bb = BB.new(); bb.C2").execute ();
  EXPECT_EQ (v.to_string (), std::string ("17"));
  v = e.parse ("var bb = BB.new(); bb.C3").execute ();
  EXPECT_EQ (v.to_string (), std::string ("-1"));
  v = e.parse ("var bb = BB.new(); bb.E.E3A").execute ();
  EXPECT_EQ (v.to_string (), std::string ("E3A"));
  v = e.parse ("BB.E.E3C").execute ();
  EXPECT_EQ (v.to_string (), std::string ("E3C"));
  v = e.parse ("var bb = BB.new(); bb.d3(BB.E.E3A, BB.E.E3C)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("2"));
}

TEST(11)
{
  tl::Eval e;
  tl::Variant v;

  //  mapping of *! to *:
  v = e.parse ("var b = Trans.new(1)*Trans.new(Vector.new(10, 20))").execute ();
  EXPECT_EQ (v.to_string (), std::string ("r90 -20,10"));
}

TEST(12)
{
  //  Keyword arguments are best tested on transformations, here CplxTrans

  tl::Eval e;
  tl::Variant v;

  v = e.parse ("var t = CplxTrans.new()").execute ();
  EXPECT_EQ (v.to_string (), std::string ("r0 *1 0,0"));
  v = e.parse ("var t = CplxTrans.new(1.5)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("r0 *1.5 0,0"));
  v = e.parse ("var t = CplxTrans.new(1, 2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("r0 *1 1,2"));
  v = e.parse ("var t = CplxTrans.new(1, y=2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("r0 *1 1,2"));
  v = e.parse ("var t = CplxTrans.new(x=1, y=2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("r0 *1 1,2"));
  v = e.parse ("var t = CplxTrans.new(u=DVector.new(1, 2))").execute ();
  EXPECT_EQ (v.to_string (), std::string ("r0 *1 1,2"));
  v = e.parse ("var t = CplxTrans.new(DVector.new(1, 2))").execute ();
  EXPECT_EQ (v.to_string (), std::string ("r0 *1 1,2"));
  v = e.parse ("var t = CplxTrans.new(u=Vector.new(1, 2))").execute ();
  EXPECT_EQ (v.to_string (), std::string ("r0 *1 1,2"));
  v = e.parse ("var t = CplxTrans.new(u=[1, 2])").execute ();
  EXPECT_EQ (v.to_string (), std::string ("r0 *1 1,2"));
  v = e.parse ("var t = CplxTrans.new(mag=1.5)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("r0 *1.5 0,0"));
  v = e.parse ("var t = CplxTrans.new(1.5, 45, true, 1, 2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m22.5 *1.5 1,2"));
  v = e.parse ("var t = CplxTrans.new(1.5, 45, true, DVector.new(1, 2))").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m22.5 *1.5 1,2"));
  v = e.parse ("var t = CplxTrans.new(1.5, x=1, y=2, mirrx=true, rot=45)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m22.5 *1.5 1,2"));
  v = e.parse ("var t = CplxTrans.new(CplxTrans.M0)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 *1 0,0"));
  v = e.parse ("var t = CplxTrans.new(CplxTrans.M0, u=DVector.new(1, 2))").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 *1 1,2"));
  v = e.parse ("var t = CplxTrans.new(CplxTrans.M0, mag=1.5, u=DVector.new(1, 2))").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 *1.5 1,2"));
  v = e.parse ("var t = CplxTrans.new(CplxTrans.M0, 1.5, DVector.new(1, 2))").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 *1.5 1,2"));
  v = e.parse ("var t = CplxTrans.new(CplxTrans.M0, mag=1.5, x=1, y=2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 *1.5 1,2"));
  v = e.parse ("var t = CplxTrans.new(CplxTrans.M0, 1.5, 1, 2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 *1.5 1,2"));
  v = e.parse ("var t = CplxTrans.new(VCplxTrans.M0)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 *1 0,0"));
  v = e.parse ("var t = CplxTrans.new(ICplxTrans.M0)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 *1 0,0"));
  v = e.parse ("var t = CplxTrans.new(DCplxTrans.M0)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 *1 0,0"));
  v = e.parse ("var t = CplxTrans.new(Trans.M0)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 *1 0,0"));
  v = e.parse ("var t = CplxTrans.new(Trans.M0, 1.5)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 *1.5 0,0"));
  v = e.parse ("var t = CplxTrans.new(Trans.M0, mag=1.5)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 *1.5 0,0"));
  v = e.parse ("var t = CplxTrans.new(t = Trans.M0, mag=1.5)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 *1.5 0,0"));
  v = e.parse ("var t = CplxTrans.new(); t.disp=[1,2]; t").execute ();
  EXPECT_EQ (v.to_string (), std::string ("r0 *1 1,2"));
  v = e.parse ("var t = ICplxTrans.new(15, 25); t.to_s(dbu=0.01)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("r0 *1 0.15000,0.25000"));
}

TEST(13)
{
  //  Keyword arguments are best tested on transformations, here Trans

  tl::Eval e;
  tl::Variant v;

  v = e.parse ("var t = Trans.new(Trans.M0, 1, 2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 1,2"));
  v = e.parse ("var t = Trans.new(Trans.M0, x = 1, y = 2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 1,2"));
  v = e.parse ("var t = Trans.new(Trans.M0, Vector.new(1, 2))").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 1,2"));
  v = e.parse ("var t = Trans.new(Trans.M0, u=Vector.new(1, 2))").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 1,2"));
  v = e.parse ("var t = Trans.new(rot=3, mirrx=true)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m135 0,0"));
  v = e.parse ("var t = Trans.new(rot=3, mirrx=true, x=1, y=2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m135 1,2"));
  v = e.parse ("var t = Trans.new(3, true, 1, 2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m135 1,2"));
  v = e.parse ("var t = Trans.new(3, true, Vector.new(1, 2))").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m135 1,2"));
  v = e.parse ("var t = Trans.new(rot=3, mirrx=true, u=Vector.new(1, 2))").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m135 1,2"));
  v = e.parse ("var t = Trans.new()").execute ();
  EXPECT_EQ (v.to_string (), std::string ("r0 0,0"));
  v = e.parse ("var t = Trans.new(DTrans.M0)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 0,0"));
  v = e.parse ("var t = Trans.new(DTrans.M0, 1, 2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 1,2"));
  v = e.parse ("var t = Trans.new(DTrans.M0, x=1, y=2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 1,2"));
  v = e.parse ("var t = Trans.new(c = DTrans.M0, x=1, y=2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("m0 1,2"));
  v = e.parse ("var t = Trans.new(Vector.new(1, 2))").execute ();
  EXPECT_EQ (v.to_string (), std::string ("r0 1,2"));
  v = e.parse ("var t = Trans.new(1, 2)").execute ();
  EXPECT_EQ (v.to_string (), std::string ("r0 1,2"));
}

TEST(14)
{
  //  Keyword arguments and errors

  tl::Eval e;
  tl::Variant v;

  try {
    v = e.parse("var t = CplxTrans.new(1.5, 2.5); t.to_s(dbu='abc')").execute();
    EXPECT_EQ (true, false);
  } catch (tl::Exception &ex) {
    EXPECT_EQ (ex.msg (), "Unexpected text after numeric value: '...abc' (argument 'dbu') at position 34 (...to_s(dbu='abc'))");
  }

  try {
    v = e.parse("var t = CplxTrans.new(1.5, 2.5); var tt = CplxTrans.new(); t.assign(other=t)").execute();
    EXPECT_EQ (true, false);
  } catch (tl::Exception &ex) {
    EXPECT_EQ (ex.msg ().find ("Keyword arguments not permitted at position 60 (...assign(other=t))"), size_t (0));
  }

  try {
    v = e.parse("var t = CplxTrans.new('abc');").execute();
    EXPECT_EQ (true, false);
  } catch (tl::Exception &ex) {
    EXPECT_EQ (ex.msg ().find ("No overload with matching arguments. Variants are:"), size_t (0));
  }

  try {
    v = e.parse("var t = CplxTrans.new(uu=17);").execute();
    EXPECT_EQ (true, false);
  } catch (tl::Exception &ex) {
    EXPECT_EQ (ex.msg ().find ("Can't match arguments. Variants are:"), size_t (0));
  }

  try {
    v = e.parse("var t = CplxTrans.new(u='17');").execute();
    EXPECT_EQ (true, false);
  } catch (tl::Exception &ex) {
    EXPECT_EQ (ex.msg ().find ("No overload with matching arguments. Variants are:"), size_t (0));
  }
}

TEST(15)
{
  //  Keyword arguments, enums and errors

  tl::Eval e;
  tl::Variant v;

  try {
    v = e.parse("var bb = BB.new; bb.d4()").execute();
    EXPECT_EQ (true, false);
  } catch (tl::Exception &ex) {
    EXPECT_EQ (ex.msg (), "Can't match arguments. Variants are:\n  string d4(int a, string b, double c, B3::E d = E3A, variant e = nil) [no value given for argument #1 and following]\n at position 19 (...d4())");
  }

  try {
    v = e.parse("var bb = BB.new; bb.d4(1, 'a')").execute();
    EXPECT_EQ (true, false);
  } catch (tl::Exception &ex) {
    EXPECT_EQ (ex.msg (), "Can't match arguments. Variants are:\n  string d4(int a, string b, double c, B3::E d = E3A, variant e = nil) [no value given for argument #3]\n at position 19 (...d4(1, 'a'))");
  }

  try {
    v = e.parse("var bb = BB.new; bb.d4(1, 'a', 2.0, xxx=17)").execute();
    EXPECT_EQ (true, false);
  } catch (tl::Exception &ex) {
    EXPECT_EQ (ex.msg (), "Can't match arguments. Variants are:\n  string d4(int a, string b, double c, B3::E d = E3A, variant e = nil) [unknown keyword parameter: xxx]\n at position 19 (...d4(1, 'a', 2.0, xxx..)");
  }

  try {
    v = e.parse("var bb = BB.new; bb.d4(a=1, b='a', c=2.0, xxx=17)").execute();
    EXPECT_EQ (true, false);
  } catch (tl::Exception &ex) {
    EXPECT_EQ (ex.msg (), "Can't match arguments. Variants are:\n  string d4(int a, string b, double c, B3::E d = E3A, variant e = nil) [unknown keyword parameter: xxx]\n at position 19 (...d4(a=1, b='a', c=2...)");
  }

  v = e.parse("var bb = BB.new; bb.d4(1, 'a', 2.0)").execute();
  EXPECT_EQ (v.to_string (), "1,a,2,100,nil");

  v = e.parse("var bb = BB.new; bb.d4(1, 'a', 2.0, e=42)").execute();
  EXPECT_EQ (v.to_string (), "1,a,2,100,42");

  v = e.parse("var bb = BB.new; bb.d4(1, 'a', c=2.0, e=42)").execute();
  EXPECT_EQ (v.to_string (), "1,a,2,100,42");

  v = e.parse("var bb = BB.new; bb.d4(c=2.0, a=1, b='a', e=42)").execute();
  EXPECT_EQ (v.to_string (), "1,a,2,100,42");

  v = e.parse("var bb = BB.new; bb.d4(1, 'a', 2.0, d=BB.E.E3B)").execute();
  EXPECT_EQ (v.to_string (), "1,a,2,101,nil");

  v = e.parse("var bb = BB.new; bb.d4(1, 'a', d=BB.E.E3B, c=2.0)").execute();
  EXPECT_EQ (v.to_string (), "1,a,2,101,nil");

  v = e.parse("var bb = BB.new; bb.d4(1, 'a', 2.0, BB.E.E3B, 42)").execute();
  EXPECT_EQ (v.to_string (), "1,a,2,101,42");
}

//  constness
TEST(16)
{
  tl::Eval e;
  tl::Variant v;
  v = e.parse ("var b=B.new(); b._is_const_object").execute ();
  EXPECT_EQ (v.to_string (), std::string ("false"));
  try {
    v = e.parse ("var b=B.new(); var bc=b._to_const_object; bc.set_str('abc')").execute ();
    EXPECT_EQ (1, 0);
  } catch (tl::Exception &ex) {
    EXPECT_EQ (ex.msg (), "Cannot call non-const method set_str, class B on a const reference at position 44 (...set_str('abc'))");
  }
  v = e.parse ("var e=E.new(); var ec=e.dup; [e._is_const_object, ec._to_const_object._is_const_object]").execute ();
  EXPECT_EQ (v.to_string (), std::string ("false,true"));
  v = e.parse ("var e=E.new(); var ec=e._to_const_object; e.x=17; [e.x, ec.x]").execute ();
  EXPECT_EQ (v.to_string (), std::string ("17,17"));
  v = e.parse ("var e=E.new(); var ec=e._to_const_object; ec._is_const_object").execute ();
  EXPECT_EQ (v.to_string (), std::string ("true"));
  v = e.parse ("var e=E.new(); var ec=e._to_const_object; ec=ec._const_cast; ec._is_const_object").execute ();
  EXPECT_EQ (v.to_string (), std::string ("false"));
  v = e.parse ("var e=E.new(); var ec=e._to_const_object; ec=ec._const_cast; ec.x=42; e.x").execute ();
  EXPECT_EQ (v.to_string (), std::string ("42"));
  v = e.parse ("var e=E.new(); var ec=e._to_const_object; e.x=17; ec.x").execute ();
  EXPECT_EQ (v.to_string (), std::string ("17"));
  try {
    v = e.parse ("var e=E.new(); var ec=e._to_const_object; e.x=17; e._destroy; ec.x").execute ();
    EXPECT_EQ (1, 0);
  } catch (tl::Exception &ex) {
    EXPECT_EQ (ex.msg (), "Object has been destroyed already at position 64 (...x)");
  }
}

