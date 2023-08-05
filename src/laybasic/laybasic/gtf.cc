
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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

#if defined(HAVE_QT)

#include "gtf.h"
#include "tlException.h"
#include "tlLog.h"
#include "tlXMLParser.h"
#include "tlAssert.h"
#include "tlVariantUserClasses.h"

#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QInputEvent>
#include <QWidget>
#include <QToolBar>
#include <QMenuBar>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QTreeView>
#include <QListView>
#include <QCheckBox>
#include <QScrollBar>
#include <QDialog>
#include <QMainWindow>
#include <QRadioButton>
#include <QPushButton>
#include <QSpinBox>
#include <QPainter>
#include <QBitmap>
#include <QFile>
#include <QBuffer>
#include <QTimer>
#include <QXmlContentHandler>

#include <fstream>
#include <sstream>
#include <map>
#include <memory>

// --------------------------------------------------------------
//  A helper class that allows putting a QImage into a tl::Variant

namespace gtf
{

static tl::VariantUserClassImpl<QImage> qimage_class_instance;

tl::Variant
image_to_variant (const QImage &img)
{
  return tl::Variant (new QImage (img), &qimage_class_instance, true);
}

class GtfEventFilter;

// --------------------------------------------------------------
//  Convert a string to a form suitable for XML output

static std::string
escape_string (const char *cp)
{
  std::string r;
  r.reserve (strlen (cp) * 2);
  while (*cp) {
    if (*cp == '&') {
      r += "&amp;";
    } else if (*cp == '<') {
      r += "&lt;";
    } else if (*cp == '>') {
      r += "&gt;";
    } else {
      r += *cp;
    }
    ++cp;
  }
  return r;
}

// --------------------------------------------------------------
//  Widget to path conversion and back

inline bool 
is_widget (QObject *o)
{
   return (dynamic_cast<const QDialog *> (o) != 0 || dynamic_cast<const QMainWindow *> (o) != 0 || dynamic_cast<const QWidget *> (o) != 0);
}

static void 
dump_children (QObject *obj, int level = 0)
{
  QObjectList children = obj->children ();
  std::string info;
  for (int i = 0; i < level; ++i) {
    info += "  ";
  }
  if (obj->objectName ().isEmpty ()) {
    info += "<unnamed>";
  } else {
    info += tl::to_string (obj->objectName ());
  }
  info += " (";
  info += obj->metaObject ()->className ();
  info += tl::sprintf(") - %p", (size_t)obj);
  tl::info << info;
  for (QObjectList::const_iterator child = children.begin (); child != children.end (); ++child) {
    if (is_widget (*child)) {
      dump_children (*child, level + 1);
    }
  }
}

void 
dump_widget_tree ()
{
  QWidgetList tl_widgets = QApplication::topLevelWidgets (); 

  tl::info << tl::to_string (QObject::tr ("Widget tree:"));
  for (QWidgetList::const_iterator tl = tl_widgets.begin (); tl != tl_widgets.end (); ++tl) { 
    if (is_widget (*tl)) {
      dump_children (*tl); 
    }
  }
  tl::info << "";
}

static bool extract_widget_path (tl::Extractor &x, std::string &name, std::string &cls, int &nwidget)
{
  name.clear ();
  cls.clear ();

  nwidget = 1;

  if (! x.at_end ()) {
    x.read (name, "(.#");
    if (*x == '(') {
      ++x;
      x.read (cls, ")#");
      if (*x == ')') {
        ++x;
      }
    }
    if (*x == '#') {
      ++x;
      x.read (nwidget);
    } 
  }

  if (! x.test (".")) {
    if (! x.at_end ()) {
      throw tl::Exception (tl::to_string (QObject::tr ("Invalid widget path: expected '.'")));
    }
    return false;
  } else {
    return true;
  }
}

static QWidget *
widget_from_path (const char *p, int xml_line)
{
  tl::Extractor x (p);

  std::string name;
  std::string cls;
  int nwidget = 1;
  bool more = false;

  QObject *w = 0;
  do {

    more = extract_widget_path (x, name, cls, nwidget);

    int n = nwidget;
    QObject *pw = w;
    if (w == 0) {
      QWidgetList tl_widgets = QApplication::topLevelWidgets ();
      for (QWidgetList::const_iterator tl = tl_widgets.begin (); tl != tl_widgets.end (); ++tl) {
        if (is_widget (*tl) && (*tl)->objectName () == tl::to_qstring (name) && (cls.empty () || cls == (*tl)->metaObject ()->className ()) && --n == 0) {
          w = *tl;
          break;
        }
      }
    } else {
      w = 0;
      QObjectList children = pw->children ();
      for (QObjectList::const_iterator child = children.begin (); child != children.end (); ++child) {
        if (is_widget (*child) && (*child)->objectName () == tl::to_qstring (name) && (cls.empty () || cls == (*child)->metaObject ()->className ()) && --n == 0) {
          w = *child;
          break;
        }
      }
    }

    if (w == 0) {
      std::string names;
      if (pw == 0) {
        QWidgetList tl_widgets = QApplication::topLevelWidgets ();
        for (QWidgetList::const_iterator tl = tl_widgets.begin (); tl != tl_widgets.end (); ++tl) {
          if (is_widget (*tl)) {
            if (! names.empty ()) {
              names += ",";
            }
            names += tl::to_string ((*tl)->objectName ());
            names += "(";
            names += (*tl)->metaObject ()->className ();
            names += ")";
          }
        }
      } else {
        QObjectList children = pw->children ();
        for (QObjectList::const_iterator child = children.begin (); child != children.end (); ++child) {
          if (is_widget (*child)) {
            if (! names.empty ()) {
              names += ",";
            }
            names += tl::to_string ((*child)->objectName ());
            names += "(";
            names += (*child)->metaObject ()->className ();
            names += ")";
          }
        }
      }
      dump_widget_tree ();
      throw tl::Exception (tl::to_string (QObject::tr ("Widget path resolution failed: '%s' is not a valid component with index %d in path '%s' (line %d)\nAlternatives are: %s")), name, nwidget, p, xml_line, names);
    }

  } while (more);

  QWidget *target_widget = dynamic_cast<QWidget *> (w);
  if (! target_widget) {
    throw tl::Exception (tl::to_string (QObject::tr ("Invalid widget path '%s': does not lead to a widget (line %d)")), p, xml_line);
  }
  return target_widget;
}

static std::string 
widget_to_path (QWidget *w, const char *pf = 0)
{
  std::string n = tl::to_string (w->objectName ());
  std::string cls = w->metaObject ()->className ();
  QWidget *pw = w->parentWidget ();

  int i = 1;
  if (pw) {
    QObjectList children = pw->children ();
    for (QObjectList::const_iterator child = children.begin (); child != children.end (); ++child) {
      if (dynamic_cast<QDialog *> (*child) != 0 || dynamic_cast<QMainWindow *> (*child) != 0 || dynamic_cast <QWidget *> (*child) != 0) { 
        if (*child == w) {
          break;
        }
        if ((*child)->objectName () == tl::to_qstring (n) && cls == (*child)->metaObject ()->className ()) {
          ++i;
        }
      }
    }
  } else {
    QWidgetList tlw = QApplication::topLevelWidgets ();
    for (QWidgetList::const_iterator itl = tlw.begin (); itl != tlw.end (); ++itl) {
      //  only QDialog or QMainWindow ancestors count as valid top level widgets
      if (dynamic_cast<QDialog *> (*itl) != 0 || dynamic_cast<QMainWindow *> (*itl) != 0 || dynamic_cast <QWidget *> (*itl) != 0) { 
        if (*itl == w) {
          break;
        }
        if ((*itl)->objectName () == tl::to_qstring (n) && cls == (*itl)->metaObject ()->className ()) {
          ++i;
        }
      }
    }
  }
  n += tl::sprintf ("(%s)", cls);
  if (i > 1) {
    n += tl::sprintf ("#%d", i);
  }

  if (pf) {
    n += ".";
    n += pf;
  }

  if (pw) {
    return widget_to_path (pw, n.c_str ());
  } else {
    return n;
  }
}

// --------------------------------------------------------------
//  Widget to track the mouse pointer

class MouseTrackerWidget
  : public QWidget
{
public: 
  MouseTrackerWidget (QWidget *parent);

  static MouseTrackerWidget *instance ();

  void set (const QMouseEvent &me);

  virtual void paintEvent (QPaintEvent *pe);

private:
  QPixmap *mp_current_pixmap;
  QPixmap m_basic_pm;
  QPixmap m_lb_pm;
  QPixmap m_mb_pm;
  QPixmap m_rb_pm;
};

//  pixmaps

MouseTrackerWidget::MouseTrackerWidget (QWidget *parent)
  : QWidget (parent, 
             Qt::Window | Qt::X11BypassWindowManagerHint | Qt::FramelessWindowHint),
    m_basic_pm (QString::fromUtf8 (":/gtf_basic.png")),
    m_lb_pm (QString::fromUtf8 (":/gtf_lb.png")),
    m_mb_pm (QString::fromUtf8 (":/gtf_mb.png")),
    m_rb_pm (QString::fromUtf8 (":/gtf_rb.png"))
{
  mp_current_pixmap = &m_basic_pm;

  setAttribute (Qt::WA_NoSystemBackground);
  setAttribute (Qt::WA_OpaquePaintEvent);

  resize (m_basic_pm.size ());

  setMask (m_basic_pm.mask ());
}

MouseTrackerWidget *
MouseTrackerWidget::instance ()
{
  static MouseTrackerWidget *inst = 0;

  if (inst == 0) {
    inst = new MouseTrackerWidget (0);
  }
  return inst;
}

void 
MouseTrackerWidget::set (const QMouseEvent &me)
{
  mp_current_pixmap = &m_basic_pm;

  if (me.button () == Qt::RightButton) {
    mp_current_pixmap = &m_rb_pm;
  } else if (me.button () == Qt::MiddleButton) {
    mp_current_pixmap = &m_mb_pm;
  } else if (me.button () == Qt::LeftButton) {
    mp_current_pixmap = &m_lb_pm;
  } else if (me.type () == QEvent::MouseMove) {
    if (me.buttons () & Qt::RightButton) {
      mp_current_pixmap = &m_rb_pm;
    } else if (me.buttons () & Qt::MiddleButton) {
      mp_current_pixmap = &m_mb_pm;
    } else if (me.buttons () & Qt::LeftButton) {
      mp_current_pixmap = &m_lb_pm;
    } 
  }

  show ();
  QPoint p = me.globalPos (); 
  move (p.x () - width () / 2, p.y () - 1);
  update ();
}

void 
MouseTrackerWidget::paintEvent (QPaintEvent * /*pe*/)
{
  QPainter painter (this);
  painter.drawPixmap (0, 0, *mp_current_pixmap);
}
  
// --------------------------------------------------------------
//  XML handler declaration

class GtfXmlHandler
  : public QXmlDefaultHandler
{
public:
  GtfXmlHandler (EventList *list);

  bool characters (const QString &ch);
  bool endElement (const QString &namespaceURI, const QString &localName, const QString &qName);
  bool startElement (const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts);
  bool error (const QXmlParseException &exception);
  bool fatalError (const QXmlParseException &exception);
  bool fatalError (const std::string &msg);
  bool warning (const QXmlParseException &exception);
  void setDocumentLocator (QXmlLocator *locator);

private:
  QXmlLocator *mp_locator;
  EventList *mp_list;
  std::vector<tl::Variant> m_data_stack;
  QString m_cdata;
  bool m_in_event;

  void enter_event (LogEventBase *event);
  void leave_event ();
};

// --------------------------------------------------------------
//  The log event base class implementation

static void
write_data (const tl::Variant &data, std::ostream &os, unsigned int level)
{
  if (data.is_list ()) {

    for (unsigned int i = 0; i < level; ++i) {
      os << "  ";
    }
    os << "<block>" << std::endl;

    for (tl::Variant::const_iterator b = data.begin (); b != data.end (); ++b) {
      write_data (*b, os, level + 1);
    }

    for (unsigned int i = 0; i < level; ++i) {
      os << "  ";
    }
    os << "</block>" << std::endl;

  } else {

    for (unsigned int i = 0; i < level; ++i) {
      os << "  ";
    }

    if (data.is_long ()) {
      os << "<int>" << data.to_long () << "</int>" << std::endl;
    } else if (data.is_a_string ()) {
      os << "<string>" << escape_string (data.to_string ()) << "</string>" << std::endl;
    } else if (data.is_user<QImage> ()) {

      QImage img (data.to_user<QImage> ());
      QByteArray ba;
      QBuffer buffer (&ba);
      buffer.open (QIODevice::WriteOnly);
      img.save (&buffer, "PNG");
      os << "<img>" << ba.toBase64 ().constData () << "</img>" << std::endl;

    } 
  }
}

void 
LogEventBase::write (std::ostream &os, bool with_endl) const
{
  std::vector< std::pair<std::string, std::string> > attrs;
  attributes (attrs);

  os << "  <" << name ();
  for (std::vector< std::pair<std::string, std::string> >::const_iterator a = attrs.begin (); a != attrs.end (); ++a) {
    os << " " << a->first << "=\"" << a->second << "\"";
  }

  if (! data ().is_nil () && ! (data ().is_list () && data ().get_list ().empty ())) {
    os << ">" << std::endl;
    if (data ().is_list ()) {
      for (tl::Variant::const_iterator b = data ().begin (); b != data ().end (); ++b) {
        write_data (*b, os, 2);
      }
    } else {
      write_data (data (), os, 2);
    }
    os << "  </" << name () << ">";
  } else {
    os << "/>";
  }

  if (with_endl) {
    os << std::endl;
  }

}

// --------------------------------------------------------------
//  The log event specializations

class LogTargetedEvent
  : public LogEventBase
{
public:
  LogTargetedEvent (const std::string &target, int xml_line = 0)
    : LogEventBase (xml_line), m_target (target)
  {
    //  .. nothing yet ..
  }

  const std::string &target () const
  {
    return m_target;
  }

  virtual void attributes (std::vector< std::pair<std::string, std::string> > &attr) const
  {
    attr.push_back (std::make_pair (std::string ("target"), m_target));
  }
  
  bool equals (const LogEventBase &b) const
  {
    const LogTargetedEvent *be = dynamic_cast <const LogTargetedEvent *> (&b);
    if (! be) {
      return false;
    }

    std::string name1, name2, cls1, cls2;
    int nwidget1, nwidget2;
    bool more1, more2;

    tl::Extractor ex1 (m_target.c_str ());
    tl::Extractor ex2 (be->m_target.c_str ());
    do {

      more1 = extract_widget_path (ex1, name1, cls1, nwidget1);
      more2 = extract_widget_path (ex2, name2, cls2, nwidget2);

      if (name1 != name2) {
        return false;
      }

      //  transition from non-class to class-based spec.:
      if (cls1.empty () == cls2.empty ()) {
        if (nwidget1 != nwidget2) {
          return false;
        }
      }

    } while (more1 && more2);
    return !more1 && !more2;
  }

  QWidget *target_widget () const
  {
    return widget_from_path (m_target.c_str (), m_xml_line);
  }

private:
  std::string m_target;
};

class LogMouseEvent 
  : public LogTargetedEvent
{
public:
  LogMouseEvent (const std::string &target, const QMouseEvent &me, int xml_line = 0)
    : LogTargetedEvent (target, xml_line),
#if QT_VERSION >= 0x60000
      m_mouse_event (new QMouseEvent (me.type (), me.position (), me.globalPosition (), me.button (), me.buttons (), me.modifiers ()))
#else
      m_mouse_event (new QMouseEvent (me.type (), me.pos (), me.button (), me.buttons (), me.modifiers ()))
#endif
  {
    //  .. nothing yet ..
  }

  virtual void issue_event () 
  {
    QWidget *target = target_widget ();
    
    if (m_mouse_event->type () == QEvent::MouseButtonPress) {
      target->setFocus (); 
    }

    QMouseEvent me (m_mouse_event->type (), m_mouse_event->pos (), m_mouse_event->globalPos (), m_mouse_event->button (), m_mouse_event->buttons (), m_mouse_event->modifiers ());
    MouseTrackerWidget::instance ()->set (me);
    Player::instance ()->issue_event (target, &me);
  }

  virtual const char *name () const
  { 
    const char *event_name = "";
    if (m_mouse_event->type () == QEvent::MouseMove) {
      event_name = "mouse_move";
    } else if (m_mouse_event->type() == QEvent::MouseButtonDblClick) {
      event_name = "mouse_button_dbl_click";
    } else if (m_mouse_event->type() == QEvent::MouseButtonPress) {
      event_name = "mouse_button_press";
    } else if (m_mouse_event->type() == QEvent::MouseButtonRelease) {
      event_name = "mouse_button_release";
    }
    return event_name;
  }

  virtual void attributes (std::vector< std::pair<std::string, std::string> > &attr) const
  {
    LogTargetedEvent::attributes (attr);

    attr.push_back (std::make_pair (std::string ("xpos"), tl::to_string (m_mouse_event->x ())));
    attr.push_back (std::make_pair (std::string ("ypos"), tl::to_string (m_mouse_event->y ())));
    if (m_mouse_event->type () == QEvent::MouseMove) {
      attr.push_back (std::make_pair (std::string ("buttons"), tl::sprintf ("%x", int (m_mouse_event->buttons ()))));
    } else {
      attr.push_back (std::make_pair (std::string ("button"), tl::sprintf ("%x", int (m_mouse_event->button ()))));
    }
    attr.push_back (std::make_pair (std::string ("modifiers"), tl::sprintf ("%x", int (m_mouse_event->modifiers ()))));
  }

  const QMouseEvent &event () const
  {
    return *m_mouse_event;
  }

  void move (const QPoint &p)
  {
    m_mouse_event.reset (new QMouseEvent (m_mouse_event->type (),
                                          m_mouse_event->pos () + p,
                                          m_mouse_event->globalPos () + p,
                                          m_mouse_event->button (),
                                          m_mouse_event->buttons (),
                                          m_mouse_event->modifiers ()));
  }

  bool equals (const LogEventBase &b) const
  {
    const LogMouseEvent *be = dynamic_cast <const LogMouseEvent *> (&b);
    if (! be) {
      return false;
    }

    return LogTargetedEvent::equals (b) && 
           m_mouse_event->type () == be->m_mouse_event->type () &&
           m_mouse_event->pos () == be->m_mouse_event->pos () &&
           m_mouse_event->modifiers () == be->m_mouse_event->modifiers () &&
           m_mouse_event->buttons () == be->m_mouse_event->buttons ();
  }

private:
  std::unique_ptr<QMouseEvent> m_mouse_event;
};

class LogKeyEvent 
  : public LogTargetedEvent
{
public:
  LogKeyEvent (const std::string &target, const QKeyEvent &ke, int xml_line = 0)
    : LogTargetedEvent (target, xml_line), m_key_event (new QKeyEvent (ke.type (), ke.key (), ke.modifiers ()))
  {
    //  .. nothing yet ..
  }

  virtual void issue_event () 
  {
    std::unique_ptr<QKeyEvent> ke (new QKeyEvent (m_key_event->type (), m_key_event->key (), m_key_event->modifiers ()));
    Player::instance ()->issue_event (target_widget (), ke.get ());
  }

  virtual const char *name () const
  {
    if (m_key_event->type() == QEvent::KeyPress) {
      return "key_press";
    } else {
      return "key_release";
    }
  }

  virtual void attributes (std::vector< std::pair<std::string, std::string> > &attr) const
  {
    LogTargetedEvent::attributes (attr);

    QChar ch;
    if (! m_key_event->text ().isEmpty ()) {
      ch = m_key_event->text ().at (0);
    }

    attr.push_back (std::make_pair (std::string ("key"), tl::sprintf ("%x", int (m_key_event->key ()))));
    attr.push_back (std::make_pair (std::string ("code"), tl::sprintf ("%x", int (ch.unicode ()))));
    attr.push_back (std::make_pair (std::string ("modifiers"), tl::sprintf ("%x", int (m_key_event->modifiers ()))));
  }

  bool equals (const LogEventBase &b) const
  {
    const LogKeyEvent *be = dynamic_cast <const LogKeyEvent *> (&b);
    if (! be) {
      return false;
    }

    return LogTargetedEvent::equals (b) && 
           m_key_event->modifiers () == be->m_key_event->modifiers () &&
           m_key_event->key () == be->m_key_event->key ();
  }

private:
  std::unique_ptr<QKeyEvent> m_key_event;
};

class LogActionEvent 
  : public LogTargetedEvent
{
public:
  LogActionEvent (const std::string &target, const std::string &action_name, int xml_line = 0)
    : LogTargetedEvent (target, xml_line), m_action_name (action_name)
  {
    //  .. nothing yet ..
  }

  virtual void issue_event () 
  {
    QList<QAction *> actions = target_widget ()->findChildren<QAction *> (tl::to_qstring (m_action_name));
    if (actions.size () == 0) {
      throw tl::Exception (tl::to_string (QObject::tr ("'%s' is not a valid action name (line %d)")), m_action_name, m_xml_line);
    } else {
      //  trigger the specified action
      (*(actions.begin ()))->trigger ();
    }
  }

  virtual const char *name () const
  {
    return "action";
  }

  virtual void attributes (std::vector< std::pair<std::string, std::string> > &attr) const
  {
    LogTargetedEvent::attributes (attr);
    attr.push_back (std::make_pair (std::string ("action"), m_action_name));
  }

  bool equals (const LogEventBase &b) const
  {
    const LogActionEvent *be = dynamic_cast <const LogActionEvent *> (&b);
    if (! be) {
      return false;
    }

    return LogTargetedEvent::equals (b) && 
           m_action_name == be->m_action_name;
  }

private:
  std::string m_action_name;
};

class LogResizeEvent 
  : public LogTargetedEvent
{
public:
  LogResizeEvent (const std::string &target, QSize size, QSize old_size, int xml_line = 0)
    : LogTargetedEvent (target, xml_line), m_size (size), m_old_size (old_size)
  {
    //  .. nothing yet ..
  }

  virtual void issue_event () 
  {
    target_widget ()->resize (m_size);
  }

  virtual const char *name () const
  {
    return "resize";
  }

  virtual void attributes (std::vector< std::pair<std::string, std::string> > &attr) const
  {
    LogTargetedEvent::attributes (attr);
    attr.push_back (std::make_pair (std::string ("xsize"), tl::to_string (m_size.width ())));
    attr.push_back (std::make_pair (std::string ("ysize"), tl::to_string (m_size.height ())));
    attr.push_back (std::make_pair (std::string ("xsize_old"), tl::to_string (m_old_size.width ())));
    attr.push_back (std::make_pair (std::string ("ysize_old"), tl::to_string (m_old_size.height ())));
  }

  QSize old_size () const
  {
    return m_old_size;
  }

  QSize size () const
  {
    return m_size;
  }
  
  bool equals (const LogEventBase &b) const
  {
    const LogResizeEvent *be = dynamic_cast <const LogResizeEvent *> (&b);
    if (! be) {
      return false;
    }

    return LogTargetedEvent::equals (b) && 
           m_size == be->m_size && 
           m_old_size == be->m_old_size;
  }

  virtual bool spontaneous () const
  {
    return true;
  }

private:
  QSize m_size;
  QSize m_old_size;
};

class LogProbeEvent 
  : public LogTargetedEvent
{
public:
  LogProbeEvent (const std::string &target, int xml_line)
    : LogTargetedEvent (target, xml_line)
  {
    //  .. nothing yet ..
  }

  LogProbeEvent (const std::string &target, const tl::Variant &d)
    : LogTargetedEvent (target, 0)
  {
    set_data (d);
  }

  virtual void issue_event () 
  {
    if (gtf::Recorder::instance () && gtf::Recorder::instance ()->recording ()) {

      QWidget *target = target_widget ();

      QEvent event (QEvent::MaxUser);
      event.ignore ();
      Player::instance ()->issue_event (target, &event);
      if (! event.isAccepted ()) {
        gtf::Recorder::instance ()->probe (target, gtf::Recorder::instance ()->probe_std (target));
      }

    }
  }

  virtual const char *name () const
  {
    return "probe";
  }
};

class LogErrorEvent 
  : public LogEventBase
{
public:
  LogErrorEvent (int xml_line)
    : LogEventBase (xml_line)
  {
    //  .. for XML reader ..
  }

  LogErrorEvent (std::string &text)
    : LogEventBase (0)
  {
    set_data (tl::Variant (text));
  }

  virtual void issue_event () 
  {
    //  .. error events are not "issued" ..
  }

  virtual const char *name () const
  {
    return "error";
  }

  virtual void attributes (std::vector< std::pair<std::string, std::string> > & /*attr*/) const
  {
    //  the error text is stored in the data
  }

  bool equals (const LogEventBase &b) const
  {
    return dynamic_cast <const LogErrorEvent *> (&b) != 0;
  }

private:
  std::string m_text;
};

// --------------------------------------------------------------
//  Implementation of action_connect and action_disconnect

/**
 *  @brief A helper class to specify a connection for monitoring 
 */
struct ConnectionSpec
{
public:
  ConnectionSpec (QAction *sender, const char *signal)
    : sender (sender), signal (signal)
  { }

