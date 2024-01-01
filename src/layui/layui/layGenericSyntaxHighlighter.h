
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#ifndef HDR_layGenericSyntaxHighlighter
#define HDR_layGenericSyntaxHighlighter

#include "tlString.h"
#include "layuiCommon.h"

#include <QSyntaxHighlighter>
#include <QString>

#include <set>
#include <map>
#include <vector>
#include <list>

namespace lay
{

/**
 *  @brief Specifies one element in the text block's user data
 */
struct LAYUI_PUBLIC SyntaxHighlighterElement
{
public:
  /**
   *  @brief Default constructor
   */
  SyntaxHighlighterElement ()
    : start_offset (0), length (0), basic_attribute_id (0)
  {
    //  .. nothing yet ..
  }

  size_t start_offset, length;
  int basic_attribute_id;
};

/**
 *  @brief The user data the highlighter attaches to the current block
 *
 *  The user data will contain useful data for bracket detection and other things
 */
class LAYUI_PUBLIC SyntaxHighlighterUserData
  : public QTextBlockUserData
{
public:
  /**
   *  @brief Constructor
   */
  SyntaxHighlighterUserData ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief The elements
   *  These objects will specific the elements that make up the block.
   */
  std::vector<SyntaxHighlighterElement> &elements ()
  {
    return m_elements;
  }

public:
  std::vector<SyntaxHighlighterElement> m_elements;
};

/**
 *  @brief A rule implementation base class
 *
 *  Rule implementations must implement this base class in order to plug into the 
 *  generic syntax highlighter framework.
 */
class LAYUI_PUBLIC GenericSyntaxHighlighterRuleBase
{
public:
  /** 
   *  @brief Base class constructor
   */
  GenericSyntaxHighlighterRuleBase ();

  /**
   *  @brief Destructor
   */
  virtual ~GenericSyntaxHighlighterRuleBase ();

  /** 
   *  @brief Matches the given string against this rule
   *  
   *  On success, this method returns true. It will use the input arguments for dynamic rules to replace the %N 
   *  placeholders. 
   *  If the rule matches, the end_index will be set to the end of the sequence and output_args 
   *  will contain any arguments matched.
   *  generationId is a counter that will incremented when a new block is highlighted. This allows caching
   *  matches in some cases (i.e. regexp matcher).
   */
  virtual bool match (const QString &input, unsigned int generation_id, int index, int &end_index, const QList<QString> &input_args, QList<QString> &output_args) const = 0;

  /**
   *  @brief Clone this rule
   */
  virtual GenericSyntaxHighlighterRuleBase *clone () const = 0;

  /**
   *  @brief Dump this rule
   */
  virtual void dump () const = 0;
};

/**
 *  @brief A specialization of GenericSyntaxHighlighterRuleBase which looks for a choice of strings
 */
class LAYUI_PUBLIC GenericSyntaxHighlighterRuleStringList
  : public GenericSyntaxHighlighterRuleBase
{
public:
  GenericSyntaxHighlighterRuleStringList (const QList<QString> &sl);
  GenericSyntaxHighlighterRuleStringList (const std::set<QString> &s, int ml);

  virtual GenericSyntaxHighlighterRuleBase *clone () const;
  virtual bool match (const QString &input, unsigned int /*generation_id*/, int index, int &end_index, const QList<QString> & /*input_args*/, QList<QString> & /*output_args*/) const;
  virtual void dump () const;

private:
  std::set<QString> m_s;
  int m_min_length;
};

/**
 *  @brief A proxy for the actual rule implementation
 * 
 *  This object will forward the match request to the actual implementation.
 */
class LAYUI_PUBLIC GenericSyntaxHighlighterRule
{
public:
  /**
   *  @brief Default constructor
   */
  GenericSyntaxHighlighterRule ();

  /**
   *  @brief Constructor from a rule base class, attribute ID and target context ID
   *
   *  This object will become owner of the rule object.
   */
  GenericSyntaxHighlighterRule (GenericSyntaxHighlighterRuleBase *rule, int attribute_id = -1, int target_context_id = 0, bool take_ownership = true);

  /**
   *  @brief Copy constructor
   */
  GenericSyntaxHighlighterRule (const GenericSyntaxHighlighterRule &d);

  /**
   *  @brief Destructor
   */
  ~GenericSyntaxHighlighterRule ();

  /** 
   *  @brief Assignment operator
   */
  GenericSyntaxHighlighterRule &operator= (const GenericSyntaxHighlighterRule &d);

