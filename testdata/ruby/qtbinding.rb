# encoding: UTF-8

# KLayout Layout Viewer
# Copyright (C) 2006-2019 Matthias Koefferlein
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

# an event filter class

# don't create a class twice since the alias orgEventFilter will 
# refer to itself
if !Object.const_defined?("EventFilter")

  class EventFilter < RBA::QObject

    alias orgEventFilter eventFilter

    def initialize 
      @log = []
    end

    def log
      @log
    end

    def eventFilter(obj, event)
      @log << (event.class.to_s + ": " + event.type.inspect)
      self.orgEventFilter(obj, event)
    end

  end

  # implementation

  class MyAction < RBA::QAction

    def initialize(p, n)
      super(p)
      self.objectName = n
      @ce = nil
    end

    def childEvent(ev)
      if @ce
        @ce.call(ev)
      end
    end
    
    def on_child_event(&ce)
      @ce = ce
    end
    
  end

  class MyObject < RBA::QObject

    def initialize
      @ef = nil
      super(nil)
    end

    alias baseEventFilter eventFilter
      
    def eventFilter(watched, event)
      if @ef.call(watched, event)
        return true
      end
      baseEventFilter(watched, event)
    end

    def on_event_filter(&ef)
      @ef = ef
    end
    
  end
  
  class MyStandardItemModel < RBA::QStandardItemModel

    def srn(rn)
      if self.respond_to?(:setRoleNames, true)
        self.setRoleNames(rn)
      else
        self.setItemRoleNames(rn)
      end
    end

  end

end