  bool operator== (const ConnectionSpec &d) const
  {
    return sender == d.sender && signal == d.signal;
  }

  bool operator< (const ConnectionSpec &d) const
  {
    if (sender != d.sender) {
      return sender < d.sender;
    }
    if (signal != d.signal) {
      return signal < d.signal;
    }
    return false;
  }

  QAction *sender;
  std::string signal;
};

/**
 *  @brief A helper class to store ActionInterceptor objects associated with the connections monitored
 */
class ConnectionMap 
{
public:
  void register_connection (const ConnectionSpec &cs)
  {
    std::map<ConnectionSpec, std::pair<ActionInterceptor *, unsigned int> >::iterator i = m_map.find (cs);
    if (i == m_map.end ()) {
      ActionInterceptor *handler = new ActionInterceptor (cs.sender, cs.sender);
      m_map.insert (std::make_pair (cs, std::make_pair (handler, (unsigned int) 1)));
      QObject::connect (cs.sender, cs.signal.c_str (), handler, SLOT(triggered ()));
    } else {
      i->second.second++;
    }
  }

  void unregister_connection (const ConnectionSpec &cs)
  {
    std::map<ConnectionSpec, std::pair<ActionInterceptor *, unsigned int> >::iterator i = m_map.find (cs);
    tl_assert (i != m_map.end ());
    tl_assert (i->second.second > 0);
    if (--i->second.second == 0) {
      QObject::disconnect (cs.sender, cs.signal.c_str (), i->second.first, SLOT(triggered ()));
      delete i->second.first;
      m_map.erase (i);
    }
  }

private:
  std::map<ConnectionSpec, std::pair<ActionInterceptor *, unsigned int> > m_map;
};

//  The monitored connections
static ConnectionMap s_action_map;

void 
action_connect (QAction *action, const char *signal, QObject *receiver, const char *slot)
{
  if (Recorder::instance ()) {
    s_action_map.register_connection (ConnectionSpec (action, signal));
  }
  QObject::connect (action, signal, receiver, slot);
}

void 
action_disconnect (QAction *action, const char *signal, QObject *receiver, const char *slot)
{
  if (Recorder::instance ()) {
    s_action_map.unregister_connection (ConnectionSpec (action, signal));
  }
  QObject::disconnect (action, signal, receiver, slot);
}

// --------------------------------------------------------------
//  Implementation of ActionInterceptor

ActionInterceptor::ActionInterceptor (QObject *parent, QAction *action)
  : QObject (parent), mp_action (action)
{
  // .. nothing yet ..
}

void 
ActionInterceptor::triggered ()
{
  if (Recorder::instance ()) {
    Recorder::instance ()->action (mp_action);
  }
}

// --------------------------------------------------------------
//  EventList implementation

EventList::EventList ()
{
  //  .. nothing yet ..
}

EventList::~EventList ()
{
  for (std::vector<gtf::LogEventBase *>::const_iterator e = m_events.begin (); e != m_events.end (); ++e) {
    delete *e;
  }
  m_events.clear ();
}

void 
EventList::load (const std::string &filename, bool no_spontaneous)
{
  QFile file (tl::to_qstring (filename));
  if (! file.exists ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("File does not exist: ")) + filename);
  }
  QXmlInputSource source (&file);

  GtfXmlHandler handler (this);
  QXmlSimpleReader reader;
  reader.setContentHandler (&handler);
  reader.setErrorHandler (&handler);

  reader.parse (&source, false /*=not incremental*/);

  //  remove spontaneous events if required
  if (no_spontaneous) {
    std::vector <LogEventBase *>::iterator ewrite = m_events.begin ();
    std::vector <LogEventBase *>::iterator e = m_events.begin ();
    while (e != m_events.end ()) {
      if (! (*e)->spontaneous ()) {
        *ewrite++ = *e;
      } else {
        delete *e;
      }
      ++e;
    }
    m_events.erase (ewrite, m_events.end ());
  }
}

