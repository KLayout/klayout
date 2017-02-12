$:.push(File::dirname($0))

load("test_prologue.rb")

class DBLayoutQuery_TestClass < TestBase

  def test_1

    ly = RBA::Layout::new
    ly.read(ENV["TESTSRC"] + "/testdata/gds/t11.gds")

    q = RBA::LayoutQuery::new("select cell.name, cell.bbox from *")
    res = []
    q.each(ly) do |iter|
      res << iter.data.inspect
    end

    assert_equal(res.size, 2)
    assert_equal(res[0], "[\"TOPTOP\", (0,0;32800,12800)]")
    assert_equal(res[1], "[\"TOP\", (0,0;900,900)]")

  end

  def test_2

    ly = RBA::Layout::new
    ly.read(ENV["TESTSRC"] + "/testdata/gds/t11.gds")

    q = RBA::LayoutQuery::new("delete TOP")
    q.execute(ly)

    q = RBA::LayoutQuery::new("select cell.name, cell.bbox from *")
    res = []
    q.each(ly) do |iter|
      res << iter.data.inspect
    end

    assert_equal(res.size, 1)
    assert_equal(res[0], "[\"TOPTOP\", ()]")

  end

  def test_3

    q = RBA::LayoutQuery::new("delete TOP")
    assert_equal(q.property_names.sort.join(","), "bbox,cell,cell_bbox,cell_index,cell_name,hier_levels,initial_cell,initial_cell_index,initial_cell_name,inst,instances,path,path_names,path_trans,references,shape,tot_weight,weight")

  end

end

load("test_epilogue.rb")

