# encoding: UTF-8

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

if !$:.member?(File::dirname($0))
  $:.push(File::dirname($0))
end

load("test_prologue.rb")

class LAYMenuTest_TestClass < TestBase

  def test_0

    if !RBA.constants.member?(:Action)
      return
    end

    a = RBA::Action::new
    b = a.dup

    a.shortcut = "Shift+F1"

    assert_equal(a.shortcut, "Shift+F1")
    assert_equal(a.effective_shortcut, "Shift+F1")
    assert_equal(b.shortcut, "")
    assert_equal(b.effective_shortcut, "")

    a.default_shortcut = "X"

    assert_equal(a.default_shortcut, "X")
    assert_equal(a.shortcut, "Shift+F1")
    assert_equal(a.effective_shortcut, "Shift+F1")

    a.shortcut = ""

    assert_equal(a.default_shortcut, "X")
    assert_equal(a.shortcut, "")
    assert_equal(a.effective_shortcut, "X")

    a.shortcut = RBA::Action::NoKeyBound
    assert_equal(a.default_shortcut, "X")
    assert_equal(a.shortcut, "none")
    assert_equal(a.effective_shortcut, "")
    a.shortcut = ""

    assert_equal(a.is_visible?, true)

    a.hidden = false
    a.visible = false

    assert_equal(a.is_visible?, false)
    assert_equal(a.is_hidden?, false)
    assert_equal(a.is_effective_visible?, false)
    assert_equal(a.effective_shortcut, "X")

    a.hidden = false
    a.visible = true

    assert_equal(a.is_visible?, true)
    assert_equal(a.is_hidden?, false)
    assert_equal(a.is_effective_visible?, true)
    assert_equal(a.effective_shortcut, "X")

    a.hidden = true
    a.visible = true

    assert_equal(a.is_visible?, true)
    assert_equal(a.is_hidden?, true)
    assert_equal(a.is_effective_visible?, false)
    assert_equal(a.effective_shortcut, "")

  end

  def test_1

    if !RBA.constants.member?(:Application)
      return
    end

    app = RBA::Application.instance
    mw = app.main_window
    menu = mw.menu

    app.set_config( "ruler-templates", "" )

    def item_str( menu ) 
      def collect_children(space, menu, item)
        res = "#{space}#{item}\n"
        menu.items(item).each { |child| res += collect_children("  #{space}", menu, child) }
        return res;
      end
      res = ""
      menu.items("").each { |item| res += collect_children("", menu, item) }
      return res
    end

    # since the menu is somewhat dynamic, this is not really a good idea
    # (in addition, the menu depends on the mode - editable or not)
    if false
      assert_equal( item_str(menu), <<RESULT )
file_menu
...
  @hcp_context_menu.save_as
RESULT
    end

    menu.delete_item("@toolbar.partial")
    menu.delete_item("@toolbar.instance")
    menu.delete_item("@toolbar.polygon")
    menu.delete_item("@toolbar.box")
    menu.delete_item("@toolbar.path")
    menu.delete_item("@toolbar.text")
    menu.delete_item("@hcp_context_menu")
    menu.delete_item("@lcp_context_menu")
    menu.delete_item("help_group")
    menu.delete_item("tools_menu")
    menu.delete_item("edit_menu")
    menu.delete_item("file_menu.reader_options")
    menu.delete_item("file_menu.session_group")
    menu.delete_item("file_menu.restore_session")
    menu.delete_item("file_menu.save_session")
    menu.delete_item("zoom_menu")

    $a1 = RBA::Action::new
    $a1.title = "New title"
    $a1.icon_text = "X"
    $a1.shortcut = "Shift+Ctrl+F7"
    menu.insert_menu( "end", "my_menu", "My menu" )
    menu.insert_item( "my_menu.end", "new_item", $a1 )
    menu.insert_item( "file_menu.begin", "new_item_1", $a1 )
    menu.insert_separator( "file_menu.#3", "sep" )
    menu.insert_item( "file_menu.#3", "new_item_2", $a1 )

    assert_equal( menu.action("my_menu").title, "My menu" )

    if false # disabled because hard to keep updated
    assert_equal( item_str(menu), <<RESULT )