void 
EventList::save (const std::string &filename)
{
  std::ostream *os;
  if (filename != "-") {
    os = new std::ofstream (filename.c_str ());
    if (! os->good ()) {
      delete os;
      throw tl::Exception (tl::to_string (QObject::tr ("Unable to open file %s to write GUI test log")), filename);
    }
  } else {
    os = &std::cout;
  }

  *os << "<testcase>" << std::endl;
  for (const_iterator e = m_events.begin (); e != m_events.end (); ++e) {
    (*e)->write (*os);
  }
  *os << "</testcase>" << std::endl;
  if (os != &std::cout) {
    delete os;
  }
}

// --------------------------------------------------------------
//  Recorder implementation

/**
 *  @brief A helper function telling if we shall log for the widget given
 *
 *  This method defines which widgets not to log for. These include ToolBar widgets
 *  and Menu widgets as well as special ones used internally by Qt.
 */
static bool
is_valid_widget (QWidget *w)
{
  if (dynamic_cast <QToolBar *> (w) != 0 ||
      dynamic_cast <QMenuBar *> (w) != 0 ||
      dynamic_cast <QMenu *> (w) != 0) {
    return false;
  } else if (w->parentWidget () == 0) {
    return (dynamic_cast <QDialog *> (w) != 0 || dynamic_cast <QMainWindow *> (w) != 0);
  } else {
    return is_valid_widget (w->parentWidget ());
  }
}

