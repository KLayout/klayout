# 
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
#

class Treetop::Runtime::SyntaxNode

  # An alias which can be used inside a derived cpp implementation
  # The default implementation collects all objects emitted by the subnodes.
  def get_cpp
    a = elements && elements.collect { |e| e.cpp }.select { |e| e }.flatten
    (a && !a.empty?) ? a : nil
  end

  # Returns the CPP objects emitted by this node. 
  # This method delivers CPP objects or arrays of CPP objects.
  def cpp
    get_cpp
  end

  # Returns the single CPP object or nil
  def cpp_reduced
    a = self.cpp
    if a.is_a?(Array) 
      a.size == 1 || raise("Internal error: more than one syntax tree node")
      a = a[0]
    end
    a
  end

  # Note: this method is required because curly braces cannot be put
  # into parser conditions (treetop syntax flaw?) 
  def text_value_ends_with_curly_brace
    text_value =~ /\}$/
  end

end

# These are a couple of emitter extensions to SyntaxNode which produce the definition syntax tree
# through the "cpp" method:

module PTypeOf 
  def cpp
    CPPTypeOf::new(qid.cpp)
  end
end

def extract_signed(t)
  if t =~ /unsigned/
    :unsigned
  elsif t =~ /signed/
    :signed
  else
    nil
  end
end

def extract_length(t)
  if t =~ /long long/
    :longlong
  elsif t =~ /long/
    :long
  elsif t =~ /short/
    :short
  else
    nil
  end
end

module PIntType 
  def cpp
    CPPPOD::new(extract_signed(text_value), extract_length(text_value), :int)
  end
end

module PCharType 
  def cpp
    CPPPOD::new(extract_signed(text_value), nil, :char)
  end
end

module PBoolType
  def cpp
    CPPPOD::new(nil, nil, :bool)
  end
end

module PSpecialType
  def cpp
    # the special type produces an ID
    CPPQualifiedId::new(false, [ CPPId::new(id.text_value, nil) ])
  end
end

module PFloatType
  def cpp
    kind = text_value =~ /double/ ? :double : :float
    CPPPOD::new(nil, extract_length(text_value), kind)
  end
end

module PVoidType
  def cpp
    CPPPOD::new(nil, nil, :void)
  end
end

module PEnumSpec
  def cpp
    CPPEnumSpec::new(id.text_value, initspec.nonterminal? ? initspec.init.text_value : nil)
  end
end

module PEnumType
  def cpp
    name = id ? id.text_value : nil
    specs = bodyspec.nonterminal? ? bodyspec.body.cpp : nil
    CPPEnum::new(name, specs, is_class.nonterminal?)
  end
end

module PConst
  def cpp
    CPPConst::new(text_value)
  end
end

module PTemplateArgs
  def cpp
    decl = self.get_cpp
    decl ? CPPTemplateArgs::new(decl) : nil
  end
end

module PId
  def cpp
    CPPId::new(id.text_value, taspec.nonterminal? ? taspec.ta.cpp_reduced : nil)
  end
end

module PQualifiedId
  def cpp
    CPPQualifiedId::new(globalspec.nonterminal?, self.get_cpp)
  end
end

module PBaseClass
  def cpp
    visibility = nil
    virtual = false
    at = attr.text_value
    if at =~ /private/
      visibility = :private
    elsif at =~ /public/
      visibility = :public
    elsif at =~ /protected/
      visibility = :protected
    end
    if at =~ /virtual/
      virtual = true
    end
    CPPBaseClass::new(visibility, virtual, cid.cpp_reduced)
  end
end

module PStructOrClassType

  def cpp

    kind = :class 
    if stype.text_value == "struct"
      kind = :struct
    elsif stype.text_value == "union"
      kind = :union
    end

    id = idspec.nonterminal? ? idspec.id.cpp_reduced : nil
    base_classes = bcspec.nonterminal? ? bcspec.bc.cpp : nil
    body_decl = bodyspec.nonterminal? ? bodyspec.body.cpp : nil

    CPPStruct::new(kind, id, base_classes, body_decl)

  end

end

module PCV
  def to_symbol
    return self.text_value == "const" ? :const : :volatile
  end
end

module PFriendDecl
  def cpp
    CPPFriendDecl::new(t.cpp)
  end
end

module PPointer
  def cpp
    CPPCV::wrap(cvspec.nonterminal? && cvspec.cv.to_symbol, CPPPointer::new(itspec.nonterminal? ? itspec.it.cpp_reduced : CPPAnonymousId::new))
  end
end

module PReference
  def cpp
    CPPCV::wrap(cvspec.nonterminal? && cvspec.cv.to_symbol, CPPReference::new(itspec.nonterminal? ? itspec.it.cpp_reduced : CPPAnonymousId::new))
  end
end

module PMemberPointer
  def cpp
    CPPMemberPointer::new(cspec.qid.cpp, itspec.nonterminal? ? itspec.it.cpp_reduced : CPPAnonymousId::new, cvspec.nonterminal? && cvspec.cv.to_symbol)
  end
end

module PArraySpec
  def cpp
    CPPArray::new(nil)
  end
end

module PFuncArgPart
  def cpp
    t.cpp_reduced
  end
end

module PFuncSpec
  def cpp
    CPPFunc::new(nil, (fa.nonterminal? && fa.text_value != "void") ? (fa.a.cpp || []) : [], cvspec.nonterminal? && cvspec.cv.to_symbol, refspec.nonterminal? && refspec.ref.text_value)
  end
