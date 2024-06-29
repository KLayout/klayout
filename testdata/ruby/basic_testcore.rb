# encoding: UTF-8

# NOTE: we need to do a "require" here since basic_testcore_defs.rb is not
# safe in multiple inclusions
require File.expand_path('../basic_testcore_defs', __FILE__)


class ObjectWithStr

  def initialize(s)
    @s = s
  end

  def to_s
    @s
  end

end

class Basic_TestClass < TestBase

  def test_FIRST

    # for testing the ut logger:
    puts "Special chars: <&>"

    GC.start 
    
    # all references of A are released now:
    ic0 = RBA::A::instance_count
    assert_equal( ic0, 0 )

    a = RBA::A.new_a(100)
    assert_equal( RBA::A::instance_count, ic0 + 1 )

    a = RBA::A.new
    assert_equal(a.get_n, 17)
    a.assign(RBA::A.new(110))
    assert_equal(a.get_n, 110)

    a = nil
    GC.start
    GC.start
    assert_equal( RBA::A::instance_count, ic0 )

    a = RBA::A.new
    assert_equal( RBA::A::instance_count, ic0 )  # delayed instantiation of detached objects - A is actually created if it is used first
    a.a2   # just check, if it can be called
    assert_equal( RBA::A::instance_count, ic0 + 1 )

    # open question: with ruby 1.8, aa is not deleted if the assert_equal is missing. Why?
    # maybe the GC does not like to be called that frequently?
    aa = a.dup
    assert_equal( RBA::A::instance_count, ic0 + 2 )

    aa = nil
    GC.start 

    if RUBY_VERSION >= "1.9.0"
      # GC works differently in >=1.9.x - but no leak (verified with valgrind)
      ic0 = RBA::A::instance_count
    else
      assert_equal( RBA::A::instance_count, ic0 + 1 )
    end

    a = nil
    GC.start

    if RUBY_VERSION >= "1.9.0"
      # GC works differently in >=1.9.x - but no leak (verified with valgrind)
      ic0 = RBA::A::instance_count
    else
      assert_equal( RBA::A::instance_count, ic0 )
    end

    a = RBA::A.new
    assert_equal( RBA::A::instance_count, ic0 )  # delayed instantiation of detached objects - A is actually created if it is used first
    a.a2   # just check, if it can be called
    assert_equal( RBA::A::instance_count, ic0 + 1 )

    # mix of getters, setters, predicates
    assert_equal( a.af, false )
    assert_equal( a.af(true), false )
    a.af = true
    assert_equal( a.af?, true )
    assert_equal( a.af?(true), true )

    # static and non-static methods can be mixed
    assert_equal( RBA::A.aa, "static_a" )
    assert_equal( a.aa, "a" )

    assert_equal( a.get_n, 17 )
    a.a5 -5
    assert_equal( a.get_n, -5 )
    a.a5 0x7fffffff
    assert_equal( a.get_n, 0x7fffffff )
    a.a5 -0x80000000
    assert_equal( a.get_n, -0x80000000 )

    assert_equal( a.a3("a"), 1 )
    assert_equal( a.a3(ObjectWithStr::new("abcde")), 5 )   # implicitly using to_s for string conversion
    assert_equal( a.a3("ab"), 2 )
    assert_equal( a.a3("µ"), 2 )  # two UTF8 bytes
    if a.respond_to?(:a3_qstr) 
      assert_equal( a.a3_qstr("a"), 1 )
      assert_equal( a.a3_qstr("ab"), 2 )
      assert_equal( a.a3_qstr("µ"), 1 )  # one UTF8 character
      assert_equal( a.a3_qstrref("a"), 1 )
      assert_equal( a.a3_qstrref("ab"), 2 )
      assert_equal( a.a3_qstrref("µ"), 1 )  # one UTF8 character
      assert_equal( a.a3_qba("a"), 1 )
      assert_equal( a.a3_qba("ab"), 2 )
      assert_equal( a.a3_qba("µ"), 2 )  # two UTF8 bytes
    end
    assert_equal( a.a3(""), 0 )

    assert_equal( a.a4([1]), 1.0 )
    assert_equal( a.a4([1, 125e-3]), 0.125 )
    assert_equal( a.a4([5, 1, -1.25]), -1.25 )

    arr = []
    a.a6 { |d| arr.push(d) } 
    assert_equal(arr, [5, 1, -1.25])

    arr = []
    a.a7 { |d| arr.push(d) } 
    assert_equal(arr, [5, 1, -1.25])

    arr = []
    a.a8 { |d| arr.push(d) } 
    assert_equal(arr, [5, 1, -1.25])

    a.destroy
    assert_equal( RBA::A::instance_count, ic0 )

    if !$leak_check 

      error_caught = false
      begin 
        a.a2  # object has been destroyed already
      rescue
        error_caught = true
      end
      assert_equal( error_caught, true )

      error_caught = false
      begin 
        a.destroy  # object has been destroyed already
      rescue
        error_caught = true
      end
      assert_equal( error_caught, true )

    end

    assert_equal( RBA::A::instance_count, ic0 )
    a = RBA::A::new_a( 55 )
    assert_equal( RBA::A::instance_count, ic0 + 1 )
    assert_equal( a.get_n, 55 )
    assert_equal( a.a_vp1( a.a_vp2 ), "abc" )
    a.destroy
    assert_equal( RBA::A::instance_count, ic0 )

    a = RBA::A::new_a(0)
    assert_equal( a.a9a(5).to_s, "true" )
    assert_equal( a.a9a(4).to_s, "false" )
    assert_equal( a.a9b(true).to_s, "5" )
    assert_equal( a.a9b(0).to_s, "5" )
    assert_equal( a.a9b(1).to_s, "5" )
    assert_equal( a.a9b(false).to_s, "-5" )
    assert_equal( a.a9b(nil).to_s, "-5" )

    ea = RBA::Enum::a
    eb = RBA::Enum::b
    ei = RBA::Enum::new(17)
    e0 = RBA::Enum::new
    assert_equal( ea.to_s, "a" )
    assert_equal( RBA::Enum::new(ea.to_s).to_s, "a" )
    assert_equal( eb.to_s, "b" )
    assert_equal( RBA::Enum::new(eb.to_s).to_s, "b" )
    assert_equal( ei.to_s, "#17" )
    assert_equal( RBA::Enum::new(ei.to_s).to_s, "#17" )
    assert_equal( e0.to_s, "#0" )
    assert_equal( RBA::Enum::new(e0.to_s).to_s, "#0" )
    assert_equal( e0.inspect, "(not a valid enum value)" )
    assert_equal( ea.inspect, "a (1)" )
    assert_equal( eb.inspect, "b (2)" )
    assert_equal( eb == ea, false )
    assert_equal( eb == eb, true )
    assert_equal( eb != ea, true )
    assert_equal( eb != eb, false )
    assert_equal( eb < ea, false )
    assert_equal( eb < eb, false )
    assert_equal( ea < eb, true )

    if RBA.constants.member?(:Enums)

      eea = RBA::Enums::new
      eei = RBA::Enums::new(3)
      eeb = RBA::Enums::new(eb)
      assert_equal( eea.to_s, "" )
      assert_equal( eea.inspect, " (0)" )
      assert_equal( RBA::Enums::new(eea.to_s).inspect, " (0)" )
      assert_equal( eei.inspect, "a|b (3)" )
      assert_equal( RBA::Enums::new(eei.to_s).inspect, "a|b (3)" )
      assert_equal( eeb.inspect, "b (2)" )
      assert_equal( RBA::Enums::new(eeb.to_s).inspect, "b (2)" )
      eeab1 = ea | eb
      eeab2 = ea | RBA::Enums::new(eb)
      eeab3 = RBA::Enums::new(ea) | eb
      eeab4 = RBA::Enums::new(ea) | RBA::Enums::new(eb)
      assert_equal( eeab1.inspect, "a|b (3)" )
      assert_equal( eeab2.inspect, "a|b (3)" )
      assert_equal( eeab3.inspect, "a|b (3)" )
      assert_equal( eeab4.inspect, "a|b (3)" )
      # Note: unsigned enum's will produce the long int, signed enums will produce the short one
      assert_equal( (~eeab4).inspect == " (-4)" || (~eeab4).inspect == " (4294967292)", true )
      assert_equal( (eeab4 & ea).inspect, "a (1)" )
      assert_equal( (eeab4 & eeb).inspect, "b (2)" )
      assert_equal( (eeab4 ^ eeb).inspect, "a (1)" )
      assert_equal( (eeab4 ^ eb).inspect, "a (1)" )
      assert_equal( eeab4.inspect, "a|b (3)" )
      eeab4 ^= ea
      assert_equal( eeab4.inspect, "b (2)" )

      assert_equal( a.get_e.to_s, "#0" )
      a.set_e( RBA::Enum::a )
      assert_equal( a.get_e.to_s, "a" )
      a.set_e( RBA::Enum::b )
      assert_equal( a.get_e.to_s, "b" )
      a.set_eptr( nil )
      assert_equal( a.get_e.to_s, "#0" )
      a.set_eptr( RBA::Enum::c )
      assert_equal( a.get_e.to_s, "c" )
      a.set_ecptr( nil )
      assert_equal( a.get_e.to_s, "#0" )
      a.set_ecptr( RBA::Enum::b )
      assert_equal( a.get_e.to_s, "b" )
      a.set_ecref( RBA::Enum::a )
      assert_equal( a.get_e.to_s, "a" )
      a.set_eref( RBA::Enum::c )
      assert_equal( a.get_e.to_s, "c" )
      assert_equal( a.get_eptr.to_s, "c" )
      assert_equal( a.get_eref.to_s, "c" )
      assert_equal( a.get_ecptr.to_s, "c" )
      assert_equal( a.get_ecref.to_s, "c" )
      a.set_ecptr( nil )
      assert_equal( a.get_ecptr, nil )
      assert_equal( a.get_ecref.to_s, "#0" )
      assert_equal( a.get_eptr, nil )
      assert_equal( a.get_eref.to_s, "#0" )

      ee = RBA::Enum::new
      assert_equal( ee.to_s, "#0" )
      a.mod_eref( ee, RBA::Enum::c )
      assert_equal( ee.to_s, "c" )
      a.mod_eptr( ee, RBA::Enum::a )
      assert_equal( ee.to_s, "a" )

      assert_equal( a.ev.inspect, "[]" )
      a.push_ev( RBA::Enum::a )
      a.push_ev( RBA::Enum::new )
      a.push_ev( RBA::Enum::b )
      assert_equal( a.ev.inspect, "[a (1), (not a valid enum value), b (2)]" )

      assert_equal( a.get_ef.inspect, " (0)" )
      a.set_ef( RBA::Enum::a )
      assert_equal( a.get_ef.to_s, "a" )
      a.set_ef( RBA::Enums::new(RBA::Enum::b) )
      assert_equal( a.get_ef.to_s, "b" )
      a.set_efptr( nil )
      assert_equal( a.get_ef.inspect, " (0)" )
      a.set_efptr( RBA::Enums::new(RBA::Enum::c) )
      assert_equal( a.get_ef.to_s, "a|b|c" )
      a.set_efcptr( nil )
      assert_equal( a.get_ef.inspect, " (0)" )
      a.set_efcptr( RBA::Enums::new(RBA::Enum::b) )
      assert_equal( a.get_ef.to_s, "b" )
      a.set_efcptr( RBA::Enum::c )
      assert_equal( a.get_ef.to_s, "a|b|c" )
      a.set_efcref( RBA::Enum::b )
      assert_equal( a.get_ef.to_s, "b" )
      a.set_efcref( RBA::Enums::new(RBA::Enum::a) )
      assert_equal( a.get_ef.to_s, "a" )
      a.set_efref( RBA::Enums::new(RBA::Enum::c) )
      assert_equal( a.get_ef.to_s, "a|b|c" )
      assert_equal( a.get_efptr.to_s, "a|b|c" )
      assert_equal( a.get_efref.to_s, "a|b|c" )
      assert_equal( a.get_efcptr.to_s, "a|b|c" )
      assert_equal( a.get_efcref.to_s, "a|b|c" )
      a.set_efcptr( nil )
      assert_equal( a.get_efcptr, nil )
      assert_equal( a.get_efcref.inspect, " (0)" )
      assert_equal( a.get_efptr, nil )
      assert_equal( a.get_efref.inspect, " (0)" )

      ee = RBA::Enums::new
      assert_equal( ee.inspect, " (0)" )
      a.mod_efref( ee, RBA::Enum::b )
      assert_equal( ee.to_s, "b" )
      a.mod_efptr( ee, RBA::Enum::a )
      assert_equal( ee.to_s, "a|b" )

    end

  end

  def test_10

    GC.start 
    
    # all references of A are released now:
    ic0 = RBA::A::instance_count
    assert_equal(ic0, 0)

    a = RBA::A.new_a_by_variant
    assert_equal(RBA::A::instance_count, ic0 + 1)

    assert_equal(a.get_n, 17)
    a.a5(-15)
    assert_equal(a.get_n, -15)

    a = nil
    GC.start
    assert_equal(RBA::A::instance_count, ic0)

    # destruction of raw instances (non-gsi-enabled) via c++
    GC.start

    ic0 = RBA::B::instance_count
    assert_equal(ic0, 0)

    b = RBA::B.new_b_by_variant
    assert_equal(RBA::B::instance_count, ic0 + 1)

    b.set_str_combine("x", "y")
    assert_equal(b.str, "xy")

    b._destroy
    assert_equal(RBA::B::instance_count, ic0)

  end

  def test_11

    # implicitly converting tuples/lists to objects by calling the constructor

    b = RBA::B::new()
    b.av_cptr = [ RBA::A::new(17), [1,2], [4,6,0.5] ]

    arr = b.each_a.collect { |a| a.get_n_const }
    assert_equal(arr, [17, 3, 5])
    
    b = RBA::B::new()
    # NOTE: this gives an error (printed only) that tuples can't be modified as out parameters
    b.av_ref = [ [1,2], [6,2,0.25], [42] ]

    arr = b.each_a.collect { |a| a.get_n_const }
    assert_equal(arr, [3, 2, 42])
    
    b = RBA::B::new()
    aa = [ [1,2], [6,2,0.25], [42] ]
    b.av_ptr = aa

    arr = b.each_a.collect { |a| a.get_n_const }
    assert_equal(arr, [3, 2, 42])
    
    # NOTE: as we used aa in "av_ptr", it got modified as out parameter and
    # now holds A object references
    arr = b.each_a.collect { |a| a.get_n_const }
    assert_equal(arr, [3, 2, 42])
    
    b.av = [] 

    arr = b.each_a.collect { |a| a.get_n_const }
    assert_equal(arr, [])
    
    b.push_a_ref([1, 7])

    arr = b.each_a.collect { |a| a.get_n_const }
    assert_equal(arr, [8])
    
    b.push_a_ptr([1, 7, 0.25])

    arr = b.each_a.collect { |a| a.get_n_const }
    assert_equal(arr, [8, 2])
    
    b.push_a_cref([42])

    arr = b.each_a.collect { |a| a.get_n_const }
    assert_equal(arr, [8, 2, 42])
    
    b.push_a_cptr([1, 16])

    arr = b.each_a.collect { |a| a.get_n_const }
    assert_equal(arr, [8, 2, 42, 17])
    
    b.push_a([4, 6, 0.5])

    arr = b.each_a.collect { |a| a.get_n_const }
    assert_equal(arr, [8, 2, 42, 17, 5])

  end

  def test_12

    a1 = RBA::A.new
    a1.a5( -15 )
    a2 = a1
    a3 = a2.dup

    assert_equal( a1.get_n, -15 )
    assert_equal( a2.get_n, -15 )
    assert_equal( a3.get_n, -15 )

    a1.a5( 11 )
    a3.a5( -11 )
    
    assert_equal( a1.get_n, 11 )
    assert_equal( a2.get_n, 11 )
    assert_equal( a3.get_n, -11 )

    assert_equal( a1.a10_s(0x70000000), "0" )
    assert_equal( a1.a10_s(0x7fffffff), "-1" )
    assert_equal( a1.a10_us(0x70000000), "0" )
    assert_equal( a1.a10_us(0x7fffffff), "65535" )
    assert_equal( a1.a10_i(-0x80000000), "-2147483648" )
    assert_equal( a1.a10_l(-0x80000000), "-2147483648" )
    assert_equal( a1.a10_ll(-0x80000000), "-2147483648" )
    assert_equal( a1.a10_ui(0xffffffff), "4294967295" )
    assert_equal( a1.a10_ul(0xffffffff), "4294967295" )
    assert_equal( a1.a10_ull(0xffffffff), "4294967295" )
    assert_equal( a1.a11_s(0x70000000), 0 )
    assert_equal( a1.a11_s(0x7fffffff), -1 )
    assert_equal( a1.a11_us(0x70000000), 0 )
    assert_equal( a1.a11_us(0x7fffffff), 65535 )
    assert_equal( a1.a11_i(-0x80000000), -2147483648 )
    assert_equal( a1.a11_l(-0x80000000), -2147483648 )
    assert_equal( a1.a11_ll(-0x80000000), -2147483648 )
    assert_equal( a1.a11_ui(0xffffffff), 4294967295 )
    assert_equal( a1.a11_ul(0xffffffff), 4294967295 )
    assert_equal( a1.a11_ull(0xffffffff), 4294967295 )

    assert_equal( a1.a10_d(5.2), "5.2" )
    if a1.respond_to?(:a10_d_qstr)
      assert_equal( a1.a10_d_qstr(5.25), "5.25" )
      assert_equal( a1.a10_d_qstrref(5.2), "5.2" )
      assert_equal( a1.a10_d_qba(5.1), "5.1" )
    end
    assert_equal( a1.a10_f(5.7), "5.7" )
    x = RBA::Value.new(1.5)
    assert_equal( x.value.to_s, "1.5" )
    assert_equal( a1.a10_fptr(x), "6.5" )
    assert_equal( x.value.to_s, "6.5" )
    assert_equal( a1.a10_fptr(1), "6" )
    assert_equal( a1.a10_fptr(nil), "nil" )
    assert_equal( a1.a10_fptr(RBA::Value.new), "nil" )
    assert_equal( x.to_s, "6.5" )
    assert_equal( x.value.to_s, "6.5" )
    x = RBA::Value.new(2.5)
    assert_equal( a1.a10_dptr(x), "8.5" )
    assert_equal( a1.a10_dptr(2), "8" )
    assert_equal( a1.a10_dptr(nil), "nil" )
    assert_equal( a1.a10_dptr(RBA::Value.new), "nil" )
    assert_equal( x.to_s, "8.5" )
    assert_equal( x.value.to_s, "8.5" )
    x = RBA::Value.new(2)
    assert_equal( a1.a10_iptr(x), "9" )
    assert_equal( a1.a10_iptr(3), "10" )
    assert_equal( a1.a10_iptr(nil), "nil" )
    assert_equal( a1.a10_iptr(RBA::Value.new), "nil" )
    assert_equal( x.to_s, "9" )
    assert_equal( x.value.to_s, "9" )
    x = RBA::Value.new(false)
    assert_equal( a1.a10_bptr(x), "true" )
    assert_equal( a1.a10_bptr(false), "true" )
    assert_equal( a1.a10_bptr(nil), "nil" )
    assert_equal( a1.a10_bptr(RBA::Value.new), "nil" )
    assert_equal( x.to_s, "true" )
    assert_equal( x.value.to_s, "true" )
    x = RBA::Value.new(10)
    assert_equal( a1.a10_uiptr(x), "20" )
    assert_equal( a1.a10_uiptr(11), "21" )
    assert_equal( a1.a10_uiptr(nil), "nil" )
    assert_equal( a1.a10_uiptr(RBA::Value.new), "nil" )
    assert_equal( x.to_s, "20" )
    assert_equal( x.value.to_s, "20" )
    x = RBA::Value.new(10)
    assert_equal( a1.a10_ulptr(x), "21" )
    assert_equal( a1.a10_ulptr(12), "23" )
    assert_equal( a1.a10_ulptr(nil), "nil" )
    assert_equal( a1.a10_ulptr(RBA::Value.new), "nil" )
    assert_equal( x.to_s, "21" )
    assert_equal( x.value.to_s, "21" )
    x = RBA::Value.new(10)
    assert_equal( a1.a10_lptr(x), "22" )
    assert_equal( a1.a10_lptr(11), "23" )
    assert_equal( a1.a10_lptr(nil), "nil" )
    assert_equal( a1.a10_lptr(RBA::Value.new), "nil" )
    assert_equal( x.to_s, "22" )
    assert_equal( x.value.to_s, "22" )
    x = RBA::Value.new(10)
    assert_equal( a1.a10_llptr(x), "23" )
    assert_equal( a1.a10_llptr(11), "24" )
    assert_equal( a1.a10_llptr(nil), "nil" )
    assert_equal( a1.a10_llptr(RBA::Value.new), "nil" )
    assert_equal( x.to_s, "23" )
    assert_equal( x.value.to_s, "23" )
    x = RBA::Value.new(10)
    assert_equal( a1.a10_ullptr(x), "24" )
    assert_equal( a1.a10_ullptr(12), "26" )
    assert_equal( a1.a10_ullptr(nil), "nil" )
    assert_equal( a1.a10_ullptr(RBA::Value.new), "nil" )
    assert_equal( x.to_s, "24" )
    assert_equal( x.value.to_s, "24" )
    assert_equal( a1.a10_sptr(nil), "nil" )
    x = RBA::Value.new("z")
    assert_equal( a1.a10_sptr(x), "zx" )
    assert_equal( a1.a10_sptr("a"), "ax" )
    assert_equal( a1.a10_sptr(RBA::Value.new), "nil" )
    assert_equal( x.to_s, "zx" )
    assert_equal( x.value.to_s, "zx" )

    # String modification is not possible in Ruby API, hence strings cannot be used
    # directly as out parameters (but as boxed values they can)
    x = "z"
    assert_equal( a1.a10_sptr(x), "zx" )
    assert_equal( x, "z" )

    begin
      # passing other objects than StringValue and a string fails
      assert_equal( a1.a10_sptr([]), "nil" )
      err = false
    rescue 
      err = true
    end
    assert_equal( err, true )

    assert_equal( a1.a10_cfptr(6.5), "6.5" )
    assert_equal( a1.a10_cfptr(nil), "nil" )
    assert_equal( a1.a10_cdptr(8.5), "8.5" )
    assert_equal( a1.a10_cdptr(nil), "nil" )
    assert_equal( a1.a10_ciptr(9), "9" )
    assert_equal( a1.a10_ciptr(nil), "nil" )
    assert_equal( a1.a10_cbptr(true), "true" )
    assert_equal( a1.a10_cbptr(nil), "nil" )
    assert_equal( a1.a10_cuiptr(20), "20" )
    assert_equal( a1.a10_cuiptr(nil), "nil" )
    assert_equal( a1.a10_culptr(21), "21" )
    assert_equal( a1.a10_culptr(nil), "nil" )
    assert_equal( a1.a10_clptr(22), "22" )
    assert_equal( a1.a10_clptr(nil), "nil" )
    assert_equal( a1.a10_cllptr(23), "23" )
    assert_equal( a1.a10_cllptr(nil), "nil" )
    assert_equal( a1.a10_cullptr(24), "24" )
    assert_equal( a1.a10_cullptr(nil), "nil" )
    assert_equal( a1.a10_csptr(nil), "nil" )
    assert_equal( a1.a10_csptr("x"), "x" )

    x = RBA::Value.new(1.5)
    assert_equal( a1.a10_fref(x), "11.5" )
    begin
      assert_equal( a1.a10_fref(nil), "nil" )
      err = false
    rescue 
      err = true
    end
    assert_equal( err, true )
    begin
      assert_equal( a1.a10_fref(RBA::Value.new), "nil" )
      err = false
    rescue 
      err = true
    end
    assert_equal( err, true )
    assert_equal( x.value.to_s, "11.5" )
    x = RBA::Value.new(2.5)
    assert_equal( a1.a10_dref(x), "13.5" )
    assert_equal( a1.a10_dref(2), "13" )
    assert_equal( x.value.to_s, "13.5" )
    x = RBA::Value.new(2)
    assert_equal( a1.a10_iref(x), "14" )
    assert_equal( a1.a10_iref(1), "13" )
    assert_equal( x.value.to_s, "14" )
    x = RBA::Value.new(false)
    assert_equal( a1.a10_bref(x), "true" )
    assert_equal( a1.a10_bref(false), "true" )
    assert_equal( x.value.to_s, "true" )
    x = RBA::Value.new(10)
    assert_equal( a1.a10_uiref(x), "24" )
    assert_equal( a1.a10_uiref(11), "25" )
    assert_equal( x.value.to_s, "24" )
    x = RBA::Value.new(10)
    assert_equal( a1.a10_ulref(x), "25" )
    assert_equal( a1.a10_ulref(12), "27" )
    assert_equal( x.value.to_s, "25" )
    x = RBA::Value.new(10)
    assert_equal( a1.a10_lref(x), "26" )
    assert_equal( a1.a10_lref(13), "29" )
    assert_equal( x.value.to_s, "26" )
    x = RBA::Value.new(10)
    assert_equal( a1.a10_llref(x), "27" )
    assert_equal( a1.a10_llref(14), "31" )
    assert_equal( x.value.to_s, "27" )
    x = RBA::Value.new(10)
    assert_equal( a1.a10_ullref(x), "28" )
    assert_equal( a1.a10_ullref(11), "29" )
    assert_equal( x.value.to_s, "28" )
    x = RBA::Value.new("x")
    assert_equal( a1.a10_sref(x), "xy" )
    assert_equal( x.value.to_s, "xy" )
    assert_equal( a1.a10_sref("p"), "py" )

    assert_equal( a1.a10_cfref(6.5), "6.5" )
    begin
      assert_equal( a1.a10_cfref(nil), "nil" )
      err = false
    rescue 
      err = true
    end
    assert_equal( err, true )
    assert_equal( a1.a10_cdref(8.5), "8.5" )
    assert_equal( a1.a10_ciref(9), "9" )
    assert_equal( a1.a10_cbref(true), "true" )
    assert_equal( a1.a10_cuiref(20), "20" )
    assert_equal( a1.a10_culref(21), "21" )
    assert_equal( a1.a10_clref(22), "22" )
    assert_equal( a1.a10_cllref(23), "23" )
    assert_equal( a1.a10_cullref(24), "24" )
    assert_equal( a1.a10_csref("x"), "x" )

  end

  def test_13

    b = RBA::B.new

    if !$leak_check 

      err_caught = false
      begin
        b.amember_cptr.get_n # cannot call non-const method on const reference
      rescue 
        err_caught = true
      end
      assert_equal( err_caught, true )

    end
      
    b.amember_cptr.a2 

    assert_equal( b.always_5, 5 )
    assert_equal( b.str, "" )
    b.set_str( "xyz" )
    assert_equal( b.str, "xyz" )
    assert_equal( b.str_ccptr, "xyz" )
    b.set_str_combine( "yx", "zz" )
    assert_equal( b.str, "yxzz" )
    assert_equal( b.str_ccptr, "yxzz" )

    arr = []

    err_caught = false

    if !$leak_check 

      begin
        b.each_a_be { |a| arr.push(a.get_n) }  # b10 is a const iterator - cannot call a1 on it
      rescue 
        err_caught = true
      end
      assert_equal( err_caught, true )
      assert_equal(arr, [])

    end
   
    err_caught = false

    if !$leak_check 

      begin
        b.each_a_be_pp { |a| arr.push(a.get_n) }  # b10p is a const iterator - cannot call a1 on it
      rescue 
        err_caught = true
      end
      assert_equal( err_caught, true )
      assert_equal(arr, [])

    end
   
    arr = []
    b.each_a_be { |a| arr.push(a.dup.get_n) } 
    assert_equal(arr, [100, 121, 144])

    arr = []
    b.dup.each_a_be { |a| arr.push(a.dup.get_n) } 
    assert_equal(arr, [100, 121, 144])

    arr = []
    b.each_a_be { |a| arr.push(a.get_n_const) } 
    assert_equal(arr, [100, 121, 144])

    arr = []
    b.each_a_be_pp { |a| arr.push(a.dup.get_n) } 
    assert_equal(arr, [100, 121, 144])

    arr = []
    b.dup.each_a_be_pp { |a| arr.push(a.dup.get_n) } 
    assert_equal(arr, [100, 121, 144])

    arr = []
    b.each_a_be_pp { |a| arr.push(a.get_n_const) } 
    assert_equal(arr, [100, 121, 144])

    arr = []
    b.each_a_be_v { |a| arr.push(a.get_n) } 
    assert_equal(arr, [100, 121, 144])

    arr = []
    b.dup.each_a_be_v { |a| arr.push(a.get_n) } 
    assert_equal(arr, [100, 121, 144])

    arr = []
    b.each_a_be_p { |a| arr.push(a.get_n) } 
    assert_equal(arr, [7100, 7121, 7144, 7169])

    arr = []
    b.dup.each_a_be_p { |a| arr.push(a.get_n) } 
    assert_equal(arr, [7100, 7121, 7144, 7169])

    aarr = b.av
    arr = []
    aarr.each { |a| arr.push(a.get_n) } 
    assert_equal(arr, [100, 121, 144])

    aarr = b.av_cref
    arr = []
    aarr.each { |a| arr.push(a.get_n) } 
    assert_equal(arr, [100, 121, 144])

    aarr = b.av_ref
    arr = []
    aarr.each { |a| arr.push(a.get_n) } 
    assert_equal(arr, [100, 121, 144])

    b.av_cref = [ RBA::A.new_a( 101 ), RBA::A.new_a( -122 ) ]
    arr = []
    b.each_a_be_v { |a| arr.push(a.get_n) } 
    assert_equal(arr, [101, -122])

    b.av_cref = []
    arr = []
    b.each_a_be_v { |a| arr.push(a.get_n) } 
    assert_equal(arr, [])

    b.av_ref = [ RBA::A.new_a( 102 ), RBA::A.new_a( -123 ) ]
    arr = []
    b.each_a_be_v { |a| arr.push(a.get_n) } 
    assert_equal(arr, [102, -123])

    b.av = [ RBA::A.new_a( 100 ), RBA::A.new_a( 121 ), RBA::A.new_a( 144 ) ]
    arr = []
    b.each_a_be_v { |a| arr.push(a.get_n) } 
    assert_equal(arr, [100, 121, 144])

    if !$leak_check 

      arr = []
      begin
        b.each_a_be_cp { |a| arr.push(a.get_n) } 
      rescue 
        err_caught = true
      end
      assert_equal( err_caught, true )
      assert_equal(arr, [])

    end

    arr = []
    b.each_a_be_cp { |a| arr.push(a.get_n_const) } 
    assert_equal(arr, [-3100, -3121])

    arr = []
    b.dup.each_a_be_cp { |a| arr.push(a.get_n_const) } 
    assert_equal(arr, [-3100, -3121])

    arr = []
    b.each_a { |a| arr.push(a.get_n_const) } 
    assert_equal(arr, [100, 121, 144])

    arr = []
    b.each_a { |a| arr.push(a.get_n) } 
    assert_equal(arr, [100, 121, 144])

    arr = []
    b.each_a_ref { |a| arr.push(a.get_n_const) } 
    assert_equal(arr, [100, 121, 144])

    arr = []
    # even though each_a_ref returns a "const A &", calling a non-const method does not work
    # since A is a managed object and is not turned into a copy.
    err_caught = false
    begin 
      b.each_a_ref { |a| arr.push(a.get_n) } 
    rescue
      err_caught = true
    end
    assert_equal(arr, [])
    assert_equal(err_caught, true)

    arr = []
    b.each_a_ptr { |a| arr.push(a.get_n_const) } 
    assert_equal(arr, [100, 121, 144])

    arr = []
    # this does not work since each_a_ptr delivers a "const *" which cannot be used to call a non-const
    # method on
    err_caught = false
    begin 
      b.each_a_ptr { |a| arr.push(a.get_n) } 
    rescue
      err_caught = true
    end
    assert_equal(arr, [])
    assert_equal(err_caught, true)

  end

  def test_13b

    b = RBA::B.new

    bb = RBA::B.new
    bb.set_str("a")
    b.push_b(bb)

    bb = RBA::B.new
    bb.set_str("y")
    b.push_b(bb)

    bb = RBA::B.new
    bb.set_str("uu")
    b.push_b(bb)

    arr = []
    b.each_b_copy { |bb| arr.push(bb.str) } 
    assert_equal(arr, ["a", "y", "uu"])
    # through enumerator
    if RUBY_VERSION > "2.0.0"
      # this creates GC leaks in 2.0.0
      assert_equal(b.each_b_copy.collect(&:str), ["a", "y", "uu"])
    end

    arr = []
    b.each_b_copy { |bb| bb.set_str(bb.str + "x"); arr.push(bb.str) } 
    assert_equal(arr, ["ax", "yx", "uux"])

    arr = []
    b.each_b_copy { |bb| arr.push(bb.str) } 
    assert_equal(arr, ["a", "y", "uu"])

    arr = []
    b.each_b_cref { |bb| arr.push(bb.str) } 
    assert_equal(arr, ["a", "y", "uu"])
    # through enumerator
    if RUBY_VERSION > "2.0.0"
      # this creates GC leaks in 2.0.0
      assert_equal(b.each_b_cref.collect(&:str), ["a", "y", "uu"])
    end

    arr = []
    # this works, since the "const B &" will be converted to a copy
    b.each_b_cref { |bb| bb.set_str(bb.str + "x"); arr.push(bb.str) } 
    assert_equal(arr, ["ax", "yx", "uux"])

    arr = []
    # since that was a copy, the b children were not modified
    b.each_b_cref { |bb| arr.push(bb.str) } 
    assert_equal(arr, ["a", "y", "uu"])

    arr = []
    b.each_b_cptr { |bb| arr.push(bb.str) } 
    assert_equal(arr, ["a", "y", "uu"])
    # through enumerator
    if RUBY_VERSION > "2.0.0"
      # this creates GC leaks in 2.0.0
      assert_equal(b.each_b_cptr.collect(&:str), ["a", "y", "uu"])
    end

    arr = []
    # const references cannot be modified
    err_caught = false
    begin
      b.each_b_cptr { |bb| bb.set_str(bb.str + "x"); arr.push(bb.str) } 
    rescue 
      err_caught = true
    end
    assert_equal(err_caught, true)
    assert_equal(arr, [])

    arr = []
    b.each_b_cptr { |bb| arr.push(bb.str) } 
    assert_equal(arr, ["a", "y", "uu"])

    arr = []
    b.each_b_ref { |bb| arr.push(bb.str) } 
    assert_equal(arr, ["a", "y", "uu"])
    # through enumerator
    if RUBY_VERSION > "2.0.0"
      # this creates GC leaks in 2.0.0
      assert_equal(b.each_b_ref.collect(&:str), ["a", "y", "uu"])
    end

    arr = []
    b.each_b_ref { |bb| bb.set_str(bb.str + "x"); arr.push(bb.str) } 
    assert_equal(arr, ["ax", "yx", "uux"])

    arr = []
    b.each_b_ref { |bb| arr.push(bb.str) } 
    assert_equal(arr, ["ax", "yx", "uux"])

    arr = []
    b.each_b_ptr { |bb| arr.push(bb.str) } 
    assert_equal(arr, ["ax", "yx", "uux"])
    # through enumerator
    if RUBY_VERSION > "2.0.0"
      # this creates GC leaks in 2.0.0
      assert_equal(b.each_b_ptr.collect(&:str), ["ax", "yx", "uux"])
    end

    arr = []
    b.each_b_ptr { |bb| bb.set_str(bb.str + "x"); arr.push(bb.str) } 
    assert_equal(arr, ["axx", "yxx", "uuxx"])

    arr = []
    b.each_b_ptr { |bb| arr.push(bb.str) } 
    assert_equal(arr, ["axx", "yxx", "uuxx"])

  end

  def _iter_find(b, find)
    b.each_a_be do |a| 
      if a.get_n_const == find
        return true
      end
    end
    return false
  end

  # Iterator break, return, continue
  def test_13c

    b = RBA::B.new

    arr = []
    b.each_a_be { |a| arr.push(a.get_n_const) } 
    assert_equal(arr, [100, 121, 144])

    arr = []
    b.each_a_be do |a| 
      if a.get_n_const == 121
        next
      end
      arr.push(a.get_n_const) 
    end
    assert_equal(arr, [100, 144])

    assert_equal(_iter_find(b, 121), true)
    assert_equal(_iter_find(b, 122), false)

  end

  def test_14

    a = RBA::A.new
    a.a5( 22 )

    b = RBA::B.new
    assert_equal( b.b3( a ), 22 )
    assert_equal( b.b4( a ), "b4_result: 22" )
    a.a5( -6 )
    assert_equal( b.b3( a ), -6 )
    assert_equal( b.b4( a ), "b4_result: -6" )
    assert_equal( b.b4( RBA::A.new ), "b4_result: 17" )

  end

  def test_15

    a = RBA::A_NC.new
    assert_equal( true, a.is_a?(RBA::A) )
    a.a5( 22 )

    b = RBA::B.new
    assert_equal( b.b3( a ), 22 )
    assert_equal( b.b4( a ), "b4_result: 22" )
    a.a5( -6 )
    assert_equal( b.b3( a ), -6 )
    assert_equal( b.b4( a ), "b4_result: -6" )
    assert_equal( b.b4( RBA::A_NC.new ), "b4_result: 17" )

  end

  if RUBY_VERSION < "3.0.0"
    # TODO: this class is going to be deprecated
    class X < Data
    end
  end
  class Y < Object
  end

  def test_16

    if $leak_check 
      return
    end

    # Test, if this throws an error (object of class X passed to A argument):
    if RUBY_VERSION < "3.0.0"
      begin
        b = RBA::B.new
        assert_equal( b.b4( X.new ), "b4_result: -6" )
        assert_equal( false, true )  # this must never hit
      rescue
        assert_equal( $!.to_s(), "allocator undefined for Basic_TestClass::X" );
      end
    end
  
    # Test, if this throws an error (object of class X passed to A argument):
    begin
      b = RBA::B.new
      assert_equal( b.b4( Y.new ), "b4_result: -6" )
      assert_equal( false, true )  # this must never hit
    rescue
      assert_equal( $!.to_s(), "Unexpected object type (expected argument of class A, got Basic_TestClass::Y) for argument #1 in B::b4" );
    end
  
    # Test, if this throws an error (object of class X passed to A argument):
    begin
      b = RBA::B.new
      bb = RBA::B.new
      assert_equal( b.b4( bb ), "b4_result: -6" )
      assert_equal( false, true )  # this must never hit
    rescue
      assert_equal( $!.to_s(), "Unexpected object type (expected argument of class A, got RBA::B) for argument #1 in B::b4" );
    end
  
  end

  def test_17

    # test copies of objects being returned

    b = RBA::B.new
    b._create

    GC.start
    a_count = RBA::A.instance_count
    a = b.make_a( 1971 );
    assert_equal( RBA::A.instance_count, a_count + 1 )

    assert_equal( a.get_n, 1971 );
    assert_equal( b.an( a ), 1971 );

    aa = b.make_a( -61 );
    assert_equal( RBA::A.instance_count, a_count + 2 )
    assert_equal( b.an_cref( aa ), -61 );
    assert_equal( a.get_n, 1971 );
    assert_equal( b.an( a ), 1971 );
    assert_equal( aa.get_n, -61 );
    assert_equal( b.an( aa ), -61 );

    aa.a5 98;
    a.a5 100;
    
    assert_equal( a.get_n, 100 );
    assert_equal( b.an( a ), 100 );
    assert_equal( aa.get_n, 98 );
    assert_equal( b.an( aa ), 98 );

    a._destroy
    aa = nil
    GC.start
    assert_equal( RBA::A.instance_count, a_count )

  end

  def test_18

    # Test references to objects (returned by b.amember_cptr)

    b = RBA::B.new
    b.set_an( 77 )
    assert_equal( b.amember_cptr.get_n_const, 77 );

    b.set_an_cref( 79 )
    assert_equal( b.amember_cptr.get_n_const, 79 );

    aref = b.amember_cptr
    err_caught = false

    if !$leak_check 

      begin 
        x = aref.get_n # cannot call non-const method on const reference (as delivered by amember_cptr)
      rescue
        err_caught = true
      end
      assert_equal( err_caught, true )
      assert_equal( aref.get_n_const, 79 );

    end

    b.set_an( -1 )
    assert_equal( aref.get_n_const, -1 );

  end

  class C_IMP1 < RBA::C 
    def f(s)
      return 615
    end
  end

  class C_IMP2 < RBA::C 
    def f(s)
      return s.size
    end
  end

  class C_IMP3 < RBA::C 
  end

  class C_IMP4 < RBA::C
    def initialize
      @x = @xx = nil
    end
    def x
      @x
    end
    def xx
      @xx
    end
    def vfunc(cd)
      @x = cd.x
      @xx = cd.xx
    end
  end

  def test_19

    c0 = RBA::C.new
    assert_equal( c0.g("x"), 1977 );

    c1 = C_IMP1.new
    assert_equal( c1.g("x"), 615 );

    c2 = C_IMP2.new
    assert_equal( c2.g("x"), 1 );
    assert_equal( c2.g(""), 0 );
    assert_equal( c2.g("abc"), 3 );
    assert_equal( c1.g("x"), 615 );

    c3 = C_IMP3.new
    assert_equal( c3.g("x"), 1977 );

    assert_equal( RBA::C.s1, 4451 );
    RBA::C.s2clr
    RBA::C.s2( 7.0 )
    assert_equal( RBA::C.s3( 5.5 ), "5.500" );

    arr = []
    RBA::C.each { |i| arr.push i }
    assert_equal( arr, [ 0, 1, 2, 3, 4, 5, 6 ] )

    assert_equal( C_IMP1.s1, 4451 );
    C_IMP1.s2( 1.0 )
    assert_equal( C_IMP1.s3( 1.5 ), "1.500" );

    arr = []
    C_IMP1.each { |i| arr.push i }
    assert_equal( arr, [ 0, 1, 2, 3, 4, 5, 6, 0 ] )

    assert_equal( C_IMP2.s1, 4451 );
    C_IMP2.s2( 2.0 )
    assert_equal( C_IMP2.s3( -1.5 ), "-1.500" );

    arr = []
    C_IMP2.each { |i| arr.push i }
    assert_equal( arr, [ 0, 1, 2, 3, 4, 5, 6, 0, 0, 1 ] )

    c4 = C_IMP4.new
    c4.call_vfunc(RBA::CopyDetector::new(17))
    assert_equal(c4.x, 17)
    assert_equal(c4.xx, 17)

  end

  def test_20

    b = RBA::B.new

    a1 = b.amember_or_nil( true )
    a2 = b.amember_ptr
    assert_equal( a1.get_n, 17 )
    assert_equal( a2.get_n, 17 )
    a1.a5( 761 )
    assert_equal( a1.get_n, 761 )
    assert_equal( a2.get_n, 761 )

    a1 = b.amember_or_nil( false )
    assert_equal( a1, nil )
    
    assert_equal( b.b15( b.amember_ptr ), true )
    assert_equal( b.b15( b.amember_or_nil( false ) ), false )
    assert_equal( b.b15( nil ), false )

  end

  def test_21

    # test client data binding to C++ objects 
    
    b = RBA::B.new
    
    b.amember_ptr.s( 117 )
    assert_equal( b.amember_ptr.g, 117 )

    n = 0
    b.b10_nc { |a| a.s( n ); n += 1 } 

    arr = []
    b.b10 { |a| arr.push( a.g ) }
    assert_equal( arr, [ 0, 1, 2 ] )

    arr = []
    b.b10p { |a| arr.push( a.g ) }
    assert_equal( arr, [ 0, 1, 2 ] )

  end

  def test_22

    # test client data binding to C++ objects 
    
    b = RBA::B.new
    
    assert_equal( b.b20a( 5.0 ), false )
    assert_equal( b.b20a( nil ), true )
    assert_equal( b.b20a( 1 ), false )
    assert_equal( b.b20a( "hallo" ), false )
    assert_equal( b.b20a( false ), false )
    assert_equal( b.b20a( true ), false )
    assert_equal( b.b20a( 10000000000000000 ), false )
    assert_equal( b.b20b( 5.0 ), true )
    assert_equal( b.b20b( nil ), false )
    assert_equal( b.b20b( 1 ), false )
    assert_equal( b.b20b( "hallo" ), false )
    assert_equal( b.b20b( false ), false )
    assert_equal( b.b20b( true ), false )
    # on 64 bit platforms this fits into a long value, therefore this test returns false:
    # assert_equal( b.b20b( 10000000000000000 ), false )
    assert_equal( b.b20c( 5.0 ), false )
    assert_equal( b.b20c( nil ), false )
    assert_equal( b.b20c( 1 ), true )
    assert_equal( b.b20c( "hallo" ), false )
    assert_equal( b.b20c( false ), false )
    assert_equal( b.b20c( true ), false )
    # on 64 bit platforms this fits into a long value, therefore this test returns true:
    # assert_equal( b.b20c( 10000000000000000 ), false )
    assert_equal( b.b20d( 5.0 ), false )
    assert_equal( b.b20d( nil ), false )
    assert_equal( b.b20d( 1 ), false )
    assert_equal( b.b20d( "hallo" ), true )
    assert_equal( b.b20d( false ), false )
    assert_equal( b.b20d( true ), false )
    assert_equal( b.b20d( 10000000000000000 ), false )
    assert_equal( b.b20e( 5.0 ), false )
    assert_equal( b.b20e( nil ), false )
    assert_equal( b.b20e( 1 ), false )
    assert_equal( b.b20e( "hallo" ), false )
    assert_equal( b.b20e( false ), true )
    assert_equal( b.b20e( true ), true )
    assert_equal( b.b20e( 10000000000000000 ), false )

    assert_equal( b.b21a( 50 ), "50" )
    assert_equal( b.b21a( true ), "true" )
    assert_equal( b.b21a( false ), "false" )
    assert_equal( b.b21a( "hallo" ), "hallo" )
    assert_equal( b.b21a( 5.5 ), "5.5" )
    assert_equal( b.b21a( nil ), "nil" )

    assert_equal( b.b21b( 50 ), 50.0 )
    assert_equal( b.b21b( true ), 1.0 )
    assert_equal( b.b21b( false ), 0.0 )
    assert_equal( b.b21b( 5.5 ), 5.5 )
    assert_equal( b.b21b( nil ), 0.0 )

    assert_equal( b.b21c( 50 ), 50 )
    assert_equal( b.b21c( true ), 1 )
    assert_equal( b.b21c( false ), 0 )
    assert_equal( b.b21c( 5.5 ), 5 )
    assert_equal( b.b21c( nil ), 0 )

    assert_equal( b.b22a( [ 1, "hallo", 5.5 ] ), 3 ) 
    assert_equal( b.b23a, [ 1, "hallo", 5.5 ] ) 
    a = [] 
    b.b24 { |x| a.push( x ) } 
    assert_equal( a, [ 1, "hallo", 5.5 ] ) 
    assert_equal( b.b22c, 5.5 )
    assert_equal( b.b22d, 5.5 )
    assert_equal( b.b22a( [ 1, "hallo" ] ), 2 ) 
    assert_equal( b.b23b, [ 1, "hallo" ] ) 
    assert_equal( b.b23d, [ 1, "hallo" ] ) 
    assert_equal( b.b23e, [ 1, "hallo" ] ) 
    assert_equal( b.b23e_null, nil ) 
    assert_equal( b.b23f, [ 1, "hallo" ] ) 
    assert_equal( b.b23f_null, nil ) 
    assert_equal( b.b22c, "hallo" )
    assert_equal( b.b22d, "hallo" )
    assert_equal( b.b22a( [ ] ), 0 ) 
    assert_equal( b.b23c, [ ] ) 
    a = [] 
    b.b24 { |x| a.push( x ) } 
    assert_equal( a, [ ] ) 
    assert_equal( b.b22b, nil )
    assert_equal( b.b22c, nil )
    assert_equal( b.b22d, nil )
    assert_equal( b.b22a( [ [ 1, "hallo" ], [ 10, 17, 20 ] ] ), 2 ) 
    assert_equal( b.b23a, [ [ 1, "hallo" ], [ 10, 17, 20 ] ] ) 
    a = [] 
    b.b24 { |x| a.push( x ) } 
    assert_equal( a, [ [ 1, "hallo" ], [ 10, 17, 20 ] ] ) 

    # ability to pass complex objects over tl::Variant:
    assert_equal( b.b22a( [ RBA::Box.new(RBA::Point.new(0, 0), RBA::Point.new(10, 20)) ] ), 1 ) 
    assert_equal( b.b22c.to_s, "(0,0;10,20)" )
    assert_equal( b.b22c.class.inspect, "RBA::Box" )

    # ability to pass complex objects over tl::Variant:
    assert_equal( b.b22a( [ RBA::DBox.new(RBA::DPoint.new(0, 0), RBA::DPoint.new(10, 20)) ] ), 1 ) 
    assert_equal( b.b22c.to_s, "(0,0;10,20)" )
    assert_equal( b.b22c.class.inspect, "RBA::DBox" )

    # ability to pass complex objects over tl::Variant:
    assert_equal( b.b22a( [ RBA::LayerInfo.new("hallo") ] ), 1 ) 
    assert_equal( b.b22c.to_s, "hallo" )
    assert_equal( b.b22c.class.inspect, "RBA::LayerInfo" )

  end

  def test_23

    b = RBA::B::new
    a = RBA::A::new

    assert_equal( b.bx, 17 )
    assert_equal( b.b30, 17 )
    assert_equal( b.bx( 5 ), "xz" )
    assert_equal( b.by( 5 ), "xz" )
    assert_equal( b.b31( 6 ), "xz" )
    assert_equal( b.b33( a ), "aref" )
    assert_equal( b.bx( a ), "aref" )
    assert_equal( b.bx( "a", 15 ), 20.5 )
    assert_equal( b.b32( "b", 25 ), 20.5 )

    GC.start
    na = RBA::A::instance_count    # instance count
    assert_equal( b.bx( a, 15 ), "aref+i" )
    GC.start
    assert_equal( RBA::A::instance_count, na )
    err_caught = false
    begin 
      # cannot cast second argument to int
      assert_equal( b.b34( a, "X" ), "aref+i" )
    rescue => ex
      err_caught = true
    end
    assert_equal( err_caught, true )
    # the exception thrown before must not leave an instance on the call stack:
    GC.start
    assert_equal( RBA::A::instance_count, na )

    err_caught = false
    begin 
      # invalid number of arguments
      assert_equal( b.by, "xz" )
    rescue
      err_caught = true
    end
    assert_equal( err_caught, true )

    err_caught = false
    begin 
      # invalid number of arguments
      assert_equal( b.bx( 1, 5, 7 ), "xz" )
    rescue
      err_caught = true
    end
    assert_equal( err_caught, true )

    b.destroy
    a.destroy

  end

  def test_24

    n0 = 0
    n1 = 0
    n2 = "" 

    # Events
    e = RBA::E.new
    e.m = 100

    e.s1 # no event installed
    assert_equal( 0, n0 )
    e.s2
    assert_equal( 0, n1 )
    e.s3
    assert_equal( "", n2 )

    assert_equal( 100, e.m )
    e.e0 { n0 += 1 }
    e.e1 { |x| n1 += x.m }
    e.e2 { |i,s| n2 += i.to_s + s; } 

    e.s1
    assert_equal( 1, n0 )
    e.s1
    assert_equal( 2, n0 )

    # using lambda
    n0 = 0
    p = lambda { n0 += 2 }
    e.e0(&p)
    e.s1
    assert_equal( 2, n0 )

    # remove event handler -> no events triggered anymore
    n0 = 0
    e.e0 -= p
    e.s1
    assert_equal( 0, n0 )

    # adding again will re-activate it
    e.e0 += p
    n0 = 0
    e.s1
    assert_equal( 2, n0 )

    # two events at once
    pp = lambda { n0 += 10 }
    n0 = 0
    e.e0 += pp
    e.s1
    assert_equal( 12, n0 )

    # clearing events
    e.e0.clear
    e.s1
    n0 = 0
    assert_equal( 0, n0 )

    # synonyms: add, connect
    e.e0.add(p)
    e.e0.connect(pp)
    n0 = 0
    e.s1
    assert_equal( 12, n0 )

    # synonyms: remove, disconnect
    e.e0.disconnect(p)
    n0 = 0
    e.s1
    assert_equal( 10, n0 )
    n0 = 0
    e.e0.remove(pp)
    e.s1
    assert_equal( 0, n0 )

    # another signal
    e.s2
    assert_equal( 100, n1 )
    e.m = 1
    e.s2
    assert_equal( 101, n1 )

    e.s3
    assert_equal( "18hallo", n2 )
    e.s3
    assert_equal( "18hallo18hallo", n2 )

    if false

      # currently, exceptions are not thrown into events

      e.e0 { raise "X" }
      error_caught = false
      begin 
        e.s1
      rescue
        error_caught = true
      end
      assert_equal( error_caught, true )

    end

    if false

      # currently, return values are not available for events

      e.e0r { 5; }
      assert_equal( e.s1r("x"), 5 )
      e.e0r { |s| s.length + 2; }
      assert_equal( e.s1r("x"), 3 )
      assert_equal( e.s1r("abcxyz"), 8 )

    end

  end

  def test_25

    # destruction of an instance via c++
    GC.start
    ic0 = RBA::A.instance_count 

    a = RBA::A::new
    a.create
    assert_equal(a.destroyed?, false)
    assert_equal(RBA::A.instance_count, ic0 + 1)
    RBA::A.a20(a)    # install static instance of A
    assert_equal(a.destroyed?, false)
    RBA::A.a20(nil) 
    assert_equal(RBA::A.instance_count, ic0)
    assert_equal(a.destroyed?, true)

    a = RBA::A::new
    a.create
    assert_equal(a.destroyed?, false)
    assert_equal(RBA::A.instance_count, ic0 + 1)
    RBA::A.a20(a)    # install static instance of A
    assert_equal(a.destroyed?, false)
    assert_equal(RBA::A.instance_count, ic0 + 1)
    RBA::A.a20(a)    # re-install static instance of A
    assert_equal(a.destroyed?, false)
    assert_equal(RBA::A.instance_count, ic0 + 1)
    
    # install another instance
    aa = RBA::A::new
    aa.create
    assert_equal(aa.destroyed?, false)
    assert_equal(RBA::A.instance_count, ic0 + 2)
    RBA::A.a20(aa)    # install static instance of A

    # original one is destroyed now, only new instance remains
    assert_equal(a.destroyed?, true)
    assert_equal(aa.destroyed?, false)
    assert_equal(RBA::A.instance_count, ic0 + 1)
    RBA::A.a20(nil)    # discard installed instance
    assert_equal(aa.destroyed, true)
    assert_equal(RBA::A.instance_count, ic0)

    # the same without create .. should work too, but not create an instance because of late 
    # instantiation in default ctor
    a = RBA::A::new
    assert_equal(a.destroyed?, false)
    assert_equal(RBA::A.instance_count, ic0)
    RBA::A.a20(a)    # install static instance of A
    assert_equal(a.destroyed?, false)
    RBA::A.a20(nil) 
    assert_equal(RBA::A.instance_count, ic0)
    assert_equal(a.destroyed?, true)

  end

  def test_26

    # destruction of an instance via c++
    GC.start
    ic0 = RBA::A.instance_count 

    1.times do
      a = RBA::A::new
      a._create
      assert_equal(a.destroyed?, false)
      assert_equal(RBA::A.instance_count, ic0 + 1)
      RBA::A.a20(a)    # install static instance of A
      assert_equal(RBA::A.a20_get == nil, false)
    end

    # makes sure the objects inside the block before are deleted
    GC.start
    GC.start  # 2.0.0 needs a second one

    assert_equal(RBA::A.instance_count, ic0)
    assert_equal(RBA::A.a20_get == nil, true)

    # "unmanage" will freeze the object and not make it destroyed by the GC
    1.times do
      a = RBA::A::new
      a._create
      assert_equal(a.destroyed?, false)
      assert_equal(RBA::A.instance_count, ic0 + 1)
      RBA::A.a20(a)    # install static instance of A
      assert_equal(RBA::A.a20_get == nil, false)
      a._unmanage
    end

    # makes sure the objects inside the block before are deleted
    GC.start

    assert_equal(RBA::A.instance_count, ic0 + 1)
    assert_equal(RBA::A.a20_get == nil, false)

    # after "manage" the object gets volatile again
    a = RBA::A.a20_get
    a._manage

    # Looks like Ruby is keeping the last A instance in some kind of cache:
    # this will release it
    a = RBA::A.new
    a._destroy
    a = nil

    # makes sure the objects inside the block before are deleted
    GC.start

    assert_equal(RBA::A.instance_count, ic0)
    assert_equal(RBA::A.a20_get == nil, true)

  end

  def test_27

    # destruction of raw instances (non-gsi-enabled) via c++
    GC.start

    assert_equal(RBA::B.inst == nil, true)
    assert_equal(RBA::B.has_inst, false)

    1.times do
      b = RBA::B::new
      RBA::B.set_inst(b)
      assert_equal(RBA::B.has_inst, true)
      assert_equal(RBA::B.inst == b, false)
      assert_equal(RBA::B.inst.addr, b.addr)
    end

    GC.start
    assert_equal(RBA::B.has_inst, false)

    1.times do 
      b = RBA::B::new
      RBA::B.set_inst(b)
      b._unmanage
      $b_addr = b.addr
      assert_equal(RBA::B.has_inst, true)
      assert_equal(RBA::B.inst == b, false)
      assert_equal(RBA::B.inst.addr, b.addr)
    end

    GC.start
    assert_equal(RBA::B.has_inst, true)
    assert_equal(RBA::B.inst.addr, $b_addr)

    # Make it managed again
    1.times do
      # NOTE: putting this inside the block makes sure the 
      # GC will really delete the temporary objects *now* we create
      # inside this expression
      RBA::B.inst._manage
    end

    GC.start
    assert_equal(RBA::B.has_inst, false)

  end

  def test_29
    
    # copy/ref semantics on return

    c = RBA::C::new
    
    cd = RBA::CopyDetector::new(42)

    cd2 = c.pass_cd_direct(cd)
    assert_equal(cd2.x, 42)
    # two copies: one for return statement and then one for the new object
    assert_equal(cd2.xx, 44)
    
    cd2 = c.pass_cd_cref(cd)
    assert_equal(cd2.x, 42)
    assert_equal(cd2.xx, 43)
    
    cd2 = c.pass_cd_cref_as_copy(cd)
    assert_equal(cd2.x, 42)
    assert_equal(cd2.xx, 43)
    
    cd2 = c.pass_cd_cref_as_ref(cd)
    assert_equal(cd2.x, 42)
    assert_equal(cd2.xx, 42)
    
    cd2 = c.pass_cd_cptr(cd)
    assert_equal(cd2.x, 42)
    assert_equal(cd2.xx, 42)
    
    cd2 = c.pass_cd_cptr_as_copy(cd)
    assert_equal(cd2.x, 42)
    assert_equal(cd2.xx, 43)
    
    cd2 = c.pass_cd_cptr_as_ref(cd)
    assert_equal(cd2.x, 42)
    assert_equal(cd2.xx, 42)
    
    cd2 = c.pass_cd_ptr(cd)
    assert_equal(cd2.x, 42)
    assert_equal(cd2.xx, 42)
    
    cd2 = c.pass_cd_ptr_as_copy(cd)
    assert_equal(cd2.x, 42)
    assert_equal(cd2.xx, 43)
    
    cd2 = c.pass_cd_ptr_as_ref(cd)
    assert_equal(cd2.x, 42)
    assert_equal(cd2.xx, 42)
    
    cd2 = c.pass_cd_ref(cd)
    assert_equal(cd2.x, 42)
    assert_equal(cd2.xx, 42)
    
    cd2 = c.pass_cd_ref_as_copy(cd)
    assert_equal(cd2.x, 42)
    assert_equal(cd2.xx, 43)
    
    cd2 = c.pass_cd_ref_as_ref(cd)
    assert_equal(cd2.x, 42)
    assert_equal(cd2.xx, 42)

  end

  def test_30

    # some basic tests for the *Value boxing classes

    val = RBA::Value::new
    assert_equal(val.to_s, "nil")
    assert_equal(val.value, nil)
    val.value = 17.5
    assert_equal(val.value, 17.5)
    assert_equal(val.to_s, "17.5")
    val.value += 1
    assert_equal(val.to_s, "18.5")
    val = RBA::Value::new(5)
    assert_equal(val.value, 5)
    val.value = nil
    assert_equal(val.value, nil)
    assert_equal(val.to_s, "nil")

    val = RBA::Value::new
    assert_equal(val.to_s, "nil")
    assert_equal(val.value, nil)
    val.value = 17.5
    assert_equal(val.value, 17.5)
    assert_equal(val.to_s, "17.5")
    val.value += 1
    assert_equal(val.to_s, "18.5")
    val = RBA::Value::new(5)
    assert_equal(val.value, 5)
    val.value = nil
    assert_equal(val.value, nil)
    assert_equal(val.to_s, "nil")

    val = RBA::Value::new
    assert_equal(val.to_s, "nil")
    assert_equal(val.value, nil)
    val.value = true
    assert_equal(val.value, true)
    assert_equal(val.to_s, "true")
    val = RBA::Value::new(true)
    assert_equal(val.value, true)
    val.value = nil
    assert_equal(val.value, nil)
    assert_equal(val.to_s, "nil")

    val = RBA::Value::new
    assert_equal(val.to_s, "nil")
    assert_equal(val.value, nil)
    val.value = 17
    assert_equal(val.value, 17)
    assert_equal(val.to_s, "17")
    val.value += 1
    assert_equal(val.to_s, "18")
    val = RBA::Value::new(5)
    assert_equal(val.value, 5)
    val.value = nil
    assert_equal(val.value, nil)
    assert_equal(val.to_s, "nil")

    val = RBA::Value::new
    assert_equal(val.to_s, "nil")
    assert_equal(val.value, nil)
    val.value = 17
    assert_equal(val.value, 17)
    assert_equal(val.to_s, "17")
    val.value += 1
    assert_equal(val.to_s, "18")
    val = RBA::Value::new(5)
    assert_equal(val.value, 5)
    val.value = nil
    assert_equal(val.value, nil)
    assert_equal(val.to_s, "nil")

    val = RBA::Value::new
    assert_equal(val.to_s, "nil")
    assert_equal(val.value, nil)
    val.value = 17
    assert_equal(val.value, 17)
    assert_equal(val.to_s, "17")
    val.value += 1
    assert_equal(val.to_s, "18")
    val = RBA::Value::new(5)
    assert_equal(val.value, 5)
    val.value = nil
    assert_equal(val.value, nil)
    assert_equal(val.to_s, "nil")

    val = RBA::Value::new
    assert_equal(val.to_s, "nil")
    assert_equal(val.value, nil)
    val.value = 2700000000
    assert_equal(val.value, 2700000000)
    assert_equal(val.to_s, "2700000000")
    val.value += 1
    assert_equal(val.to_s, "2700000001")
    val = RBA::Value::new(500000000)
    assert_equal(val.value, 500000000)
    val.value = nil
    assert_equal(val.value, nil)
    assert_equal(val.to_s, "nil")

    val = RBA::Value::new
    assert_equal(val.to_s, "nil")
    assert_equal(val.value, nil)
    val.value = 170000000000
    assert_equal(val.to_s, "170000000000")
    assert_equal(val.value, 170000000000)
    val.value += 1
    assert_equal(val.to_s, "170000000001")
    val = RBA::Value::new(50000000000)
    assert_equal(val.value, 50000000000)
    val.value = nil
    assert_equal(val.value, nil)
    assert_equal(val.to_s, "nil")

    val = RBA::Value::new
    assert_equal(val.to_s, "nil")
    assert_equal(val.value, nil)
    val.value = 170000000000
    assert_equal(val.value, 170000000000)
    assert_equal(val.to_s, "170000000000")
    val.value += 1
    assert_equal(val.to_s, "170000000001")
    val = RBA::Value::new(50000000000)
    assert_equal(val.value, 50000000000)
    val.value = nil
    assert_equal(val.value, nil)
    assert_equal(val.to_s, "nil")

    val = RBA::Value::new
    assert_equal(val.value, nil)
    assert_equal(val.to_s, "nil")
    val.value = "abc"
    assert_equal(val.value, "abc")
    assert_equal(val.to_s, "abc")
    val.value += "x"
    assert_equal(val.to_s, "abcx")
    val = RBA::Value::new("uv")
    assert_equal(val.value, "uv")
    val.value = nil
    assert_equal(val.value, nil)
    assert_equal(val.to_s, "nil")

  end

  def test_31

    # some basic tests with derived and base classes

    RBA::X.init
    RBA::Y.init
    x = RBA::X.new("hallo")
    assert_equal(true, x.is_a?(RBA::X))
    assert_equal(false, x.is_a?(RBA::A))
    assert_equal(false, x.is_a?(RBA::Y))
    assert_equal("hallo", x.s)
    assert_equal("X", x.cls_name)
    cxp = RBA::X.x_cptr
    assert_equal("X::a", cxp.s)
    begin 
      cxp.s = "x"
      error_caught = false
    rescue
      error_caught = true
    end
    assert_equal(true, error_caught)
    xp = RBA::X.x_ptr
    assert_equal("X::a", xp.s)
    xp.s = "x"
    assert_equal("x", xp.s)

    y = RBA::Y.new("hallo")
    assert_equal(true, y.is_a?(RBA::X))
    assert_equal(false, y.is_a?(RBA::A))
    assert_equal(true, y.is_a?(RBA::Y))
    assert_equal("hallo", y.s)
    assert_equal("Y", y.cls_name)
    assert_equal(5, y.i)
    cyp = RBA::Y.y_cptr
    assert_equal("Y::a", cyp.s)
    assert_equal("Y", cyp.cls_name)
    begin 
      cyp.s = "y"
      error_caught = false
    rescue
      error_caught = true
    end
    assert_equal(true, error_caught)
    yp = RBA::Y.y_ptr
    assert_equal("Y", yp.cls_name)
    assert_equal("Y::a", yp.s)
    yp.s = "y"
    assert_equal("y", yp.s)
    assert_equal(1, yp.i)
    yp.s = "abc"
    assert_equal(3, yp.i)
    assert_equal("RBA::Y", yp.class.to_s)

  end

  def test_32

    # run test only if we have Qt bindings 
    if !RBA.constants.find { |x| x == :QStringPair }
      return
    end

    # QPair<String, String>
    p = RBA::QStringPair.new
    p.first = "a"
    p.second = "b"
    assert_equal("a", p.first)
    assert_equal("b", p.second)
    pp = p.dup
    assert_equal("a", pp.first)
    assert_equal("b", pp.second)
    pp.first = "u"
    assert_equal("a", p.first)
    assert_equal("b", p.second)
    assert_equal("u", pp.first)
    assert_equal("b", pp.second)
    assert_equal(pp == p, false)
    assert_equal(pp != p, true)
    pp = RBA::QStringPair.new("a", "b")
    assert_equal("a", pp.first)
    assert_equal("b", pp.second)
    assert_equal(pp == p, true)
    assert_equal(pp != p, false)

  end

  def test_33

    # run test only if we have Qt bindings 
    if !RBA.constants.find { |x| x == :QByteArrayPair }
      return
    end

    # QPair<QByteArray, QByteArray>
    p = RBA::QByteArrayPair.new
    p.first = "a"
    p.second = "b"
    assert_equal("a", p.first)
    assert_equal("b", p.second)
    pp = p.dup
    assert_equal("a", pp.first)
    assert_equal("b", pp.second)
    pp.first = "u"
    assert_equal("a", p.first)
    assert_equal("b", p.second)
    assert_equal("u", pp.first)
    assert_equal("b", pp.second)
    assert_equal(pp == p, false)
    assert_equal(pp != p, true)
    pp = RBA::QByteArrayPair.new("a", "b")
    assert_equal("a", pp.first)
    assert_equal("b", pp.second)
    assert_equal(pp == p, true)
    assert_equal(pp != p, false)

  end

  def test_34

    # run test only if we have Qt bindings 
    if !RBA.constants.find { |x| x == :QDialog }
      return
    end

    # QDialog and QWidget
    # Hint: QApplication creates some leaks (FT, GTK). Hence it must not be used in the leak_check case ..
    if !$leak_check 

      app_inst = RBA::QCoreApplication.instance
      assert_equal("RBA::Application", app_inst.class.to_s)

      qd = RBA::QDialog.new
      RBA::QApplication.setActiveWindow(qd)
      assert_equal(RBA::QApplication.activeWindow.inspect, qd.inspect)
      assert_equal("RBA::QDialog", RBA::QApplication.activeWindow.class.to_s)
      qd._destroy
      assert_equal(RBA::QApplication.activeWindow.inspect, "nil")

      qd = RBA::QDialog.new
      RBA::QApplication.setActiveWindow(qd)
      assert_equal(RBA::QApplication.activeWindow.inspect, qd.inspect)
      assert_equal("RBA::QDialog", RBA::QApplication.activeWindow.class.to_s)
      qd._destroy
      assert_equal(RBA::QApplication.activeWindow.inspect, "nil")

    end

  end

  def test_35

    # vectors of pointers

    RBA::X.init
    RBA::Y.init

    vx_cptr = RBA::X.vx_cptr
    assert_equal(2, vx_cptr.size)

    begin 
      vx_cptr[0].s = "y"
      error_caught = false
    rescue
      error_caught = true
    end
    assert_equal(true, error_caught)

    vx = RBA::X.vx
    assert_equal(2, vx.size)
    assert_equal("X::a", vx[0].s)
    assert_equal("X::b", vx[1].s)

    vx_ptr = RBA::X.vx_ptr
    assert_equal(2, vx_ptr.size)
    assert_equal("RBA::X", vx_ptr[0].class.to_s)
    assert_equal("RBA::X", vx_ptr[1].class.to_s)

    vx_ptr[0].s = "u"
    assert_equal("u", vx_cptr[0].s)
    assert_equal("X::a", vx[0].s)
    assert_equal("X::b", vx[1].s)

    vy0_ptr = RBA::Y.vy0_ptr
    assert_equal(1, vy0_ptr.size)
    assert_equal("nil", vy0_ptr[0].inspect)

    vy_cptr = RBA::Y.vy_cptr
    assert_equal(2, vy_cptr.size)

    begin 
      vy_cptr[0].s = "y"
      error_caught = false
    rescue
      error_caught = true
    end
    assert_equal(true, error_caught)

    vy_cptr = RBA::Y.vyasx_cptr
    assert_equal(2, vy_cptr.size)

    begin 
      vy_cptr[0].s = "y"
      error_caught = false
    rescue
      error_caught = true
    end
    assert_equal(true, error_caught)

    vy_ptr = RBA::Y.vy_ptr
    assert_equal(2, vy_ptr.size)
    assert_equal("RBA::Y", vy_ptr[0].class.to_s)
    assert_equal("RBA::Y", vy_ptr[1].class.to_s)

    vy_ptr[0].s = "uvw"
    assert_equal("uvw", vy_cptr[0].s)
    assert_equal(3, vy_cptr[0].i)

    vy_ptr = RBA::Y.vyasx_ptr
    assert_equal(2, vy_ptr.size)
    assert_equal("RBA::Y", vy_ptr[0].class.to_s)
    assert_equal("RBA::Y", vy_ptr[1].class.to_s)

    vy_ptr[0].s = "uvw"
    assert_equal("uvw", vy_cptr[0].s)
    assert_equal(3, vy_cptr[0].i)

    y = RBA::Y.new("")
    yc = y.vx_dyn_count
    y.vx_dyn_make
    assert_equal(yc + 1, y.vx_dyn_count)
    y.vx_dyn_destroy
    assert_equal(yc, y.vx_dyn_count)

    y.vx_dyn_make
    assert_equal(yc + 1, y.vx_dyn_count)
    yy = y.vx_dyn
    assert_equal(1, yy.size)
    assert_equal("RBA::Y", yy[0].class.to_s)
    assert_equal(true, yy[0] != nil)
    yy[0]._destroy
    assert_equal(true, yy[0].destroyed?)
    assert_equal(yc, y.vx_dyn_count)

    y.vx_dyn_make
    assert_equal(yc + 1, y.vx_dyn_count)
    yy = y.vx_dyn
    assert_equal(1, yy.size)
    assert_equal("RBA::Y", yy[0].class.to_s)
    assert_equal(true, yy[0] != nil)
    y.vx_dyn_destroy
    assert_equal(true, yy[0].destroyed?)
    assert_equal(yc, y.vx_dyn_count)

  end

  def test_36

    x = XEdge.new
    assert_equal("XEdge", x.class.inspect)
    assert_equal("(1,2;3,4)", x.to_s)

  end

  def test_37

    # protected methods
    ok = false
    a = RBA::A.new
    e = ""
    begin
      a.a10_prot # cannot be called - is protected
      ok = true
    rescue => ex
      e = ex.to_s
    end
    assert_equal(e =~ /^protected method `a10_prot' called/, 0)
    assert_equal(ok, false)
    assert_equal(a.call_a10_prot(1.25), "1.25")

  end

  def test_38

    # mixed const / non-const reference and events
    ec = RBA::E.ic
    assert_equal(ec.is_const_object?, true)
    enc = RBA::E.inc
    # Now, ec has turned into a non-const reference as well!
    # This is strange but is a consequence of the unique C++/Ruby binding and there can 
    # only be a non-const or const ruby object!
    assert_equal(ec.is_const_object?, false)
    assert_equal(enc.is_const_object?, false)

    # the "true reference" is a not copy since E is derived from ObjectBase
    ec.x = 15
    assert_equal(ec.x, 15);
    ec2 = RBA::E.ic
    assert_equal(ec2.x, 15);
    ec2 = RBA::E.icref
    assert_equal(ec2.x, 15);
    ec2.x = 17
    assert_equal(ec2.x, 17);
    assert_equal(ec.x, 17);
    assert_equal(ec2.is_const_object?, false) # because it's a copy

    # the "true reference" is a not copy since E is derived from ObjectBase
    enc2 = RBA::E.incref
    assert_equal(enc2.x, 17);
    enc2.x = 19
    assert_equal(enc2.x, 19);
    assert_equal(ec.x, 19);  # because the non-const reference by incref is not a copy

  end

  def test_39

    # mixed const / non-const reference and events
    fc = RBA::F.ic
    assert_equal(fc.is_const_object?, true)
    fnc = RBA::F.inc
    # In contrase to E, the fc reference is not touched because F is not derived
    # from ObjectBase
    assert_equal(fc.is_const_object?, true)
    assert_equal(fnc.is_const_object?, false)

    # the "true reference" is a copy
    fnc.x = 15
    assert_equal(fc.x, 15);
    fc2 = RBA::F.ic
    assert_equal(fc2.x, 15);
    fc2 = RBA::F.icref
    assert_equal(fc2.x, 15);
    fc2.x = 17
    assert_equal(fc2.x, 17);
    assert_equal(fc.x, 15);
    assert_equal(fc2.is_const_object?, false) # because it's a copy

    # the "true reference" is a copy
    fnc2 = RBA::F.incref
    assert_equal(fnc2.x, 15);
    fnc2.x = 19
    assert_equal(fnc2.x, 19);
    assert_equal(fc.x, 19);  # because the non-const reference by incref is not a copy

  end

  def test_40

    # optional arguments
    g = RBA::G.new

    assert_equal(g.iv, 0)
    g.set_iva(2)
    assert_equal(g.iv, 2)
    g.set_ivb(3)
    assert_equal(g.iv, 3)
    g.set_ivb
    assert_equal(g.iv, 1)
    g.set_sv1a("hello")
    assert_equal(g.sv, "hello")

    failed = false
    begin
      g.set_sv1a
    rescue 
      failed = true
    end
    assert_equal(failed, true)

    g.set_sv1b("world")
    assert_equal(g.sv, "world")
    g.set_sv1b
    assert_equal(g.sv, "value")
    g.set_sv2a("hello")
    assert_equal(g.sv, "hello")

    failed = false
    begin
      g.set_sv2a
    rescue 
      failed = true
    end
    assert_equal(failed, true)

    g.set_sv2b("world")
    assert_equal(g.sv, "world")
    g.set_sv2b
    assert_equal(g.sv, "value")

    g.set_vva(17, "c")
    assert_equal(g.iv, 17)
    assert_equal(g.sv, "c")

    failed = false
    begin
      g.set_svva
    rescue 
      failed = true
    end
    assert_equal(failed, true)

    failed = false
    begin
      g.set_svva(11)
    rescue 
      failed = true
    end
    assert_equal(failed, true)

    g.set_vvb(11)
    assert_equal(g.iv, 11)
    assert_equal(g.sv, "value")
    g.set_vvb(10, "nil")
    assert_equal(g.iv, 10)
    assert_equal(g.sv, "nil")

    failed = false
    begin
      g.set_svvb
    rescue 
      failed = true
    end
    assert_equal(failed, true)

    g.set_vvc(11)
    assert_equal(g.iv, 11)
    assert_equal(g.sv, "value")
    g.set_vvc
    assert_equal(g.iv, 1)
    assert_equal(g.sv, "value")
    g.set_vvc(17, "nil")
    assert_equal(g.iv, 17)
    assert_equal(g.sv, "nil")

  end

  def test_41

    # maps 
    b = RBA::B.new

    b.insert_map1(1, "hello")
    assert_equal(b.map1.inspect, "{1=>\"hello\"}")

    b.map1 = {}
    b.insert_map1(2, "hello")
    assert_equal(b.map1_cref.inspect, "{2=>\"hello\"}")

    b.map1 = {}
    b.insert_map1(3, "hello")
    assert_equal(b.map1_cptr.inspect, "{3=>\"hello\"}")

    b.map1 = {}
    b.insert_map1(4, "hello")
    assert_equal(b.map1_ref.inspect, "{4=>\"hello\"}")

    b.map1 = {}
    b.insert_map1(5, "hello")
    assert_equal(b.map1_ptr.inspect, "{5=>\"hello\"}")

    assert_equal(b.map1_cptr_null == nil, true);
    assert_equal(b.map1_ptr_null == nil, true);

    b.map1 = { 42 => 1, -17 => true }
    assert_equal(b.map1.inspect, "{-17=>\"true\", 42=>\"1\"}")

    b.map1 = { 42 => "1", -17 => "true" }
    assert_equal(b.map1.inspect, "{-17=>\"true\", 42=>\"1\"}")

    b.map1 = {}
    b.set_map1_cref({ 42 => "2", -17 => "true" })
    assert_equal(b.map1.inspect, "{-17=>\"true\", 42=>\"2\"}")

    b.map1 = {}
    b.set_map1_cptr({ 42 => "3", -17 => "true" })
    assert_equal(b.map1.inspect, "{-17=>\"true\", 42=>\"3\"}")

    b.map1 = {}
    b.set_map1_cptr(nil)
    assert_equal(b.map1.inspect, "{}")

    b.map1 = {}
    b.set_map1_cptr(nil)
    assert_equal(b.map1.inspect, "{}")

    b.map1 = {}
    b.set_map1_ref({ 42 => "4", -17 => "true" })
    assert_equal(b.map1.inspect, "{-17=>\"true\", 42=>\"4\"}")

    b.map1 = {}
    b.set_map1_ptr({ 42 => "5", -17 => "true" })
    assert_equal(b.map1.inspect, "{-17=>\"true\", 42=>\"5\"}")

    b.map1 = {}
    b.set_map1_ptr(nil)
    assert_equal(b.map1.inspect, "{}")

    b.map2 = {}
    b.map2 = { 'xy' => 1, -17 => true }
    assert_equal(b.map2.inspect, "{-17=>true, \"xy\"=>1}")

    assert_equal(b.map2_null == nil, true)

  end

  class Z_IMP1 < RBA::Z 
    def f(x)
      return x.cls_name()
    end
  end

  class Z_IMP2 < RBA::Z 
    def f(x)
      return x.class.to_s()
    end
  end

  const_defined?(:Z_IMP3) && remove_const(:Z_IMP3)
  class Z_IMP3 < RBA::Z 
    alias f_org f
    def f(x)
      return self.f_org(x) + "*"
    end
  end

  def test_42

    # virtual functions and sub-classes 
    z = RBA::Z.new
    assert_equal(z.f(nil), "(nil)")
    assert_equal(z.f(RBA::X.new("hello")), "hello")

    z1 = Z_IMP1::new
    assert_equal(z1.f(RBA::X.new("a")), "X")
    assert_equal(z1.f(RBA::Y.new("b")), "Y")
    assert_equal(z1.f_with_x("a"), "X")
    assert_equal(z1.f_with_y("b"), "Y")
    assert_equal(z1.f_with_yy("b"), "YY")

    z2 = Z_IMP2::new
    assert_equal(z2.f(RBA::X.new("1")), "RBA::X")
    assert_equal(z2.f(RBA::Y.new("2")), "RBA::Y")
    assert_equal(z2.f_with_x("1"), "RBA::X")
    assert_equal(z2.f_with_y("2"), "RBA::Y")
    assert_equal(z2.f_with_yy("3"), "RBA::Y")

    z1 = Z_IMP3::new
    assert_equal(z1.f(RBA::X.new("x")), "x*")
    assert_equal(z1.f(RBA::Y.new("y")), "y*")
    assert_equal(z1.f_with_x("x"), "x*")
    assert_equal(z1.f_with_y("y"), "y*")
    assert_equal(z1.f_with_yy("yy"), "yy*")

  end

  def test_50
  
    # advanced containers and out parameters

    b = RBA::B::new
    a1 = RBA::A::new
    a1.s 42
    a1.a5 11
    a2 = RBA::A::new
    a2.s 17
    a2.a5 22
    a3 = RBA::A::new
    a3.s 33
    a3.a5 33

    b.set_map_iaptr( { 1 => a1, 2 => a2 } )
    assert_equal(b.map_iaptr.inspect, "{1=>42, 2=>17}")
    assert_equal(b.map_iaptr_cref.inspect, "{1=>42, 2=>17}")
    assert_equal(b.map_iaptr_ref.inspect, "{1=>42, 2=>17}")
    assert_equal(b.map_iaptr_cptr.inspect, "{1=>42, 2=>17}")
    assert_equal(b.map_iaptr_ptr.inspect, "{1=>42, 2=>17}")
    b.set_map_iaptr_cptr(nil)
    assert_equal(b.map_iaptr.inspect, "{}")
    b.set_map_iaptr_cptr( { 17 => a2, 42 => a1 } )
    assert_equal(b.map_iaptr.inspect, "{17=>17, 42=>42}")
    b.set_map_iaptr_ptr( { 18 => a2, 43 => a1 } )
    assert_equal(b.map_iaptr.inspect, "{18=>17, 43=>42}")
    b.set_map_iaptr_ref( { 1 => a2, 3 => a1 } )
    assert_equal(b.map_iaptr.inspect, "{1=>17, 3=>42}")
    b.set_map_iaptr_cref( { 2 => a2, 4 => a1 } )
    assert_equal(b.map_iaptr.inspect, "{2=>17, 4=>42}")
    b.set_map_iaptr_ptr( { } )
    assert_equal(b.map_iaptr.inspect, "{}")
    b.set_map_iaptr_cref( { 2 => a2, 4 => a1 } )
    assert_equal(b.map_iaptr.inspect, "{2=>17, 4=>42}")
    b.set_map_iaptr_ptr(nil)
    assert_equal(b.map_iaptr.inspect, "{}")

    m = { 2 => a1, 4 => a2 }
    # map as an "out" parameter:
    RBA::B.insert_map_iaptr(m, 3, a3)
    assert_equal(m.inspect, "{2=>42, 3=>33, 4=>17}")

    b.set_map_iacptr( { 1 => a1, 2 => a2 } )
    assert_equal(b.map_iacptr.inspect, "{1=>42, 2=>17}")
    m = { 2 => a1, 4 => a2 }
    # map as an "out" parameter:
    RBA::B.insert_map_iacptr(m, 5, a3)
    assert_equal(m.inspect, "{2=>42, 4=>17, 5=>33}")

    b.set_map_ia( { 1 => a1, 2 => a2 } )
    # because we have raw copies, the Ruby-add-on is lost and
    # only a1 (built-in) remains as an attribute:
    assert_equal(b.map_ia.inspect, "{1=>a1=11, 2=>a1=22}")
    m = { 2 => a1, 4 => a2 }
    # map as an "out" parameter:
    RBA::B.insert_map_ia(m, 5, a3)
    assert_equal(m.inspect, "{2=>a1=11, 4=>a1=22, 5=>a1=33}")

    b.set_map_iav( { 1 => [ a1, a2 ], 2 => [] } )
    # because we have raw copies, the Ruby-add-on is lost and
    # only a1 (built-in) remains as an attribute:
    assert_equal(b.map_iav.inspect, "{1=>[a1=11, a1=22], 2=>[]}")
    m = { 1 => [ a1, a2 ], 2 => [] }
    # map as an "out" parameter:
    RBA::B.push_map_iav(m, 2, a3)
    assert_equal(m.inspect, "{1=>[a1=11, a1=22], 2=>[a1=33]}")
    RBA::B.insert_map_iav(m, 5, [ a1, a3 ])
    assert_equal(m.inspect, "{1=>[a1=11, a1=22], 2=>[a1=33], 5=>[a1=11, a1=33]}")

    v = [ [ "a", "aa" ], [] ]
    RBA::B.push_vvs( v, [ "1", "2" ] )
    assert_equal(v.inspect, "[[\"a\", \"aa\"], [], [\"1\", \"2\"]]")
    b.set_vvs( [ [ "1" ], [ "2", "3" ] ] )
    assert_equal(b.vvs, [["1"], ["2", "3"]])
    assert_equal(b.vvs_ref, [["1"], ["2", "3"]])
    assert_equal(b.vvs_cref, [["1"], ["2", "3"]])
    assert_equal(b.vvs_ptr, [["1"], ["2", "3"]])
    assert_equal(b.vvs_cptr, [["1"], ["2", "3"]])
    b.set_vvs_ref( [ [ "1" ], [ "2", "3" ] ] )
    assert_equal(b.vvs, [["1"], ["2", "3"]])
    b.set_vvs_cref( [ [ "2" ], [ "1", "3" ] ] )
    assert_equal(b.vvs, [["2"], ["1", "3"]])
    b.set_vvs_ptr( [ [ "1" ], [ "3", "2" ] ] )
    assert_equal(b.vvs, [["1"], ["3", "2"]])
    b.set_vvs_ptr(nil)
    assert_equal(b.vvs, [])
    b.set_vvs_cptr( [ [ "0" ], [ "3", "2" ] ] )
    assert_equal(b.vvs, [["0"], ["3", "2"]])
    b.set_vvs_cptr(nil)
    assert_equal(b.vvs, [])

    v = [ "a", "b" ]
    RBA::B::push_ls(v, "x")
    assert_equal(v, [ "a", "b", "x" ])
    b.set_ls([ "1" ])
    assert_equal(b.ls, [ "1" ])
    b.set_ls([])
    assert_equal(b.ls, [])

    v = [ "a", "b" ]
    RBA::B::push_ss(v, "x")
    assert_equal(v, [ "a", "b", "x" ])
    b.set_ss([ "1" ])
    assert_equal(b.ss, [ "1" ])
    b.set_ss([])
    assert_equal(b.ss, [])

    if b.respond_to?(:set_qls) 

      v = [ "a", "b" ]
      RBA::B::push_qls(v, "x")
      assert_equal(v, [ "a", "b", "x" ])
      b.set_qls([ "1" ])
      assert_equal(b.qls, [ "1" ])
      b.set_qls([])
      assert_equal(b.qls, [])

      v = [ "a", 1 ]
      RBA::B::push_qlv(v, 2.5)
      assert_equal(v, [ "a", 1, 2.5 ])
      b.set_qlv([ 17, "1" ])
      assert_equal(b.qlv, [ 17, "1" ])
      b.set_qlv([])
      assert_equal(b.qlv, [])

      v = [ "a", "b" ]
      RBA::B::push_qsl(v, "x")
      assert_equal(v, [ "a", "b", "x" ])
      b.set_qsl([ "1" ])
      assert_equal(b.qsl, [ "1" ])
      b.set_qsl([])
      assert_equal(b.qsl, [])

      v = [ "a", "b" ]
      RBA::B::push_qvs(v, "x")
      assert_equal(v, [ "a", "b", "x" ])
      b.set_qvs([ "1" ])
      assert_equal(b.qvs, [ "1" ])
      b.set_qvs([])
      assert_equal(b.qvs, [])

      v = [ "a", "b" ]
      RBA::B::push_qss(v, "x")
      assert_equal(v.sort, [ "a", "b", "x" ])
      b.set_qss([ "1" ])
      assert_equal(b.qss, [ "1" ])
      b.set_qss([])
      assert_equal(b.qss, [])

      v = { 1 => "a", 17 => "b" }
      RBA::B::insert_qmap_is(v, 2, "x")
      assert_equal(v, { 1 => "a", 17 => "b", 2 => "x" })
      b.set_qmap_is({ 1 => "t", 17 => "b" })
      assert_equal(b.qmap_is, { 1 => "t", 17 => "b" })
      b.set_qmap_is({})
      assert_equal(b.qmap_is, {})

      v = { 1 => "a", 17 => "b" }
      RBA::B::insert_qhash_is(v, 2, "x")
      assert_equal(v, { 1 => "a", 17 => "b", 2 => "x" })
      b.set_qhash_is({ 1 => "t", 17 => "b" })
      assert_equal(b.qhash_is, { 1 => "t", 17 => "b" })
      b.set_qhash_is({})
      assert_equal(b.qhash_is, {})

    end

  end

  def test_51
  
    # new subclass and child class declarations
    y2 = RBA::Y2::new
    assert_equal(y2.x1, 2)
    assert_equal(y2.x2, 42)
    assert_equal(y2.is_a?(RBA::X), true)

    y3 = RBA::Z::Y3::new
    assert_equal(y3.x1, 3)
    assert_equal(y3.x2, 42)
    assert_equal(y3.is_a?(RBA::X), true)

    y4 = RBA::Z::Y4::new
    assert_equal(y4.x1, 4)
    assert_equal(y4.is_a?(RBA::X), false)

  end

  def test_60

    if !RBA.constants.member?(:SQ)
      return
    end
  
    sq = RBA::SQ::new

    got_s0 = false
    sq.s0 { got_s0 = true }
    sq.trigger_s0
    assert_equal(got_s0, true)

    got_s1 = nil
    sq.s1 { |iv| got_s1 = iv }
    sq.trigger_s1(17)
    assert_equal(got_s1, 17)
    sq.trigger_s1(42)
    assert_equal(got_s1, 42)

    got_s2_1 = nil
    got_s2_2 = nil
    sq.tag = 999
    sq.s2 { |is,obj| got_s2_1 = is; got_s2_2 = obj }
    sq.trigger_s2("foo")
    assert_equal(got_s2_1, "foo")
    assert_equal(got_s2_2.tag, 999)
    sq.tag = 111
    got_s2_2 = nil 
    sq.trigger_s2("bar")
    assert_equal(got_s2_1, "bar")
    assert_equal(got_s2_2.tag, 111)

    # clear handler (with clear)
    sq.s2.clear

    sq.tag = 0
    got_s2_1 = nil
    got_s2_2 = nil
    sq.trigger_s2("z")
    assert_equal(got_s2_1, nil)
    assert_equal(got_s2_2, nil)

    # attach again with set
    l = lambda { |is,obj| got_s2_1 = is; got_s2_2 = obj }
    sq.s2.set(l)

    sq.tag = 2222
    got_s2_1 = nil 
    got_s2_2 = nil 
    sq.trigger_s2("u")
    assert_equal(got_s2_1, "u")
    assert_equal(got_s2_2.tag, 2222)

    # clear handler (with remove)
    sq.s2 -= l

    sq.tag = 0
    got_s2_1 = nil 
    got_s2_2 = nil 
    sq.trigger_s2("v")
    assert_equal(got_s2_1, nil)
    assert_equal(got_s2_2, nil)

  end

  def test_61
  
    if !RBA.constants.member?(:SQ)
      return
    end
  
    sq = RBA::SQ::new

    got_s0a = 0
    got_s0b = 0
    p1 = lambda { got_s0a += 1 }
    p1b = lambda { got_s0a += 1 }
    p2 = lambda { got_s0b += 1 }
    sq.s0 = p1
    sq.trigger_s0
    assert_equal(got_s0a, 1)
    assert_equal(got_s0b, 0)

    got_s0a = 0
    got_s0b = 0
    sq.s0 = p2
    sq.trigger_s0
    assert_equal(got_s0a, 0)
    assert_equal(got_s0b, 1)

    got_s0a = 0
    got_s0b = 0
    sq.s0 += p1
    sq.trigger_s0
    assert_equal(got_s0a, 1)
    assert_equal(got_s0b, 1)

    got_s0a = 0
    got_s0b = 0 
    # same proc is not added again
    sq.s0 += p1
    sq.trigger_s0
    assert_equal(got_s0a, 1)
    assert_equal(got_s0b, 1)

    got_s0a = 0
    got_s0b = 0
    # second proc p1 with same effect
    sq.s0 += p1b
    sq.trigger_s0
    assert_equal(got_s0a, 2)
    assert_equal(got_s0b, 1)

    got_s0a = 0
    got_s0b = 0
    sq.s0 -= p1
    sq.trigger_s0
    assert_equal(got_s0a, 1)
    assert_equal(got_s0b, 1)

    got_s0a = 0
    got_s0b = 0
    sq.s0 -= p1b
    sq.trigger_s0
    assert_equal(got_s0a, 0)
    assert_equal(got_s0b, 1)

    got_s0a = 0
    got_s0b = 0
    sq.s0 -= p1
    sq.trigger_s0
    assert_equal(got_s0a, 0)
    assert_equal(got_s0b, 1)

    got_s0a = 0
    got_s0b = 0
    sq.s0 -= p2
    sq.trigger_s0
    assert_equal(got_s0a, 0)
    assert_equal(got_s0b, 0)

  end

  def test_70
  
    se = RBA::SE::new

    got_s0 = false
    se.s0 { got_s0 = true }
    se.trigger_s0
    assert_equal(got_s0, true)

    got_s1 = nil
    se.s1 { |iv| got_s1 = iv }
    se.trigger_s1(17)
    assert_equal(got_s1, 17)
    se.trigger_s1(42)
    assert_equal(got_s1, 42)

    got_s2_1 = nil
    got_s2_2 = nil
    se.tag = 999
    se.s2 { |is,obj| got_s2_1 = is; got_s2_2 = obj }
    se.trigger_s2("foo")
    assert_equal(got_s2_1, "foo")
    assert_equal(got_s2_2.tag, 999)
    se.tag = 111
    got_s2_2 = nil 
    se.trigger_s2("bar")
    assert_equal(got_s2_1, "bar")
    assert_equal(got_s2_2.tag, 111)

  end

  def test_71
  
    se = RBA::SE::new

    got_s0a = 0
    got_s0b = 0
    p1 = lambda { got_s0a += 1 }
    p1b = lambda { got_s0a += 1 }
    p2 = lambda { got_s0b += 1 }
    se.s0 = p1
    se.trigger_s0
    assert_equal(got_s0a, 1)
    assert_equal(got_s0b, 0)

    got_s0a = 0
    got_s0b = 0
    se.s0 = p2
    se.trigger_s0
    assert_equal(got_s0a, 0)
    assert_equal(got_s0b, 1)

    got_s0a = 0
    got_s0b = 0
    se.s0 += p1
    se.trigger_s0
    assert_equal(got_s0a, 1)
    assert_equal(got_s0b, 1)

    got_s0a = 0
    got_s0b = 0 
    # same proc is not added again
    se.s0 += p1
    se.trigger_s0
    assert_equal(got_s0a, 1)
    assert_equal(got_s0b, 1)

    got_s0a = 0
    got_s0b = 0
    # second proc p1 with same effect
    se.s0 += p1b
    se.trigger_s0
    assert_equal(got_s0a, 2)
    assert_equal(got_s0b, 1)

    got_s0a = 0
    got_s0b = 0
    se.s0 -= p1
    se.trigger_s0
    assert_equal(got_s0a, 1)
    assert_equal(got_s0b, 1)

    got_s0a = 0
    got_s0b = 0
    se.s0 -= p1b
    se.trigger_s0
    assert_equal(got_s0a, 0)
    assert_equal(got_s0b, 1)

    got_s0a = 0
    got_s0b = 0
    se.s0 -= p1
    se.trigger_s0
    assert_equal(got_s0a, 0)
    assert_equal(got_s0b, 1)

    got_s0a = 0
    got_s0b = 0
    se.s0 -= p2
    se.trigger_s0
    assert_equal(got_s0a, 0)
    assert_equal(got_s0b, 0)

  end

  def test_72

    GC.start

    nx = RBA::X::instances
    z = RBA::Z::new

    x = RBA::X::new("1")
    z.set_x(x)
    assert_equal(RBA::X::instances, nx + 1)

    # weird. On WIN/32bit, this makes the test pass (enables GC somehow?):
    puts("ANYTHING")

    x = nil
    z.set_x(nil)
    GC.start

    assert_equal(RBA::X::instances, nx)

    x = RBA::X::new("2")
    assert_equal(RBA::X::instances, nx + 1)
    z.set_x_keep(x)

    x = nil
    GC.start

    assert_equal(RBA::X::instances, nx + 1)

    # weird. On WIN/32bit, this makes the test pass (enables GC somehow?):
    puts("ANYTHING")

    # this will release the object - hence it's going to be deleted
    z.set_x_keep(nil)
    GC.start
    assert_equal(RBA::X::instances, nx)

  end
  
  def test_73

    begin

      poly = RBA::Polygon::new(RBA::Box::new(0, 0, 100, 100))
      
      # passing exceptions over iterators is critical because it involves
      # a Ruby/C++ and C++/Ruby transition
      poly.each_edge do |e|
        raise MyException::new("some exception")
      end

    rescue => ex
      assert_equal(ex.class.to_s, "MyException")
      assert_equal(ex.to_s, "some exception")
    end

    begin
      raise MyException::new("another exception")
    rescue => ex
      assert_equal(ex.class.to_s, "MyException")
      assert_equal(ex.to_s, "another exception")
    end

  end

  def test_QByteArray

    # QByteArray

    if RBA::A.respond_to?(:ia_cref_to_qba)

      qba = RBA::A::ia_cref_to_qba([ 16, 42, 0, 8 ])
      assert_equal(qba.inspect, "\"\\x10*\\x00\\b\"")

      assert_equal(RBA::A::qba_to_ia(qba), [ 16, 42, 0, 8 ])
      assert_equal(RBA::A::qba_cref_to_ia(qba), [ 16, 42, 0, 8 ])
      assert_equal(RBA::A::qba_cptr_to_ia(qba), [ 16, 42, 0, 8 ])
      assert_equal(RBA::A::qba_ref_to_ia(qba), [ 16, 42, 0, 8 ])
      assert_equal(RBA::A::qba_ptr_to_ia(qba), [ 16, 42, 0, 8 ])

      qba = RBA::A::ia_cref_to_qba_cref([ 17, 42, 0, 8 ])
      assert_equal(qba.inspect, "\"\\x11*\\x00\\b\"")
      qba = RBA::A::ia_cref_to_qba_ref([ 18, 42, 0, 8 ])
      assert_equal(qba.inspect, "\"\\x12*\\x00\\b\"")
      qba = RBA::A::ia_cref_to_qba_cptr([ 19, 42, 0, 8 ])
      assert_equal(qba.inspect, "\"\\x13*\\x00\\b\"")
      qba = RBA::A::ia_cref_to_qba_ptr([ 20, 42, 0, 8 ])
      assert_equal(qba.inspect, "\"\\x14*\\x00\\b\"")

      assert_equal(RBA::A::qba_to_ia("\x00\x01\x02"), [ 0, 1, 2 ])

    end

  end

  def test_QByteArrayView

    # QByteArrayView

    if RBA::A.respond_to?(:ia_cref_to_qbav)

      qbav = RBA::A::ia_cref_to_qbav([ 16, 42, 0, 8 ])
      assert_equal(qbav.inspect, "\"\\x10*\\x00\\b\"")

      assert_equal(RBA::A::qbav_to_ia(qbav), [ 16, 42, 0, 8 ])
      assert_equal(RBA::A::qbav_cref_to_ia(qbav), [ 16, 42, 0, 8 ])
      assert_equal(RBA::A::qbav_cptr_to_ia(qbav), [ 16, 42, 0, 8 ])
      assert_equal(RBA::A::qbav_ref_to_ia(qbav), [ 16, 42, 0, 8 ])
      assert_equal(RBA::A::qbav_ptr_to_ia(qbav), [ 16, 42, 0, 8 ])

      qbav = RBA::A::ia_cref_to_qbav_cref([ 17, 42, 0, 8 ])
      assert_equal(qbav.inspect, "\"\\x11*\\x00\\b\"")
      qbav = RBA::A::ia_cref_to_qbav_ref([ 18, 42, 0, 8 ])
      assert_equal(qbav.inspect, "\"\\x12*\\x00\\b\"")
      qbav = RBA::A::ia_cref_to_qbav_cptr([ 19, 42, 0, 8 ])
      assert_equal(qbav.inspect, "\"\\x13*\\x00\\b\"")
      qbav = RBA::A::ia_cref_to_qbav_ptr([ 20, 42, 0, 8 ])
      assert_equal(qbav.inspect, "\"\\x14*\\x00\\b\"")

      assert_equal(RBA::A::qbav_to_ia("\x00\x01\x02"), [ 0, 1, 2 ])

    end

  end

  def test_QString

    # QString

    if RBA::A.respond_to?(:ia_cref_to_qs)

      qs = RBA::A::ia_cref_to_qs([ 16, 42, 0, 8 ])
      assert_equal(qs.inspect, "\"\\x10*\\x00\\b\"")

      assert_equal(RBA::A::qs_to_ia(qs), [ 16, 42, 0, 8 ])
      assert_equal(RBA::A::qs_cref_to_ia(qs), [ 16, 42, 0, 8 ])
      assert_equal(RBA::A::qs_cptr_to_ia(qs), [ 16, 42, 0, 8 ])
      assert_equal(RBA::A::qs_ref_to_ia(qs), [ 16, 42, 0, 8 ])
      assert_equal(RBA::A::qs_ptr_to_ia(qs), [ 16, 42, 0, 8 ])

      qs = RBA::A::ia_cref_to_qs_cref([ 17, 42, 0, 8 ])
      assert_equal(qs.inspect, "\"\\x11*\\x00\\b\"")
      qs = RBA::A::ia_cref_to_qs_ref([ 18, 42, 0, 8 ])
      assert_equal(qs.inspect, "\"\\x12*\\x00\\b\"")
      qs = RBA::A::ia_cref_to_qs_cptr([ 19, 42, 0, 8 ])
      assert_equal(qs.inspect, "\"\\x13*\\x00\\b\"")
      qs = RBA::A::ia_cref_to_qs_ptr([ 20, 42, 0, 8 ])
      assert_equal(qs.inspect, "\"\\x14*\\x00\\b\"")

      assert_equal(RBA::A::qs_to_ia("\x00\x01\x02"), [ 0, 1, 2 ])

    end

  end

  def test_QLatin1String

    # QLatin1String

    if RBA::A.respond_to?(:ia_cref_to_ql1s)

      ql1s = RBA::A::ia_cref_to_ql1s([ 16, 42, 0, 8 ])
      assert_equal(ql1s.inspect, "\"\\x10*\\x00\\b\"")

      assert_equal(RBA::A::ql1s_to_ia(ql1s), [ 16, 42, 0, 8 ])
      assert_equal(RBA::A::ql1s_cref_to_ia(ql1s), [ 16, 42, 0, 8 ])
      assert_equal(RBA::A::ql1s_cptr_to_ia(ql1s), [ 16, 42, 0, 8 ])
      assert_equal(RBA::A::ql1s_ref_to_ia(ql1s), [ 16, 42, 0, 8 ])
      assert_equal(RBA::A::ql1s_ptr_to_ia(ql1s), [ 16, 42, 0, 8 ])

      ql1s = RBA::A::ia_cref_to_ql1s_cref([ 17, 42, 0, 8 ])
      assert_equal(ql1s.inspect, "\"\\x11*\\x00\\b\"")
      ql1s = RBA::A::ia_cref_to_ql1s_ref([ 18, 42, 0, 8 ])
      assert_equal(ql1s.inspect, "\"\\x12*\\x00\\b\"")
      ql1s = RBA::A::ia_cref_to_ql1s_cptr([ 19, 42, 0, 8 ])
      assert_equal(ql1s.inspect, "\"\\x13*\\x00\\b\"")
      ql1s = RBA::A::ia_cref_to_ql1s_ptr([ 20, 42, 0, 8 ])
      assert_equal(ql1s.inspect, "\"\\x14*\\x00\\b\"")

      assert_equal(RBA::A::ql1s_to_ia("\x00\x01\x02"), [ 0, 1, 2 ])

    end

  end

  def test_QStringView

    # QStringView

    if RBA::A.respond_to?(:ia_cref_to_qsv)

      qsv = RBA::A::ia_cref_to_qsv([ 16, 42, 0, 8 ])
      assert_equal(qsv.inspect, "\"\\x10*\\x00\\b\"")

      assert_equal(RBA::A::qsv_to_ia(qsv), [ 16, 42, 0, 8 ])
      assert_equal(RBA::A::qsv_cref_to_ia(qsv), [ 16, 42, 0, 8 ])
      assert_equal(RBA::A::qsv_cptr_to_ia(qsv), [ 16, 42, 0, 8 ])
      assert_equal(RBA::A::qsv_ref_to_ia(qsv), [ 16, 42, 0, 8 ])
      assert_equal(RBA::A::qsv_ptr_to_ia(qsv), [ 16, 42, 0, 8 ])

      qsv = RBA::A::ia_cref_to_qsv_cref([ 17, 42, 0, 8 ])
      assert_equal(qsv.inspect, "\"\\x11*\\x00\\b\"")
      qsv = RBA::A::ia_cref_to_qsv_ref([ 18, 42, 0, 8 ])
      assert_equal(qsv.inspect, "\"\\x12*\\x00\\b\"")
      qsv = RBA::A::ia_cref_to_qsv_cptr([ 19, 42, 0, 8 ])
      assert_equal(qsv.inspect, "\"\\x13*\\x00\\b\"")
      qsv = RBA::A::ia_cref_to_qsv_ptr([ 20, 42, 0, 8 ])
      assert_equal(qsv.inspect, "\"\\x14*\\x00\\b\"")

      assert_equal(RBA::A::qsv_to_ia("\x00\x01\x02"), [ 0, 1, 2 ])

    end

  end

  def test_binaryStrings

    # binary strings (non-Qt)

    ba = RBA::A::ia_cref_to_ba([ 16, 42, 1, 8 ])
    assert_equal(ba.inspect, "\"\\x10*\\x01\\b\"")

    assert_equal(RBA::A::ba_to_ia(ba), [ 16, 42, 1, 8 ])
    assert_equal(RBA::A::ba_cref_to_ia(ba), [ 16, 42, 1, 8 ])
    assert_equal(RBA::A::ba_cptr_to_ia(ba), [ 16, 42, 1, 8 ])
    assert_equal(RBA::A::ba_ref_to_ia(ba), [ 16, 42, 1, 8 ])
    assert_equal(RBA::A::ba_ptr_to_ia(ba), [ 16, 42, 1, 8 ])

    ba = RBA::A::ia_cref_to_ba_cref([ 17, 42, 0, 8 ])
    assert_equal(ba.inspect, "\"\\x11*\\x00\\b\"")
    ba = RBA::A::ia_cref_to_ba_ref([ 18, 42, 0, 8 ])
    assert_equal(ba.inspect, "\"\\x12*\\x00\\b\"")
    ba = RBA::A::ia_cref_to_ba_cptr([ 19, 42, 0, 8 ])
    assert_equal(ba.inspect, "\"\\x13*\\x00\\b\"")
    ba = RBA::A::ia_cref_to_ba_ptr([ 20, 42, 0, 8 ])
    assert_equal(ba.inspect, "\"\\x14*\\x00\\b\"")

    assert_equal(RBA::A::ba_to_ia("\x00\x01\x02"), [ 0, 1, 2 ])

  end

  def test_optional

    if RBA::B.respond_to?(:int_to_optional)

      assert_equal(RBA::B::int_to_optional(1, true), 1)
      assert_equal(RBA::B::int_to_optional(1, false), nil)
      assert_equal(RBA::B::int_to_optional_a(1, true).get_n, 1)
      assert_equal(RBA::B::int_to_optional_a(1, false), nil)

      assert_equal(RBA::B::optional_to_int(1, -1), 1)
      assert_equal(RBA::B::optional_to_int(nil, -1), -1)
      assert_equal(RBA::B::optional_cref_to_int(2, -1), 2)
      assert_equal(RBA::B::optional_cref_to_int(nil, -1), -1)
      assert_equal(RBA::B::optional_ref_to_int(3, -1), 3)
      assert_equal(RBA::B::optional_ref_to_int(nil, -1), -1)

      assert_equal(RBA::B::optional_cptr_to_int(4, -1), 4)
      assert_equal(RBA::B::optional_ptr_to_int(5, -1), 5)

      assert_equal(RBA::B::optional_a_to_int(RBA::A::new(1), -1), 1)
      assert_equal(RBA::B::optional_a_to_int(nil, -1), -1)
      assert_equal(RBA::B::optional_a_cref_to_int(RBA::A::new(2), -1), 2)
      assert_equal(RBA::B::optional_a_cref_to_int(nil, -1), -1)
      assert_equal(RBA::B::optional_a_ref_to_int(RBA::A::new(3), -1), 3)
      assert_equal(RBA::B::optional_a_ref_to_int(nil, -1), -1)

      assert_equal(RBA::B::optional_a_cptr_to_int(RBA::A::new(4), -1), 4)
      assert_equal(RBA::B::optional_a_ptr_to_int(RBA::A::new(5), -1), 5)

    end

  end

  # Tests multi-base mixins (only constants and enums available)
  def test_multiBaseMixins
    
    bb = RBA::BB::new  # base classes B1,B2,B3
    bb.set1(17)                                          # B1
    assert_equal(bb.get1, 17)                            # B1
    bb.set1(21)                                          # B1
    assert_equal(RBA::B3::E::E3B.to_i, 101)              # B3

    assert_equal(bb.get1, 21)                            # B1
    assert_equal(RBA::BB::C2, 17)                        # B2
    assert_equal(RBA::BB::C3, -1)                        # B3
    assert_equal(RBA::BB::E::E3B.to_i, 101)              # B3
    assert_equal(bb.d3(RBA::BB::E::E3C, RBA::BB::E::E3A), -2)  # BB with B3 enums
    assert_equal(bb.d3(RBA::BB::E::E3A, RBA::BB::E::E3C), 2)   # BB with B3 enums

  end

  # Custom factory implemented in Ruby
  def test_80

    gc = RBA::GObject.g_inst_count
    gf = RBAGFactory::new
    go = RBA::GFactory.create_f(gf, 17)
    assert_equal(go.g_virtual, 34)
    assert_equal(go.g_org, 0)
    assert_equal(RBA::GObject.g_inst_count, gc + 1)
    go = nil
    GC.start
    assert_equal(RBA::GObject.g_inst_count, gc)

  end

  # keyword arguments, enums and error messages
  def test_81

    bb = RBA::BB::new
    
    m = ""
    begin
      bb.d4()
    rescue => ex
      m = ex.to_s
    end
    assert_equal(m, "Can't match arguments. Variants are:\n  string d4(int a, string b, double c, B3::E d = E3A, variant e = nil) [no value given for argument #1 and following]\n in BB::d4")

    m = ""
    begin
      bb.d4(1, "a")
    rescue => ex
      m = ex.to_s
    end
    assert_equal(m, "Can't match arguments. Variants are:\n  string d4(int a, string b, double c, B3::E d = E3A, variant e = nil) [no value given for argument #3]\n in BB::d4")

    m = ""
    begin
      bb.d4(1, "a", 2.0, xxx: 17)
    rescue => ex
      m = ex.to_s
    end
    assert_equal(m, "Can't match arguments. Variants are:\n  string d4(int a, string b, double c, B3::E d = E3A, variant e = nil) [unknown keyword parameter: xxx]\n in BB::d4")

    m = ""
    begin
      bb.d4(a: 1, b: "a", c: 2.0, xxx: 17)
    rescue => ex
      m = ex.to_s
    end
    assert_equal(m, "Can't match arguments. Variants are:\n  string d4(int a, string b, double c, B3::E d = E3A, variant e = nil) [unknown keyword parameter: xxx]\n in BB::d4")

    assert_equal(bb.d4(1, "a", 2.0), "1,a,2,100,nil")
    assert_equal(bb.d4(1, "a", 2.0, e: 42), "1,a,2,100,42")
    assert_equal(bb.d4(1, "a", c: 2.0, e: 42), "1,a,2,100,42")
    assert_equal(bb.d4(c: 2.0, a: 1, b: "a", e: 42), "1,a,2,100,42")
    assert_equal(bb.d4(1, "a", 2.0, d: RBA::BB::E::E3B), "1,a,2,101,nil")
    assert_equal(bb.d4(1, "a", d: RBA::BB::E::E3B, c: 2.5), "1,a,2.5,101,nil")
    assert_equal(bb.d4(1, "a", 2.0, RBA::BB::E::E3B, 42), "1,a,2,101,42")

  end

end