class ErrorLogRecorder
  : public tl::Channel
{
public:
  ErrorLogRecorder (Recorder *rec)
    : mp_rec (rec)
  { }

protected:
  virtual void puts (const char *s) { mp_rec->errlog_puts (s); }
  virtual void endl () { mp_rec->errlog_endl (); }
  virtual void end () { mp_rec->errlog_end (); }
  virtual void begin () { mp_rec->errlog_begin (); }
  virtual void yield () { }

private:
  Recorder *mp_rec;
};

Recorder *Recorder::ms_instance = 0;

Recorder::Recorder (QObject *parent, const std::string &log_file)
  : QObject (parent), m_recording (false), m_save_incremental (false), m_log_file (log_file)
{
  //  register the listener for error messages
  mp_error_channel = new ErrorLogRecorder (this);
  tl::error.add (mp_error_channel, false);

  tl_assert (ms_instance == 0);
  ms_instance = this;
}

Recorder::~Recorder ()
{
  delete mp_error_channel;
  mp_error_channel = 0;

  stop ();
  ms_instance = 0;
}

void 
Recorder::start ()
{
  tl_assert (! m_recording);
  m_recording = true;
  QCoreApplication *app = QCoreApplication::instance ();
  app->installEventFilter (this);
}

void 
Recorder::stop ()
{
  if (m_recording) {
    m_recording = false;
    QCoreApplication *app = QCoreApplication::instance ();
    app->removeEventFilter (this);
  }
}

