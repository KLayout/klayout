
# 
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
#

class CPPObject

  # Propagate setting of the visibility down to the children 
  # (set_visibility will replace the default visibility with public or private)
  def set_visibility
  end

  # Inject scoped objects (like "class A::B { ... };") into their target 
  # scope.
  def inject_scoped
  end

  # delivers a string representation of the name or nil if the object does not have a name 
  # to contribute
  def myself
    nil
  end

  # delivers a string representation of the "weak" name or nil if the object does not have a name 
  # to contribute. "weak names" are such of second order - e.g. forward declarations.
  def myself_weak
    nil
  end

  # delivers a CPPQualifiedId representation of the object's location or nil if the object
  # does not have a location to contribute
  def myid
    nil
  end

  # sets the CPPQualifiedId
  def setid(id)
  end

  # supposed to establish the parent link
  def set_parent(p)
  end

  # removes a child from our members
  def remove(d)
  end

  # inserts a child into our members
  def insert(d)
  end

end

class CPPType
  
  def func
    i = self
    while i.respond_to?(:inner)
      if i.is_a?(CPPFunc) && (i.inner.is_a?(CPPQualifiedId) || i.inner.is_a?(CPPAnonymousId))
        return i
      end
      i = i.inner
    end
    nil
  end

  def return_type
    rt = self.dup
    f = self.func
    if f
      i = self
      idup = rt
      while i.respond_to?(:inner)
        i = i.inner
        if i == f
          idup.inner = CPPAnonymousId::new
          break
        end
        idup.inner = i.dup
        idup = idup.inner
      end
    end
    rt
  end

  def name_substituted_type(sub)
    rt = self.dup
    i = self
    idup = rt
    while i.respond_to?(:inner)
      ii = i.inner
      if ii.is_a?(CPPQualifiedId) || ii.is_a?(CPPAnonymousId)
        idup.inner = sub
        break
      end
      i = ii
      idup.inner = i.dup
      idup = idup.inner
    end
    rt
  end

  def anonymous_type
    name_substituted_type(CPPAnonymousId::new)
  end

  def renamed_type(name)
    name_substituted_type(CPPQualifiedId::new(false, [name]))
  end

  def name
    i = self
    while i.respond_to?(:inner)
      ii = i.inner
      if ii.is_a?(CPPQualifiedId)
        return ii.to_s
      end
      i = ii
    end
    nil
  end

  def is_void?
    self.concrete.is_a?(CPPPOD) && self.concrete.to_s == "void" && (self.inner.is_a?(CPPAnonymousId) || self.inner.is_a?(CPPQualifiedId))
  end

end

module QualifiedNameResolver

  attr_accessor :parent

  def global_scope
    o = self
    while o.myself && o.parent
      o = o.parent
    end
    o
  end

  # requirements
  #  - children -> must deliver a list of child objects
  #  - myself -> must deliver my name

  def set_parent(parent = nil)
    self.parent = parent
    @id2obj = {}
    self.children.each do |d|
      d.myself && (@id2obj[d.myself] = d)
    end 
    self.children.each do |d|
      d.myself_weak && (@id2obj[d.myself_weak] ||= d)
    end 
    self.children.each do |d|
      d.set_parent(self)
    end
    # Add other children, for example contributed by base classes
    if self.respond_to?(:other_children)
      self.other_children.each do |d|
        d.myself && (@id2obj[d.myself] = d)
      end
    end
  end

  def dump_ids
    @id2obj.keys.sort.each do |k|
      puts("--> #{k}")
    end
  end

  # by default the objects don't have a weak identity
  def myself_weak
    nil
  end

  # returns a list of names of child objects
  def ids
    (@id2obj || {}).keys.sort
  end

  def id2obj(id)
    @id2obj && @id2obj[id]
  end

  def resolve_qid(qid, stop = nil, include_other = true)

    qid.is_a?(CPPQualifiedId) || raise("Argument of resolve_qid must be a CPPQualifiedId object")

    obj = nil
    if qid.global && self.parent
      root = self
      while root.parent
        root = root.parent
      end
      obj = root.resolve_qid(qid, nil, false)
    else
      obj = id2obj(qid.parts[0].id)
      if obj && qid.parts.size > 1
        # The part may be a typedef: resolve it in that case before we proceed
        while obj && obj.is_a?(CPPTypedef) 
          obj = obj.type.concrete.is_a?(CPPQualifiedId) && self.resolve_qid(obj.type.concrete, stop, include_other)
        end
        if obj
          qid_new = qid.dup
          qid_new.parts = qid.parts[1 .. -1]
          obj = obj.respond_to?(:resolve_qid) && obj.resolve_qid(qid_new, stop, include_other)
        end
      end
      if ! obj && include_other
        # try base classes
        self.other_children.each do |bc|
          if bc != self && bc.respond_to?(:resolve_qid)
            (obj = bc.resolve_qid(qid, self, false)) && break
          end
        end
      end
      if ! obj && self.parent && self.parent != stop
        obj = self.parent.resolve_qid(qid, stop, include_other)
      end
    end

    obj

  end

  def inject_scoped

    self.children.each do |d|

      d.inject_scoped

      qid = d.myid
      if qid

        qid.is_a?(CPPQualifiedId) || raise("Argument of resolve_qid must be a CPPQualifiedId object")

        if qid.parts.size > 1

          qid = qid.dup
          while qid.parts.size > 1
            obj = id2obj(qid.parts[0].id)
            if obj
              qid.parts = qid.parts[1 .. -1]
            else
              break
            end
          end

          if obj && qid.parts.size == 1
            # This copies the visibility which is not quite correct, since the injection case
            # is usually used for providing an implementation outside a class. That does not 
            # mean the outside implementation will provide the visibility. Instead a forward
            # declaration inside the target scope will do. Since that is lost in our implementation
            # currently that is not possible.
            self.remove(d)
            d.setid(qid)
            obj.insert(d)
          end

        end

      end

    end

  end

