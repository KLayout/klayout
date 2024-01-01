# encoding: UTF-8

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

if !$:.member?(File::dirname($0))
  $:.push(File::dirname($0))
end

load("test_prologue.rb")


class Tl_TestClass < TestBase

  # Expression basics
  def test_1_Expression

    ctx = RBA::ExpressionContext::new
    assert_equal(ctx.eval("1+2"), 3)
    ctx.var("a", 21)
    assert_equal(ctx.eval("2*a"), 42)

    expr = RBA::Expression::new
    res = expr.eval
    assert_equal(res.class.to_s, "NilClass")
    assert_equal(res.to_s, "")

    expr = RBA::Expression.eval("1+2")
    assert_equal(expr.class.to_s, "Float")
    assert_equal(expr.to_s, "3.0")

    expr = RBA::Expression::new
    expr.text = "1+2"
    res = expr.eval
    assert_equal(res.class.to_s, "Float")
    assert_equal(res.to_s, "3.0")

    expr = RBA::Expression::new
    expr.var("a", 5)
    expr.text = "a+to_i(2)"
    res = expr.eval
    assert_equal(res.class.to_s == "Fixnum" || res.class.to_s == "Integer", true)
    assert_equal(res.to_s, "7")
    expr.var("a", 7)
    res = expr.eval
    assert_equal(res.to_s, "9")

    RBA::Expression::global_var("xxx", 17.5)
    expr = RBA::Expression::new("xxx+1")
    res = expr.eval
    assert_equal(res.class.to_s, "Float")
    assert_equal(res.to_s, "18.5")

    expr = RBA::Expression::new("a+b*2", { "a" => 18, "b" => 2.5 })
    res = expr.eval
    assert_equal(res.class.to_s, "Float")
    assert_equal(res.to_s, "23.0")

    expr = RBA::Expression::new("[a[1],a[2],a[0],a[4],a[3]]", { "a" => [ 17, "a", nil, [ 2, 7 ], { 8 => "x", "u" => 42 } ] })
    res = expr.eval
    assert_equal(res.class.to_s, "Array")
    assert_equal(res.inspect, ["a", nil, 17, {8=>"x", "u"=>42}, [2, 7]].inspect)

    expr = RBA::Expression::new("a[1]", { "a" => [ 17, "a", nil, [ 2, 7 ], { 8 => "x", "u" => 42 } ] })
    res = expr.eval
    assert_equal(res.class.to_s, "String")
    assert_equal(res.to_s, "a")

    expr = RBA::Expression::new("a[4]", { "a" => [ 17, "a", nil, [ 2, 7 ], { 8 => "x", "u" => 42 } ] })
    res = expr.eval
    assert_equal(res.class.to_s, "Hash")
    assert_equal(res.inspect, {8=>"x", "u"=>42}.inspect)

  end

  # Advanced expressions 
  def test_2_Expression

    box1 = RBA::Box::new(0, 100, 200, 300)
    box2 = RBA::Box::new(50, 150, 250, 350)
    expr = RBA::Expression::new("a", { "a" => box1, "b" => box2 })
    res = expr.eval

    assert_equal(res.to_s, "(0,100;200,300)")

    # boxes are non-managed objects -> passing the object through the expression does not persist their ID
    assert_not_equal(res.object_id, box1.object_id)
    assert_not_equal(res.object_id, box2.object_id)

    # -------------------------------------------------

    box1 = RBA::Box::new(0, 100, 200, 300)
    box2 = RBA::Box::new(50, 150, 250, 350)
    expr = RBA::Expression::new("a&b", { "a" => box1, "b" => box2 })
    res = expr.eval

    assert_equal(res.to_s, "(50,150;200,300)")

    # computed objects are entirely new ones
    assert_not_equal(res.object_id, box1.object_id)
    assert_not_equal(res.object_id, box2.object_id)

    # -------------------------------------------------

    box1 = RBA::Box::new(0, 100, 200, 300)
    box2 = RBA::Box::new(50, 150, 250, 350)
    expr = RBA::Expression::new("x=a&b; y=x; z=y; [x,y,z]", { "a" => box1, "b" => box2, "x" => nil, "y" => nil, "z" => nil })
    res = expr.eval

    assert_equal(res.map(&:to_s), %w((50,150;200,300) (50,150;200,300) (50,150;200,300)))

    # all objects are individual copies
    assert_not_equal(res[0].object_id, box1.object_id)
    assert_not_equal(res[0].object_id, box2.object_id)
    assert_not_equal(res[1].object_id, res[0].object_id)
    assert_not_equal(res[2].object_id, res[0].object_id)

    # -------------------------------------------------

    box1 = RBA::Box::new(0, 100, 200, 300)
    box2 = RBA::Box::new(50, 150, 250, 350)
    expr = RBA::Expression::new("var x=a&b; var y=x; var z=y; [x,y,z]", { "a" => box1, "b" => box2 })
    res = expr.eval

    assert_equal(res.map(&:to_s), %w((50,150;200,300) (50,150;200,300) (50,150;200,300)))

    # all objects are individual copies
    assert_not_equal(res[0].object_id, box1.object_id)
    assert_not_equal(res[0].object_id, box2.object_id)
    assert_not_equal(res[1].object_id, res[0].object_id)
    assert_not_equal(res[2].object_id, res[0].object_id)

    # destruction of the expression's object space does not matter since we have copies
    expr._destroy
    assert_equal(res.map(&:to_s), %w((50,150;200,300) (50,150;200,300) (50,150;200,300)))

    # -------------------------------------------------

    region1 = RBA::Region::new
    region1 |= RBA::Box::new(0, 100, 200, 300)
    region2 = RBA::Region::new
    region2 |= RBA::Box::new(50, 150, 250, 350)
    expr = RBA::Expression::new("a", { "a" => region1, "b" => region2 })
    res = expr.eval

    # regions are managed objects -> passing the object through the expression persists it's object ID
    assert_equal(res.object_id, region1.object_id)
    assert_not_equal(res.object_id, region2.object_id)

    # -------------------------------------------------

    region1 = RBA::Region::new
    region1 |= RBA::Box::new(0, 100, 200, 300)
    region2 = RBA::Region::new
    region2 |= RBA::Box::new(50, 150, 250, 350)
    expr = RBA::Expression::new("a&b", { "a" => region1, "b" => region2, "x" => nil, "y" => nil, "z" => nil })
    res = expr.eval

    assert_equal(res.to_s, "(50,150;50,300;200,300;200,150)")

    # The returned object (as a new one) is an entirely fresh one
    assert_not_equal(res.object_id, region1.object_id)
    assert_not_equal(res.object_id, region2.object_id)

    # -------------------------------------------------

    region1 = RBA::Region::new
    region1 |= RBA::Box::new(0, 100, 200, 300)
    region2 = RBA::Region::new
    region2 |= RBA::Box::new(50, 150, 250, 350)
    expr = RBA::Expression::new("x=a&b; y=x; z=y; [x,y,z]", { "a" => region1, "b" => region2, "x" => nil, "y" => nil, "z" => nil })
    res = expr.eval

    assert_equal(res.map(&:to_s), %w((50,150;50,300;200,300;200,150) (50,150;50,300;200,300;200,150) (50,150;50,300;200,300;200,150)))

    # regions are managed objects -> passing the object through the expression persists it's object ID
    assert_not_equal(res[0].object_id, region1.object_id)
    assert_not_equal(res[0].object_id, region2.object_id)
    assert_equal(res[1].object_id, res[0].object_id)
    assert_equal(res[2].object_id, res[0].object_id)

    # -------------------------------------------------

    region1 = RBA::Region::new
    region1 |= RBA::Box::new(0, 100, 200, 300)
    region2 = RBA::Region::new
    region2 |= RBA::Box::new(50, 150, 250, 350)
    expr = RBA::Expression::new("var x=a&b; var y=x; var z=y; [x,y,z]", { "a" => region1, "b" => region2 })
    res = expr.eval

    assert_equal(res.map(&:to_s), %w((50,150;50,300;200,300;200,150) (50,150;50,300;200,300;200,150) (50,150;50,300;200,300;200,150)))

    # regions are managed objects -> passing the object through the expression persists it's object ID
    assert_not_equal(res[0].object_id, region1.object_id)
    assert_not_equal(res[0].object_id, region2.object_id)
    assert_equal(res[1].object_id, res[0].object_id)
    assert_equal(res[2].object_id, res[0].object_id)

    # the result objects live in the expression object space and are destroyed with the expression
    expr._destroy

    assert_equal(res.size, 3)
    assert_equal(res[0].destroyed?, true)
    assert_equal(res[1].destroyed?, true)
    assert_equal(res[2].destroyed?, true)

  end

  # Glob pattern
  def test_3_GlobPattern

    pat = RBA::GlobPattern::new("a*b")

    assert_equal(pat.case_sensitive, true)
    assert_equal(pat.head_match, false)

    assert_equal(pat.match("ab") != nil, true)
    assert_equal(pat.match("axb") != nil, true)
    assert_equal(pat.match("Axb") != nil, false)
    assert_equal(pat.match("abx") != nil, false)
    assert_equal(pat.match("xab") != nil, false)

    pat.case_sensitive = false
    assert_equal(pat.case_sensitive, false)

    assert_equal(pat.match("ab") != nil, true)
    assert_equal(pat.match("axb") != nil, true)
    assert_equal(pat.match("Axb") != nil, true)
    assert_equal(pat.match("abx") != nil, false)
    assert_equal(pat.match("xab") != nil, false)

    pat.head_match = true
    assert_equal(pat.head_match, true)

    assert_equal(pat.match("ab") != nil, true)
    assert_equal(pat.match("axb") != nil, true)
    assert_equal(pat.match("Axb") != nil, true)
    assert_equal(pat.match("abx") != nil, true)
    assert_equal(pat.match("abx") == [], true)
    assert_equal(pat.match("xab") != nil, false)

    pat = RBA::GlobPattern::new("(*)a(*)")

    assert_equal(pat.match("xb") != nil, false)
    res = pat.match("xab")
    assert_equal(res != nil, true)
    assert_equal(res.join("/"), "x/b")

  end

  class MyRecipeExecutable < RBA::Executable

    def initialize(params)
      @params = params
    end

    def execute
      a = @params["A"] || 0
      b = @params["B"] || 0.0
      c = @params["C"] || 1.0
      b * a * c
    end

  end

  class MyRecipe < RBA::Recipe

    def initialize
      super("rba_test_recipe", "description")
    end

    def executable(params)
      return MyRecipeExecutable::new(params)
    end

  end

  # Recipe
  def test_4_Recipe

    # make sure there isn't a second instance
    GC.start

    my_recipe = MyRecipe::new
    my_recipe._create # makes debugging easier

    assert_equal(my_recipe.name, "rba_test_recipe")
    assert_equal(my_recipe.description, "description")

    g = my_recipe.generator("A" => 6, "B" => 7.0)
    assert_equal(g, "rba_test_recipe: A=#6,B=##7")
    assert_equal("%g" % RBA::Recipe::make(g), "42")
    assert_equal("%g" % RBA::Recipe::make(g, "C" => 1.5).to_s, "63")

    my_recipe._destroy
    my_recipe = nil
    GC.start

  end

end

load("test_epilogue.rb")
