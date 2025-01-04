
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

#if defined(HAVE_QT)

#include "layGenericSyntaxHighlighter.h"

#include "tlString.h"

#include <iostream>
#include <limits>
#include <memory>
#include <cstdio>

#include <QDomDocument>
#include <QRegExp>

// #define DEBUG_HIGHLIGHTER

/**
 *  @brief Provide a compare operator for QList<QString> because Qt doesn't
 */
static bool operator<(const QList<QString> &a, const QList<QString> &b)
{
  if (a.size () != b.size ()) {
    return a.size () < b.size ();
  } else {
    for (QList<QString>::const_iterator ia = a.begin (), ib = b.begin (); ia != a.end (); ++ia, ++ib) {
      if (*ia != *ib) {
        return *ia < *ib;
      }
    }
    return false;
  }
}

inline bool 
is_word_char (QChar c)
{
  return c.isLetterOrNumber () || c == QChar::fromLatin1 ('_');
}

namespace lay
{

/**
 *  @brief A helper function to replace the tokens in dynamic rules
 */
static QString replace_tokens (const QString &input, const QList<QString> &input_args)
{
  QString output = input;

  int i = 1;
  for (QList<QString>::const_iterator a = input_args.begin (); a != input_args.end (); ++a, ++i) {
    QString tok (2, QChar::fromLatin1 ('%'));
    tok[1] = QChar ('0' + i);
    output.replace (tok, *a);
  }

  return output;
}

// --------------------------------------------------------------------------------------------
//  Implementation of GenericSyntaxHighlighterRuleBase

GenericSyntaxHighlighterRuleBase::GenericSyntaxHighlighterRuleBase ()
{
  //  .. nothing yet ..
}

GenericSyntaxHighlighterRuleBase::~GenericSyntaxHighlighterRuleBase ()
{
  //  .. nothing yet ..
}

// --------------------------------------------------------------------------------------------
//  Implementation of GenericSyntaxHighlighterRuleStringList

GenericSyntaxHighlighterRuleStringList::GenericSyntaxHighlighterRuleStringList (const QList<QString> &sl)
{
  m_min_length = std::numeric_limits<int>::max ();
  for (QList<QString>::const_iterator s = sl.begin (); s != sl.end (); ++s) {
    m_s.insert (*s);
    m_min_length = std::min (m_min_length, int (s->length ()));
  }
}

GenericSyntaxHighlighterRuleStringList::GenericSyntaxHighlighterRuleStringList (const std::set<QString> &s, int ml)
  : m_s (s), m_min_length (ml)
{
  //  .. nothing yet ..
}

GenericSyntaxHighlighterRuleBase *GenericSyntaxHighlighterRuleStringList::clone () const
{
  return new GenericSyntaxHighlighterRuleStringList (m_s, m_min_length);
}

bool GenericSyntaxHighlighterRuleStringList::match (const QString &input, unsigned int /*generation_id*/, int index, int &end_index, const QList<QString> & /*input_args*/, QList<QString> & /*output_args*/) const
{
  if (input.length () - index < m_min_length) {
    return false;
  }

  if (index > 0 && is_word_char (input [index - 1])) {
    return false;
  }

  QString ms = input.mid (index);
  std::set<QString>::const_iterator sp = m_s.upper_bound (ms);
  if (sp != m_s.begin ()) {
    --sp;
    if (ms.startsWith (*sp) && (index + sp->length () == input.length () || ! is_word_char (input [index + sp->length ()]))) {
      end_index = index + sp->length ();
      return true;
    }
  }

  return false;
}

void GenericSyntaxHighlighterRuleStringList::dump () const
{
  if (! m_s.empty ()) {
    std::cout << "    rule(string list) '" << tl::to_string (*m_s.begin ()) << " ...'" << std::endl;
  } else {
    std::cout << "    rule(string list) ''" << std::endl;
  }
}

// --------------------------------------------------------------------------------------------
//  Further rule specializations

/**
 *  @brief A specialization of GenericSyntaxHighlighterRuleBase which looks for a string
 */
class GenericSyntaxHighlighterRuleString
  : public GenericSyntaxHighlighterRuleBase
{
public:
  GenericSyntaxHighlighterRuleString (const QString &s, bool insensitive = false, bool dynamic = false)
    : m_s (s), m_insensitive (insensitive), m_dynamic (dynamic)
  {
    //  .. nothing yet ..
  }

  virtual GenericSyntaxHighlighterRuleBase *clone () const
  {
    return new GenericSyntaxHighlighterRuleString (m_s, m_dynamic);
  }

  virtual bool match (const QString &input, unsigned int /*generation_id*/, int index, int &end_index, const QList<QString> &input_args, QList<QString> & /*output_args*/) const
  {
    QString s;
    const QString *ps = &m_s;

    if (m_dynamic) {
      s = replace_tokens (m_s, input_args);
      ps = &s;
    }

    if (input.length () - index < ps->length ()) {
      return false;
    }

    for (int i = 0; i < ps->length (); ++i) {
      if (input [i + index] != (*ps) [i]) {
        return false;
      }
    }

    end_index = index + ps->length ();
    return true;
  }

  virtual void dump () const
  {
    std::cout << "    rule(string) '" << tl::to_string (m_s) << "' dynamic=" << m_dynamic << ", insensitive=" << m_insensitive << std::endl;
  }

private:
  QString m_s;
  bool m_insensitive, m_dynamic;
};

/**
 *  @brief A specialization of GenericSyntaxHighlighterRuleBase which looks for a range between two strings
 */
class GenericSyntaxHighlighterRuleRange
  : public GenericSyntaxHighlighterRuleBase
{
public:
  GenericSyntaxHighlighterRuleRange (const QString &s1, const QString &s2, bool dynamic = false)
    : m_s1 (s1), m_s2 (s2), m_dynamic (dynamic)
  {
    //  .. nothing yet ..
  }

  virtual GenericSyntaxHighlighterRuleBase *clone () const
  {
    return new GenericSyntaxHighlighterRuleRange (m_s1, m_s2, m_dynamic);
  }

  virtual bool match (const QString &input, unsigned int /*generation_id*/, int index, int &end_index, const QList<QString> &input_args, QList<QString> & /*output_args*/) const
  {
    QString s;
    const QString *ps = &m_s1;

    if (m_dynamic) {
      s = replace_tokens (*ps, input_args);
      ps = &s;
    }

    if (input.length () - index < ps->length ()) {
      return false;
    }

    for (int i = 0; i < ps->length (); ++i) {
      if (input [i + index] != (*ps) [i]) {
        return false;
      }
    }

    index = index + ps->length ();

    ps = &m_s2;
    if (m_dynamic) {
      s = replace_tokens (*ps, input_args);
      ps = &s;
    }

    if (input.length () - index < ps->length ()) {
      return false;
    }

    index = input.indexOf (*ps, index); 
    if (index < 0) {
      return false;
    }

    end_index = index + ps->length ();
    return true;
  }

  virtual void dump () const
  {
    std::cout << "    rule(range) '" << tl::to_string (m_s1) << "'..'" << tl::to_string (m_s2) << "' dynamic=" << m_dynamic << std::endl;
  }

private:
  QString m_s1, m_s2;
  bool m_dynamic;
};

/**
 *  @brief A specialization of GenericSyntaxHighlighterRuleBase which looks for an integer value
 */
class GenericSyntaxHighlighterRuleInt
  : public GenericSyntaxHighlighterRuleBase
{
public:
  GenericSyntaxHighlighterRuleInt ()
  {
    //  .. nothing yet ..
  }

  virtual GenericSyntaxHighlighterRuleBase *clone () const
  {
    return new GenericSyntaxHighlighterRuleInt ();
  }

  virtual bool match (const QString &input, unsigned int /*generation_id*/, int index, int &end_index, const QList<QString> & /*input_args*/, QList<QString> & /*output_args*/) const
  {
    end_index = index;
    if (end_index < input.length () && input [end_index] == QChar::fromLatin1 ('-')) {
      ++end_index;
    }

    bool any = false;
    while (end_index < input.length () && input [end_index].isNumber ()) {
      any = true;
      ++end_index;
    }

    return any;
  }

  virtual void dump () const
  {
    std::cout << "    rule(int)" << std::endl;
  }
};

/**
 *  @brief A specialization of GenericSyntaxHighlighterRuleBase which looks for a float value
 */
class GenericSyntaxHighlighterRuleFloat
  : public GenericSyntaxHighlighterRuleBase
{
public:
  GenericSyntaxHighlighterRuleFloat ()
  {
    //  .. nothing yet ..
  }

  virtual GenericSyntaxHighlighterRuleBase *clone () const
  {
    return new GenericSyntaxHighlighterRuleFloat ();
  }

  virtual bool match (const QString &input, unsigned int /*generation_id*/, int index, int &end_index, const QList<QString> & /*input_args*/, QList<QString> & /*output_args*/) const
  {
    bool any = false;

    end_index = index;
    if (end_index < input.length () && input [end_index] == QChar::fromLatin1 ('-')) {
      ++end_index;
    }

    while (end_index < input.length () && input [end_index].isNumber ()) {
      any = true;
      ++end_index;
    }

    if (end_index < input.length () && input [end_index] == QChar::fromLatin1 ('.')) {
      ++end_index;
      while (end_index < input.length () && input [end_index].isNumber ()) {
        any = true;
        ++end_index;
      }
    }

    if (! any) {
      return false;
    }

    if (end_index < input.length () && input [end_index].toLower () == QChar::fromLatin1 ('e')) {
      ++end_index;
      if (end_index < input.length () && input [end_index] == QChar::fromLatin1 ('-')) {
        ++end_index;
      }
      while (end_index < input.length () && input [end_index].isNumber ()) {
        ++end_index;
      }
    }

    return true;
  }

  virtual void dump () const
  {
    std::cout << "    rule(float)" << std::endl;
  }
};

/**
 *  @brief A specialization of GenericSyntaxHighlighterRuleBase which looks for an identifier
 */
class GenericSyntaxHighlighterRuleIdentifier
  : public GenericSyntaxHighlighterRuleBase
{
public:
  GenericSyntaxHighlighterRuleIdentifier ()
  {
    //  .. nothing yet ..
  }

  virtual GenericSyntaxHighlighterRuleBase *clone () const
  {
    return new GenericSyntaxHighlighterRuleIdentifier ();
  }

  virtual bool match (const QString &input, unsigned int /*generation_id*/, int index, int &end_index, const QList<QString> & /*input_args*/, QList<QString> & /*output_args*/) const
  {
    for (end_index = index; end_index != input.length (); ++end_index) {
      if (end_index == index && ! input [end_index].isLetter ()) {
        break;
      } else if (! input [end_index].isLetterOrNumber ()) {
        break;
      }
    }
    return end_index != index;
  }

  virtual void dump () const
  {
    std::cout << "    rule(identifier)" << std::endl;
  }
};

/**
 *  @brief A specialization of GenericSyntaxHighlighterRuleBase which looks for a line continuation
 */
class GenericSyntaxHighlighterRuleLineContinue
  : public GenericSyntaxHighlighterRuleBase
{
public:
  GenericSyntaxHighlighterRuleLineContinue ()
  {
    //  .. nothing yet ..
  }

  virtual GenericSyntaxHighlighterRuleBase *clone () const
  {
    return new GenericSyntaxHighlighterRuleLineContinue ();
  }

  virtual bool match (const QString &input, unsigned int /*generation_id*/, int index, int & /*end_index*/, const QList<QString> & /*input_args*/, QList<QString> & /*output_args*/) const
  {
    return input.length () == index + 1 && input [index] == QChar::fromLatin1 ('\\');
  }

  virtual void dump () const
  {
    std::cout << "    rule(line continue)" << std::endl;
  }
};

/**
 *  @brief A specialization of GenericSyntaxHighlighterRuleBase which looks for spaces
 */
class GenericSyntaxHighlighterRuleSpaces
  : public GenericSyntaxHighlighterRuleBase
{
public:
  GenericSyntaxHighlighterRuleSpaces ()
  {
    //  .. nothing yet ..
  }

  virtual GenericSyntaxHighlighterRuleBase *clone () const
  {
    return new GenericSyntaxHighlighterRuleSpaces ();
  }

  virtual bool match (const QString &input, unsigned int /*generation_id*/, int index, int &end_index, const QList<QString> & /*input_args*/, QList<QString> & /*output_args*/) const
  {
    bool any = false;
    while (index < input.length () && input [index].isSpace ()) {
      any = true;
      ++index;
    }
    if (any) {
      end_index = index;
      return true;
    } else {
      return false;
    }
  }

  virtual void dump () const
  {
    std::cout << "    rule(spaces)" << std::endl;
  }
};

/**
 *  @brief A specialization of GenericSyntaxHighlighterRuleBase which looks for a character from a range
 */
class GenericSyntaxHighlighterRuleAnyChar
  : public GenericSyntaxHighlighterRuleBase
{
public:
  GenericSyntaxHighlighterRuleAnyChar (const QString &s)
    : m_s (s)
  {
    //  .. nothing yet ..
  }

  virtual GenericSyntaxHighlighterRuleBase *clone () const
  {
    return new GenericSyntaxHighlighterRuleAnyChar (m_s);
  }

  virtual bool match (const QString &input, unsigned int /*generation_id*/, int index, int &end_index, const QList<QString> & /*input_args*/, QList<QString> & /*output_args*/) const
  {
    if (m_s.indexOf (input [index]) >= 0) {
      end_index = index + 1;
      return true;
    } else {
      return false;
    }
  }

  virtual void dump () const
  {
    std::cout << "    rule(any char) '" << tl::to_string (m_s) << std::endl;
  }

private:
  QString m_s;
};

/**
 *  @brief A specialization of GenericSyntaxHighlighterRuleBase which looks for a regexp
 */
class GenericSyntaxHighlighterRuleRegExp
  : public GenericSyntaxHighlighterRuleBase
{
public:
  enum anchor_t { AnchorNone = 0, AnchorWB, AnchorNWB, AnchorStart };

  GenericSyntaxHighlighterRuleRegExp (const QString &re, bool dynamic = false)
    : m_re (re), m_dynamic (dynamic), m_c (QChar::Null), m_anchor (AnchorNone), m_last_generation_id (0), m_last_index (-1)
  {
    //  look for the shortcut character
    if (! m_dynamic && re.length () > 0 &&
      QString::fromUtf8 ("\\.[({^$|").indexOf (re [0]) < 0 && 
      (re.length () <= 1 || QString::fromUtf8 ("*?{").indexOf (re [1]) < 0)) {
      m_c = re [0];
    } else if (re.startsWith (QString::fromUtf8 ("\\b"))) {
      m_anchor = AnchorWB;
    } else if (re.startsWith (QString::fromUtf8 ("\\B"))) {
      m_anchor = AnchorNWB;
    } else if (re.startsWith (QString::fromUtf8 ("^"))) {
      m_anchor = AnchorStart;
    }
  }

  GenericSyntaxHighlighterRuleRegExp (const QRegExp &re, bool dynamic, QChar c)
    : m_re (re), m_dynamic (dynamic), m_c (c), m_anchor (AnchorNone), m_last_generation_id (0), m_last_index (-1)
  {
    //  .. nothing yet ..
  }

  virtual GenericSyntaxHighlighterRuleBase *clone () const
  {
    return new GenericSyntaxHighlighterRuleRegExp (m_re, m_dynamic, m_c);
  }

  virtual bool match (const QString &input, unsigned int generation_id, int index, int &end_index, const QList<QString> &input_args, QList<QString> &output_args) const
  {
    //  shortcut
    if (m_c != QChar::Null && (input.length () <= index || input [index] != m_c)) {
      return false;
    }

    //  anchor shortcut
    if (m_anchor != AnchorNone) {
      if (m_anchor == AnchorStart && index > 0) {
        return false;
      }
      if (m_anchor == AnchorWB || m_anchor == AnchorNWB) {
        bool at_wb = index <= 0 || !is_word_char (input [index - 1]);
        if ((m_anchor == AnchorWB) == !at_wb) {
          return false;
        }
      }
    }

    //  cache the next index in question
    if (! m_dynamic && generation_id == m_last_generation_id && (index < m_last_index || m_last_index < 0)) {
      return false;
    }

    m_last_generation_id = generation_id;
    m_last_index = -1;

    if (m_dynamic) {

      QRegExp re (replace_tokens (m_re.pattern (), input_args));
      int p = re.indexIn (input, index);
      m_last_index = p;
      if (p == index) {

        end_index = p + re.matchedLength ();
        if (re.capturedTexts ().size () > 1) {
          output_args = re.capturedTexts ();
          output_args.pop_front ();
        }
        return true;

      } else {
        return false;
      }

    } else {

      int p = m_re.indexIn (input, index);
      m_last_index = p;
      if (p == index) {

        end_index = p + m_re.matchedLength ();
        //  Qt 4.2.3 needs that const_cast:
        if (const_cast<QRegExp &> (m_re).capturedTexts ().size () > 1) {
          output_args = const_cast<QRegExp &> (m_re).capturedTexts ();
          output_args.pop_front ();
        }
        return true;

      } else {
        return false;
      }

    }

  }

  virtual void dump () const
  {
    std::cout << "    rule(regexp) '" << tl::to_string (m_re.pattern ()) << "' dynamic=" << m_dynamic << std::endl;
  }

private:
  QRegExp m_re;
  bool m_dynamic;
  QChar m_c;
  anchor_t m_anchor;
  mutable unsigned int m_last_generation_id;
  mutable int m_last_index;
};

// ---------------------------------------------------------------------------------
//  Implementation of GenericSyntaxHighlighterRule

GenericSyntaxHighlighterRule::GenericSyntaxHighlighterRule ()
  : mp_rule (0), m_attribute_id (-1), m_target_context_id (-1), m_owner (true), m_lookahead (false), m_first_non_space (false), m_column (-1)
{
  //  .. nothing yet ..
}

GenericSyntaxHighlighterRule::GenericSyntaxHighlighterRule (GenericSyntaxHighlighterRuleBase *rule, int attribute_id, int target_context_id, bool take_ownership)
  : mp_rule (rule), m_attribute_id (attribute_id), m_target_context_id (target_context_id), m_owner (take_ownership), m_lookahead (false), m_first_non_space (false), m_column (-1)
{
  //  .. nothing yet ..
}

GenericSyntaxHighlighterRule::GenericSyntaxHighlighterRule (const GenericSyntaxHighlighterRule &d)
{
  m_owner = d.m_owner;
  if (d.m_owner) {
    mp_rule = (d.mp_rule ? d.mp_rule->clone () : 0);
  } else {
    mp_rule = d.mp_rule;
  }
  m_attribute_id = d.m_attribute_id;
  m_target_context_id = d.m_target_context_id;
  m_lookahead = d.m_lookahead;
  m_first_non_space = d.m_first_non_space;
  m_column = d.m_column;
}

GenericSyntaxHighlighterRule::~GenericSyntaxHighlighterRule ()
{
  if (m_owner) {
    delete mp_rule;
  }
  mp_rule = 0;
}

GenericSyntaxHighlighterRule &
GenericSyntaxHighlighterRule::operator= (const GenericSyntaxHighlighterRule &d) 
{
  if (this != &d) {
    if (m_owner && mp_rule) {
      delete mp_rule;
    }
    m_owner = d.m_owner;
    m_lookahead = d.m_lookahead;
    m_first_non_space = d.m_first_non_space;
    m_column = d.m_column;
    if (m_owner) {
      mp_rule = d.mp_rule ? d.mp_rule->clone () : 0;
    } else {
      mp_rule = d.mp_rule;
    }
    m_attribute_id = d.m_attribute_id;
    m_target_context_id = d.m_target_context_id;
  }
  return *this;
}

bool 
GenericSyntaxHighlighterRule::match (const QString &input, unsigned int generation_id, int index, int &end_index, const QList<QString> &input_args, QList<QString> &output_args) const
{
  if (m_column >= 0 && std::max (0, index) != m_column) {
    return false;
  }

  if (m_first_non_space) {
    for (int i = std::max (0, index) - 1; i >= 0; --i) {
      if (! input[i].isSpace ()) {
        return false;
      }
    }
  }

  if (mp_rule && mp_rule->match (input, generation_id, index, end_index, input_args, output_args)) {

    if (m_lookahead) {
      end_index = index;
    }

    //  match child rules if there are some 
    int new_ei = 0;
    QList<QString> new_oa;

    for (std::list<GenericSyntaxHighlighterRule>::const_iterator r = m_child_rules.begin (); r != m_child_rules.end (); ++r) {
      if (r->match (input, generation_id, end_index, new_ei, input_args, new_oa)) {
        end_index = new_ei;
        break;
      }
    }

    return true;

  } else {
    return false;
  }
}

void 
GenericSyntaxHighlighterRule::dump () const
{
  std::cout << "    [attribute=" << m_attribute_id << ", context_id=" << m_target_context_id << ", column=" << m_column << ", first-non-space=" << m_first_non_space << ", lookahead=" << m_lookahead << "]" << std::endl;
  mp_rule->dump ();
  if (! m_child_rules.empty ()) {
    std::cout << "    <-- begin children -->" << std::endl;
    for (std::list<GenericSyntaxHighlighterRule>::const_iterator r = m_child_rules.begin (); r != m_child_rules.end (); ++r) {
      r->dump ();
    }
    std::cout << "    <-- end children -->" << std::endl;
  }
}

// ---------------------------------------------------------------------------------
//  Implementation of GenericSyntaxHighlighterContext

GenericSyntaxHighlighterContext::GenericSyntaxHighlighterContext ()
  : m_id (-1), m_fallthrough_context (no_context), m_linebegin_context (no_context), m_lineend_context (no_context), m_attribute_id (-1)
{
  // .. nothing yet ..
}

void 
GenericSyntaxHighlighterContext::add_rule (const GenericSyntaxHighlighterRule &rule)
{
  if (! rule.is_null ()) {
    m_rules.push_back (rule);
  }
}

bool
GenericSyntaxHighlighterContext::match (const QString &string, unsigned int generation_id, int index, int &end_index, const QList<QString> &input_args, QList<QString> &output_args, int &new_context, int &attribute_id) const
{
  end_index = index;
  output_args.clear ();
  new_context = no_context;
  attribute_id = m_attribute_id;

  if (index < 0) {
    // before the line
    index = 0;
    if (m_linebegin_context != no_context) {
      end_index = 0;
      new_context = m_linebegin_context;
      return true;
    }
  }

  if (index == string.length ()) {
    if (m_lineend_context != no_context && m_lineend_context != 0 /*#stay cannot be a lineend context*/) {
      end_index = index;
      new_context = m_lineend_context;
      return true;
    } else {
      return false;
    }
  }

  bool any_matched = false;
  bool has_fallthrough = (m_fallthrough_context != no_context && m_fallthrough_context != 0 /*fallthrough cannot be #stay*/);

  for (std::list<GenericSyntaxHighlighterRule>::const_iterator r = m_rules.begin (); r != m_rules.end (); ++r) {
    int ei = 0;
    QList<QString> oa;
    if (r->match (string, generation_id, index, ei, input_args, oa) && ei > end_index /*also avoids zero-width matches*/) {
      end_index = ei;
      output_args = oa;
      new_context = r->target_context_id ();
      attribute_id = r->attribute_id ();
      any_matched = true;
    }
  }

  if (any_matched) {
    return true;
  } else if (has_fallthrough) {
    end_index = index;
    new_context = m_fallthrough_context;
    return true;
  } else {
    return false;
  }
}

void 
GenericSyntaxHighlighterContext::include (const GenericSyntaxHighlighterContext &other)
{
  //  TODO: don't create copies here but rather reference?
  for (std::list<GenericSyntaxHighlighterRule>::const_iterator r = other.m_rules.begin (); r != other.m_rules.end (); ++r) {
    add_rule (*r);
  }
}

void 
GenericSyntaxHighlighterContext::dump () const
{
  std::cout << "  [context id=" << m_id << ", fallthrough=" << m_fallthrough_context << ", linebegin=" << m_linebegin_context << ", lineend=" << m_lineend_context << ", attribute=" << m_attribute_id << "]" << std::endl;
  for (std::list<GenericSyntaxHighlighterRule>::const_iterator r = m_rules.begin (); r != m_rules.end (); ++r) {
    std::cout << "  ";
    r->dump ();
  }
}

// ---------------------------------------------------------------------------------
//  Implementation of GenericSyntaxHighlighterContexts

GenericSyntaxHighlighterContexts::GenericSyntaxHighlighterContexts ()
  : m_initial_context (0)
{
  //  .. nothing yet ..
}

void 
GenericSyntaxHighlighterContexts::insert (const QString &name, const GenericSyntaxHighlighterContext &c)
{
  GenericSyntaxHighlighterContext &new_context = context (name);
  //  since the assignment destroys the ID, we have to restore it
  int id = new_context.id ();
  new_context = c;
  new_context.set_id (id);
  new_context.set_name (name);
  if (m_initial_context <= 0) {
    m_initial_context = id;
  }
}

GenericSyntaxHighlighterContext &
GenericSyntaxHighlighterContexts::context (const QString &name)
{
  std::map<QString, GenericSyntaxHighlighterContext>::iterator c = m_contexts_by_name.find (name);
  if (c != m_contexts_by_name.end ()) {
    return c->second;
  } else {
    GenericSyntaxHighlighterContext *context = &m_contexts_by_name.insert (std::make_pair (name, GenericSyntaxHighlighterContext ())).first->second;
    m_contexts_by_id.push_back (context);
    context->set_id (int (m_contexts_by_id.size ()));
    context->set_name (name);
    return *context;
  }
}

const GenericSyntaxHighlighterContext &
GenericSyntaxHighlighterContexts::context (const QString &name) const
{
  std::map<QString, GenericSyntaxHighlighterContext>::const_iterator c = m_contexts_by_name.find (name);
  tl_assert (c != m_contexts_by_name.end ());
  return c->second;
}

GenericSyntaxHighlighterContext &
GenericSyntaxHighlighterContexts::context (int id)
{
  tl_assert (id > 0 && id <= int (m_contexts_by_id.size ()));
  return *m_contexts_by_id [id - 1];
}

const GenericSyntaxHighlighterContext &
GenericSyntaxHighlighterContexts::context (int id) const
{
  tl_assert (id > 0 && id <= int (m_contexts_by_id.size ()));
  return *m_contexts_by_id [id - 1];
}

void 
GenericSyntaxHighlighterContexts::dump () const
{
    std::cout << "[contexts]" << std::endl;
  for (std::map<QString, GenericSyntaxHighlighterContext>::const_iterator c = m_contexts_by_name.begin (); c != m_contexts_by_name.end (); ++c) {
    std::cout << tl::to_string (c->first) << ":" << std::endl;
    c->second.dump ();
  }
}

// ---------------------------------------------------------------------------------
//  Implementation of GenericSyntaxHighlighterAttributes

GenericSyntaxHighlighterAttributes::GenericSyntaxHighlighterAttributes (const GenericSyntaxHighlighterAttributes *basic_attributes)
  : mp_basic_attributes (basic_attributes)
{
  if (! basic_attributes) {
    add (QString::fromUtf8 ("Normal"),            dsNormal,         false, false, false, false, 0,         0,         0,         0);
    add (QString::fromUtf8 ("Alert"),             dsAlert,          true,  false, false, false, "#BF0303", "#9C0D0D", "#F7E7E7", 0);
    add (QString::fromUtf8 ("Base-N Integer"),    dsBaseN,          false, false, false, false, "#B07E00", "#FFDD00", 0,         0);
    add (QString::fromUtf8 ("Character"),         dsChar,           false, false, false, false, "#FF80E0", "#FF80E0", 0,         0);
    add (QString::fromUtf8 ("Comment"),           dsComment,        false, true,  false, false, "#888786", "#A6C2E4", 0,         0);
    add (QString::fromUtf8 ("Data Type"),         dsDataType,       false, false, false, false, "#0057AE", "#00316E", 0,         0);
    add (QString::fromUtf8 ("Decimal/Value"),     dsDecVal,         false, false, false, false, "#B07E00", "#FFDD00", 0,         0);
    add (QString::fromUtf8 ("Error"),             dsError,          false, false, true,  false, "#BF0303", "#9C0D0D", 0,         0);
    add (QString::fromUtf8 ("Floating Point"),    dsFloat,          false, false, false, false, "#B07E00", "#FFDD00", 0,         0);
    add (QString::fromUtf8 ("Function"),          dsFunction,       false, false, false, false, "#442886", "#442886", 0,         0);
    add (QString::fromUtf8 ("Keyword"),           dsKeyword,        true,  false, false, false, 0,         0,         0,         0);
    add (QString::fromUtf8 ("Others"),            dsOthers,         false, false, false, false, "#006E26", "#80FF80", 0,         0);
    add (QString::fromUtf8 ("Region Marker"),     dsRegionMarker,   false, false, false, false, "#0057AE", "#00316E", "#E1EAF8", 0);
    add (QString::fromUtf8 ("String"),            dsString,         false, false, false, false, "#BF0303", "#9C0D0D", 0,         0);
    add (QString::fromUtf8 ("Operator"),          dsOperator,       false, false, false, false, "#1F1C1B", 0,         0,         0);
    add (QString::fromUtf8 ("Control Flow"),      dsControlFlow,    true,  false, false, false, "#1F1C1B", 0,         0,         0);
    add (QString::fromUtf8 ("Built-in"),          dsBuiltIn,        true,  false, false, false, "#644A9B", "#452886", 0,         0);
    add (QString::fromUtf8 ("Variable"),          dsVariable,       false, false, false, false, "#0057AE", "#00316e", 0,         0);
    add (QString::fromUtf8 ("Extension"),         dsExtension,      false, false, false, false, "#0095FF", 0,         0,         0);
    add (QString::fromUtf8 ("Preprocessor"),      dsPreprocessor,   false, false, false, false, "#006E28", "#006e28", 0,         0);
    add (QString::fromUtf8 ("Import"),            dsImport,         false, false, false, false, "#FF5500", "#FF5500", 0,         0);
    add (QString::fromUtf8 ("Verbatim String"),   dsVerbatimString, false, false, false, false, "#BF0303", "#9C0E0E", 0,         0);
    add (QString::fromUtf8 ("Special String"),    dsSpecialString,  false, false, false, false, "#FF5500", "#FF5500", 0,         0);
    add (QString::fromUtf8 ("Special Character"), dsSpecialChar,    false, false, false, false, "#3DAEE9", "#FCFCFC", 0,         0);
    add (QString::fromUtf8 ("Attribute"),         dsAttribute,      false, false, false, false, "#0057AE", "#00316E", 0,         0);
  }
}

void 
GenericSyntaxHighlighterAttributes::add (const QString &name, int id, bool bold, bool italic, bool underline, bool strikeout, const char *foreground, const char * /*fg_selected*/, const char *background, const char * /*bg_selected*/)
{
  QTextCharFormat fmt;
  if (bold) {
    fmt.setFontWeight (QFont::Bold);
  }
  if (italic) {
    fmt.setFontItalic (true);
  }
  if (underline) {
    fmt.setFontUnderline (true);
  }
  if (strikeout) {
    fmt.setFontStrikeOut (true);
  }
  if (foreground) {
    fmt.setForeground (QColor (QString::fromUtf8 (foreground)));
  }
  if (background) {
    fmt.setBackground (QColor (QString::fromUtf8 (background)));
  }
  //  TODO: fg_selected, bg_selected

  while (int (m_attributes.size ()) <= id) {
    m_attributes.push_back (std::make_pair (-1, QTextCharFormat ()));
  }
  m_attributes [id].second = fmt;
  m_ids.insert (std::make_pair (name, id));
}

void
GenericSyntaxHighlighterAttributes::assign (const GenericSyntaxHighlighterAttributes &other)
{
  m_attributes = other.m_attributes;
  m_ids = other.m_ids;
}

bool
GenericSyntaxHighlighterAttributes::has_attribute (const QString &name) const
{
  return m_ids.find (name) != m_ids.end ();
}

int 
GenericSyntaxHighlighterAttributes::id (const QString &name)
{
  std::map<QString, int>::const_iterator i = m_ids.find (name);
  if (i == m_ids.end ()) {
    int n = int (m_attributes.size ());
    m_attributes.push_back (std::make_pair (dsNormal, QTextCharFormat ()));
    m_ids.insert (std::make_pair (name, n));
    return n;
  } else {
    return i->second;
  }
}

int 
GenericSyntaxHighlighterAttributes::id (const QString &name) const
{
  std::map<QString, int>::const_iterator i = m_ids.find (name);
  tl_assert (i != m_ids.end ());
  return i->second;
}

QTextCharFormat  
GenericSyntaxHighlighterAttributes::specific_style (int id) const
{
  if (id >= 0 && id < int (m_attributes.size ())) {
    return m_attributes [id].second;
  } else {
    return QTextCharFormat ();
  }
}

int  
GenericSyntaxHighlighterAttributes::basic_id (int id) const
{
  if (id >= 0 && id < int (m_attributes.size ())) {
    return m_attributes [id].first;
  } else {
    return -1;
  }
}

void 
GenericSyntaxHighlighterAttributes::set_style (int id, const QTextCharFormat &format)
{
  if (id < 0 || id >= int (m_attributes.size ())) {
    return;
  } 

  m_attributes [id].second = format;
}

void 
GenericSyntaxHighlighterAttributes::set_styles (int id, int basic_style_id,  const QTextCharFormat &format)
{
  if (id < 0 || id >= int (m_attributes.size ())) {
    return;
  } 

  m_attributes [id].first = basic_style_id;
  m_attributes [id].second = format;
}

QTextCharFormat 
GenericSyntaxHighlighterAttributes::format_for (int id) const
{
  if (id < 0 || id >= int (m_attributes.size ())) {
    return QTextCharFormat ();
  } else {
    int bs = m_attributes[id].first;
    QTextCharFormat fmt;
    if (mp_basic_attributes) {
      fmt = mp_basic_attributes->format_for (bs);
    }
    fmt.merge (m_attributes[id].second);
    return fmt;
  }
}

std::string 
GenericSyntaxHighlighterAttributes::to_string () const
{
  std::string s;

  for (std::map<QString, int>::const_iterator id = m_ids.begin (); id != m_ids.end (); ++id) {

    if (! s.empty ()) {
      s += ",";
    }
    s += tl::to_quoted_string (tl::to_string (id->first)) + "=";

    QTextCharFormat style = specific_style (id->second);

    std::string a;
    if (style.hasProperty (QTextFormat::FontUnderline)) {
      if (! a.empty ()) {
        a += ",";
      }
      a += "underline:" + tl::to_string (style.boolProperty (QTextFormat::FontUnderline));
    }
    if (style.hasProperty (QTextFormat::FontStrikeOut)) {
      if (! a.empty ()) {
        a += ",";
      }
      a += "strikeout:" + tl::to_string (style.boolProperty (QTextFormat::FontStrikeOut));
    }
    if (style.hasProperty (QTextFormat::FontItalic)) {
      if (! a.empty ()) {
        a += ",";
      }
      a += "italic:" + tl::to_string (style.boolProperty (QTextFormat::FontItalic));
    }
    if (style.hasProperty (QTextFormat::FontWeight)) {
      if (! a.empty ()) {
        a += ",";
      }
      a += "bold:" + tl::to_string (style.intProperty (QTextFormat::FontWeight) == QFont::Bold);
    }
    if (style.hasProperty (QTextFormat::ForegroundBrush)) {
      if (! a.empty ()) {
        a += ",";
      }
      a += "color:" + tl::to_quoted_string (tl::to_string (style.brushProperty (QTextFormat::ForegroundBrush).color ().name ()));
    }
    if (style.hasProperty (QTextFormat::BackgroundBrush)) {
      if (! a.empty ()) {
        a += ",";
      }
      a += "background:" + tl::to_quoted_string (tl::to_string (style.brushProperty (QTextFormat::BackgroundBrush).color ().name ()));
    }

    s += "(" + a + ")";

  }

  s += ";";

  return s;
}

void 
GenericSyntaxHighlighterAttributes::read (tl::Extractor &ex)
{
  while (! ex.at_end () && ! ex.test (";")) {

    std::string s;
    ex.read_quoted (s);

    ex.test ("=");
    ex.test ("(");

    QTextCharFormat style;

    while (! ex.at_end () && ! ex.test (")")) {

      if (ex.test ("underline")) {

        ex.test (":");
        bool f = false;
        ex.read (f);
        style.setProperty (QTextFormat::FontUnderline, f);

      } else if (ex.test ("strikeout")) {

        ex.test (":");
        bool f = false;
        ex.read (f);
        style.setProperty (QTextFormat::FontStrikeOut, f);

      } else if (ex.test ("italic")) {

        ex.test (":");
        bool f = false;
        ex.read (f);
        style.setProperty (QTextFormat::FontItalic, f);

      } else if (ex.test ("bold")) {

        ex.test (":");
        bool f = false;
        ex.read (f);
        style.setProperty (QTextFormat::FontWeight, f ? QFont::Bold : QFont::Normal);

      } else if (ex.test ("color")) {

        ex.test (":");
        std::string cs;
        ex.read_quoted (cs);
        style.setProperty (QTextFormat::ForegroundBrush, QBrush (QColor (tl::to_qstring (cs))));

      } else if (ex.test ("background")) {

        ex.test (":");
        std::string cs;
        ex.read_quoted (cs);
        style.setProperty (QTextFormat::BackgroundBrush, QBrush (QColor (tl::to_qstring (cs))));

      } else {
        ++ex;
      }

      ex.test (",");

    }

    for (std::map<QString, int>::const_iterator id = m_ids.begin (); id != m_ids.end (); ++id) {
      if (tl::to_string (id->first) == s) {
        set_style (id->second, style);
        break;
      }
    }

    ex.test (",");

  }

}

// ---------------------------------------------------------------------------------
//  Implementation of GenericSyntaxHighlighterState

GenericSyntaxHighlighterState::GenericSyntaxHighlighterState (const GenericSyntaxHighlighterContexts *contexts)
  : mp_contexts (contexts)
{
  //  Start with the first context and an empty input argument list
  m_stack.push_back (std::make_pair (mp_contexts->initial_context_id (), QList<QString> ()));
}

bool  
GenericSyntaxHighlighterState::operator< (const GenericSyntaxHighlighterState &d) const
{
  return m_stack < d.m_stack;
}

bool  
GenericSyntaxHighlighterState::operator== (const GenericSyntaxHighlighterState &d) const
{
  return m_stack == d.m_stack;
}

bool 
GenericSyntaxHighlighterState::match (const QString &string, unsigned int generation_id, int index, int &end_index, int &def_attribute_id, int &attribute_id)
{
  const GenericSyntaxHighlighterContext &ctx = mp_contexts->context (m_stack.back ().first);
  def_attribute_id = ctx.attribute_id ();

  int nc = 0;
  QList<QString> oa;
  if (ctx.match (string, generation_id, index, end_index, m_stack.back ().second, oa, nc, attribute_id)) {

    if (nc > 0) {
      m_stack.push_back (std::make_pair (nc, oa));
    } else if (nc < 0) {
      while (nc < 0 && ! m_stack.empty ()) {
        m_stack.pop_back ();
        ++nc;
      }
      if (m_stack.empty ()) {
        m_stack.push_back (std::make_pair (mp_contexts->initial_context_id (), QList<QString> ()));
      }
    }

    return true;

  } else {
    return false;
  }

}

int 
GenericSyntaxHighlighterState::current_context_id () const
{
  if (m_stack.empty ()) {
    return 0;
  } else {
    return m_stack.back ().first;
  }
}

// ---------------------------------------------------------------------------------
//  Implementation of GenericSyntaxHighlighter

static GenericSyntaxHighlighterRuleStringList 
parse_list (QDomElement e) 
{
  QList<QString> items;

  for (QDomNode n = e.firstChild(); !n.isNull(); n = n.nextSibling()) {
    if (n.isElement()) {
      QDomElement e = n.toElement ();
      if (e.tagName () == QString::fromUtf8 ("item")) {
        items.push_back (e.text ().trimmed ());
      }
    }
  }

  return GenericSyntaxHighlighterRuleStringList (items);
}

static int 
context_name_to_id (const QString &nr, GenericSyntaxHighlighterContexts &contexts)
{
  QString n = nr.trimmed ();
  if (n == QString::fromUtf8 ("#stay")) {
    return 0;
  } else if (n.startsWith (QString::fromUtf8 ("#pop"))) {
    return -(n.split (QString::fromUtf8 ("#pop")).size () - 1);
  } else {
    return contexts.context (n).id ();
  }
}

static bool 
string_to_bool (const QString &n)
{
  QString nt = n.trimmed ().toLower ();
  if (nt == QString::fromUtf8 ("true")) {
    return true;
  } else if (nt == QString::fromUtf8 ("false")) {
    return false;
  } else if (nt == QString::fromUtf8 ("1")) {
    return true;
  } else if (nt == QString::fromUtf8 ("0")) {
    return false;
  } else {
    return false;
  }
}

static GenericSyntaxHighlighterRule 
parse_rule (QDomElement e, GenericSyntaxHighlighterContexts &contexts, std::map<QString, GenericSyntaxHighlighterRuleStringList> &lists, GenericSyntaxHighlighterAttributes &attributes)
{
  GenericSyntaxHighlighterRule rule;

  bool dynamic = false;
  if (e.hasAttribute (QString::fromUtf8 ("dynamic"))) {
    dynamic = string_to_bool (e.attributeNode (QString::fromUtf8 ("dynamic")).value ());
  }

  if (e.tagName () == QString::fromUtf8 ("LineContinue")) {

    rule = GenericSyntaxHighlighterRule (new GenericSyntaxHighlighterRuleLineContinue ());

  } else if (e.tagName () == QString::fromUtf8 ("RegExpr")) {

    QString s = e.attributeNode (QString::fromUtf8 ("String")).value ();
    rule = GenericSyntaxHighlighterRule (new GenericSyntaxHighlighterRuleRegExp (s, dynamic));

  } else if (e.tagName () == QString::fromUtf8 ("Detect2Chars")) {

    QString s1 = e.attributeNode (QString::fromUtf8 ("char")).value ();
    QString s2 = e.attributeNode (QString::fromUtf8 ("char1")).value ();
    rule = GenericSyntaxHighlighterRule (new GenericSyntaxHighlighterRuleString (s1 + s2, false, dynamic));

  } else if (e.tagName () == QString::fromUtf8 ("DetectChar")) {

    QString s = e.attributeNode (QString::fromUtf8 ("char")).value ();
    rule = GenericSyntaxHighlighterRule (new GenericSyntaxHighlighterRuleString (s, false, dynamic));

  } else if (e.tagName () == QString::fromUtf8 ("DetectSpaces")) {

    rule = GenericSyntaxHighlighterRule (new GenericSyntaxHighlighterRuleSpaces ());

  } else if (e.tagName () == QString::fromUtf8 ("DetectIdentifier")) {

    rule = GenericSyntaxHighlighterRule (new GenericSyntaxHighlighterRuleIdentifier ());

  } else if (e.tagName () == QString::fromUtf8 ("AnyChar")) {

    QString s = e.attributeNode (QString::fromUtf8 ("String")).value ();
    rule = GenericSyntaxHighlighterRule (new GenericSyntaxHighlighterRuleAnyChar (s));

  } else if (e.tagName () == QString::fromUtf8 ("RangeDetect")) {

    QString s1 = e.attributeNode (QString::fromUtf8 ("char")).value ();
    QString s2 = e.attributeNode (QString::fromUtf8 ("char1")).value ();
    rule = GenericSyntaxHighlighterRule (new GenericSyntaxHighlighterRuleRange (s1, s2, dynamic));

  } else if (e.tagName () == QString::fromUtf8 ("StringDetect")) {

    QString s = e.attributeNode (QString::fromUtf8 ("String")).value ();
    bool insensitive = string_to_bool (e.attributeNode (QString::fromUtf8 ("insensitive")).value ());
    rule = GenericSyntaxHighlighterRule (new GenericSyntaxHighlighterRuleString (s, insensitive, dynamic));

  } else if (e.tagName () == QString::fromUtf8 ("Int")) {

    rule = GenericSyntaxHighlighterRule (new GenericSyntaxHighlighterRuleInt ());

  } else if (e.tagName () == QString::fromUtf8 ("Float")) {

    rule = GenericSyntaxHighlighterRule (new GenericSyntaxHighlighterRuleFloat ());

  } else if (e.tagName () == QString::fromUtf8 ("HlCOct")) {

    //  TODO: can be done more efficiently
    rule = GenericSyntaxHighlighterRule (new GenericSyntaxHighlighterRuleRegExp (QString::fromUtf8 ("0[0-9]+"), false));

  } else if (e.tagName () == QString::fromUtf8 ("HlCHex")) {

    //  TODO: can be done more efficiently
    rule = GenericSyntaxHighlighterRule (new GenericSyntaxHighlighterRuleRegExp (QString::fromUtf8 ("0x[0-9a-fA-F]+"), false));

  } else if (e.tagName () == QString::fromUtf8 ("HlCStringChar")) {

    //  TODO: can be done more efficiently
    rule = GenericSyntaxHighlighterRule (new GenericSyntaxHighlighterRuleRegExp (QString::fromUtf8 ("\\[abefnrtv\"\'\\]|\\x[0-9a-fA-F]+|\\0[0-9]{1,3}"), false));

  } else if (e.tagName () == QString::fromUtf8 ("HlCChar")) {

    //  TODO: can be done more efficiently
    rule = GenericSyntaxHighlighterRule (new GenericSyntaxHighlighterRuleRegExp (QString::fromUtf8 ("'(?:.|\\[abefnrtv\"\'\\]|\\x[0-9a-fA-F]+|\\0[0-9]{1,3})'"), false));

  } else if (e.tagName () == QString::fromUtf8 ("keyword")) {

    QString s = e.attributeNode (QString::fromUtf8 ("String")).value ().trimmed ();
    if (lists.find (s) != lists.end ()) {
      rule = GenericSyntaxHighlighterRule (&lists.find (s)->second, -1, 0, false);
    }

  }

  if (e.hasAttribute (QString::fromUtf8 ("context"))) {
    rule.set_target_context_id (context_name_to_id (e.attributeNode (QString::fromUtf8 ("context")).value (), contexts));
  }

  if (e.hasAttribute (QString::fromUtf8 ("attribute"))) {
    QString n = e.attributeNode (QString::fromUtf8 ("attribute")).value ();
    rule.set_attribute_id (attributes.id (n));
  }

  if (e.hasAttribute (QString::fromUtf8 ("lookAhead"))) {
    rule.set_lookahead (string_to_bool (e.attributeNode (QString::fromUtf8 ("lookAhead")).value ()));
  }

  if (e.hasAttribute (QString::fromUtf8 ("firstNonSpace"))) {
    rule.set_first_non_space (string_to_bool (e.attributeNode (QString::fromUtf8 ("firstNonSpace")).value ()));
  }

  if (e.hasAttribute (QString::fromUtf8 ("column"))) {
    rule.set_column (e.attributeNode (QString::fromUtf8 ("column")).value ().toInt ());
  }

  for (QDomNode n = e.firstChild(); !n.isNull(); n = n.nextSibling()) {
    if (n.isElement()) {
      QDomElement ee = n.toElement();
      rule.add_child_rule (parse_rule (ee, contexts, lists, attributes));
    }
  }

  return rule;
}

static GenericSyntaxHighlighterContext 
parse_context (QDomElement e, const std::map<QString, QDomElement> &contexts_by_name, GenericSyntaxHighlighterContexts &contexts, std::map<QString, GenericSyntaxHighlighterRuleStringList> &lists, GenericSyntaxHighlighterAttributes &attributes)
{
  GenericSyntaxHighlighterContext context;

  for (QDomNode n = e.firstChild(); !n.isNull(); n = n.nextSibling()) {
    if (n.isElement()) {
      QDomElement ee = n.toElement();
      if (ee.tagName () == QString::fromUtf8 ("IncludeRules")) {
        QString included_name = ee.attributeNode (QString::fromUtf8 ("context")).value ();
        std::map<QString, QDomElement>::const_iterator c2n = contexts_by_name.find (included_name);
        if (c2n != contexts_by_name.end ()) {
          context.include (parse_context (c2n->second, contexts_by_name, contexts, lists, attributes));
        }
      } else {
        context.add_rule (parse_rule (ee, contexts, lists, attributes));
      }
    }
  }

  if (e.hasAttribute (QString::fromUtf8 ("attribute"))) {
    QString n = e.attributeNode (QString::fromUtf8 ("attribute")).value ();
    context.set_attribute_id (attributes.id (n));
  }
  if (e.hasAttribute (QString::fromUtf8 ("lineEndContext"))) {
    context.set_lineend_context (context_name_to_id (e.attributeNode (QString::fromUtf8 ("lineEndContext")).value (), contexts));
  }
  if (e.hasAttribute (QString::fromUtf8 ("lineBeginContext"))) {
    context.set_linebegin_context (context_name_to_id (e.attributeNode (QString::fromUtf8 ("lineBeginContext")).value (), contexts));
  }

  if (e.hasAttribute (QString::fromUtf8 ("fallthrough")) && string_to_bool (e.attributeNode (QString::fromUtf8 ("fallthrough")).value ())) {
    context.set_fallthrough_context (context_name_to_id (e.attributeNode (QString::fromUtf8 ("fallthroughContext")).value (), contexts));
  }

  return context;
}

static void 
parse_item_data (QDomElement e, GenericSyntaxHighlighterAttributes &attributes, bool initialize)
{
  QString name = e.attributeNode (QString::fromUtf8 ("name")).value ();

  //  skip attribute if already present so we don't overwrite specific settings
  if (! initialize && attributes.has_attribute (name)) {
    return;
  }

  int attribute_id = attributes.id (name);

  def_style ds = dsNormal;
  QTextCharFormat format;

  if (e.hasAttribute (QString::fromUtf8 ("color"))) {
    format.setForeground (QColor (e.attributeNode (QString::fromUtf8 ("color")).value ()));
  }

  if (e.hasAttribute (QString::fromUtf8 ("selColor"))) {
    // TODO: not implemented yet 
  }

  if (e.hasAttribute (QString::fromUtf8 ("bold"))) {
    format.setFontWeight (string_to_bool (e.attributeNode (QString::fromUtf8 ("bold")).value ()) ? QFont::Bold : QFont::Normal);
  }

  if (e.hasAttribute (QString::fromUtf8 ("italic"))) {
    format.setFontItalic (string_to_bool (e.attributeNode (QString::fromUtf8 ("italic")).value ()));
  }

  if (e.hasAttribute (QString::fromUtf8 ("underline"))) {
    format.setFontUnderline (string_to_bool (e.attributeNode (QString::fromUtf8 ("underline")).value ()));
  }

  if (e.hasAttribute (QString::fromUtf8 ("strikeout"))) {
    format.setFontStrikeOut (string_to_bool (e.attributeNode (QString::fromUtf8 ("strikeout")).value ()));
  }

  if (e.hasAttribute (QString::fromUtf8 ("defStyleNum"))) {
    QString s = e.attributeNode (QString::fromUtf8 ("defStyleNum")).value ();
    if (s == QString::fromUtf8 ("dsNormal")) { ds = dsNormal; }
    else if (s == QString::fromUtf8 ("dsAlert")) { ds = dsAlert; }
    else if (s == QString::fromUtf8 ("dsBaseN")) { ds = dsBaseN; }
    else if (s == QString::fromUtf8 ("dsChar")) { ds = dsChar; }
    else if (s == QString::fromUtf8 ("dsComment")) { ds = dsComment; }
    else if (s == QString::fromUtf8 ("dsDataType")) { ds = dsDataType; }
    else if (s == QString::fromUtf8 ("dsDecVal")) { ds = dsDecVal; }
    else if (s == QString::fromUtf8 ("dsError")) { ds = dsError; }
    else if (s == QString::fromUtf8 ("dsFloat")) { ds = dsFloat; }
    else if (s == QString::fromUtf8 ("dsFunction")) { ds = dsFunction; }
    else if (s == QString::fromUtf8 ("dsKeyword")) { ds = dsKeyword; }
    else if (s == QString::fromUtf8 ("dsOthers")) { ds = dsOthers; }
    else if (s == QString::fromUtf8 ("dsRegionMarker")) { ds = dsRegionMarker; }
    else if (s == QString::fromUtf8 ("dsString")) { ds = dsString; }
    else if (s == QString::fromUtf8 ("dsOperator")) { ds = dsOperator; }
    else if (s == QString::fromUtf8 ("dsControlFlow")) { ds = dsControlFlow; }
    else if (s == QString::fromUtf8 ("dsBuiltIn")) { ds = dsBuiltIn; }
    else if (s == QString::fromUtf8 ("dsVariable")) { ds = dsVariable; }
    else if (s == QString::fromUtf8 ("dsExtension")) { ds = dsExtension; }
    else if (s == QString::fromUtf8 ("dsPreprocessor")) { ds = dsPreprocessor; }
    else if (s == QString::fromUtf8 ("dsImport")) { ds = dsImport; }
    else if (s == QString::fromUtf8 ("dsVerbatimString")) { ds = dsVerbatimString; }
    else if (s == QString::fromUtf8 ("dsSpecialString")) { ds = dsSpecialString; }
    else if (s == QString::fromUtf8 ("dsSpecialString")) { ds = dsSpecialString; }
    else if (s == QString::fromUtf8 ("dsSpecialChar")) { ds = dsSpecialChar; }
    else if (s == QString::fromUtf8 ("dsAttribute")) { ds = dsAttribute; }
  }

  attributes.set_styles (attribute_id, ds, format);
}

GenericSyntaxHighlighter::GenericSyntaxHighlighter (QObject *parent, QIODevice &input, GenericSyntaxHighlighterAttributes *attributes, bool initialize_attributes)
  : QSyntaxHighlighter (parent), mp_attributes (attributes), m_generation_id (0)
{
  QDomDocument d;
  d.setContent (&input, true);

  QDomElement highlighting;
  for (QDomNode n = d.documentElement ().firstChild(); !n.isNull(); n = n.nextSibling()) {
    if (n.isElement()) {
      QDomElement e = n.toElement();
      if (e.tagName () == QString::fromUtf8 ("highlighting")) {
        highlighting = e;
        break;
      }
    }
  }

  if (highlighting.isNull ()) {
    return;
  }

  for (QDomNode n = highlighting.firstChild(); !n.isNull(); n = n.nextSibling()) {
    if (n.isElement()) {

      QDomElement e = n.toElement();
      if (e.tagName () == QString::fromUtf8 ("list")) {

        m_lists.insert (std::make_pair (e.attributeNode (QString::fromUtf8 ("name")).value (), parse_list (e)));

      } else if (e.tagName () == QString::fromUtf8 ("contexts")) {

        //  first analyze the list of contexts and their dependencies

        std::map<QString, QDomElement> contexts_by_name;

        for (QDomNode nn = e.firstChild(); !nn.isNull(); nn = nn.nextSibling()) {
          if (nn.isElement()) {
            QDomElement ee = nn.toElement ();
            if (ee.tagName () == QString::fromUtf8 ("context")) {
              QString context_name = ee.attributeNode (QString::fromUtf8 ("name")).value ();
              contexts_by_name[context_name] = ee;
            }
          }
        }

        for (QDomNode nn = e.firstChild(); !nn.isNull(); nn = nn.nextSibling()) {
          if (nn.isElement()) {
            QDomElement ee = nn.toElement ();
            if (ee.tagName () == QString::fromUtf8 ("context")) {
              QString context_name = ee.attributeNode (QString::fromUtf8 ("name")).value ();
              m_contexts.insert (context_name, parse_context (ee, contexts_by_name, m_contexts, m_lists, *mp_attributes));
            }
          }
        }

      } else if (e.tagName () == QString::fromUtf8 ("itemDatas")) {

        for (QDomNode nn = e.firstChild(); !nn.isNull(); nn = nn.nextSibling()) {
          if (nn.isElement()) {
            QDomElement ee = nn.toElement ();
            if (ee.tagName () == QString::fromUtf8 ("itemData")) {
              parse_item_data (ee, *mp_attributes, initialize_attributes);
            }
          }
        }

      }

    }
  }

#if defined(DEBUG_HIGHLIGHTER)
  m_contexts.dump ();
#endif
}

void 
GenericSyntaxHighlighter::highlightBlock(const QString &text)
{
  ++m_generation_id;

#if defined(DEBUG_HIGHLIGHTER)
  std::cout << "Highlighting '" << text.toAscii().constData() << "'" << std::endl;
#endif
  if (m_contexts.is_empty ()) {
    return;
  }

  GenericSyntaxHighlighterState state (&m_contexts);

  //  get the previous state
  int ps = previousBlockState ();
  if (ps >= 0) {
    state = *m_states_by_id [ps];
  }

  int index = -1; // marks "before line" initially
#if defined(DEBUG_HIGHLIGHTER)
  std::cout << index << ":" << m_contexts.context (state.current_context_id ()).name ().toAscii ().constData () << std::endl;
#endif
  int end_index = 0, last_index= -1;
  int attribute_id = 0, def_attribute_id = 0;

  std::unique_ptr<SyntaxHighlighterUserData> user_data (new SyntaxHighlighterUserData ());

  while (std::max (0, index) < text.length ()) {

    if (state.match (text, m_generation_id, index, end_index, def_attribute_id, attribute_id)) {

      if (index < 0) {
        index = 0;
      }

      //  apply def_attribute_id to last_index .. index 
      if (last_index >= 0 && def_attribute_id >= 0) {
        setFormat(last_index, index - last_index, mp_attributes->format_for (def_attribute_id));
      }

      //  save this element's information
      if (last_index >= 0) {
        SyntaxHighlighterElement el;
        el.start_offset = last_index;
        el.length = index - last_index;
        el.basic_attribute_id = mp_attributes->basic_id (def_attribute_id);
        user_data->elements ().push_back (el);
      }

      last_index = -1;

      //  apply attribute_id to index .. end_index
      if (end_index > index && attribute_id >= 0) {
        setFormat(index, end_index - index, mp_attributes->format_for (attribute_id));
      }

      //  save this element's information
      if (end_index > index) {
        SyntaxHighlighterElement el;
        el.start_offset = index;
        el.length = end_index - index;
        el.basic_attribute_id = mp_attributes->basic_id (attribute_id);
        user_data->elements ().push_back (el);
      }

      index = end_index;
#if defined(DEBUG_HIGHLIGHTER)
      std::cout << " -> " << index << ":" << m_contexts.context (state.current_context_id ()).name ().toAscii ().constData () << std::endl;
#endif

    } else {

      if (index < 0) {
        index = 0;
      }

      if (last_index < 0 && !text [index].isSpace ()) {
        last_index = index;
      }

      ++index;

    }

  }

  //  apply def_attribute_id to last_index .. index 
  if (last_index >= 0 && def_attribute_id >= 0) {
    setFormat(last_index, index - last_index, mp_attributes->format_for (def_attribute_id));
  }

  //  apply def_attribute_id to index .. end of string 
  if (index < text.length () && def_attribute_id >= 0) {
    setFormat(index, text.length () - index, mp_attributes->format_for (def_attribute_id));
  }

  //  match potential line-end context
  state.match (text, m_generation_id, index, end_index, def_attribute_id, attribute_id);

  //  set the new state
  std::map<GenericSyntaxHighlighterState, int>::iterator s = m_state_cache.find (state);
  if (s == m_state_cache.end ()) {
    int id = (int) m_states_by_id.size ();
    s = m_state_cache.insert (std::make_pair (state, id)).first;
    m_states_by_id.push_back (&s->first);
  }

  setCurrentBlockState (s->second);
  setCurrentBlockUserData (user_data.release ());

#if defined(DEBUG_HIGHLIGHTER)
  std::cout << "# states=" << m_state_cache.size() << std::endl;
#endif
}

}

#endif