end

module PInnerTypeWithCV
  def cpp
    CPPCV::wrap(cvspec.to_symbol, it.cpp_reduced)
  end
end

module PInnerType
  def cpp
    if pfx.nonterminal?
      pfx.elements.inject(it.cpp_reduced) do |r,e|
        ee = e.spec.cpp_reduced
        ee.inner = r
        ee
      end
    else
      it.cpp
    end
  end
end

module PStorageClass
  def cpp
    if text_value =~ /^static/
      return CPPAttr::new(:static)
    elsif text_value =~ /^extern/
      return CPPAttr::new(:extern)
    else
      return nil
    end
  end
end

module PVirtual
  def cpp 
    return CPPAttr::new(:virtual)
  end
end

module PInline
  def cpp 
    return CPPAttr::new(:inline)
  end
end

module PType

  def cpp
    # This is the class/struct/union/enum declaration if there is one
    d = ct.cpp
    if d.is_a?(Array)
      r = d.select { |i| i.is_a?(CPPStruct) || i.is_a?(CPPEnum) }
    elsif d.is_a?(CPPStruct) || d.is_a?(CPPEnum)
      r = [d]
    else
      r = []
    end
    # Create each declaration
    ot = CPPCV::wrap(cvspec.nonterminal? && cvspec.cv.to_symbol, ct.cpp_reduced)
    if il.nonterminal? 
      r << CPPType::new(ot, il.t1.cpp_reduced, il.i1.nonterminal? ? il.i1.is1.text_value : nil)
      il.tt.elements.each do |t|
        r << CPPType::new(ot, t.t2.cpp_reduced, t.i2.nonterminal? ? t.i2.is2.text_value : nil) 
      end
    else
      r << CPPType::new(ot, CPPAnonymousId::new, pi.nonterminal? ? pi.is.text_value : nil)
    end
    r
  end

end

module PEllipsis
  def cpp
    CPPEllipsis::new
  end
end

module PTypeWoComma
  def cpp
    ot = CPPCV::wrap(cvspec.nonterminal? && cvspec.cv.to_symbol, ct.cpp_reduced)
    if il.nonterminal?
      CPPType::new(ot, il.t.cpp_reduced, il.i.nonterminal? ? il.i.is.text_value : nil)
    else
      CPPType::new(ot, CPPAnonymousId::new, pi.nonterminal? ? pi.is.text_value : nil)
    end
  end
end

module PTypeForTemplate
  def cpp
    ot = CPPCV::wrap(cvspec.nonterminal? && cvspec.cv.to_symbol, ct.cpp_reduced)
    CPPType::new(ot, il.nonterminal? ? il.t.cpp_reduced : CPPAnonymousId::new, nil)
  end
end

module PUsing
  def cpp
    CPPUsingSpec::new(id.cpp_reduced, :default)
  end
end

module PTypedef
  def cpp
    t.cpp.collect do |d|
      if d.is_a?(CPPStruct)
        CPPStructDeclaration::new(d, nil, :default)
      elsif d.is_a?(CPPEnum)
        CPPEnumDeclaration::new(d, :default)
      else
        CPPTypedef::new(d, :default)
      end
    end
  end
end

module PPrivateClassStructBodyDeclarations
  def cpp
    decl = d.cpp
    decl && decl.collect do |d| 
      if d.respond_to?(:visibility) 
        d.visibility = :private
      end
      d
    end
  end
end

module PProtectedClassStructBodyDeclarations
  def cpp
    decl = d.cpp
    decl && decl.collect do |d| 
      if d.respond_to?(:visibility) 
        d.visibility = :protected
      end
      d
    end
  end
end

module PPublicClassStructBodyDeclarations
  def cpp
    decl = d.cpp
    decl && decl.collect do |d| 
      if d.respond_to?(:visibility) 
        d.visibility = :public
      end
      d
    end
  end
end

module PTemplateDecl
  def cpp
    CPPTemplateDecl::new(self.get_cpp)
  end
end

module PClassTemplateArg
  def cpp
    CPPClassTemplateArg::new(t.cpp, dtspec.nonterminal? ? dtspec.cpp : nil)
  end
end

module PDeclaration
  def cpp
    td = nil
    if template.nonterminal? 
      td = template.d.cpp
    end
    storage_class = nil
    virtual = nil
    inline = nil
    if attr.nonterminal? && attr.cpp
      attr.cpp.each do |d|
        if d.attr == :virtual
          virtual = true
        elsif d.attr == :inline
          inline = true
        elsif d.attr == :static
          storage_class = :static
        elsif d.attr == :extern
          storage_class = :extern
        end
      end
    end
    # TODO: abstract declaration determination should be based on initializers on the
    # inner types
    t.cpp.collect do |d|
      if d.is_a?(CPPStruct)
        CPPStructDeclaration::new(d, td, :default)
      elsif d.is_a?(CPPEnum)
        CPPEnumDeclaration::new(d, :default)
      else
        CPPDeclaration::new(d, td, :default, storage_class, virtual, inline)
      end
    end
  end
end

module PExternBlock
  def cpp
    (self.get_cpp || []).collect do |d| 
      if d.is_a?(CPPDeclaration)
        d.storage_class = :extern
      end
      d
    end
  end
end

module PNamespace
  def cpp
    CPPNamespace::new(n.text_value, decls.cpp || [])
  end
end

module PModule
  def cpp
    CPPModule::new(self.get_cpp)
  end
end