void 
Recorder::action (QAction *action)
{
  if (m_recording) {
    QWidget *parent = dynamic_cast <QWidget *> (action->parent ());
    tl_assert (parent != 0);
    m_events.add (new LogActionEvent (widget_to_path (parent), tl::to_string (action->objectName ())));
  }
}

void 
Recorder::probe (QWidget *widget, const tl::Variant &data)
{
  if (m_recording) {
    m_events.add (new LogProbeEvent (widget_to_path (widget), data));
  }
}

void 
Recorder::errlog_begin ()
{
  if (m_recording) {
    m_error_text = "";
  }
}

void 
Recorder::errlog_end ()
{
  if (m_recording) {
    m_events.add (new LogErrorEvent (m_error_text));
  }
}

void 
Recorder::errlog_endl ()
{
  if (m_recording) {
    m_error_text += "\n";
  }
}

void 
Recorder::errlog_puts (const char *s)
{
  if (m_recording) {
    m_error_text += s;
  }
}

void 
Recorder::save_incremental (bool si)
{
  m_save_incremental = si;
}

static tl::Variant
probe_widget (QTreeView *tv)
{
  tl::Variant list = tl::Variant::empty_list ();

  QAbstractItemModel *model = tv->model ();

  int rows = model->rowCount ();
  int columns = model->columnCount ();

  if (rows > 0 && columns > 0) {

    QModelIndex col0 (model->index (0, 0));
    while (col0.isValid ()) {

      int lvl = 0;
      for (QModelIndex pindex = col0; pindex.isValid (); ++lvl) {
        pindex = model->parent (pindex);
      }

      tl::Variant el = tl::Variant::empty_list ();
      el.push (tl::Variant (long (lvl)));
      el.push (tl::Variant ((tv->selectionModel ()->isSelected (col0)) ? "Selected" : "Not selected"));
      
      for (int c = 0; c < columns; ++c) {

        QModelIndex coln (col0.sibling (col0.row (), c));
        QVariant font = model->data (coln, Qt::FontRole);
        QVariant deco = model->data (coln, Qt::DecorationRole);
        QVariant display = model->data (coln, Qt::DisplayRole);

        if (! deco.value<QIcon> ().isNull ()) {

          QImage img = deco.value<QIcon> ().pixmap (tv->iconSize ()).toImage ();
          el.push (image_to_variant (img));

        } else {

          std::string t = tl::to_string (display.toString ());
          t += " (";
          QFont f (font.value<QFont> ());
          bool first = true;
          if (f.bold ()) {
            t += "Bold";
            first = false;
          }
          if (f.strikeOut ()) {
            if (! first) {
              t += ",";
            }
            t += "StrikeOut";
            first = false;
          }
          if (f.italic ()) {
            if (! first) {
              t += ",";
            }
            t += "Italic";
            first = false;
          }
          t += ")";
          el.push (tl::Variant (t));

        }

      }

      list.push (el);

      col0 = tv->indexBelow (col0);

    }

  }

  return list;
}

static tl::Variant
probe_widget (QTextEdit *te)
{
  tl::Variant ret = tl::Variant::empty_list ();
  QStringList lines = te->toPlainText ().split (QString::fromUtf8 ("\n"));
  for (QStringList::const_iterator s = lines.begin (); s != lines.end (); ++s) {
    ret.push (tl::to_string (*s));
  }
  return ret;
}

