
$:.push(File::dirname($0))

load("test_prologue.rb")

def mapping_to_s(ly1, ly2, lm)
  r = ""
  ly1.layer_indices.each do |li|
    s = ly1.get_info(li).to_s
    if lm.has_mapping?(li)
      s += "=>" + ly2.get_info(lm.layer_mapping(li)).to_s
    end
    r == "" || (r += ";")
    r += s
  end
  r
end

class DBLayerMapping_TestClass < TestBase

  def test_1

    mp = RBA::LayerMapping::new
    mp.map(0, 1)
    assert_equal(mp.has_mapping?(0), true)
    assert_equal(mp.has_mapping?(1), false)
    assert_equal(mp.layer_mapping(0), 1)
    mp.clear
    assert_equal(mp.has_mapping?(0), false)
    assert_equal(mp.has_mapping?(1), false)
    mp.map(1, 2)
    assert_equal(mp.layer_mapping(1), 2)

    ly1 = RBA::Layout::new

    a1 = ly1.insert_layer(RBA::LayerInfo::new(1, 0))
    a2 = ly1.insert_layer(RBA::LayerInfo::new(2, 0))
    a3 = ly1.insert_layer(RBA::LayerInfo::new("A"))

    ly2 = RBA::Layout::new

    b1 = ly2.insert_layer(RBA::LayerInfo::new("A"))
    b2 = ly2.insert_layer(RBA::LayerInfo::new(3, 0))
    b3 = ly2.insert_layer(RBA::LayerInfo::new(2, 0))

    mp = RBA::LayerMapping::new
    mp.create(ly1, ly2)
    assert_equal(mapping_to_s(ly2, ly1, mp), "A=>A;3/0;2/0=>2/0")

    mp = RBA::LayerMapping::new
    nl = mp.create_full(ly1, ly2)
    assert_equal(mapping_to_s(ly2, ly1, mp), "A=>A;3/0=>3/0;2/0=>2/0")
    assert_equal(nl.inspect, "[3]")

  end

end

load("test_epilogue.rb")