  /**
   *  @brief Returns true, if this rule does not have an implementation
   */
  bool is_null () const
  {
    return mp_rule == 0;
  }

  /**
   *  @brief Sets the attribute associated with this context
   */
  void set_attribute_id (int id)
  {
    m_attribute_id = id;
  }

  /**
   *  @brief Gets the attribute associated with this context
   */
  int attribute_id () const
  {
    return m_attribute_id;
  }

  /**
   *  @brief Sets the target context ID
   */
  void set_target_context_id (int id)
  {
    m_target_context_id = id;
  }

  /**
   *  @brief Gets the target context ID
   */
  int target_context_id () const
  {
    return m_target_context_id;
  }

  /**
   *  @brief Sets the lookahead flag
   *
   *  If true, the rule does not consume the match string
   */
  void set_lookahead (bool f)
  {
    m_lookahead = f;
  }

  /**
   *  @brief Gets the lookahead flag
   */
  bool lookahead () const
  {
    return m_lookahead;
  }

  /**
   *  @brief Sets the matching column
   *
   *  Set this value to <0 to unset the matching column
   */
  void set_column (int c)
  {
    m_column = c;
  }

  /**
   *  @brief Gets the matching column
   */
  int column () const
  {
    return m_column;
  }

  /**
   *  @brief Sets the first non-space flag
   *
   *  If true, this rule only matches if the match is the first non-space character 
   *  after the current position
   */
  void set_first_non_space (bool f)
  {
    m_first_non_space = f;
  }

  /**
   *  @brief Gets the first non-space flag
   */
  bool first_non_space () const
  {
    return m_first_non_space;
  }

  /**
   *  @brief Adds a rule as a child rule
   */
  void add_child_rule (const GenericSyntaxHighlighterRule &rule) 
  {
    if (! rule.is_null ()) {
      m_child_rules.push_back (rule);
    }
  }

  /**
   *  @brief Matches the string against the given rule
   *
   *  Returns true, if the rule matches. In this case, end_index will be set to the end of the sequence and 
   *  output_args will contain any arguments matched.
   */
  bool match (const QString &input, unsigned int generation_id, int index, int &end_index, const QList<QString> &input_args, QList<QString> &output_args) const;

  /**
   *  @brief Dump the contents of this rule
   */
  void dump () const;

private:
  GenericSyntaxHighlighterRuleBase *mp_rule;
  int m_attribute_id;
  int m_target_context_id;
  bool m_owner;
  bool m_lookahead, m_first_non_space;
  int m_column;

  std::list<GenericSyntaxHighlighterRule> m_child_rules;
};

/**
 *  @brief A syntax highlighter context
 *
 *  A context is a number of rules that are applied sequentially. The first match is taken and determines the target context.
 *  Target contexts are identified by an integer context ID, corresponding to the context names in the Kate highlighter scripts.
 *  Special context ID's are 0 (#stay), -n (#pop n times).
 *  A context is associated with an attribute, which is given by an integer ID.
 */
class LAYUI_PUBLIC GenericSyntaxHighlighterContext
{
public:
  enum {
    no_context = 0x7ffffff
  };

  /**
   *  @brief Constructor
   */
  GenericSyntaxHighlighterContext ();

  /**
   *  @brief Gets the context's Id
   */
  int id () const
  {
    return m_id;
  }

  /**
   *  @brief Get the context's name
   */
  const QString &name () const
  {
    return m_name;
  }

  /**
   *  @brief Add a new rule with a target context
   */
  void add_rule (const GenericSyntaxHighlighterRule &rule);

  /**
   *  @brief Sets the fallthrough context
   *
   *  To disable a fallthrough context, specify no_context for the value.
   */
  void set_fallthrough_context (int context_id)
  {
    m_fallthrough_context = context_id;
  }

  /**
   *  @brief Gets the fallthrough context
   *
   *  This method returns no_context if no fallthrough context was specified.
   */
  int fallthrough_context () const
  {
    return m_fallthrough_context;
  }

  /**
   *  @brief Set the line-begin context
   *
   *  To disable a line-begin context, specify no_context for the value.
   */
  void set_linebegin_context (int context_id)
  {
    m_linebegin_context = context_id;
  }

  /**
   *  @brief Gets the line-begin context
   *
   *  This method returns no_context if no line-begin context was specified.
   */
  int linebegin_context () const
  {
    return m_linebegin_context;
  }

  /**
   *  @brief Set the line-end context
   *
   *  To disable a line-end context, specify no_context for the value.
   */
  void set_lineend_context (int context_id)
  {
    m_lineend_context = context_id;
  }