static tl::Variant
probe_widget (QLineEdit *le)
{
  return tl::Variant (tl::to_string (le->text ()));
}

static tl::Variant
probe_widget (QSpinBox *sb)
{
  return tl::Variant (long (sb->value ()));
}

static tl::Variant
probe_widget (QCheckBox *cb)
{
  return tl::Variant (long (cb->isChecked ()));
}

static tl::Variant
probe_widget (QComboBox *cmb)
{
  return tl::Variant (tl::to_string (cmb->lineEdit ()->text ()));
}

static tl::Variant
probe_widget (QListView *lv)
{
  tl::Variant list = tl::Variant::empty_list ();

  QAbstractItemModel *model = lv->model ();

  int rows = model->rowCount ();
  int columns = model->columnCount ();

  if (rows > 0 && columns > 0) {

    QModelIndex col0 (model->index (0, 0));
    while (col0.isValid ()) {

      tl::Variant el = tl::Variant::empty_list ();
      el.push (tl::Variant ((lv->selectionModel ()->isSelected (col0)) ? "Selected" : "Not selected"));
      
      for (int c = 0; c < columns; ++c) {

        QModelIndex coln (col0.sibling (col0.row (), c));
        QVariant font = model->data (coln, Qt::FontRole);
        QVariant deco = model->data (coln, Qt::DecorationRole);
        QVariant display = model->data (coln, Qt::DisplayRole);

        if (! deco.value<QIcon> ().isNull ()) {

          QImage img = deco.value<QIcon> ().pixmap (lv->iconSize ()).toImage ();
          el.push (image_to_variant (img));

        } else {

          std::string t = tl::to_string (display.toString ());
          t += " (";
          QFont f (font.value<QFont> ());
          bool first = true;
          if (f.bold ()) {
            t += "Bold";
            first = false;
          }
          if (f.strikeOut ()) {
            if (! first) {
              t += ",";
            }
            t += "StrikeOut";
            first = false;
          }
          if (f.italic ()) {
            if (! first) {
              t += ",";
            }
            t += "Italic";
            first = false;
          }
          t += ")";
          el.push (tl::Variant (t));

        }

      }

      list.push (el);

      col0 = col0.sibling (col0.row () + 1, 0);

    }

  }

  return list;
}

static tl::Variant
probe_widget (QRadioButton *rb)
{
  return tl::Variant (long (rb->isChecked ()));
}

static tl::Variant
probe_widget (QPushButton *pb)
{
  if (! pb->icon ().isNull ()) {
    return tl::Variant (image_to_variant (pb->icon ().pixmap (pb->iconSize ()).toImage ()));
  } else {
    return tl::Variant (tl::to_string (pb->text ()));
  }
}

tl::Variant
Recorder::probe_std (QWidget *w)
{
  if (dynamic_cast <QTreeView *> (w)) {
    return probe_widget (dynamic_cast <QTreeView *> (w));
  }
  if (dynamic_cast <QLineEdit *> (w)) {
    return probe_widget (dynamic_cast <QLineEdit *> (w));
  }
  if (dynamic_cast <QTextEdit *> (w)) {
    return probe_widget (dynamic_cast <QTextEdit *> (w));
  }
  if (dynamic_cast <QSpinBox *> (w)) {
    return probe_widget (dynamic_cast <QSpinBox *> (w));
  }
  if (dynamic_cast <QCheckBox *> (w)) {
    return probe_widget (dynamic_cast <QCheckBox *> (w));
  }
  if (dynamic_cast <QComboBox *> (w)) {
    return probe_widget (dynamic_cast <QComboBox *> (w));
  }
  if (dynamic_cast <QListView *> (w)) {
    return probe_widget (dynamic_cast <QListView *> (w));
  }
  if (dynamic_cast <QRadioButton *> (w)) {
    return probe_widget (dynamic_cast <QRadioButton *> (w));
  }
  if (dynamic_cast <QPushButton *> (w)) {
    return probe_widget (dynamic_cast <QPushButton *> (w));
  } 
  return tl::Variant ();
}

/*
 * HINT: it was not a good idea to suppress mouse clicks to the context menu - this is caught by the
 * system anyway (the release event is not received) and the additional event may be required to perform
 * selects on cell trees for example. 
static bool has_context_menu (QWidget *rec)
{
  if (rec->contextMenuPolicy () == Qt::CustomContextMenu || rec->contextMenuPolicy () == Qt::ActionsContextMenu) {
    return true;
  } else if (rec->contextMenuPolicy () != Qt::PreventContextMenu && rec->parentWidget ()) {
    //  deferred to the parent
    return has_context_menu (rec->parentWidget ());
  }
  return false;
}
*/