file_menu
  file_menu.new_item_1
  file_menu.new_layout
  file_menu.new_panel
  file_menu.new_item_2
  file_menu.sep
  file_menu.post_new_group
  file_menu.open
  file_menu.open_same_panel
  file_menu.open_new_panel
  file_menu.close
  file_menu.clone
  file_menu.reload
  file_menu.pull_in
  file_menu.open_recent_group
  file_menu.open_recent_menu
  file_menu.import_group
  file_menu.import_menu
    file_menu.import_menu.import_gerber_menu
      file_menu.import_menu.import_gerber_menu.import_gerber_new
      file_menu.import_menu.import_gerber_menu.import_gerber_new_free
      file_menu.import_menu.import_gerber_menu.import_gerber_open
      file_menu.import_menu.import_gerber_menu.import_gerber_recent
    file_menu.import_menu.import_stream
    file_menu.import_menu.import_lef
    file_menu.import_menu.import_def
  file_menu.save_group
  file_menu.save
  file_menu.save_as
  file_menu.setup_group
  file_menu.setup
  file_menu.misc_group
  file_menu.screenshot
  file_menu.layout_props
  file_menu.layer_group
  file_menu.load_layer_props
  file_menu.save_layer_props
  file_menu.exit_group
  file_menu.exit
bookmark_menu
  bookmark_menu.goto_bookmark_menu
  bookmark_menu.bookmark_view
  bookmark_menu.bookmark_mgm_group
  bookmark_menu.manage_bookmarks
  bookmark_menu.load_bookmarks
  bookmark_menu.save_bookmarks
help_menu
  help_menu.about
@toolbar
  @toolbar.select
  @toolbar.move
  @toolbar.ruler
my_menu
  my_menu.new_item