  /**
   *  @brief Gets the line-end context
   *
   *  This method returns no_context if no line-end context was specified.
   */
  int lineend_context () const
  {
    return m_lineend_context;
  }

  /**
   *  @brief Sets the attribute associated with this context
   */
  void set_attribute_id (int id)
  {
    m_attribute_id = id;
  }

  /**
   *  @brief Gets the attribute associated with this context
   */
  int attribute_id () const
  {
    return m_attribute_id;
  }

  /**
   *  @brief Include another context
   */
  void include (const GenericSyntaxHighlighterContext &other);

  /**
   *  @brief Match the context against the given string and return true, if any rule matches
   *
   *  If any rule matches, this method will return true. In that case, end_index to the to the next character inside
   *  "string", output_args will contain the new arguments (if there are any) and new_context will be set to the new
   *  context ID (0 to stay, negative for #pop's).
   *  attribute_id will be set to the ID of the attribute of the rule that was found.
   */
  bool match (const QString &string, unsigned int generation_id, int index, int &end_index, const QList<QString> &input_args, QList<QString> &output_args, int &new_context, int &attribute_id) const;

  /**
   *  @brief Dump the contents of this context
   */
  void dump () const;

private:
  friend class GenericSyntaxHighlighterContexts;

  int m_id;
  QString m_name;
  int m_fallthrough_context;
  int m_linebegin_context, m_lineend_context;
  int m_attribute_id;
  std::list<GenericSyntaxHighlighterRule> m_rules;

  /**
   *  @brief Sets the context's Id
   */
  void set_id (int id) 
  {
    m_id = id;
  }

  /**
   *  @brief Set the context's name
   */
  void set_name (const QString &n)
  {
    m_name = n;
  }
};

/**
 *  @brief A collection of (named) contexts
 */
class LAYUI_PUBLIC GenericSyntaxHighlighterContexts
{
public:
  GenericSyntaxHighlighterContexts ();

  /**
   *  @brief Insert a context with the given name
   *
   *  If an context with that name already exists, it is overwritten
   */
  void insert (const QString &name, const GenericSyntaxHighlighterContext &c);

  /**
   *  @brief Gets the initial context ID
   *
   *  The initial context is by default the first one that is inserted with "insert".
   */
  int initial_context_id () const
  {
    return m_initial_context;
  }

  /**
   *  @brief Gets a context by name and creates the context if it does not exist already.
   *
   *  If a new context is created, a new ID is assigned.
   */
  GenericSyntaxHighlighterContext &context (const QString &name);

  /**
   *  @brief Gets a context by name
   *
   *  If no context with that name exists, an exception is thrown.
   */
  const GenericSyntaxHighlighterContext &context (const QString &name) const;

  /**
   *  @brief Gets a context by ID (non-const version)
   *
   *  If no context with that name exists, an exception is thrown.
   */
  GenericSyntaxHighlighterContext &context (int id);

  /**
   *  @brief Gets a context by ID
   *
   *  If no context with that name exists, an exception is thrown.
   */
  const GenericSyntaxHighlighterContext &context (int id) const;

  /**
   *  @brief Returns true, if there are no contexts
   */
  bool is_empty () const
  {
    return m_contexts_by_name.empty ();
  }

  /**
   *  @brief Dump the contents 
   */
  void dump () const;

private:
  std::map<QString, GenericSyntaxHighlighterContext> m_contexts_by_name;
  std::vector<GenericSyntaxHighlighterContext *> m_contexts_by_id;
  int m_initial_context;
};

enum def_style {
  dsNormal = 0,
  dsAlert,
  dsBaseN,
  dsChar,
  dsComment,
  dsDataType,
  dsDecVal,
  dsError,
  dsFloat,
  dsFunction,
  dsKeyword,
  dsOthers,
  dsRegionMarker,
  dsString,
  dsOperator,
  dsControlFlow,
  dsBuiltIn,
  dsVariable,
  dsExtension,
  dsPreprocessor,
  dsImport,
  dsVerbatimString,
  dsSpecialString,
  dsSpecialChar,
  dsAttribute,
  dsLast
};

/**
 *  @brief A collection of attributes
 */
class LAYUI_PUBLIC GenericSyntaxHighlighterAttributes
{
public:
  typedef std::map<QString, int>::const_iterator const_iterator;

  /**
   *  @brief The constructor
   */
  GenericSyntaxHighlighterAttributes (const GenericSyntaxHighlighterAttributes *basic_attributes = 0);