class QtBinding_TestClass < TestBase

  def test_00

    # initial test 

  end

  def test_10

    a = MyAction.new(nil, "a")
    a.text = "mytext"
    a.checkable = true
    assert_equal(a.isChecked, false)
    a.checked = true
    assert_equal(a.text, "mytext")
    assert_equal(a.objectName, "a")
    a.text += "."
    assert_equal(a.text, "mytext.")
    assert_equal(a.isChecked, true)

    t = ""
    a.triggered do |checked|
      t += "[#{checked}]";
    end
    assert_equal(t, "")
    a.trigger # also toggles checked state
    assert_equal(t, "[false]")
    t = ""
    a.trigger # also toggles checked state
    assert_equal(t, "[true]")

    #  force GC and destruction
    a = nil
    GC.start
    a = MyAction.new(nil, "anew")
    assert_equal(a.objectName, "anew")
    a = nil
    GC.start

  end

  def test_11

    a = RBA::QAction.new(nil)
    aa = MyAction.new(a, "aa")
    assert_equal(aa.objectName, "aa")

    # destroying a will also destroy aa
    a.destroy
    assert_equal(a._destroyed?, true)
    assert_equal(aa._destroyed?, true)

  end

  def test_12

    a = RBA::QAction.new(nil)
    aa = RBA::QAction.new(a)
    aa.objectName = "aa"

    # destroying in the GC will also destroy aa
    a = nil
    GC.start
    a = RBA::QAction.new(nil)
    a = nil
    GC.start

    assert_equal(aa._destroyed?, true)

  end

  def test_13

    a = RBA::QAction.new(nil)
    aa = RBA::QAction.new(a)
    aa.objectName = "aa"
    aa.text = "aatext"

    cc = []
    a.children.each do |c|
      cc.push(c.objectName)
    end
    assert_equal(cc.join(","), "aa")

    # aa now is kept by a
    aa = nil
    GC.start
    aa = RBA::QAction.new(nil)
    aa = nil
    GC.start

    # fetch aa again
    a.children.each do |c|
      if c.objectName == "aa"
        aa = c
      end
    end
    assert_equal(aa != nil, true)
    assert_equal(aa.class.to_s, "RBA::QAction")
    assert_equal(aa.text, "aatext")
    assert_equal(aa._destroyed?, false)

  end

  def test_20

    no_event = false

    x = []
    ef = MyObject.new
    ef.on_event_filter do |watched,event|
      x.push(watched.inspect + ":" + event.inspect)
      no_event
    end

    ce_log = []
    a = MyAction.new(nil, "a")
    a.on_child_event do |ce|
      ce_log.push("#{ce.added}:#{ce.child.objectName}")
    end

    a.installEventFilter(ef)

    aa = MyAction.new(nil, "aa1")
    assert_equal(ce_log.join(","), "")
    aa.setParent(a)
    assert_equal(ce_log.join(","), "true:aa1")
    ce_log = []

    # destroy aa
    aa.destroy
    aa = nil
    assert_equal(ce_log.join(","), "false:aa1")
    ce_log = []

    no_event = true
    aa = MyAction.new(nil, "aa2")
    aa.setParent(a)
    assert_equal(ce_log.join(","), "")
    ce_log = []

    no_event = false
    aa.destroy
    aa = nil
    assert_equal(ce_log.join(","), "false:aa2")
    ce_log = []

  end

  def test_30

    # dialog construction, cleanup, object dependency ...

    mw = nil # more fun than with RBA::Application::instance.main_window because Ruby's GC does all the cleanup

    dialog = RBA::QDialog::new(mw)
    label = RBA::QLabel::new(dialog)
    layout = RBA::QHBoxLayout::new(dialog)
    layout.addWidget(label)
    GC.start

    dialog = RBA::QDialog::new(mw)
    label = RBA::QLabel::new(dialog)
    layout = RBA::QHBoxLayout::new(dialog)
    layout.addWidget(label)
    label.destroy
    GC.start

    dialog = RBA::QDialog::new(mw)
    label = RBA::QLabel::new(dialog)
    layout = RBA::QHBoxLayout::new(dialog)
    layout.addWidget(label)
    layout.destroy
    GC.start

    dialog = RBA::QDialog::new(mw)
    label = RBA::QLabel::new(dialog)
    layout = RBA::QHBoxLayout::new(dialog)
    layout.addWidget(label)
    dialog.destroy
    GC.start

    dialog = RBA::QDialog::new(mw)
    label = RBA::QLabel::new(dialog)
    layout = RBA::QHBoxLayout::new(dialog)
    layout.addWidget(label)
    GC.start

  end

  def test_31

    # Optional arguments, enums, QFlag's

    mw = nil # more fun than with RBA::Application::instance.main_window because Ruby's GC does all the cleanup

    mb = RBA::QMessageBox::new(RBA::QMessageBox::Critical, "title", "text")
    assert_equal(mb.icon.to_i != RBA::QMessageBox::Warning.to_i, true)
    assert_equal(mb.icon.to_i == RBA::QMessageBox::Critical.to_i, true)
    assert_equal(mb.standardButtons.to_i == RBA::QMessageBox::NoButton.to_i, true)

    mb = RBA::QMessageBox::new(RBA::QMessageBox::Critical, "title", "text", RBA::QMessageBox::Ok)
    assert_equal(mb.standardButtons.to_i == RBA::QMessageBox::Ok.to_i, true)

    mb = RBA::QMessageBox::new(RBA::QMessageBox::Critical, "title", "text", RBA::QMessageBox::Ok | RBA::QMessageBox::Cancel)
    assert_equal(mb.standardButtons.to_i == RBA::QMessageBox::Ok.to_i + RBA::QMessageBox::Cancel.to_i, true)

  end

  def test_40

    # Lifetime management of objects/methods not using QObject::parent
    # QTreeWidget (parent)/QTreeWidgetItem (child)

    # constructor with parent-like argument:
    tw = RBA::QTreeWidget::new
    ti = RBA::QTreeWidgetItem::new(tw)
    # strange, but true:
    assert_equal(ti.parent, nil)
    assert_equal(tw.topLevelItemCount, 1)
    ti = nil
    # force delete of the ti item unless it's referenced
    # NOTE: it's not sufficient to just set ti to nil, we also
    # have to create a new QTreeWidgetItem (special for Ruby >1.9?)
    ti = RBA::QTreeWidgetItem::new
    GC.start
    # gives 1, because the tree widget item is kept by
    # the tree widget:
    assert_equal(tw.topLevelItemCount, 1)

    # the tree item belongs to the widget, hence it's destroyed with
    # the widget
    ti = tw.topLevelItem(0)
    tw._destroy
    # gives true, because tw owns ti too.
    assert_equal(ti._destroyed?, true)

    # The same works for insert too
    tw = RBA::QTreeWidget::new
    ti = RBA::QTreeWidgetItem::new
    tw.insertTopLevelItem(0, ti)
    assert_equal(tw.topLevelItemCount, 1)
    ti = nil
    # force delete of the ti item unless it's referenced
    ti = RBA::QTreeWidgetItem::new
    GC.start
    # gives 1, because the tree widget item is kept by
    # the tree widget:
    assert_equal(tw.topLevelItemCount, 1)

    # the tree item belongs to the widget, hence it's destroyed with
    # the widget
    ti = tw.topLevelItem(0)
    tw._destroy
    # gives true, because tw owns ti
    assert_equal(ti._destroyed?, true)

    # And add:
    tw = RBA::QTreeWidget::new
    ti = RBA::QTreeWidgetItem::new
    tw.addTopLevelItem(ti)
    assert_equal(tw.topLevelItemCount, 1)
    ti = nil
    # force delete of the ti item unless it's referenced
    ti = RBA::QTreeWidgetItem::new
    GC.start
    # gives 1, because the tree widget item is kept by
    # the tree widget:
    assert_equal(tw.topLevelItemCount, 1)

    # the tree item belongs to the widget, hence it's destroyed with
    # the widget
    ti = tw.topLevelItem(0)
    tw._destroy
    # gives true, because tw owns ti
    assert_equal(ti._destroyed?, true)

    # But the item is released when we take it and add:
    tw = RBA::QTreeWidget::new
    ti = RBA::QTreeWidgetItem::new
    tw.addTopLevelItem(ti)
    assert_equal(tw.topLevelItemCount, 1)
    ti = nil
    # force delete of the ti item unless it's referenced
    ti = RBA::QTreeWidgetItem::new
    GC.start
    # gives 1, because the tree widget item is kept by
    # the tree widget:
    assert_equal(tw.topLevelItemCount, 1)

    ti = tw.takeTopLevelItem(0)
    tw._destroy
    # gives false, because we took ti and tw no longer owns it
    assert_equal(ti._destroyed?, false)

    # And we can destroy a child too
    tw = RBA::QTreeWidget::new
    ti = RBA::QTreeWidgetItem::new
    tw.addTopLevelItem(ti)
    assert_equal(tw.topLevelItemCount, 1)
    ti._destroy
    assert_equal(tw.topLevelItemCount, 0)

  end

  def test_41

    # Lifetime management of objects/methods not using QObject::parent
    # QTreeWidgetItem (parent)/QTreeWidgetItem (child)

    # constructor with parent-like argument (supported by QObject parent/child relationship):
    tw = RBA::QTreeWidgetItem::new
    ti = RBA::QTreeWidgetItem::new(tw)
    # that's not QObject::parent - this one still is 0 (not seen by RBA)
    assert_equal(ti.parent, tw)
    assert_equal(tw.childCount, 1)
    ti = nil
    # force delete of the ti item unless it's referenced
    # NOTE: it's not sufficient to just set ti to nil, we also
    # have to create a new QTreeWidgetItem (special for Ruby >1.9?)
    ti = RBA::QTreeWidgetItem::new
    GC.start
    # gives 1, because the tree widget item is kept by
    # the tree widget:
    assert_equal(tw.childCount, 1)

    # the tree item belongs to the widget, hence it's destroyed with
    # the widget
    ti = tw.child(0)
    tw._destroy
    # gives true, because tw owns ti too.
    assert_equal(ti._destroyed?, true)

    # The same works for insert too
    tw = RBA::QTreeWidgetItem::new
    ti = RBA::QTreeWidgetItem::new
    tw.insertChild(0, ti)
    assert_equal(tw.childCount, 1)
    ti = nil
    # force delete of the ti item unless it's referenced
    ti = RBA::QTreeWidgetItem::new
    GC.start
    # gives 1, because the tree widget item is kept by
    # the tree widget:
    assert_equal(tw.childCount, 1)

    # the tree item belongs to the widget, hence it's destroyed with
    # the widget
    ti = tw.child(0)
    tw._destroy
    # gives true, because tw owns ti
    assert_equal(ti._destroyed?, true)

    # And add:
    tw = RBA::QTreeWidgetItem::new
    ti = RBA::QTreeWidgetItem::new
    tw.addChild(ti)
    assert_equal(tw.childCount, 1)
    ti = nil
    # force delete of the ti item unless it's referenced
    ti = RBA::QTreeWidgetItem::new
    GC.start
    # gives 1, because the tree widget item is kept by
    # the tree widget:
    assert_equal(tw.childCount, 1)

    # the tree item belongs to the widget, hence it's destroyed with
    # the widget
    ti = tw.child(0)
    tw._destroy
    # gives true, because tw owns ti
    assert_equal(ti._destroyed?, true)

    # But the item is released when we take it and add:
    tw = RBA::QTreeWidgetItem::new
    ti = RBA::QTreeWidgetItem::new
    tw.addChild(ti)
    assert_equal(tw.childCount, 1)
    ti = nil
    # force delete of the ti item unless it's referenced
    ti = RBA::QTreeWidgetItem::new
    GC.start
    # gives 1, because the tree widget item is kept by
    # the tree widget:
    assert_equal(tw.childCount, 1)

    ti = tw.takeChild(0)
    tw._destroy
    # gives false, because we took ti and tw no longer owns it
    assert_equal(ti._destroyed?, false)

    # And we can destroy a child too
    tw = RBA::QTreeWidgetItem::new
    ti = RBA::QTreeWidgetItem::new
    tw.addChild(ti)
    assert_equal(tw.childCount, 1)
    ti._destroy
    assert_equal(tw.childCount, 0)

  end

  def test_42

    # QKeyEvent and related issues

    ef = EventFilter::new

    widget = RBA::QLineEdit::new
    widget.setText("ABC")

    RBA::QApplication::processEvents

    widget.installEventFilter(ef)

    ke = RBA::QKeyEvent::new(RBA::QEvent::KeyPress, RBA::Qt::Key_O.to_i, RBA::Qt::ShiftModifier, "O")
    RBA::QCoreApplication::postEvent(widget, ke)

    ke = RBA::QKeyEvent::new(RBA::QEvent::KeyPress, RBA::Qt::Key_Left.to_i, RBA::Qt::NoModifier)
    RBA::QCoreApplication::postEvent(widget, ke)

    ke = RBA::QKeyEvent::new(RBA::QEvent::KeyPress, RBA::Qt::Key_P.to_i, RBA::Qt::NoModifier, "p")
    RBA::QCoreApplication::postEvent(widget, ke)

    RBA::QApplication::processEvents

    GC.start

    assert_equal(ef.log.select { |s| s !~ /RBA::QKeyEvent(_Native)?: ShortcutOverride/ }.join("\n"), "RBA::QKeyEvent: KeyPress (6)\nRBA::QKeyEvent: KeyPress (6)\nRBA::QKeyEvent: KeyPress (6)")

    ef = nil
    ef = EventFilter::new
    GC.start

    assert_equal(widget.text, "ABCpO")
    ef = nil
    widget = nil
    GC.start

  end

  def test_43

    # QHash bindings 

    slm = MyStandardItemModel::new
    rn = slm.roleNames
    assert_equal(rn.keys.sort.collect { |k| "#{k}=>\"#{rn[k]}\"" }.join(", "), "0=>\"display\", 1=>\"decoration\", 2=>\"edit\", 3=>\"toolTip\", 4=>\"statusTip\", 5=>\"whatsThis\"")
    rn[7] = "blabla"
    slm.srn(rn)
    rn = slm.roleNames
    assert_equal(rn.keys.sort.collect { |k| "#{k}=>\"#{rn[k]}\"" }.join(", "), "0=>\"display\", 1=>\"decoration\", 2=>\"edit\", 3=>\"toolTip\", 4=>\"statusTip\", 5=>\"whatsThis\", 7=>\"blabla\"")

  end

  def test_44

    # Ability to monitor native child objects

    parent = RBA::QScrollArea::new
    parent.show  # this makes resize actually change the widget's size
    child = parent.viewport

    # ensure parent and child are working
    assert_equal(child._destroyed?, false)

    parent.resize(200, 200)
    assert_equal(child.width() > 100, true)
    assert_equal(child.height() > 100, true)
    
    parent.resize(100, 100)
    assert_equal(child.width() <= 100, true)
    assert_equal(child.height() <= 100, true)

    # now if we delete the parent, the child needs to become disconnected

    parent._destroy()
    assert_equal(parent._destroyed?, true)
    assert_equal(child._destroyed?, true)

  end

  def test_45

    # Ability to connect to signals while ignoring arguments and 
    # to emit signals

    b = RBA::QPushButton::new

    triggered = ""
    b.clicked { |checked| triggered += (checked ? "1" : "0") }

    assert_equal(triggered, "")
    b.emit_clicked(true)
    assert_equal(triggered, "1")
    b.emit_clicked(false)
    assert_equal(triggered, "10")

    b.clicked { triggered += "*" }

    b.emit_clicked(true)
    assert_equal(triggered, "10*")
    b.emit_clicked(false)
    assert_equal(triggered, "10**")

  end

end 

load("test_epilogue.rb")
