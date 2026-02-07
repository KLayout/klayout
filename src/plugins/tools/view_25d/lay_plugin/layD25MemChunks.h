
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#ifndef HDR_layD25MemChunks
#define HDR_layD25MemChunks

#include <QDialog>
#include <QOpenGLFunctions>

#include "tlObject.h"
#include <string.h>  //  for memcpy

namespace lay
{

template <class Obj>
struct gl_type2enum
{
  GLenum operator() () const;
};

template <>
struct gl_type2enum<float>
{
  GLenum operator() () const { return GL_FLOAT; }
};


/**
 *  @brief Provides a semi-contiguous array of objects
 *
 *  The objects are kept in chunks of ChunkLen items.
 *  The blocks can be accessed individually. The array can be
 *  cleared and new items can be added. No insert or delete.
 *
 *  This object is intended to be used for keeping vertex,
 *  color or point data for OpenGL.
 */
template <class Obj, size_t ChunkLen = 1024>
class mem_chunks
{
public:

  struct chunk {
  public:
    chunk ()
      : m_len (0), m_next (0)
    {
      //  .. nothing yet ..
    }

    chunk (const chunk &other)
      : m_len (0), m_next (0)
    {
      operator= (other);
    }

    chunk &operator= (const chunk &other)
    {
      if (this != &other) {
        memcpy (&m_objects, &other.m_objects, sizeof (m_objects));
        m_len = other.m_len;
      }
      return *this;
    }

    const Obj *front () const { return m_objects; }
    size_t size () const { return m_len; }
    const chunk *next () const { return m_next; }

  private:
    friend class mem_chunks;

    Obj m_objects [ChunkLen];
    size_t m_len;
    chunk *m_next;
  };

  class iterator
  {
  public:
    iterator (chunk *ch = 0)
      : mp_chunk (ch)
    {
      //  .. nothing yet ..
    }

    bool operator== (iterator other) const
    {
      return mp_chunk == other.mp_chunk;
    }

    bool operator!= (iterator other) const
    {
      return mp_chunk != other.mp_chunk;
    }

    const chunk &operator* () const
    {
      return *mp_chunk;
    }

    const chunk *operator-> () const
    {
      return mp_chunk;
    }

    void operator++ ()
    {
      mp_chunk = mp_chunk->next ();
    }

  private:
    const chunk *mp_chunk;
  };

  /**
   *  @brief Default constructor
   *  Creates an empty array
   */
  mem_chunks ()
    : mp_chunks (0), mp_last_chunk (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Destructor
   */
  ~mem_chunks ()
  {
    clear ();
  }

  /**
   *  @brief Copy constructor
   */
  mem_chunks (const mem_chunks &other)
    : mp_chunks (0), mp_last_chunk (0)
  {
    operator= (other);
  }

  /**
   *  @brief Assignment
   */
  mem_chunks &operator= (const mem_chunks &other)
  {
    if (this != &other) {
      clear ();
      const chunk *ch = other.mp_chunks;
      while (ch) {
        if (! mp_chunks) {
          mp_last_chunk = mp_chunks = new chunk (*ch);
        } else {
          mp_last_chunk->m_next = new chunk (*ch);
          mp_last_chunk = mp_last_chunk->m_next;
        }
        ch = ch->next ();
      }
    }

    return *this;
  }

  /**
   *  @brief Clears the array
   */
  void clear ()
  {
    chunk *ch = mp_chunks;
    mp_chunks = 0;
    mp_last_chunk = 0;
    while (ch) {
      chunk *del = ch;
      ch = ch->m_next;
      delete del;
    }
  }

  /**
   *  @brief Adds an element to the array
   */
  void add (const Obj &element)
  {
    if (! mp_last_chunk) {
      mp_chunks = mp_last_chunk = new chunk ();
    }

    if (mp_last_chunk->m_len >= ChunkLen) {
      mp_last_chunk->m_next = new chunk ();
      mp_last_chunk = mp_last_chunk->m_next;
    }

    mp_last_chunk->m_objects [mp_last_chunk->m_len++] = element;
  }

  /**
   *  @brief Adds two elements
   */
  void add (const Obj &e1, const Obj &e2)
  {
    add (e1);
    add (e2);
  }

  /**
   *  @brief Adds three elements
   */
  void add (const Obj &e1, const Obj &e2, const Obj &e3)
  {
    add (e1);
    add (e2);
    add (e3);
  }

  /**
   *  @brief begin iterator
   */
  iterator begin () const { return iterator (mp_chunks); }

  /**
   *  @brief end iterator
   */
  iterator end () const { return iterator (); }

  /**
   *  @brief Draw to the given context
   */
  void draw_to (QOpenGLFunctions *ctx, GLuint location, GLenum mode) const
  {
    for (iterator c = begin (); c != end (); ++c) {
      ctx->glVertexAttribPointer (location, 3, gl_type2enum<Obj> () (), GL_FALSE, 0, c->front ());
      ctx->glDrawArrays (mode, 0, GLsizei (c->size () / 3));
    }
  }

private:
  chunk *mp_chunks;
  chunk *mp_last_chunk;
};

}

#endif