RESULT
    end

    assert_equal( menu.is_valid?( "file_menu.#0" ), true )
    assert_equal( menu.is_valid?( "fill_menu" ), false )
    assert_equal( menu.is_menu?( "fill_menu" ), false )
    assert_equal( menu.is_menu?( "file_menu" ), true )
    assert_equal( menu.is_menu?( "file_menu.#0" ), false )
    assert_equal( menu.is_separator?( "fill_menu" ), false )
    assert_equal( menu.is_separator?( "file_menu.#0" ), false )
    assert_equal( menu.is_separator?( "file_menu.sep" ), true )

    assert_equal( menu.action( "file_menu.#0" ).title, "New title" )
    assert_equal( menu.action( "file_menu.#0" ).shortcut, "Shift+Ctrl+F7" )
    assert_equal( menu.action( "file_menu.#3" ).icon_text, "X" )
    assert_equal( menu.action( "my_menu.new_item" ).is_visible?, true )
    assert_equal( menu.action( "file_menu.#3" ).is_checked?, false )
    assert_equal( menu.action( "my_menu.new_item" ).is_enabled?, true )

    $a1.visible = false
    assert_equal( menu.action( "my_menu.new_item" ).is_visible?, false )
    assert_equal( menu.action( "my_menu.new_item" ).is_checked?, false )
    assert_equal( menu.action( "my_menu.new_item" ).is_enabled?, true )

    $a1.checked = true
    assert_equal( menu.action( "file_menu.#3" ).is_visible?, false )
    assert_equal( menu.action( "file_menu.#3" ).is_checked?, false )
    assert_equal( menu.action( "file_menu.#3" ).is_checkable?, false )
    assert_equal( menu.action( "file_menu.#3" ).is_enabled?, true )

    $a1.checked = false
    $a1.checkable = true;
    assert_equal( menu.action( "my_menu.new_item" ).is_visible?, false )
    assert_equal( menu.action( "my_menu.new_item" ).is_checked?, false )
    assert_equal( menu.action( "my_menu.new_item" ).is_checkable?, true )
    assert_equal( menu.action( "my_menu.new_item" ).is_enabled?, true )
    $a1.checked = true
    assert_equal( menu.action( "file_menu.#0" ).is_checked?, true )

    $a1.enabled = false
    assert_equal( menu.action( "my_menu.new_item" ).is_visible?, false )
    assert_equal( menu.action( "my_menu.new_item" ).is_checked?, true )
    assert_equal( menu.action( "my_menu.new_item" ).is_enabled?, false )

    $a1.visible = true
    assert_equal( menu.action( "my_menu.new_item" ).is_visible?, true )
    assert_equal( menu.action( "my_menu.new_item" ).is_checked?, true )
    assert_equal( menu.action( "my_menu.new_item" ).is_enabled?, false )

    $a1.enabled = true
    assert_equal( menu.action( "my_menu.new_item" ).is_visible?, true )
    assert_equal( menu.action( "my_menu.new_item" ).is_checked?, true )
    assert_equal( menu.action( "my_menu.new_item" ).is_enabled?, true )

  end

  if RBA.constants.member?(:Action)

    class MenuHandler < RBA::Action
      def initialize( t, k, &action ) 
        self.title = t
        self.shortcut = k
        @action = action
      end
      def triggered 
        @action.call( self ) 
      end
    private
      @action
    end

  end

  def test_2

    if !RBA.constants.member?(:Application)
      return
    end

    app = RBA::Application.instance
    mw = app.main_window
    menu = mw.menu

    triggered = false

    f7_handler = MenuHandler.new( "Ruby Test", "F7" ) {
      triggered = true
    } 
    # This makes the application crash
    $f7_handler = f7_handler

    menu.insert_item("file_menu.begin", "new_item", f7_handler)
    menu.action("file_menu.#0").trigger

    assert_equal( triggered, true )

    triggered = false

    menu.insert_item("help_menu.end", "new_item_copy", menu.action("file_menu.#0") )

    menu.action("help_menu.new_item_copy").trigger

    assert_equal( triggered, true )

  end

  def test_3

    if !RBA.constants.member?(:AbstractMenu)
      return
    end

    map = RBA::AbstractMenu::unpack_key_binding("'path.a':X;'path.b':''")
    assert_equal(map["path.a"], "X")
    assert_equal(map["path.b"], "")
    assert_equal(map["path.c"], nil)

    map2 = RBA::AbstractMenu::unpack_key_binding(RBA::AbstractMenu::pack_key_binding(map))
    assert_equal(map == map2, true)

    map = RBA::AbstractMenu::unpack_menu_items_hidden("'path.a':true;'path.b':false")
    assert_equal(map["path.a"], true)
    assert_equal(map["path.b"], false)
    assert_equal(map["path.c"], nil)

    map2 = RBA::AbstractMenu::unpack_menu_items_hidden(RBA::AbstractMenu::pack_menu_items_hidden(map))
    assert_equal(map == map2, true)

  end

  def test_4

    if !RBA.constants.member?(:Action)
      return
    end

    action = RBA::Action::new
    action.title = "title:n1"

    menu = RBA::AbstractMenu::new

    assert_equal(menu.action("s1.n1"), nil)
    assert_equal(menu.action("s1"), nil)

    menu.insert_menu("end", "s1", "submenu1")
    menu.insert_menu("end", "s2", "submenu2")

    menu.insert_item("s1.end", "n1", action)
    menu.insert_item("s2.end", "n1", action)

    assert_equal(menu.action("s1.n1") == action, true)
    assert_equal(menu.action("s2.n1") == action, true)

    menu._destroy

    # proof of transfer of ownership
    assert_equal(action._destroyed?, true)

  end

  def test_5

    if !RBA.constants.member?(:Action)
      return
    end

    action = RBA::Action::new
    action.title = "title:n1"

    menu = RBA::AbstractMenu::new

    assert_equal(menu.action("s1.n1"), nil)
    assert_equal(menu.action("s1"), nil)

    menu.insert_menu("end", "s1", "submenu1")
    menu.insert_menu("end", "s2", "submenu2")

    menu.insert_item("s1.end", "n1", action)
    menu.insert_item("s2.end", "n1", action)

    menu.delete_item ("s2");

    assert_equal(menu.action("s1.n1") == action, true)
    assert_equal(menu.action("s2.n1") == nil, true)

    menu._destroy

    assert_equal(action._destroyed?, true)

  end

  def test_6

    if !RBA.constants.member?(:Application)
      return
    end

    app = RBA::Application.instance
    mw = app.main_window

    assert_equal(mw.get_key_bindings["file_menu.exit"], "Ctrl+Q")

    # key bindings

    mw.set_key_bindings({"file_menu.exit" => "F2"})
    assert_equal(mw.get_key_bindings["file_menu.exit"], "F2")

    mw.set_key_bindings({"file_menu.exit" => ""})
    assert_equal(mw.get_key_bindings["file_menu.exit"], "")

    mw.set_key_bindings(mw.get_default_key_bindings)
    assert_equal(mw.get_key_bindings["file_menu.exit"], "Ctrl+Q")

    mw.set_key_bindings({"file_menu.exit" => ""})
    assert_equal(mw.get_key_bindings["file_menu.exit"], "")

    # menu items hidden

    assert_equal(mw.get_menu_items_hidden["file_menu.exit"], false)

    mw.set_menu_items_hidden({"file_menu.exit" => true})
    assert_equal(mw.get_menu_items_hidden["file_menu.exit"], true)

    mw.set_menu_items_hidden(mw.get_default_menu_items_hidden)
    assert_equal(mw.get_menu_items_hidden["file_menu.exit"], false)

    mw.set_menu_items_hidden({"file_menu.exit" => true})
    assert_equal(mw.get_menu_items_hidden["file_menu.exit"], true)

    # reset for the next pass
    mw.set_menu_items_hidden(mw.get_default_menu_items_hidden)
    mw.set_key_bindings(mw.get_default_key_bindings)

  end

  if RBA.constants.member?(:Action)

    class MyAction < RBA::Action
      attr_accessor :dyn_visible, :dyn_enabled
      def initialize
        self.dyn_visible = true
        self.dyn_enabled = true
      end
      def wants_visible
        self.dyn_visible
      end
      def wants_enabled
        self.dyn_enabled
      end
    end

    def test_7

      a = MyAction::new

      assert_equal(a.is_effective_visible?, true)
      a.hidden = true
      assert_equal(a.is_effective_visible?, false)
      a.hidden = false
      assert_equal(a.is_effective_visible?, true)
      a.visible = false
      assert_equal(a.is_effective_visible?, false)
      a.visible = true
      assert_equal(a.is_effective_visible?, true)
      a.dyn_visible = false
      assert_equal(a.is_effective_visible?, false)
      a.dyn_visible = true
      assert_equal(a.is_effective_visible?, true)

      assert_equal(a.is_effective_enabled?, true)
      a.enabled = false
      assert_equal(a.is_effective_enabled?, false)
      a.enabled = true
      assert_equal(a.is_effective_enabled?, true)
      a.dyn_enabled = false
      assert_equal(a.is_effective_enabled?, false)
      a.dyn_enabled = true
      assert_equal(a.is_effective_enabled?, true)

    end

  end

  def test_8

    if !RBA.constants.member?(:Action)
      return
    end

    action = RBA::Action::new
    action.title = "title:n1"

    menu = RBA::AbstractMenu::new

    assert_equal(menu.action("s1.n1"), nil)
    assert_equal(menu.action("s1"), nil)

    menu.insert_menu("end", "s1", "submenu1")
    menu.insert_menu("end", "s2", "submenu2")

    menu.insert_item("s1.end", "n1", action)
    menu.insert_item("s1.end", "n2", action)
    menu.insert_item("s2.end", "n1", action)

    assert_equal(menu.action("s1.n1") == action, true)
    assert_equal(menu.action("s1.n2") == action, true)
    assert_equal(menu.action("s2.n1") == action, true)

    assert_equal(menu.is_valid?("s1.n1"), true)
    assert_equal(menu.is_valid?("s1.n2"), true)
    assert_equal(menu.is_valid?("s2.n1"), true)

    menu.clear_menu("s1")

    assert_equal(menu.is_valid?("s1.n1"), false)
    assert_equal(menu.is_valid?("s1.n2"), false)
    assert_equal(menu.is_valid?("s2.n1"), true)

    menu.clear_menu("s2")

    assert_equal(menu.is_valid?("s1.n1"), false)
    assert_equal(menu.is_valid?("s1.n2"), false)
    assert_equal(menu.is_valid?("s2.n1"), false)

    # proof of transfer of ownership
    assert_equal(action._destroyed?, true)

    menu._destroy

  end

end

load("test_epilogue.rb")