  /**
   *  @brief Get the iterator delivering the names and ID's (begin)
   */
  const_iterator begin () const
  {
    return m_ids.begin ();
  }

  /**
   *  @brief Get the iterator delivering the names and ID's (end)
   */
  const_iterator end () const
  {
    return m_ids.end ();
  }

  /**
   *  @brief Gets a value indicating whether the given name is present already
   */
  bool has_attribute (const QString &name) const;

  /**
   *  @brief Get the attribute ID for a given name
   *
   *  If no attribute with that ID exists, it is created.
   */
  int id (const QString &name);

  /**
   *  @brief Get the attribute ID for a given name
   *
   *  If no attribute with that ID exists, an exception is thrown
   */
  int id (const QString &name) const;

  /**
   *  @brief Gets the specific style for a given attribute ID
   */
  QTextCharFormat specific_style (int id) const;

  /**
   *  @brief Gets the basic attribute ID for a given attribute ID
   */
  int basic_id (int id) const;

  /**
   *  @brief Set the special styles for a given attribute ID
   */
  void set_style (int id, const QTextCharFormat &format);
  
  /**
   *  @brief Set the basic style and special styles for a given attribute ID
   */
  void set_styles (int id, int basic_style_id,  const QTextCharFormat &format);
  
  /**
   *  @brief Gets the effective format for a given ID
   */
  QTextCharFormat format_for (int id) const;

  /** 
   *  @brief Assign the styles from another set of attributes
   *
   *  This method does not copy the basic_attributes pointer
   */
  void assign (const GenericSyntaxHighlighterAttributes &other);

  /**
   *  @brief Serialize the attributes to string
   */
  std::string to_string () const;

  /**
   *  @brief Read from a string
   */
  void read (tl::Extractor &ex);

private:
  const GenericSyntaxHighlighterAttributes *mp_basic_attributes;
  std::vector<std::pair<int, QTextCharFormat> > m_attributes;
  std::map<QString, int> m_ids;

  void add (const QString &name, int id, bool bold, bool italic, bool underline, bool strikeout, const char *foreground, const char *fg_selected, const char *background, const char *bg_selected);
};

/**
 *  @brief The parser's state
 */
class LAYUI_PUBLIC GenericSyntaxHighlighterState
{
public:
  GenericSyntaxHighlighterState (const GenericSyntaxHighlighterContexts *contexts);

  /**
   *  @brief Compares this state against another (less)
   */
  bool operator< (const GenericSyntaxHighlighterState &d) const;

  /**
   *  @brief Compares this state against another (equal)
   */
  bool operator== (const GenericSyntaxHighlighterState &d) const;

  /**
   *  @brief Match the given string and return true if the match succeeds.
   * 
   *  This method will match the given string based on the current state. On success, this method modifies
   *  the state and returns true. On success, end_index to the end of the sequence found, attribute_id will 
   *  hold the attribute ID of the rule found. def_attribute_id will always contain the attribute ID of the 
   *  previous state, also if the method returns false.
   */
  bool match (const QString &string, unsigned int generation_id, int index, int &end_index, int &def_attribute_id, int &attribute_id);

  /**
   *  @brief Gets the current context ID
   */
  int current_context_id () const;

private:
  std::vector<std::pair<int, QList<QString> > > m_stack;
  const GenericSyntaxHighlighterContexts *mp_contexts;
};

/**
 *  @brief A generic syntax highlighter using "Kate"'s syntax highlight scripts
 */
class LAYUI_PUBLIC GenericSyntaxHighlighter
  : public QSyntaxHighlighter
{
public: 
  /**
   *  @brief Creates a GenericSyntaxHighlighter
   *  @param parent The owner of the highlighter
   *  @param input The stream from which to pull
   *  @param attributes The attributes
   *  @param initialize_attributes If true, the attributes are initialized from the itemData lines
   */
  GenericSyntaxHighlighter (QObject *parent, QIODevice &input, GenericSyntaxHighlighterAttributes *attributes, bool initialize_attributes);
  
  /**
   *  @brief Implementation of the highlighter
   */
  void highlightBlock(const QString &text);

private:
  GenericSyntaxHighlighterContexts m_contexts;
  std::map<QString, GenericSyntaxHighlighterRuleStringList> m_lists;
  std::map<GenericSyntaxHighlighterState, int> m_state_cache;
  std::vector<const GenericSyntaxHighlighterState *> m_states_by_id;
  GenericSyntaxHighlighterAttributes *mp_attributes;
  unsigned int m_generation_id;
};

}

#endif

#endif  //  defined(HAVE_QT)