end

class CPPDeclaration

  include QualifiedNameResolver

  def children
    []
  end

  def other_children
    []
  end

  def myself
    self.type.name 
  end

end

class CPPEnumDeclaration

  include QualifiedNameResolver

  def children
    []
  end

  def other_children
    []
  end

  def myself
    # exclude forward declarations
    self.enum.specs && self.enum.name.to_s
  end

end

class CPPEnumSpec

  include QualifiedNameResolver

  def children
    []
  end

  def other_children
    []
  end

  def myself
    self.name.to_s
  end

end

class CPPStruct

  attr_accessor :parent

  def global_scope
    self.parent && self.parent.global_scope
  end

  def set_visibility

    (self.body_decl || []).each do |bd|
      if bd.respond_to?(:visibility) && bd.visibility == :default
        if self.kind == :struct || self.kind == :union
          bd.visibility = :public
        else 
          bd.visibility = :private
        end
      end
      bd.set_visibility
    end

  end

end

class CPPTypedef

  include QualifiedNameResolver

  def myself
    self.type.name
  end

  def children
    []
  end

  def other_children
    []
  end

end

class CPPStructDeclaration

  include QualifiedNameResolver

  def children

    # take this chance to set the parent to struct
    self.struct.parent = self
    c = self.struct.body_decl || []

    # add enum constants (CPPEnumSpec)
    (self.struct.body_decl || []).each do |bd|
      if bd.is_a?(CPPEnumDeclaration) && bd.enum && bd.enum.specs && !bd.enum.is_class
        c += bd.enum.specs
      end
    end

    c

  end

  def remove(d)
    self.struct.body_decl && self.struct.body_decl.delete(d)
  end

  def insert(d)
    self.struct.body_decl ||= []
    self.struct.body_decl << d
  end

  def other_children

    # add base classes both as sub-namespace and individual parts
    # and add self so scoping is possible into ourself 
    c = [ self ]

    (self.struct.base_classes || []).each do |bc|
      # The parent may be null for template base classes which are 
      # forward-declared .. we're not interested in this case.
      if self.parent
        bc_obj = self.parent.resolve_qid(bc.class_id, nil, false)
        # NOTE: it may look strange to test whether the base class is the class itself but
        # since we do a half-hearted job of resolving template variants, this may happen
        # if we derive a template specialization from another one (specifically 
        # "template<class T> struct is_default_constructible : is_default_constructible<> { .. }"
        if bc_obj != self && bc_obj.is_a?(CPPStructDeclaration)
          c << bc_obj
          c += bc_obj.children
          c += bc_obj.other_children
        end
      end
    end

    c

  end

  def myself_weak
    # the weak identity will also include forward declarations
    self.struct.id.to_s
  end

  def myself
    # forward declarations (struct.body_decl == nil and no base classes) don't produce a name and
    # will therefore not contribute 
    (self.struct.body_decl || self.struct.base_classes) && self.struct.id.to_s
  end

  def myid
    # forward declarations (struct.body_decl == nil and no base classes) don't produce a name and
    # will therefore not contribute 
    (self.struct.body_decl || self.struct.base_classes) && self.struct.id
  end

  def setid(id)
    self.struct.id = id
  end

  def set_visibility
    self.struct && self.struct.set_visibility
  end

end

class CPPNamespace

  include QualifiedNameResolver

  def children

    # take this opportunity to join identical namespaces
    if self.members
      new_members = []
      ns = {}
      self.members.each do |d|
        if d.is_a?(CPPNamespace)
          if !ns[d.myself]
            ns[d.myself] = d
            new_members << d
          else
            ns[d.myself].members += d.members
          end
        else
          new_members << d
        end
      end
      self.members = new_members
    end      

    self.members || []

  end

  def other_children
    # add self so scoping is possible into ourself 
    [ self ]
  end

  def myself
    self.name.to_s
  end

  def remove(d)
    self.members.delete(d)
  end

  def insert(d)
    self.members << d
  end

  def set_visibility

    (self.members || []).each do |m|
      if m.respond_to?(:visibility) && m.visibility == :default
        m.visibility = :public
      end
      m.set_visibility
    end

  end

end

class CPPModule

  include QualifiedNameResolver

  def children

    # take this opportunity to join identical namespaces
    new_decls = []
    ns = {}
    self.decls.each do |d|
      if d.is_a?(CPPNamespace)
        if !ns[d.myself]
          ns[d.myself] = d
          new_decls << d
        else
          ns[d.myself].members += d.members
        end
      else
        new_decls << d
      end
    end

    self.decls = new_decls

  end

  def other_children
    []
  end

  def remove(d)
    self.decls.delete(d)
  end

  def insert(d)
    self.decls << d
  end

  def myself
    nil
  end

  def set_visibility
    (self.decls || []).each do |d|
      if d.respond_to?(:visibility) && d.visibility == :default
        d.visibility = :public
      end
      d.set_visibility
    end
  end

end
