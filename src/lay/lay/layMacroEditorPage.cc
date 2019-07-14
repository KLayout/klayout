
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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


#include "layMacroEditorPage.h"
#include "lymMacroInterpreter.h"
#include "tlExceptions.h"
#include "tlString.h"

#include <cstdio>
#include <iostream>

#include <set>
#include <map>
#include <vector>

#include <QSyntaxHighlighter>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QLabel>
#include <QChar>
#include <QResource>
#include <QBuffer>

namespace lay
{

// ----------------------------------------------------------------------------------------------
//  Utility wrapper around QTextBlock::firstLineNumber - emulation for Qt < 4.5

static int firstLineNumber (const QTextBlock &b)
{
#if QT_VERSION >= 0x040500  
  return b.firstLineNumber ();
#else
  return QTextCursor (b).blockNumber ();
#endif
}

// ----------------------------------------------------------------------------------------------
//  MacroEditorTextWidget implementation

MacroEditorTextWidget::MacroEditorTextWidget (QWidget *parent)
  : TextEditWidget (parent)
{ }

void MacroEditorTextWidget::paintEvent (QPaintEvent *event)
{
  //  lacking any other good way to detect scrolling, we catch the paint events of the viewport 
  //  to detect a change in the geometry
  QRect r (0, -verticalScrollBar ()->value (), 1, height ());
  if (r != m_r) {
    m_r = r;
    emit contentsChanged ();
  }
  return TextEditWidget::paintEvent (event);
}

// ----------------------------------------------------------------------------------------------
//  MacroEditorHighlighters implementation

MacroEditorHighlighters::MacroEditorHighlighters (QObject *parent)
  : m_basic_attributes ()
{
  //  TODO: more languages
  m_attributes.push_back (std::make_pair ("ruby", GenericSyntaxHighlighterAttributes (&m_basic_attributes)));
  m_attributes.push_back (std::make_pair ("python", GenericSyntaxHighlighterAttributes (&m_basic_attributes)));

  for (std::vector<std::pair<std::string, GenericSyntaxHighlighterAttributes> >::iterator a = m_attributes.begin (); a != m_attributes.end (); ++a) {
    //  Note: this loads and initializes the attributes
    delete highlighter_for_scheme (parent, a->first, &a->second); 
  }
}

QSyntaxHighlighter *
MacroEditorHighlighters::highlighter_for (QObject *parent, lym::Macro::Interpreter lang, const std::string &dsl_name)
{
  std::string scheme = scheme_for (lang, dsl_name);

  for (std::vector<std::pair<std::string, GenericSyntaxHighlighterAttributes> >::iterator a = m_attributes.begin (); a != m_attributes.end (); ++a) {
    if (a->first == scheme) {
      return highlighter_for_scheme (parent, a->first, &a->second);
    }
  }

  return 0;
}

lay::GenericSyntaxHighlighter *
MacroEditorHighlighters::highlighter_for_scheme (QObject *parent, const std::string &scheme, GenericSyntaxHighlighterAttributes *attributes)
{
  if (! scheme.empty ()) {

    QResource res (tl::to_qstring (":/syntax/" + scheme + ".xml"));
    QByteArray data = qUncompress (QByteArray ((const char *) res.data (), int (res.size ())));
    QBuffer input (&data);
    input.open (QIODevice::ReadOnly);
    lay::GenericSyntaxHighlighter *hl = new GenericSyntaxHighlighter (parent, input, attributes);
    input.close ();

    return hl;

  } else {
    return 0;
  }
}

GenericSyntaxHighlighterAttributes *
MacroEditorHighlighters::attributes_for (lym::Macro::Interpreter lang, const std::string &dsl_name)
{
  std::string scheme = scheme_for (lang, dsl_name);

  for (std::vector<std::pair<std::string, GenericSyntaxHighlighterAttributes> >::iterator a = m_attributes.begin (); a != m_attributes.end (); ++a) {
    if (a->first == scheme) {
      return &a->second;
    }
  }

  return 0;
}

GenericSyntaxHighlighterAttributes *
MacroEditorHighlighters::basic_attributes ()
{
  return &m_basic_attributes;
}

std::string 
MacroEditorHighlighters::to_string () const
{
  std::string s = "basic:" + m_basic_attributes.to_string ();

  for (std::vector<std::pair<std::string, GenericSyntaxHighlighterAttributes> >::const_iterator a = m_attributes.begin (); a != m_attributes.end (); ++a) {
    s += tl::to_word_or_quoted_string (a->first) + ":" + a->second.to_string ();
  }
  
  return s;
}

void 
MacroEditorHighlighters::load (const std::string &s)
{
  try {

    GenericSyntaxHighlighterAttributes def;

    tl::Extractor ex (s.c_str ());

    while (! ex.at_end ()) {

      std::string t;
      ex.read_word_or_quoted (t);
      ex.test (":");

      GenericSyntaxHighlighterAttributes *attributes = &def;
      if (t == "basic") {
        attributes = &m_basic_attributes;
      } else {
        for (std::vector<std::pair<std::string, GenericSyntaxHighlighterAttributes> >::iterator a = m_attributes.begin (); a != m_attributes.end (); ++a) {
          if (a->first == t) {
            attributes = &a->second;
            break;
          }
        }
      }

      attributes->read (ex);

    }

  } catch (...) {
    //  ignore errors;
  }
}

std::string 
MacroEditorHighlighters::scheme_for (lym::Macro::Interpreter lang, const std::string &dsl_name)
{
  if (lang == lym::Macro::Ruby) {
    return "ruby";
  } else if (lang == lym::Macro::Python) {
    return "python";
  } else if (lang == lym::Macro::DSLInterpreter) {
    return lym::MacroInterpreter::syntax_scheme (dsl_name);
  } else {
    return std::string ();
  }
}

// ----------------------------------------------------------------------------------------------
//  MacroEditorExecutionModel implementation

MacroEditorExecutionModel::MacroEditorExecutionModel (QObject * /*parent*/)
  : m_current_line (-1), m_run_mode (false), m_interpreter (lym::Macro::None)
{
  // .. nothing yet ..
}

void MacroEditorExecutionModel::set_interpreter (lym::Macro::Interpreter lang)
{
  m_interpreter = lang;
  if (lang == lym::Macro::None) {
    set_breakpoints (std::set<int> ());
  }
}

void MacroEditorExecutionModel::set_breakpoints (const std::set<int> &b)
{
  if (m_interpreter == lym::Macro::None) {
    return;
  }

  if (m_breakpoints != b) {
    m_breakpoints = b;
    emit breakpoints_changed ();
  }
}

void MacroEditorExecutionModel::toggle_breakpoint (int line)
{
  if (m_interpreter == lym::Macro::None) {
    return;
  }

  if (m_breakpoints.find (line) == m_breakpoints.end ()) {
    m_breakpoints.insert (line);
  } else {
    m_breakpoints.erase (line);
  }
  emit breakpoints_changed ();
}

void MacroEditorExecutionModel::set_breakpoint (int line)
{
  if (m_interpreter == lym::Macro::None) {
    return;
  }

  if (m_breakpoints.find (line) == m_breakpoints.end ()) {
    m_breakpoints.insert (line);
    emit breakpoints_changed ();
  }
}

void MacroEditorExecutionModel::remove_breakpoint (int line)
{
  if (m_interpreter == lym::Macro::None) {
    return;
  }

  if (m_breakpoints.find (line) != m_breakpoints.end ()) {
    m_breakpoints.erase (line);
    emit breakpoints_changed ();
  }
}

void MacroEditorExecutionModel::set_current_line (int line, bool force_event)
{
  if (m_interpreter == lym::Macro::None) {
    return;
  }

  if (force_event || line != m_current_line) {
    m_current_line = line;
    emit current_line_changed ();
  }
}

void MacroEditorExecutionModel::set_run_mode (bool run_mode)
{
  if (m_interpreter == lym::Macro::None) {
    return;
  }

  if (m_run_mode != run_mode) {
    m_run_mode = run_mode;
    emit run_mode_changed ();
  }
}

// ----------------------------------------------------------------------------------------------
//  MacroEditorSidePanel implementation 

const int sidePanelMargin = 4;

MacroEditorSidePanel::MacroEditorSidePanel (QWidget *parent, MacroEditorTextWidget *text, MacroEditorExecutionModel *exec_model)
  : QWidget (parent), mp_text (text), mp_exec_model (exec_model), 
    m_breakpoint_pixmap (QString::fromUtf8 (":/breakpointmark.png")),
    m_breakpoint_disabled_pixmap (QString::fromUtf8 (":/breakpointmarkdisabled.png")),
    m_exec_point_pixmap (QString::fromUtf8 (":/execmark.png")),
    m_debugging_on (true)
{
  connect (text, SIGNAL (contentsChanged ()), this, SLOT (redraw ()));
  connect (text, SIGNAL (cursorPositionChanged ()), this, SLOT (redraw ()));
  connect (exec_model, SIGNAL (breakpoints_changed ()), this, SLOT (redraw ()));
  connect (exec_model, SIGNAL (current_line_changed ()), this, SLOT (redraw ()));
  connect (exec_model, SIGNAL (run_mode_changed ()), this, SLOT (redraw ()));
}

void MacroEditorSidePanel::set_debugging_on (bool on)
{
  if (m_debugging_on != on) {
    m_debugging_on = on;
    update ();
  }
}

QSize MacroEditorSidePanel::sizeHint () const
{
  return QSize (QFontMetrics (mp_text->font ()).width (QString::fromUtf8 ("12345")) + 3 * sidePanelMargin + m_breakpoint_pixmap.width (), 0);
}

void MacroEditorSidePanel::mousePressEvent (QMouseEvent *event)
{
  if (event->button () == Qt::LeftButton) {

    // toggle a breakpoint if required

    QTextBlock b = mp_text->cursorForPosition (QPoint (0, -mp_text->viewport ()->rect ().top ())).block ();

    int line = -1;

    while (b.isValid ()) {

      QRect rc = mp_text->cursorRect (QTextCursor (b));
      rc.translate (0, mp_text->frameWidth () + mp_text->viewport ()->rect ().top ());
      QRect rt (0, rc.top (), width () - 1, rc.height ());

      if (rt.contains (event->pos ())) {
        line = firstLineNumber (b) + 1;
        break;
      }

      b = b.next ();

    }

    if (line >= 0) {
      mp_exec_model->toggle_breakpoint (line);
    }

  }
}

void MacroEditorSidePanel::redraw ()
{
  update ();
}

void MacroEditorSidePanel::set_watermark (const QString &wm) 
{
  if (m_watermark != wm) {
    m_watermark = wm;
    update ();
  }
}

void MacroEditorSidePanel::paintEvent (QPaintEvent *)
{
  QPainter p (this);
  QPen sepPen (palette ().color (QPalette::Dark));
  QPen textPen (palette ().color (QPalette::Dark));
  QPen hlTextPen (palette ().color (QPalette::Light));
  QBrush hlBrush (palette ().color (QPalette::Dark));

  QRect rsel = mp_text->cursorRect (mp_text->textCursor ());

  //  paint background 
  for (QTextBlock b = mp_text->cursorForPosition (QPoint (0, -mp_text->viewport ()->rect ().top ())).block (); b.isValid (); b = b.next ()) {

    QRect rc = mp_text->cursorRect (QTextCursor (b));
    rc.translate (0, mp_text->frameWidth () + mp_text->viewport ()->rect ().top ());

    QRect rt (sidePanelMargin + m_breakpoint_pixmap.width (), rc.top (), width (), rc.height ());

    int rsel_center = (rsel.bottom () + rsel.top ()) / 2;
    if (rc.top () < rsel_center && rc.bottom () > rsel_center) {
      p.fillRect (rt, hlBrush);
    }

  }

  //  paint watermark text
  if (! m_watermark.isEmpty ()) {

    p.save ();

    p.rotate (-90.0);
    p.setPen (QPen (QColor (0, 0, 0, 10)));
    QFont ip_font (QString::fromUtf8 ("Helvetica"));
    ip_font.setWeight (QFont::Bold);
    ip_font.setPixelSize (width ());
    p.setFont (ip_font);

    p.drawText (QRect (-height (), 0, height (), width ()), Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextDontClip | Qt::TextSingleLine, m_watermark);

    p.restore ();

  }

  //  paint foreground 
  for (QTextBlock b = mp_text->cursorForPosition (QPoint (0, -mp_text->viewport ()->rect ().top ())).block (); b.isValid (); b = b.next ()) {

    int l = firstLineNumber (b) + 1;
    QRect rc = mp_text->cursorRect (QTextCursor (b));
    rc.translate (0, mp_text->frameWidth () + mp_text->viewport ()->rect ().top ());

    QRect rt (sidePanelMargin + m_breakpoint_pixmap.width (), rc.top (), width (), rc.height ());

    p.setFont (b.charFormat ().font ());
    int rsel_center = (rsel.bottom () + rsel.top ()) / 2;
    if (rc.top () < rsel_center && rc.bottom () > rsel_center) {
      p.setPen (hlTextPen);
    } else {
      p.setPen (textPen);
    }

    p.drawText (rt.adjusted (sidePanelMargin, 0, 0, 0), Qt::AlignLeft | Qt::AlignBottom | Qt::TextDontClip | Qt::TextSingleLine, QString::number (l));

    p.setPen (sepPen);
    p.drawLine (0, rc.top (), width () - 1, rc.top ());

    if (rc.top () >= rect ().bottom ()) {
      break;
    }

    if (mp_exec_model->breakpoints ().find (l) != mp_exec_model->breakpoints ().end ()) {
      int icon_size = std::min (m_breakpoint_pixmap.height (), rt.height ()); 
      QRect rpt (0, rt.center ().y () - icon_size / 2 + 1, icon_size, icon_size);
      if (m_debugging_on) {
        p.drawPixmap (rpt, m_breakpoint_pixmap, m_breakpoint_pixmap.rect ());
      } else {
        p.drawPixmap (rpt, m_breakpoint_disabled_pixmap, m_breakpoint_pixmap.rect ());
      }
    }

    if (mp_exec_model->run_mode () && mp_exec_model->current_line () == l) {
      int icon_size = std::min (m_exec_point_pixmap.height (), rt.height ()); 
      QRect rpt (sidePanelMargin, rt.center ().y () - icon_size / 2 + 1, icon_size, icon_size);
      p.drawPixmap (rpt, m_exec_point_pixmap, m_exec_point_pixmap.rect ());
    }

  }
}

// ----------------------------------------------------------------------------------------------
//  MacroEditorPage implementation

MacroEditorPage::MacroEditorPage (QWidget * /*parent*/, MacroEditorHighlighters *highlighters)
  : mp_macro (0), mp_highlighters (highlighters), mp_highlighter (0), m_error_line (-1), m_ntab (8), m_nindent (2)
{
  QVBoxLayout *layout = new QVBoxLayout (this);
  
  mp_readonly_label = new QLabel (this);
  mp_readonly_label->setText (QObject::tr ("Macro is read-only and cannot be edited"));
  mp_readonly_label->hide ();
  layout->addWidget (mp_readonly_label);

  QHBoxLayout *hlayout = new QHBoxLayout ();
  layout->addLayout (hlayout);

  mp_exec_model = new MacroEditorExecutionModel (this);
  mp_text = new MacroEditorTextWidget (this);
  mp_side_panel = new MacroEditorSidePanel (this, mp_text, mp_exec_model);
  hlayout->addWidget (mp_side_panel);
  hlayout->addWidget (mp_text);

  mp_text->setWordWrapMode(QTextOption::NoWrap);
  mp_text->setTabStopWidth (m_ntab * QFontMetrics (mp_text->font ()).width (QString::fromUtf8 ("x")));
  m_is_modified = false;
 
  connect (mp_text, SIGNAL (textChanged ()), this, SLOT (text_changed ()));
  connect (mp_text, SIGNAL (cursorPositionChanged ()), this, SLOT (cursor_position_changed ()));
  connect (mp_exec_model, SIGNAL (breakpoints_changed ()), this, SLOT (breakpoints_changed ()));
  connect (mp_exec_model, SIGNAL (current_line_changed ()), this, SLOT (current_line_changed ()));
  connect (mp_exec_model, SIGNAL (run_mode_changed ()), this, SLOT (run_mode_changed ()));

  mp_text->installEventFilter (this);
}

void MacroEditorPage::update ()
{
  if (mp_macro) {

    QString mt = tl::to_qstring (mp_macro->text ());
    QString et = mp_text->toPlainText ();

    if (mt != et) {

      //  Leave trailing lines as far as they are identical - that way we deal with the 
      //  "header changed" case gracefully and don't destroy breakpoints if we change macro properties

      int nm = mt.size ();
      int ne = et.size ();
      while (nm > 0 && ne > 0 && mt [nm - 1] == et [ne - 1]) {
        --nm;
        --ne;
      }

      QTextCursor c = mp_text->textCursor ();

      QTextCursor cursor (mp_text->document ());
      cursor.beginEditBlock ();
      cursor.movePosition (QTextCursor::NextCharacter, QTextCursor::KeepAnchor, int (ne));
      cursor.removeSelectedText ();
      cursor.insertText (mt.left (int (nm)));
      cursor.endEditBlock ();

      mp_text->setTextCursor (c);

    }

  }
}

void MacroEditorPage::set_debugging_on (bool on)
{
  mp_side_panel->set_debugging_on (on);
}

void MacroEditorPage::commit ()
{
  if (mp_macro) {
    mp_macro->set_text (tl::to_string (mp_text->toPlainText ()));
  }
}

void MacroEditorPage::current_line_changed ()
{
  if (mp_exec_model->current_line () >= 0) {
    goto_line (mp_exec_model->current_line ());
  }

  emit edit_trace (false);

  update_extra_selections ();
}

void MacroEditorPage::run_mode_changed ()
{
  if (mp_exec_model->run_mode ()) {
    set_error_line (0);
  }

  mp_text->setReadOnly (! mp_macro || mp_macro->is_readonly () || mp_exec_model->run_mode ());
  update_extra_selections ();
}

void MacroEditorPage::breakpoints_changed ()
{
  //  update the breakpoint's block list
  m_breakpoints.clear ();
  if (! mp_exec_model->breakpoints ().empty ()) {
    const QTextDocument *doc = mp_text->document ();
    for (QTextBlock b = doc->begin(); b != doc->end(); b = b.next()) {
      if (mp_exec_model->breakpoints ().find (firstLineNumber (b) + 1) != mp_exec_model->breakpoints ().end ()) {
        m_breakpoints.insert (b);
        //  Right now, the user data is just used as a flag for a breakpoint
        b.setUserData (new QTextBlockUserData ());
      } else {
        //  Right now, the user data is just used as a flag for a breakpoint
        b.setUserData (0);
      }
    }
  }
}

static bool valid_element (const SyntaxHighlighterElement &e)
{
  return e.basic_attribute_id != lay::dsComment && e.basic_attribute_id != lay::dsString;
}

void MacroEditorPage::cursor_position_changed ()
{
  QTextCursor cursor = mp_text->textCursor ();

  //  prepare a format for the bracket highlights
  QTextCharFormat fmt;
  fmt.setForeground (Qt::red);
  fmt.setBackground (QColor (224, 224, 224));
  QFont f = fmt.font ();
  f.setBold (true);
  fmt.setFont (f);

  QTextBlock b = cursor.block ();
  SyntaxHighlighterUserData *user_data = dynamic_cast<SyntaxHighlighterUserData *> (b.userData());
  if (user_data) {

    //  Look for matching brackets and highlight the other one
    //  NOTE: the whole scheme is somewhat more complex than it could be. It's
    //  based on the syntax highlighter elements and we confine ourselves to
    //  elements not being comment or string. So we need to iterate over elements
    //  and over characters inside these elements.

#if QT_VERSION < 0x40700
    size_t pos = size_t (cursor.position() - cursor.block().position());
#else
    size_t pos = size_t (cursor.positionInBlock ());
#endif

    std::vector<SyntaxHighlighterElement>::const_iterator e;
    for (e = user_data->elements ().begin (); e != user_data->elements ().end (); ++e) {
      if (e->start_offset <= pos && e->start_offset + e->length > pos) {
        break;
      }
    }

    static const QString open_rbracket (QString::fromUtf8 ("("));
    static const QString open_sqbracket (QString::fromUtf8 ("["));
    static const QString open_cbracket (QString::fromUtf8 ("{"));
    static const QString close_rbracket (QString::fromUtf8 (")"));
    static const QString close_sqbracket (QString::fromUtf8 ("]"));
    static const QString close_cbracket (QString::fromUtf8 ("}"));

    bool forward = false, backward = false;
    if (e != user_data->elements ().end () && valid_element (*e)) {
      QString t = b.text ().mid (int (pos), 1);
      forward = (t == open_rbracket || t == open_sqbracket || t == open_cbracket);
    }
    if (e != user_data->elements ().begin () && e[-1].start_offset + e[-1].length >= pos && valid_element (e[-1])) {
      QString t = b.text ().mid (int (pos) - 1, 1);
      backward = (t == close_rbracket || t == close_sqbracket || t == close_cbracket);
    }

    if (forward) {
      backward = false;
    } else if (backward) {
      --e;
    }

    if (forward || backward) {

      std::vector<QString> bs;

      int found = -1;
      while (true) {

        QString t = b.text ().mid (int (e->start_offset), int (e->length)).trimmed ();

        if (valid_element (*e)) {

          if (forward) {

            for (int p = 0; p != t.size () && found < 0; ++p) {
              if (p + e->start_offset >= pos) {
                QString c = t.mid (p, 1);
                if (c == open_rbracket) {
                  bs.push_back (close_rbracket);
                } else if (c == open_cbracket) {
                  bs.push_back (close_cbracket);
                } else if (c == open_sqbracket) {
                  bs.push_back (close_sqbracket);
                } else if (c == bs.back ()) {
                  bs.pop_back ();
                  if (bs.empty ()) {
                    found = p + int (e->start_offset);
                  }
                }
              }
            }

          } else if (backward) {

            for (int p = t.size (); p > 0 && found < 0; ) {
              --p;
              if (p + int (e->start_offset) < int (pos)) {
                QString c = t.mid (p, 1);
                if (c == close_rbracket) {
                  bs.push_back (open_rbracket);
                } else if (c == close_cbracket) {
                  bs.push_back (open_cbracket);
                } else if (c == close_sqbracket) {
                  bs.push_back (open_sqbracket);
                } else if (c == bs.back ()) {
                  bs.pop_back ();
                  if (bs.empty ()) {
                    found = p + int (e->start_offset);
                  }
                }
              }
            }

          }

        }

        if (found >= 0) {
          break;
        }

        if (forward) {
          ++e;
          if (e == user_data->elements ().end ()) {
            break;
          }
        } else {
          if (e == user_data->elements ().begin ()) {
            break;
          }
          --e;
        }

      }

      if (found >= 0) {

        QList<QTextEdit::ExtraSelection> extra_selections = mp_text->extraSelections ();
        for (QList<QTextEdit::ExtraSelection>::iterator i = extra_selections.begin (); i != extra_selections.end (); ) {
          if (i->format == fmt) {
            i = extra_selections.erase (i);
          } else {
            ++i;
          }
        }

        QTextEdit::ExtraSelection es;
        es.format = fmt;

        es.cursor = QTextCursor (b);
        es.cursor.setPosition (b.position () + found);
        es.cursor.movePosition (QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 1);
        extra_selections.push_back (es);

        es.cursor = QTextCursor (b);
        es.cursor.setPosition (b.position () + found);
        es.cursor.movePosition (QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 1);
        extra_selections.push_back (es);

        mp_text->setExtraSelections (extra_selections);

      }

    }

  }
}

void MacroEditorPage::text_changed () 
{
  m_is_modified = true;

  //  update the breakpoint's line numbers
  std::set<int> bl;
  for (std::set<QTextBlock>::const_iterator b = m_breakpoints.begin (); b != m_breakpoints.end (); ++b) {
    if (b->isValid () && b->userData () != 0) {
      bl.insert (firstLineNumber (*b) + 1);
    }
  }
  mp_exec_model->set_breakpoints (bl);

  emit edit_trace (true);
}

void MacroEditorPage::set_ntab (int n)
{
  if (n != m_ntab)  {
    m_ntab = n;
    mp_text->setTabStopWidth (m_ntab * QFontMetrics (mp_text->font ()).width (QString::fromUtf8 ("x")));
  }
}

void MacroEditorPage::set_nindent (int n)
{
  m_nindent = n;
}

void MacroEditorPage::set_font (const std::string &family, int size)
{
  QFont f = font ();
  if (! family.empty ()) {
    f.setFamily (tl::to_qstring (family));
  } else {
    f.setFamily (QString::fromUtf8 ("Monospace"));
  }
  f.setFixedPitch (true);
  if (size > 0) {
    f.setPointSize (size);
  }
  mp_text->setFont (f);
}

void MacroEditorPage::apply_attributes ()
{
  if (mp_highlighter) {
    mp_highlighter->rehighlight ();
  }
}

void MacroEditorPage::connect_macro (lym::Macro *macro)
{
  if (mp_macro != macro) {

    if (mp_highlighter) {
      delete mp_highlighter;
      mp_highlighter = 0;
    }

    if (mp_macro) {
      disconnect (mp_macro, SIGNAL (changed ()), this, SLOT (update ()));
    }

    mp_macro = macro;

    if (mp_macro) {

      connect (mp_macro, SIGNAL (changed ()), this, SLOT (update ()));

      lym::Macro::Interpreter lang = macro->interpreter ();
      if (lang == lym::Macro::DSLInterpreter) {
        lang = lym::MacroInterpreter::debugger_scheme (macro->dsl_interpreter ());
      }

      mp_exec_model->set_interpreter (lang);

      mp_text->blockSignals (true);
      mp_text->setPlainText (tl::to_qstring (mp_macro->text ()));
      mp_text->setReadOnly (macro->is_readonly ());
      mp_readonly_label->setVisible (macro->is_readonly ());
      mp_highlighter = mp_highlighters->highlighter_for (mp_text, mp_macro->interpreter (), mp_macro->dsl_interpreter ());
      if (mp_highlighter) {
        mp_highlighter->setDocument (mp_text->document ());
      }
      mp_text->blockSignals (false);

      m_is_modified = false;

    } else {
      mp_exec_model->set_interpreter (lym::Macro::None);
    }

    mp_side_panel->set_watermark (mp_macro ? tl::to_qstring (mp_macro->interpreter_name ()) : QString ());

  }
}

bool
MacroEditorPage::find_prev ()
{
  update_extra_selections ();

  QTextCursor c = mp_text->textCursor ();

  bool first = true;
  for (QTextBlock b = c.block (); true; ) {

    int o = (first ? c.position () - b.position () : -1);
    first = false;

    int i = -1;
    int l = 0;
    int p = 0;
    while (true) {
      int ii = m_current_search.indexIn (b.text (), p);
      if (ii >= 0 && (o < 0 || ii < o)) {
        l = m_current_search.matchedLength ();
        i = ii;
        p = ii + 1;
      } else {
        break;
      }
    }
    if (i >= 0) {
      QTextCursor newc (b);
      newc.setPosition (i + b.position () + l);
      newc.setPosition (i + b.position (), QTextCursor::KeepAnchor);
      mp_text->setTextCursor (newc);
      return true;
    }

    if (b == mp_text->document ()->begin()) {
      b = mp_text->document ()->end ();
    }
    b = b.previous ();
    if (b == c.block ()) {
      break;
    }

  }

  return false;
}

bool
MacroEditorPage::find_next ()
{
  update_extra_selections ();

  QTextCursor c = mp_text->textCursor ();

  bool first = true;
  for (QTextBlock b = c.block (); true; ) {

    int o = (first ? std::max (0, c.position () + 1 - b.position ()) : 0);
    first = false;

    int i = m_current_search.indexIn (b.text (), o);
    if (i >= 0) {
      QTextCursor newc (b);
      newc.setPosition (i + b.position () + m_current_search.matchedLength ());
      newc.setPosition (i + b.position (), QTextCursor::KeepAnchor);
      mp_text->setTextCursor (newc);
      emit edit_trace (false);
      return true;
    }

    b = b.next ();
    if (b == mp_text->document ()->end()) {
      b = mp_text->document ()->begin ();
    }
    if (b == c.block ()) {
      break;
    }

  }

  return false;
}

void
MacroEditorPage::set_editor_focus ()
{
  mp_text->setFocus (Qt::OtherFocusReason);
}

static QString interpolate_string (const QString &replace, const QRegExp &re)
{
  if (re.patternSyntax () == QRegExp::FixedString) {

    return replace;

  } else {

    QString r = replace;
    //  In some older Qt versions, capturedTexts is not const:
    QStringList ct = const_cast<QRegExp &> (re).capturedTexts ();

    //  TODO: this is simple implementation for the \\ sequence which also implies that "\ " is an escape sequence
    r.replace (QString::fromUtf8 ("\\ "), QString::fromUtf8 (" "));
    r.replace (QString::fromUtf8 ("\\\\"), QString::fromUtf8 ("\\ "));
    for (int i = ct.size () - 1; i >= 0; --i) {
      r.replace (QString::fromUtf8 ("\\") + QString::number (i), ct [i]);
    }
    r.replace (QString::fromUtf8 ("\\ "), QString::fromUtf8 ("\\"));

    return r;

  }
}

void 
MacroEditorPage::replace_and_find_next (const QString &replace)
{
  if (! mp_macro || mp_macro->is_readonly ()) {
    return;
  }

  QTextCursor c = mp_text->textCursor ();
  if (c.hasSelection ()) {
    c.insertText (interpolate_string (replace, m_current_search));
  }

  find_next ();
}

void 
MacroEditorPage::replace_all (const QString &replace)
{
  if (! mp_macro || mp_macro->is_readonly ()) {
    return;
  }

  const QTextDocument *doc = mp_text->document ();

  QTextCursor c = mp_text->textCursor ();
  c.beginEditBlock ();

  for (QTextBlock b = doc->begin(); b != doc->end(); b = b.next()) {

    int o = 0;

    while (true) {

      int i = m_current_search.indexIn (b.text (), o);
      if (i < 0) {
        break;
      } else if (m_current_search.matchedLength () == 0) {
        break;  //  avoid an infinite loop
      }

      QString r = interpolate_string (replace, m_current_search);

      c.setPosition (i + b.position () + m_current_search.matchedLength ());
      c.setPosition (i + b.position (), QTextCursor::KeepAnchor);
      c.insertText (r);

      o = i + r.size ();

   }

  }

  c.endEditBlock ();
}

void 
MacroEditorPage::set_search (const QRegExp &text)
{
  m_current_search = text;
  m_error_line = -1;
  update_extra_selections ();
}

void 
MacroEditorPage::set_error_line (int line)
{
  m_error_line = line - 1;
  goto_line (line);
  update_extra_selections ();
}

void
MacroEditorPage::goto_line (int line)
{
  if (line > 0) {
    const QTextDocument *doc = mp_text->document ();
    for (QTextBlock b = doc->begin(); b != doc->end(); b = b.next()) {
      if (firstLineNumber (b) + 1 == line) {
        mp_text->setTextCursor (QTextCursor (b));
        mp_text->ensureCursorVisible ();
        break;
      }
    }
  }
}

void
MacroEditorPage::goto_position (int line, int pos)
{
  if (line > 0) {
    const QTextDocument *doc = mp_text->document ();
    for (QTextBlock b = doc->begin(); b != doc->end(); b = b.next()) {
      if (firstLineNumber (b) + 1 == line) {
        QTextCursor cursor (b);
        cursor.movePosition (QTextCursor::Right, QTextCursor::MoveAnchor, pos);
        mp_text->setTextCursor (cursor);
        mp_text->ensureCursorVisible ();
        break;
      }
    }
  }
}

void
MacroEditorPage::update_extra_selections ()
{
  QList<QTextEdit::ExtraSelection> extra_selections;

  if (m_error_line >= 0) {

    const QTextDocument *doc = mp_text->document ();
    for (QTextBlock b = doc->begin(); b != doc->end(); b = b.next()) {
      if (firstLineNumber (b) == m_error_line) {
        QTextEdit::ExtraSelection es;
        es.cursor = QTextCursor (b);
        es.cursor.select (QTextCursor::LineUnderCursor);
        es.format.setBackground (QColor (Qt::red).lighter ());
        extra_selections.push_back (es);
        break;
      }
    }

  } else if (mp_exec_model->run_mode () && mp_exec_model->current_line () >= 0) {

    const QTextDocument *doc = mp_text->document ();
    for (QTextBlock b = doc->begin(); b != doc->end(); b = b.next()) {
      if (firstLineNumber (b) == mp_exec_model->current_line () - 1) {
        QTextEdit::ExtraSelection es;
        es.cursor = QTextCursor (b);
        es.cursor.select (QTextCursor::LineUnderCursor);
        es.format.setBackground (QColor (Qt::lightGray));
        extra_selections.push_back (es);
        break;
      }
    }

  } else if (m_current_search != QRegExp ()) {

    const QTextDocument *doc = mp_text->document ();
    for (QTextBlock b = doc->begin(); b != doc->end(); b = b.next()) {
      QString t = b.text ();
      int o = 0;
      int i = 0;
      while ((i = m_current_search.indexIn (t, o)) >= 0) {
        int l = m_current_search.matchedLength ();
        if (l > 0) {
          o = i + l;
          QTextEdit::ExtraSelection es;
          es.cursor = QTextCursor (b);
          es.cursor.setPosition (b.position () + i);
          es.cursor.movePosition (QTextCursor::NextCharacter, QTextCursor::KeepAnchor, l);
          es.format.setBackground (Qt::yellow);
          extra_selections.push_back (es);
        } else {
          //  avoid endless loop on empty search
          break;
        }
      }
    }

  }

  mp_text->setExtraSelections (extra_selections);
}

int
MacroEditorPage::current_line () const
{
  return firstLineNumber (mp_text->textCursor ().block ()) + 1;
}

int
MacroEditorPage::current_pos () const
{
  return mp_text->textCursor ().position () - mp_text->textCursor ().block ().position ();
}

bool
MacroEditorPage::eventFilter (QObject *watched, QEvent *event)
{
  if (watched == mp_text) {

    if (event->type () == QEvent::KeyPress) {

      m_error_line = -1;
      mp_text->setExtraSelections (QList<QTextEdit::ExtraSelection> ());

      QKeyEvent *ke = dynamic_cast<QKeyEvent *> (event);
      if (ke && ke->key () == Qt::Key_Tab && (ke->modifiers () & Qt::ShiftModifier) == 0) {

        if (!mp_text->isReadOnly ()) {

          QTextBlock bs, be;
          bool adjust_end = false;

          bool indent = false;
          if (mp_text->textCursor ().hasSelection ()) {
            bs = mp_text->document ()->findBlock (mp_text->textCursor ().selectionStart ());
            be = mp_text->document ()->findBlock (mp_text->textCursor ().selectionEnd ());
            if (be != bs) {
              indent = true;
              QTextCursor se (mp_text->document ());
              se.setPosition (mp_text->textCursor ().selectionEnd ());
              if (se.atBlockStart ()) {
                be = be.previous ();
                adjust_end = true;
              }
            }
          }

          if (indent) {

            //  tab out
            QTextCursor c (mp_text->document ());
            c.setPosition (bs.position ());
            c.beginEditBlock ();

            for (QTextBlock b = bs; ; b = b.next()) {

              c.setPosition (b.position ());
              QString text = b.text ();

              bool has_tabs = false;
              int p = 0;
              int i = 0;
              for (; i < text.length (); ++i) {
                if (text [i] == QChar::fromLatin1 (' ')) {
                  ++p;
                } else if (text [i] == QChar::fromLatin1 ('\t')) {
                  p = (p - p % m_ntab) + m_ntab;
                  has_tabs = true;
                } else {
                  break;
                }
              }

              if (has_tabs) {
                for ( ; i > 0; --i) {
                  c.deleteChar ();
                }
                c.insertText (QString (m_nindent + p, QChar::fromLatin1 (' ')));
              } else {
                c.insertText (QString (m_nindent, QChar::fromLatin1 (' ')));
              }

              if (b == be) {
                break;
              }

            }

            c.endEditBlock ();

            c.setPosition (bs.position ());
            if (adjust_end) {
              c.setPosition (be.next ().position (), QTextCursor::KeepAnchor);
            } else {
              c.setPosition (be.position () + be.text ().length (), QTextCursor::KeepAnchor);
            }
            mp_text->setTextCursor (c);

          } else {

            QTextCursor c = mp_text->textCursor ();
            QString text = c.block ().text ();
            int col = c.position () - c.block ().position ();

            int p = 0;
            for (int i = 0; i < text.length () && i < col; ++i) {
              if (text [i] == QChar::fromLatin1 ('\t')) {
                p = (p - p % m_ntab) + m_ntab;
              } else {
                ++p;
              }
            }

            c.insertText (QString (m_nindent - p % m_nindent, QChar::fromLatin1 (' ')));
            mp_text->setTextCursor (c);

          }

        }

        return true;

      } else if (ke && (ke->key () == Qt::Key_Backtab || (ke->key () == Qt::Key_Tab && (ke->modifiers () & Qt::ShiftModifier) != 0))) {
        
        if (mp_text->textCursor ().hasSelection () && !mp_text->isReadOnly ()) {

          //  tab in 
          QTextBlock bs = mp_text->document ()->findBlock (mp_text->textCursor ().selectionStart ());
          QTextBlock be = mp_text->document ()->findBlock (mp_text->textCursor ().selectionEnd ());
          bool adjust_end = false;
          if (be != bs) {
            QTextCursor se (mp_text->document ());
            se.setPosition (mp_text->textCursor ().selectionEnd ());
            if (se.atBlockStart ()) {
              be = be.previous ();
              adjust_end = true;
            }
          }

          QTextCursor c (mp_text->document ());
          c.setPosition (bs.position ());
          c.beginEditBlock ();

          for (QTextBlock b = bs; ; b = b.next()) {

            c.setPosition (b.position ());
            QString text = b.text ();
            int n = m_nindent;
            int p = 0;
            for (int i = 0; i < text.length () && n > 0; ++i) {
              if (text [i] == QChar::fromLatin1 (' ')) {
                ++p;
                --n;
                c.deleteChar ();
              } else if (text [i] == QChar::fromLatin1 ('\t')) {
                c.deleteChar ();
                int pp = p;
                p = (p - p % m_ntab) + m_ntab;
                if (p - pp >= n) {
                  if (p - pp > n) {
                    c.insertText (QString (p - pp - n, QChar::fromLatin1 (' ')));
                  }
                  n = 0;
                } else {
                  n -= p - pp;
                }
              } else {
                break;
              }
            }

            if (b == be) {
              break;
            }

          }

          c.endEditBlock ();

          c.setPosition (bs.position ());
          if (adjust_end) {
            c.setPosition (be.next ().position (), QTextCursor::KeepAnchor);
          } else {
            c.setPosition (be.position () + be.text ().length (), QTextCursor::KeepAnchor);
          }
          mp_text->setTextCursor (c);

        }

        return true;

      } else if (ke && ke->key () == Qt::Key_Backspace) {

        if (!mp_text->textCursor ().hasSelection () && !mp_text->isReadOnly()) {

          QTextCursor c = mp_text->textCursor ();
          QString text = c.block ().text ();
          int col = c.position () - c.block ().position ();
          if (col > 0) {

            int p = 0;
            bool only_space_before = true;

            for (int i = 0; i < text.length () && i < col; ++i) {
              if (text [i] == QChar::fromLatin1 ('\t')) {
                p = (p - p % m_ntab) + m_ntab;
              } else if (text [i] == QChar::fromLatin1 (' ')) {
                ++p;
              } else {
                only_space_before = false;
              }
            }

            if (only_space_before) {

              for (int i = 0; i < col; ++i) {
                c.deletePreviousChar ();
              }

              c.insertText (QString (std::max (0, ((p - 1) / m_nindent) * m_nindent), QChar::fromLatin1 (' ')));
              mp_text->setTextCursor (c);

              return true;

            }

          }

        }

      } else if (ke && ke->key () == Qt::Key_Escape) {

        //  Handle Esc to clear the selection

        QTextCursor c = mp_text->textCursor ();
        c.clearSelection ();
        mp_text->setTextCursor (c);

        return true;

      } else if (ke && ke->key () == Qt::Key_Return) {

        if (!mp_text->isReadOnly ()) {

          //  Implement auto-indent on return

          QTextCursor c = mp_text->textCursor ();
          QTextBlock b = c.block ();

          c.insertBlock ();

          QString l;
          if (b.isValid ()) {
            QString text = b.text ();
            for (int i = 0; i < text.length (); ++i) {
              if (text [i] == QChar::fromLatin1 ('\t') || text [i] == QChar::fromLatin1 (' ')) {
                l += text [i];
              } else {
                break;
              }
            }
          }

          c.insertText (l);
          mp_text->setTextCursor (c);

          return true;

        }

      } else if (ke && ke->key () == Qt::Key_F1) {

        QTextCursor c = mp_text->textCursor ();
        if (c.selectionStart () == c.selectionEnd ()) {
          c.select (QTextCursor::WordUnderCursor);
        }
        emit help_requested (c.selectedText ());

        return true;

      } else if (ke && ke->key () == Qt::Key_F3) {

        //  Jump to the next occurence of the search string

        if (m_current_search != QRegExp ()) {
          if ((ke->modifiers () & Qt::ShiftModifier) != 0) {
            find_prev ();
          } else {
            find_next ();
          }
          update_extra_selections ();
        }

        return true;

      }

    }

  }

  return false;
}

}

