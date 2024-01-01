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

testlog = ""

# an event filter class

class EventFilter(pya.QObject):

  _log = []

  def log(self):
    return self._log

  def eventFilter(self, obj, event):
    self._log.append(type(event).__name__ + ": " + repr(event.type()))
    pya.QObject.eventFilter(self, obj, event)

# QAction implementation

class MyAction(pya.QAction):

  ce = None

  def __init__(self, p, n):
    pya.QAction.__init__(self, p)
    self.objectName = n
    ce = None

  def childEvent(self, ev):
    if self.ce:
      self.ce(ev)
  
  def on_child_event(self, _ce):
    self.ce = _ce
  
class MyStandardItemModel(pya.QStandardItemModel):
  # make this method public
  def srn(self, rn):
    if hasattr(self.__class__, "setRoleNames"):
      self.setRoleNames(rn)
    else:
      self.setItemRoleNames(rn)

# Another event filter 

class MyObject(pya.QObject):

  ef = None

  def eventFilter(self, watched, event):
    if self.ef(watched, event):
      return True
    return pya.QObject.eventFilter(self, watched, event)

  def on_event_filter(self, ef):
    self.ef = ef
  
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
  
# The Qt binding tests

class QtBindingTest(unittest.TestCase):

  def test_00(self):

    # all references of PA are released now:
    pass

  # ...

  def test_10(self):

    a = MyAction(None, "a")
    a.text = "mytext"
    a.checkable = True
    self.assertEqual(a.isChecked(), False)
    a.checked = True
    self.assertEqual(a.text, "mytext")
    self.assertEqual(a.objectName, "a")
    a.text += "."
    self.assertEqual(a.text, "mytext.")
    self.assertEqual(a.checked, True)

    global testlog
    testlog = ""

    def f(checked):
      global testlog
      testlog += "[" + str(checked) + "]"

    a.triggered(f)
    self.assertEqual(testlog, "")
    a.trigger() # also toggles checked state
    self.assertEqual(testlog, "[False]")
    testlog = ""
    a.trigger() # also toggles checked state
    self.assertEqual(testlog, "[True]")

  def test_11(self):

    a = pya.QAction(None)
    aa = MyAction(a, "aa")
    self.assertEqual(aa.objectName, "aa")

    # destroying a will also destroy aa
    a.destroy()
    self.assertEqual(a._destroyed(), True)
    self.assertEqual(aa._destroyed(), True)

  def test_12(self):

    a = pya.QAction(None)
    aa = pya.QAction(a)
    aa.objectName = "aa"

    # destroying a will also destroy aa
    a = None

    self.assertEqual(aa._destroyed(), True)

  def test_13(self):

    a = pya.QAction(None)
    aa = pya.QAction(a)
    aa.objectName = "aa"
    aa.text = "aatext"

    cc = []
    for c in a.children():
      cc.append(c.objectName)
    self.assertEqual(",".join(cc), "aa")

    # aa now is kept by a
    aa = None

    # fetch aa again
    for c in a.children():
      if c.objectName == "aa":
        aa = c
    self.assertEqual(aa != None, True)
    self.assertEqual(type(aa), pya.QAction)
    self.assertEqual(aa.text, "aatext")
    self.assertEqual(aa._destroyed(), False)

  def test_20(self):

    global no_event
    global ce_log

    no_event = False

    def event_filter(watched, event):
      global no_event
      return no_event

    ef = MyObject()
    ef.on_event_filter(event_filter)

    def child_event(ce):
      global ce_log
      ce_log.append(str(ce.added()) + ":" + ce.child().objectName)

    ce_log = []
    a = MyAction(None, "a")
    a.on_child_event(child_event)

    a.installEventFilter(ef)

    aa = MyAction(None, "aa")
    self.assertEqual(",".join(ce_log), "")
    aa.setParent(a)
    self.assertEqual(",".join(ce_log), "True:aa")
    ce_log = []

    # destroy aa
    aa.destroy()
    aa = None
    self.assertEqual(",".join(ce_log), "False:aa")
    ce_log = []

    no_event = True
    aa = MyAction(None, "aa")
    aa.setParent(a)
    self.assertEqual(",".join(ce_log), "")
    ce_log = []

    no_event = False
    aa.destroy()
    aa = None
    self.assertEqual(",".join(ce_log), "False:aa")
    ce_log = []

  def test_30(self):

    # dialog construction, cleanup, object dependency ...

    mw = None 

    dialog = pya.QDialog(mw)
    label = pya.QLabel(dialog)
    layout = pya.QHBoxLayout(dialog)
    layout.addWidget(label)

    dialog = pya.QDialog(mw)
    label = pya.QLabel(dialog)
    layout = pya.QHBoxLayout(dialog)
    layout.addWidget(label)
    label.destroy()

    dialog = pya.QDialog(mw)
    label = pya.QLabel(dialog)
    layout = pya.QHBoxLayout(dialog)
    layout.addWidget(label)
    layout.destroy()

    dialog = pya.QDialog(mw)
    label = pya.QLabel(dialog)
    layout = pya.QHBoxLayout(dialog)
    layout.addWidget(label)
    dialog.destroy()

    dialog = pya.QDialog(mw)
    label = pya.QLabel(dialog)
    layout = pya.QHBoxLayout(dialog)
    layout.addWidget(label)

    dialog = None
    label = None
    layout = None

  def test_31(self):

    # Optional arguments, enums, QFlag's

    mw = None

    mb = pya.QMessageBox(pya.QMessageBox.Critical, "title", "text")
    self.assertEqual(mb.icon.to_i() != pya.QMessageBox.Warning.to_i(), True)
    self.assertEqual(mb.icon.to_i() == pya.QMessageBox.Critical.to_i(), True)
    self.assertEqual(mb.standardButtons.to_i() == pya.QMessageBox.NoButton.to_i(), True)

    mb = pya.QMessageBox(pya.QMessageBox.Critical, "title", "text", pya.QMessageBox.Ok)
    self.assertEqual(mb.standardButtons.to_i() == pya.QMessageBox.Ok.to_i(), True)

    mb = pya.QMessageBox(pya.QMessageBox.Critical, "title", "text", pya.QMessageBox.Ok | pya.QMessageBox.Cancel)
    self.assertEqual(mb.standardButtons.to_i() == pya.QMessageBox.Ok.to_i() + pya.QMessageBox.Cancel.to_i(), True)

  def test_40(self):

    # Lifetime management of objects/methods not using QObject.parent
    # QTreeWidget (parent)/QTreeWidgetItem (child)

    # constructor with parent-like argument:
    tw = pya.QTreeWidget()
    ti = pya.QTreeWidgetItem(tw)
    # strange, but true:
    self.assertEqual(ti.parent(), None)
    self.assertEqual(tw.topLevelItemCount, 1)
    ti = None
    # gives 1, because the tree widget item is kept by
    # the tree widget:
    self.assertEqual(tw.topLevelItemCount, 1)

    # the tree item belongs to the widget, hence it's destroyed with
    # the widget
    ti = tw.topLevelItem(0)
    tw._destroy()
    # gives true, because tw owns ti too.
    self.assertEqual(ti._destroyed(), True)

    # The same works for insert too
    tw = pya.QTreeWidget()
    ti = pya.QTreeWidgetItem()
    tw.insertTopLevelItem(0, ti)
    self.assertEqual(tw.topLevelItemCount, 1)
    ti = None
    # gives 1, because the tree widget item is kept by
    # the tree widget:
    self.assertEqual(tw.topLevelItemCount, 1)

    # the tree item belongs to the widget, hence it's destroyed with
    # the widget
    ti = tw.topLevelItem(0)
    tw._destroy()
    # gives true, because tw owns ti
    self.assertEqual(ti._destroyed(), True)

    # And add:
    tw = pya.QTreeWidget()
    ti = pya.QTreeWidgetItem()
    tw.addTopLevelItem(ti)
    self.assertEqual(tw.topLevelItemCount, 1)
    ti = None
    # gives 1, because the tree widget item is kept by
    # the tree widget:
    self.assertEqual(tw.topLevelItemCount, 1)

    # the tree item belongs to the widget, hence it's destroyed with
    # the widget
    ti = tw.topLevelItem(0)
    tw._destroy()
    # gives true, because tw owns ti
    self.assertEqual(ti._destroyed(), True)

    # But the item is released when we take it and add:
    tw = pya.QTreeWidget()
    ti = pya.QTreeWidgetItem()
    tw.addTopLevelItem(ti)
    self.assertEqual(tw.topLevelItemCount, 1)
    ti = None
    # gives 1, because the tree widget item is kept by
    # the tree widget:
    self.assertEqual(tw.topLevelItemCount, 1)

    ti = tw.takeTopLevelItem(0)
    tw._destroy()
    # gives false, because we took ti and tw no longer owns it
    self.assertEqual(ti._destroyed(), False)

    # And we can destroy a child too
    tw = pya.QTreeWidget()
    ti = pya.QTreeWidgetItem()
    tw.addTopLevelItem(ti)
    self.assertEqual(tw.topLevelItemCount, 1)
    ti._destroy()
    self.assertEqual(tw.topLevelItemCount, 0)

  def test_41(self):

    # Lifetime management of objects/methods not using QObject.parent
    # QTreeWidgetItem (parent)/QTreeWidgetItem (child)

    # constructor with parent-like argument (supported by QObject parent/child relationship):
    tw = pya.QTreeWidgetItem()
    ti = pya.QTreeWidgetItem(tw)
    # that's not QObject.parent - this one still is 0 (not seen by RBA)
    self.assertEqual(ti.parent(), tw)
    self.assertEqual(tw.childCount(), 1)
    ti = None
    # gives 1, because the tree widget item is kept by
    # the tree widget:
    self.assertEqual(tw.childCount(), 1)

    # the tree item belongs to the widget, hence it's destroyed with
    # the widget
    ti = tw.child(0)
    tw._destroy()
    # gives true, because tw owns ti too.
    self.assertEqual(ti._destroyed(), True)

    # The same works for insert too
    tw = pya.QTreeWidgetItem()
    ti = pya.QTreeWidgetItem()
    tw.insertChild(0, ti)
    self.assertEqual(tw.childCount(), 1)
    ti = None
    # gives 1, because the tree widget item is kept by
    # the tree widget:
    self.assertEqual(tw.childCount(), 1)

    # the tree item belongs to the widget, hence it's destroyed with
    # the widget
    ti = tw.child(0)
    tw._destroy()
    # gives true, because tw owns ti
    self.assertEqual(ti._destroyed(), True)

    # And add:
    tw = pya.QTreeWidgetItem()
    ti = pya.QTreeWidgetItem()
    tw.addChild(ti)
    self.assertEqual(tw.childCount(), 1)
    ti = None
    # gives 1, because the tree widget item is kept by
    # the tree widget:
    self.assertEqual(tw.childCount(), 1)

    # the tree item belongs to the widget, hence it's destroyed with
    # the widget
    ti = tw.child(0)
    tw._destroy()
    # gives true, because tw owns ti
    self.assertEqual(ti._destroyed(), True)

    # But the item is released when we take it and add:
    tw = pya.QTreeWidgetItem()
    ti = pya.QTreeWidgetItem()
    tw.addChild(ti)
    self.assertEqual(tw.childCount(), 1)
    ti = None
    # gives 1, because the tree widget item is kept by
    # the tree widget:
    self.assertEqual(tw.childCount(), 1)

    ti = tw.takeChild(0)
    tw._destroy()
    # gives false, because we took ti and tw no longer owns it
    self.assertEqual(ti._destroyed(), False)

    # And we can destroy a child too
    tw = pya.QTreeWidgetItem()
    ti = pya.QTreeWidgetItem()
    tw.addChild(ti)
    self.assertEqual(tw.childCount(), 1)
    ti._destroy()
    self.assertEqual(tw.childCount(), 0)

  def test_42(self):

    # QKeyEvent and related issues

    ef = EventFilter()

    widget = pya.QLineEdit()
    widget.setText("ABC")

    pya.QApplication.processEvents()

    widget.installEventFilter(ef)

    ke = pya.QKeyEvent(pya.QEvent.KeyPress, pya.Qt.Key_O.to_i(), pya.Qt.ShiftModifier, "O")
    pya.QCoreApplication.postEvent(widget, ke)

    ke = pya.QKeyEvent(pya.QEvent.KeyPress, pya.Qt.Key_Left.to_i(), pya.Qt.NoModifier)
    pya.QCoreApplication.postEvent(widget, ke)

    ke = pya.QKeyEvent(pya.QEvent.KeyPress, pya.Qt.Key_P.to_i(), pya.Qt.NoModifier, "p")
    pya.QCoreApplication.postEvent(widget, ke)

    pya.QApplication.processEvents()

    s1 = "QKeyEvent: ShortcutOverride (51)\nQKeyEvent: KeyPress (6)\nQKeyEvent: ShortcutOverride (51)\nQKeyEvent: KeyPress (6)\nQKeyEvent: ShortcutOverride (51)\nQKeyEvent: KeyPress (6)"
    s2 = "QKeyEvent: KeyPress (6)\nQKeyEvent: KeyPress (6)\nQKeyEvent: KeyPress (6)"
    s3 = "QKeyEvent_Native: ShortcutOverride (51)\nQKeyEvent: KeyPress (6)\nQKeyEvent_Native: ShortcutOverride (51)\nQKeyEvent: KeyPress (6)\nQKeyEvent_Native: ShortcutOverride (51)\nQKeyEvent: KeyPress (6)"
    self.assertIn("\n".join(ef.log()), (s1, s2, s3))

    ef = None

    self.assertEqual(widget.text, "ABCpO")
    widget = None

  def test_43(self):

    # QHash bindings 

    slm = MyStandardItemModel()
    rn = slm.roleNames()
    if sys.version_info < (3, 0):
      self.assertEqual(map2str(rn), "{0: display, 1: decoration, 2: edit, 3: toolTip, 4: statusTip, 5: whatsThis}")
    else:
      self.assertEqual(map2str(rn), "{0: b'display', 1: b'decoration', 2: b'edit', 3: b'toolTip', 4: b'statusTip', 5: b'whatsThis'}")
    rnNew = slm.roleNames()
    rnNew[7] = "blabla"
    slm.srn(rnNew)
    rn = slm.roleNames()
    if sys.version_info < (3, 0):
      self.assertEqual(map2str(rn), "{0: display, 1: decoration, 2: edit, 3: toolTip, 4: statusTip, 5: whatsThis, 7: blabla}")
    else:
      self.assertEqual(map2str(rn), "{0: b'display', 1: b'decoration', 2: b'edit', 3: b'toolTip', 4: b'statusTip', 5: b'whatsThis', 7: b'blabla'}")

  def test_44(self):

    # Ability to monitor native child objects

    parent = pya.QScrollArea()
    parent.show()  # this makes resize actually change the widget's size
    child = parent.viewport

    # ensure parent and child are working
    self.assertEqual(child._destroyed(), False)

    parent.resize(200, 200)
    self.assertEqual(child.width > 100, True)
    self.assertEqual(child.height > 100, True)
    
    parent.resize(100, 100)
    self.assertEqual(child.width <= 100, True)
    self.assertEqual(child.height <= 100, True)

    # now if we delete the parent, the child needs to become disconnected

    parent._destroy()
    self.assertEqual(parent._destroyed(), True)
    self.assertEqual(child._destroyed(), True)

  def test_45(self):

    triggered = ""

    class TriggerLog:
      triggered = ""
      def triggered1(self, b):
        if b:
          self.triggered += "1"
        else:
          self.triggered += "0"
      def triggered0(self):
        self.triggered += "*"

    # Ability to connect to signals while ignoring arguments and 
    # to emit signals

    b = pya.QPushButton()

    log = TriggerLog()

    b.clicked(log.triggered1)

    self.assertEqual(log.triggered, "")
    b.emit_clicked(True)
    self.assertEqual(log.triggered, "1")
    b.emit_clicked(False)
    self.assertEqual(log.triggered, "10")

    b.clicked(log.triggered0)

    b.emit_clicked(True)
    self.assertEqual(log.triggered, "10*")
    b.emit_clicked(False)
    self.assertEqual(log.triggered, "10**")

    # We do the same with free functions since they behave differently in Python:

    global trigger_log 
    trigger_log = ""

    def triggered_f0():
      global trigger_log
      trigger_log += "x"
      
    def triggered_f1(b):
      global trigger_log
      if b:
        trigger_log += "+"
      else:
        trigger_log += "-"
        
    b.clicked(triggered_f1)

    self.assertEqual(trigger_log, "")
    b.emit_clicked(True)
    self.assertEqual(trigger_log, "+")
    b.emit_clicked(False)
    self.assertEqual(trigger_log, "+-")

    b.clicked(triggered_f0)

    b.emit_clicked(True)
    self.assertEqual(trigger_log, "+-x")
    b.emit_clicked(False)
    self.assertEqual(trigger_log, "+-xx")

  def test_51(self):

    # issue #707 (QJsonValue constructor ambiguous)
    if "QJsonValue" in pya.__dict__:

      v = pya.QJsonValue("hello")
      self.assertEqual(v.toString(), "hello")
      self.assertEqual(v.toVariant(), "hello")
      self.assertEqual(v.toInt(), 0)

      v = pya.QJsonValue(17)
      self.assertEqual(v.toString(), "")
      self.assertEqual(v.toVariant(), 17)
      self.assertEqual(v.toInt(), 17)

      v = pya.QJsonValue(2.5)
      self.assertEqual(v.toString(), "")
      self.assertEqual(v.toVariant(), 2.5)
      self.assertEqual(v.toDouble(), 2.5)

      v = pya.QJsonValue(True)
      self.assertEqual(v.toString(), "")
      self.assertEqual(v.toVariant(), True)
      self.assertEqual(v.toBool(), True)

  def test_52(self):

    # issue #708 (Image serialization to QByteArray)
    img = pya.QImage(10, 10, pya.QImage.Format_Mono)
    img.fill(0)

    buf = pya.QBuffer()
    img.save(buf, "PNG")

    self.assertEqual(len(buf.data) > 100, True)
    self.assertEqual(buf.data[0:8], b'\x89PNG\r\n\x1a\n')

  def test_53(self):

    # issue #771 (QMimeData not working)
    mimeData = pya.QMimeData()
    mimeData.setData("application/json",'{"test":"test"}')
    jsonData = mimeData.data("application/json");
    if sys.version_info < (3, 0):
      self.assertEqual(str(jsonData), '{"test":"test"}')
    else:
      self.assertEqual(str(jsonData), 'b\'{"test":"test"}\'')

  def test_54(self):

    # issue #1029 (Crash for QBrush passed to setData)
    item = pya.QTreeWidgetItem()
    item.setBackground(0, pya.QBrush(pya.QColor(0xFF, 0xFF, 0x00)))
    self.assertEqual(item.background(0).color.red, 255)
    self.assertEqual(item.background(0).color.green, 255)
    self.assertEqual(item.background(0).color.blue, 0)

# run unit tests
if __name__ == '__main__':
  suite = unittest.TestLoader().loadTestsFromTestCase(QtBindingTest)

  if not unittest.TextTestRunner(verbosity = 1).run(suite).wasSuccessful():
    sys.exit(1)