bool 
Recorder::eventFilter (QObject *object, QEvent *event)
{
  //  do not handle events that are not targeted towards widgets
  QWidget *rec = dynamic_cast <QWidget *> (object);
  if (! rec) {
    return false;
  }

  if (Player::instance () && Player::instance ()->playing ()) {
      
    //  handling in playing mode:

    //  suppress spontaneous input events in playing mode - the user may not interact
    if (! Player::instance ()->event_issued () && dynamic_cast <QInputEvent *> (event) != 0) {
      return true;
    }

    //  only log events issued by the player
    if (event->type () != QEvent::Resize &&
        (Player::instance ()->event_issued () != event || Player::instance ()->event_target () != object)) {
      return false;
    }

  } else {

    //  only log key events that are targeted towards widgets that do not have the focus
    //  this propagation of events is done automatically on replay in the same fashion.
    if (dynamic_cast <QKeyEvent *> (event) != 0 && ! rec->hasFocus ()) {
      return false;
    }

    //  do not log propagation events for mouse events
    if (dynamic_cast <QMouseEvent *> (event) != 0 && ! event->spontaneous ()) {
      return false;
    }

  }

  if (event->type() == QEvent::KeyPress || event->type () == QEvent::KeyRelease) {

    QKeyEvent *keyEvent = dynamic_cast <QKeyEvent *> (event); 

    //  Do not log Shift, Ctrl or Alt key events
    if (keyEvent && 
        keyEvent->key () != Qt::Key_Control &&
        keyEvent->key () != Qt::Key_Alt &&
        keyEvent->key () != Qt::Key_Shift && 
        is_valid_widget (rec)) {
      m_events.add (new LogKeyEvent (widget_to_path (rec), *keyEvent));
      if (m_save_incremental) {
        save ();
      }
    }

  } else if (event->type() == QEvent::MouseButtonDblClick || 
             event->type() == QEvent::MouseButtonPress || 
             event->type() == QEvent::MouseButtonRelease) {

    QMouseEvent *mouseEvent = dynamic_cast <QMouseEvent *> (event); 

    //  Pressing the mouse with Ctrl+Alt pressed issues a window probe
    if (mouseEvent &&
        (mouseEvent->button () & Qt::LeftButton) != 0 && 
        (mouseEvent->modifiers () & Qt::ControlModifier) != 0 &&
        (mouseEvent->modifiers () & Qt::AltModifier) != 0) {

      if (event->type() == QEvent::MouseButtonPress) {

        //  send the test event to make the object print its content
        QEvent event (QEvent::MaxUser);
        event.ignore ();

        for (QWidget *w = rec; w != 0; w = w->parentWidget ()) {
          QApplication::instance ()->sendEvent (w, &event);
          if (event.isAccepted ()) {
            tl::info << tl::to_string (QObject::tr ("Probed widget ")) << widget_to_path (w);
            return true;
          }
        } 

        //  if there is no special handling, try the default implementation
        for (QWidget *w = rec; w != 0; w = w->parentWidget ()) {
          tl::Variant p = probe_std (w);
          if (! p.is_nil ()) {
            probe (w, p);
            tl::info << tl::to_string (QObject::tr ("Probed widget ")) << widget_to_path (w);
            return true;
          }
        } 

      }

      //  eat probe events
      return true;

/* HINT: see above (has_context_menu)
    } else if (event->type() == QEvent::MouseButtonPress 
               && (mouseEvent->button () & Qt::RightButton) != 0 
               && has_context_menu (rec)) {

      //  suppress context menu events 
*/

    } else if (mouseEvent && is_valid_widget (rec)) {
      m_events.add (new LogMouseEvent (widget_to_path (rec), *mouseEvent));
      if (m_save_incremental) {
        save ();
      }
    }

  } else if (event->type() == QEvent::MouseMove) {

    if (is_valid_widget (rec)) {

      QMouseEvent *mouseEvent = dynamic_cast <QMouseEvent *> (event); 
      std::string wp = widget_to_path (rec);

      //  compress mouse events into a single one, if the buttons are the same
      LogMouseEvent *me_log = 0;
      if (mouseEvent &&
          ! m_events.empty () && 
          (me_log = dynamic_cast <LogMouseEvent *> (m_events.back ())) != 0 &&
          (me_log->event ().type () == QEvent::MouseMove &&
           me_log->event ().buttons () == mouseEvent->buttons () &&
           me_log->event ().button () == mouseEvent->button () &&
           me_log->event ().modifiers () == mouseEvent->modifiers () &&
           me_log->target () == wp)) {
        //  compress by adding the difference between the original and the 
        //  current event. This avoids not taking into account widgets (like
        //  splitters) that move with the mouse and thus shift the reference
        //  coordinate system.
        QPoint d = mouseEvent->globalPos () - me_log->event ().globalPos ();
        me_log->move (d);
        if (m_save_incremental) {
          save ();
        }
      } else if (mouseEvent) {
        m_events.add (new LogMouseEvent (wp, *mouseEvent));
        if (m_save_incremental) {
          save ();
        }
      }

    }
    
  } else if (event->type() == QEvent::Resize) {

    //  resize events are logged only for top-level widgets
    QResizeEvent *resizeEvent = dynamic_cast <QResizeEvent *> (event); 
    if (resizeEvent && rec->parentWidget () == 0 && is_valid_widget (rec)) {

      std::string target (widget_to_path (rec));

      //  compress resize events into a single event
      LogResizeEvent *re_log = 0;
      QSize old_size (resizeEvent->oldSize ());
      if (! m_events.empty () && 
          (re_log = dynamic_cast <LogResizeEvent *> (m_events.back ())) != 0) {
        if (re_log->target () == target) {
          old_size = re_log->old_size ();
          delete m_events.back ();
          m_events.pop_back ();
        }
      } 

      m_events.add (new LogResizeEvent (target, resizeEvent->size (), old_size));
      if (m_save_incremental) {
        save ();
      }

    }

  } 
  return false;
}

// --------------------------------------------------------------
//  Implementation of gtf::Player

Player *Player::ms_instance = 0;

Player::Player (QObject *parent)
  : QObject (parent),
    m_ms (0), m_playing_active (false), m_playing_index (0), m_breakpoint (-1), mp_event_issued (0), mp_event_target (0)
{
  tl_assert (ms_instance == 0);
  ms_instance = this;
  mp_timer = new QTimer (this);
  connect (mp_timer, SIGNAL (timeout ()), this, SLOT (timer ()));
}

Player::~Player ()
{
  ms_instance = 0;
  delete mp_timer;
  mp_timer = 0;
}

void 
Player::timer ()
{
  if (m_playing_active && m_playing_index < m_events.size () 
      && (m_breakpoint < 0 || m_events [m_playing_index]->xml_line () <= m_breakpoint)) {
    //  restart timer (before issueing the event - because this might block in a modal dialog)
    mp_timer->setSingleShot (true);
    mp_timer->start (m_ms);
    try {
      if (tl::verbosity () >= 10) {
        std::ostringstream info;
        info.imbue (std::locale ("C"));
        m_events [m_playing_index]->write (info, false /*no endl*/); 
        tl::info << m_events [m_playing_index]->xml_line () << ": " << info.str ();
      }
      m_events [m_playing_index++]->issue_event ();
    } catch (tl::Exception &ex) {
      tl::error << ex.msg ();
      exit (1);
    }
  } else {
    //  done.
    m_playing_active = false;
    MouseTrackerWidget::instance ()->hide ();
  }
}

void 
Player::replay (int ms, int stop_at_line)
{
  m_ms = ms;
  m_playing_active = true;
  m_breakpoint = stop_at_line;
  mp_timer->setSingleShot (true);
  mp_timer->start (0);
}

void
Player::issue_event (QWidget *target, QEvent *event)
{
  mp_event_issued = event;
  mp_event_target = target;
  QApplication::sendEvent (target, event);
  mp_event_issued = 0;
  mp_event_target = 0;
}

// --------------------------------------------------------------
//  Implementation of the XML handler

GtfXmlHandler::GtfXmlHandler (EventList *list)
  : mp_locator (0),
    mp_list (list),
    m_in_event (false)
{
  //  .. nothing yet ..
}

void
GtfXmlHandler::setDocumentLocator (QXmlLocator *locator)
{
  mp_locator = locator;
}

bool 
GtfXmlHandler::characters (const QString &ch)
{
  m_cdata += ch;
  //  continue
  return true;
}

bool 
GtfXmlHandler::endElement (const QString & /*namespaceURI*/, const QString &localName, const QString & /*qName*/)
{
  if (localName == QString::fromUtf8 ("block")) {

    tl_assert (m_data_stack.size () >= 2);
    m_data_stack.end ()[-2].push (m_data_stack.end ()[-1]);
    m_data_stack.pop_back ();

  } else if (localName == QString::fromUtf8 ("string")) {

    tl_assert (m_data_stack.size () >= 1);
    m_data_stack.end ()[-1].push (tl::Variant (tl::to_string (m_cdata)));

  } else if (localName == QString::fromUtf8 ("int")) {

    long l = 0;
    tl::from_string (tl::to_string (m_cdata), l);
    tl_assert (m_data_stack.size () >= 1);
    m_data_stack.end ()[-1].push (tl::Variant (l));

  } else if (localName == QString::fromUtf8 ("img")) {

    QByteArray ba (QByteArray::fromBase64 (m_cdata.toUtf8 ()));
    QImage img;
    img.loadFromData (ba);
    tl_assert (m_data_stack.size () >= 1);
    m_data_stack.end ()[-1].push (image_to_variant (img));

  } else if (localName == QString::fromUtf8 ("mouse_button_release") || 
             localName == QString::fromUtf8 ("mouse_button_press") ||
             localName == QString::fromUtf8 ("mouse_button_dbl_click") ||
             localName == QString::fromUtf8 ("mouse_move") ||
             localName == QString::fromUtf8 ("key_press") ||
             localName == QString::fromUtf8 ("key_release") ||
             localName == QString::fromUtf8 ("action") ||
             localName == QString::fromUtf8 ("resize") ||
             localName == QString::fromUtf8 ("probe") ||
             localName == QString::fromUtf8 ("error") ||
             localName == QString::fromUtf8 ("block")) {
    leave_event ();
  }

  //  continue
  return true;
}

bool 
GtfXmlHandler::startElement (const QString & /*namespaceURI*/, const QString &localName, const QString & /*qName*/, const QXmlAttributes &atts)
{
  if (localName == QString::fromUtf8 ("mouse_button_release") || 
      localName == QString::fromUtf8 ("mouse_button_press") ||
      localName == QString::fromUtf8 ("mouse_button_dbl_click")) {

    int xpos = atts.value (QString::fromUtf8 ("xpos")).toInt ();
    int ypos = atts.value (QString::fromUtf8 ("ypos")).toInt ();
    int button = atts.value (QString::fromUtf8 ("button")).toInt (0, 16);
    int modifiers = atts.value (QString::fromUtf8 ("modifiers")).toInt (0, 16);

    QEvent::Type type;
    if (localName == QString::fromUtf8 ("mouse_button_release")) {
      type = QEvent::MouseButtonRelease;
    } else if (localName == QString::fromUtf8 ("mouse_button_press")) {
      type = QEvent::MouseButtonPress;
    } else {
      type = QEvent::MouseButtonDblClick;
    }
       
    QMouseEvent mouse_event (type, QPoint (xpos, ypos), Qt::MouseButton (button), Qt::MouseButtons (button), Qt::KeyboardModifiers (modifiers));
    enter_event (new LogMouseEvent (tl::to_string (atts.value (QString::fromUtf8 ("target"))), mouse_event, mp_locator->lineNumber ()));

  } else if (localName == QString::fromUtf8 ("mouse_move")) {

    int xpos = atts.value (QString::fromUtf8 ("xpos")).toInt ();
    int ypos = atts.value (QString::fromUtf8 ("ypos")).toInt ();
    int buttons = atts.value (QString::fromUtf8 ("buttons")).toInt (0, 16);
    int modifiers = atts.value (QString::fromUtf8 ("modifiers")).toInt (0, 16);

    QMouseEvent mouse_event (QEvent::MouseMove, QPoint (xpos, ypos), Qt::MouseButton (Qt::NoButton), Qt::MouseButtons (buttons), Qt::KeyboardModifiers (modifiers));
    enter_event (new LogMouseEvent (tl::to_string (atts.value (QString::fromUtf8 ("target"))), mouse_event, mp_locator->lineNumber ()));

  } else if (localName == QString::fromUtf8 ("key_press") ||
             localName == QString::fromUtf8 ("key_release")) {

    int key = atts.value (QString::fromUtf8 ("key")).toInt (0, 16);
    QChar text_char = QChar (atts.value (QString::fromUtf8 ("code")).toInt (0, 16));
    QString text = text_char;
    int modifiers = atts.value (QString::fromUtf8 ("modifiers")).toInt (0, 16);

    QEvent::Type type;
    if (localName == QString::fromUtf8 ("key_press")) {
      type = QEvent::KeyPress;
    } else {
      type = QEvent::KeyRelease;
    }

    QKeyEvent key_event (type, Qt::Key (key), Qt::KeyboardModifiers (modifiers), text);
    enter_event (new LogKeyEvent (tl::to_string (atts.value (QString::fromUtf8 ("target"))), key_event, mp_locator->lineNumber ()));

  } else if (localName == QString::fromUtf8 ("action")) {

    enter_event (new LogActionEvent (tl::to_string (atts.value (QString::fromUtf8 ("target"))), tl::to_string (atts.value (QString::fromUtf8 ("action"))), mp_locator->lineNumber ()));

  } else if (localName == QString::fromUtf8 ("resize")) {

    int xsize = atts.value (QString::fromUtf8 ("xsize")).toInt ();
    int ysize = atts.value (QString::fromUtf8 ("ysize")).toInt ();
    int xsize_old = atts.value (QString::fromUtf8 ("xsize_old")).toInt ();
    int ysize_old = atts.value (QString::fromUtf8 ("ysize_old")).toInt ();

    enter_event (new LogResizeEvent (tl::to_string (atts.value (QString::fromUtf8 ("target"))), QSize (xsize, ysize), QSize (xsize_old, ysize_old), mp_locator->lineNumber ()));

  } else if (localName == QString::fromUtf8 ("probe")) {

    enter_event (new LogProbeEvent (tl::to_string (atts.value (QString::fromUtf8 ("target"))), mp_locator->lineNumber ()));

  } else if (localName == QString::fromUtf8 ("error")) {

    enter_event (new LogErrorEvent (mp_locator->lineNumber ()));

  } else if (localName == QString::fromUtf8 ("block")) {

    m_data_stack.push_back (tl::Variant::empty_list ());

  } else if (localName == QString::fromUtf8 ("string") ||
             localName == QString::fromUtf8 ("int") ||
             localName == QString::fromUtf8 ("img")) {

    m_cdata = QString ();

  }

  //  continue
  return true;
}

void
GtfXmlHandler::enter_event (LogEventBase *event)
{
  if (m_in_event) {
    // TODO: (?) error ("Unexpected element");
    return;
  }

  mp_list->add (event);

  m_data_stack.clear ();
  m_data_stack.push_back (tl::Variant::empty_list ());

  m_in_event = true;
}

void
GtfXmlHandler::leave_event ()
{
  if (! m_in_event) {
    // TODO: (?) throw tl::XMLLocatedException (tl::to_string (QObject::tr ("Unexpected element")), ex.lineNumber (), ex.columnNumber ());
    return;
  }

  mp_list->end ()[-1]->set_data (m_data_stack [0]);
  m_data_stack.clear ();

  m_in_event = false;
}

bool 
GtfXmlHandler::error (const QXmlParseException &ex)
{
  throw tl::XMLLocatedException (tl::to_string (ex.message ()), ex.lineNumber (), ex.columnNumber ());
}

bool 
GtfXmlHandler::fatalError (const std::string &msg)
{
  throw tl::XMLLocatedException (msg.c_str (), mp_locator->lineNumber (), mp_locator->columnNumber ());
}

bool 
GtfXmlHandler::fatalError (const QXmlParseException &ex)
{
  throw tl::XMLLocatedException (tl::to_string (ex.message ()), ex.lineNumber (), ex.columnNumber ());
}

bool 
GtfXmlHandler::warning (const QXmlParseException &ex)
{
  tl::XMLLocatedException lex (tl::to_string (ex.message ()), ex.lineNumber (), ex.columnNumber ());
  tl::warn << lex.msg ();
  //  continue
  return true;
}

}

#endif

