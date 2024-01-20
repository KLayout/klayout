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

class Module
  def def_initializer(*args)
    self.class_eval <<END
      def initialize(#{args.join(", ")})
        #{args.map { |arg| "@#{arg} = #{arg}" }.join("\n")}
      end
      def initialize_copy(other)
        # TODO: is there a better way to check whether dup can be used?
        # Implement Hash?
        #{args.map { |arg| "a = other.#{arg}\n"+
                           "if a.is_a?(TrueClass) || a.is_a?(FalseClass) || a.is_a?(NilClass) || a.is_a?(Fixnum) || a.is_a?(Float) || a.is_a?(Symbol)\n"+
                           "  @#{arg} = a\n"+
                           "elsif a.is_a?(Array)\n"+
                           "  @#{arg} = a.collect { |aa| aa.dup }\n"+
                           "else\n"+
                           "  @#{arg} = a.dup\n"+
                           "end"
                   }.join("\n")}
      end
END
  end
end

# @brief The base class for all CPP objects
class CPPObject
end

# @brief Denotes type nesting wrappers
# Type nesting provide a modification the renders a different type
# for the inner "user" of the type. For example:
# "int **x" starts with "int", adds a pointer, adds another pointer and
# ends at "x" (the symbol). "int" is the "concrete type", the two 
# pointers are type wrappers and "x" is the innermost elements. 
# The type s read from the inside out: "x" is a pointer to a pointer to an int.
class CPPOuterType < CPPObject
end

# @brief An array specification
# @attribute inner The inner type (the type which makes use of the array)
# "inner" is either another CPPOuterType or a CPPQualifiedId (the innermost)
# part.
class CPPArray < CPPOuterType

  attr_accessor :inner
  def_initializer :inner 

  def to_s
    if self.inner.is_a?(CPPPointer) 
      "(" + self.inner.to_s + ") []"
    else
      self.inner.to_s + " []"
    end
  end

  def dump(i)
    i + "CPPArray\n" + i + " inner:\n" + self.inner.dump(i + "  ")
  end

end

# @brief A function specification
# @attribute inner The inner type (which makes use of the function)
# @attribute args The function arguments
# @attribute cv A const/voilatile specification if the function is a method (:const, :volatile)
# "inner" is either another CPPOuterType or a CPPQualifiedId (the innermost)
# part.
# "args" is an array of CPPType or CPPInitializedType objects, optionally terminated with 
# a CPPEllipsis.
# "cv" is a CPPCV object.
class CPPFunc < CPPOuterType

  attr_accessor :inner, :args, :cv, :ref
  def_initializer :inner, :args, :cv, :ref

  def to_s
    a = self.args
    a ||= []
    if !a.is_a?(Array)
      a = [a]
    end
    self.inner.to_s + " (" + a.join(", ") + ")" + (self.cv ? " " + self.cv.to_s : "") + (self.ref ? " " + self.ref.to_s : "")
  end

  def dump(i)
    i + "CPPFunc\n" + i + " inner:\n" + self.inner.dump(i + "  ") + "\n" + 
    i + " cv: " + self.cv.to_s + "\n" + 
    i + " ref: " + self.ref.to_s + "\n" + 
    i + " args:\n" + (self.args || []).collect { |f| f.dump(i + "  ") }.join("\n")
  end

end

# @brief A pointer declaration
# @attribute inner The inner type which sees the outer type converted into a pointer
# "inner" is either another CPPOuterType or a CPPQualifiedId (the innermost)
# part.
class CPPPointer < CPPOuterType

  attr_accessor :inner
  def_initializer :inner

  def to_s
    "* " + self.inner.to_s
  end

  def dump(i)
    i + "CPPPointer\n" + i + " inner:\n" + self.inner.dump(i + "  ")
  end

end

# @brief A reference declaration
# @attribute inner The inner type which sees the outer type converted into a reference
# "inner" is either another CPPOuterType or a CPPQualifiedId (the innermost)
# part.
class CPPReference < CPPOuterType

  attr_accessor :inner
  def_initializer :inner

  def to_s
    "& " + self.inner.to_s
  end

  def dump(i)
    i + "CPPReference\n" + i + " inner:\n" + self.inner.dump(i + "  ")
  end

end

# @brief A member/method pointer declaration
# @attribute qid The class of which a member/method is addressed (a CPPQualifiedId object)
# @attribute inner The inner type which sees the outer type converted into a member/method pointer
# @attribute cv A CPPCV object describing whether the method is a const or volatile one
# Functions are converted to method pointers, plain type into member pointers.
# "inner" is either another CPPOuterType or a CPPQualifiedId (the innermost)
# part.
class CPPMemberPointer < CPPOuterType

  attr_accessor :qid, :inner
  def_initializer :qid, :inner

  def to_s
    self.qid.to_s + "::* " + self.inner.to_s
  end

  def dump(i)
    i + "CPPMemberPointer\n" + i + " inner:\n" + self.inner.dump(i + "  ") + 
    i + " qid: " + self.qid.to_s
  end

end

# @param Adds const or volatile declaration 
# @attribute cv :const or :volatile
# @attribute inner The inner expression which sees the const/volatile declaration
class CPPCV < CPPOuterType

  attr_accessor :cv, :inner
  def_initializer :cv, :inner

  def to_s
    (self.cv ? (self.cv.to_s + " ") : "") + self.inner.to_s
  end

  def dump(i)
    i + "CPPCV\n" + 
    i + " cv: " + self.cv.to_s + "\n" +
    i + " inner:\n" + self.inner.dump(i + "  ")
  end

end

def CPPCV::wrap(cv, inner)
  cv ? CPPCV::new(cv, inner) : inner
end

# @brief A constant uses as a template instance argument
# @attribute value A string giving the value
class CPPConst < CPPObject

  attr_accessor :value
  def_initializer :value

  def to_s
    self.value
  end

  def dump(i)
    i + "CPPConst\n" + i + " value: " + self.value.to_s
  end

end

# @brief A class the template instance arguments
# @attribute args An array of CPPConst (for constants) or CPPType objects (for types)
# This class is used for both template instances or declarations.
class CPPTemplateArgs < CPPObject

  attr_accessor :args
  def_initializer :args

  def to_s
    self.args.collect { |a| a.to_s }.join(", ")
  end

  def dump(i)
    i + "CPPTemplateArgs\n" +
    i + " args:\n" + (self.args || []).collect { |a| a.dump(i + "  ") }.join("\n")
  end

end

# @brief An class or namespace name, optionally with template arguments
# @attribute id The basic id (a string)
# @attribute template_args A CPPTemplateArgs object describing the template arguments or nil, if it does not describe a template instance or declaration
class CPPId < CPPObject

  attr_accessor :id, :template_args
  def_initializer :id, :template_args

  def to_s
    ta = self.template_args.to_s
    if ta =~ />$/
      ta += " "
    end
    self.id + (self.template_args ? ( "<" + ta + ">" ) : "")
  end

  def dump(i)
    i + "CPPId\n" +
    i + " id: " + self.id.to_s + 
    i + " template_args:\n" + self.template_args.dump(i + "  ")
  end

end

# @brief An anonymous ID
# This object is used in place of CPPQualifiedId if no name is given
class CPPAnonymousId < CPPObject

  def to_s
    "" 
  end

  def dump(i)
    i + "CPPAnonymousId"
  end

end

# @brief An id, optionally qualified by a namespace
# @attribute global If true, the Id is rooted in the global namespace
# @attribute parts A sequence of CPPId objects forming the namespace sequence
class CPPQualifiedId < CPPObject

  attr_accessor :global, :parts
  def_initializer :global, :parts

  def to_s
    (self.global ? "::" : "") + (self.parts || []).collect { |p| p.to_s }.join("::")
  end

  def dump(i)
    n = (self.global ? "::" : "") + (self.parts || []).collect { |p| "[" + p.to_s + "]" }.join("::")
    i + "CPPQualifiedId (" + n + ")"
  end

end

# @brief A "plain old type" (double, float, int, char, ...)
# @attribute signed Is nil, :signed or :unsigned (for the types supporting that)
# @attribute length Is nil, :short, :long or :longlong for the types supporting that
# @attribute type The basic type (:int, :char, :bool, :float, :double)
class CPPPOD < CPPObject

  attr_accessor :signed, :length, :type
  def_initializer :signed, :length, :type

  def to_s
    
    s = ""
    if self.signed == :signed
      s += "signed "
    elsif self.signed == :unsigned
      s += "unsigned "
    end
    if self.length == :short
      s += "short "
    elsif self.length == :long
      s += "long "
    elsif self.length == :longlong
      s += "long long "
    end

    s + self.type.to_s

  end

  def dump(i)
    i + "CPPPOD (" + self.to_s + ")"
  end

end

# @brief A base class declarations
# @attribute visibility :default, :public, :private or :protected
# @attribute virtual Is true, if the class is a virtual base class
# @attribute class_id A CPPQualifiedId object pointing to the base class
class CPPBaseClass < CPPObject

  attr_accessor :visibility, :virtual, :class_id
  def_initializer :visibility, :virtual, :class_id

  def to_s
    (self.visibility ? self.visibility.to_s + " " : "") + (self.virtual ? "virtual " : "") + self.class_id.to_s
  end

  def dump(i)
    i + "CPPBaseClass\n" + 
    i + " visibility: " + self.visibility.to_s + 
    i + " virtual:\n" + self.virtual.to_s + 
    i + " class_id:\n" + self.class_id.to_s  
  end

end

# @brief Describes a structure, class or union
# @attribute kind :struct, :class or :union
# @attribute id The name of the struct, class or union
# @attribute base_classes The base class declarations (an array of CPPBaseClass objects)
# @attribute body_decl An array or CPPUsingSpec, CPPFriendDecl, CPPTypedef, CPPEnumDeclaration, CPPStructDeclaration or CPPDeclaration objects 
# "body_decl" forms the body of the class. It contains friend declarations, using specs, typedef's, enum's,
# nested struct's or method and member declarations.
class CPPStruct < CPPObject

  attr_accessor :kind, :id, :base_classes, :body_decl
  def_initializer :kind, :id, :base_classes, :body_decl

  def to_s
    self.kind.to_s + " " + self.id.to_s
  end

  def dump(i)
    l = i + self.kind.to_s + ": " + self.id.to_s + "\n"
    l += (self.base_classes || []).collect { |b| i + "  < " + b.to_s + "\n" }.join("")
    l += (self.body_decl || []).collect { |b| b.dump(i + "  ") }.join("\n")
  end

end

# @param Describes a type derived with "__typeof"
# @attribute what The object from which the type is derived (a CPPQualifiedId)
class CPPTypeOf < CPPObject

  attr_accessor :what
  def_initializer :what

  def to_s
    "__typeof(" + what.to_s + ")"
  end

  def dump(i)
    i + "CPPTypeOf\n" + 
    i + " what: " + self.what.to_s  
  end

end

# @param Describes an ellipsis inside a function argument list
class CPPEllipsis < CPPObject

  def to_s
    "..."
  end

  def dump(i)
    i + "CPPEllipsis"  
  end

end

# @brief A general type definition
# @attribute concrete The concrete part of the type: a CPPPOD, CPPStruct, CPPEnum, CPPTypeof or CPPQualifiedId object)
# @attribute inner The "inner part": one of the CPPOuterType-derived classes or CPPQualifiedId
# @attribute init A string indicating the initialization expression
# If the concrete type is a class, struct, union, enum or typedef, a CPPQualifiedId is used for the 
# attribute, describing the initial type (which can be complex already in the case of a typedef).
# The "inner" declarations add pointers, references, arrays of functions and finally the identifier.
# Without any further qualification, "inner" is a CPPQualifiedId object.
class CPPType < CPPObject

  attr_accessor :concrete, :inner, :init
  def_initializer :concrete, :inner, :init

  def to_s
    i = self.inner.to_s
    s = self.concrete.to_s + " " + i
    # nicen:
    s.gsub(/\s+/, " ").sub(/^\s*/, "").sub(/\s*$/, "").gsub(/ \(/, "(").gsub(/\* /, "*").gsub(/& /, "&")
  end

  def dump(i)
    i + "CPPType\n" + 
    i + " init: " + (self.init ? self.init.to_s : "nil") + "\n" + 
    i + " concrete:\n" + (self.concrete ? self.concrete.dump(i + "  ") : i + "  nil") + "\n" + 
    i + " inner:\n" + self.inner.dump(i + "  ")
  end

end

# @brief A "using" instruction
# @attribute qid The qualified Id of the using specification
# @attribute visibility :default, :public, :private or :protected
class CPPUsingSpec < CPPObject

  attr_accessor :qid, :visibility
  def_initializer :qid, :visibility

  def dump(i)
    i + "using [" + self.visibility.to_s + "]: " + self.qid.to_s
  end

end

# @brief A typedef instruction
# @attribute type The declared type (a CPPType object, the inner name is the name of the defined type)
# @attribute visibility :default, :public, :private or :protected
class CPPTypedef < CPPObject

  attr_accessor :type, :visibility
  def_initializer :type, :visibility

  def dump(i)
    i + "typedef [" + self.visibility.to_s + "]: " + self.type.to_s
  end

end

# @brief A friend declaration 
# @attribute decl An array of friend types (an array of CPPType objects)
class CPPFriendDecl < CPPObject

  attr_accessor :decl
  def_initializer :decl

  def dump(i)
    self.decl.collect { |d| i + "friend: " + d.to_s }.join("\n")
  end

end

# @brief A type template argument (with an optional initializer)
# @attribute type The template argument (a type)
# @attribute def_type The default type (nil or a CPPType object) 
class CPPClassTemplateArg < CPPObject

  attr_accessor :type, :def_type
  def_initializer :type, :def_type

  def to_s
    if self.def_type
      self.type.to_s + "=" + self.def_type.to_s
    else
      self.type.to_s
    end
  end

end

# @brief A template declaration
# @attribute parts An array of CPPClassTemplateArg or CPPDirectTemplateArg objects
# CPPClassTemplateArg objects describe type arguments while CPPDirectTemplateArg objects
# describe value arguments (i.e. int).
class CPPTemplateDecl < CPPObject

  attr_accessor :parts
  def_initializer :parts

  def to_s
    (self.parts || []).collect { |p| p.to_s }.join(", ")
  end

end

# @brief An internal object, does not appear in the final parsed tree
class CPPAttr < CPPObject
  attr_accessor :attr
  def_initializer :attr
end

# @brief An struct/class/union declaration inside a namespace or class
# @attribute struct The CPPStruct object describing the class, struct or union
# @attribute template_decl nil or a CPPTemplateDecl object if the declaration is a template declaration
# @attribute visibility :default, :public, :private or :protected
class CPPStructDeclaration < CPPObject

  attr_accessor :struct, :template_decl, :visibility
  def_initializer :struct, :template_decl, :visibility

  def dump(i)
    l = i + self.struct.kind.to_s + "_decl [" + self.visibility.to_s + "]: "
    if self.template_decl
      l += "template<" + self.template_decl.to_s + ">"
    end
    l += "\n"
    l += self.struct.dump(i + "  ")
  end

end

# @brief An enum declaration inside a namespace or class
# @attribute enum The CPPEnum object describing the enum
# @attribute visibility :default, :public, :private or :protected
class CPPEnumDeclaration < CPPObject

  attr_accessor :enum, :visibility
  def_initializer :enum, :visibility

  def dump(i)
    i + "enum_decl [" + self.visibility.to_s + "]:\n" + self.enum.dump(i + "  ")
  end

end

# @brief A declaration of either a function, a type, a member or a method
# @attribute type the declared type: a CPPType object
# @attribute template_decl nil or a CPPTemplateDecl object if the declaration is a template declaration
# @attribute visibility :default, :public, :private or :protected
# @attribute storage_class nil, :extern or :static
# @attribute virtual Is true for virtual methods
# @attribute inline Is true for inline declarations
class CPPDeclaration < CPPObject

  attr_accessor :type, :template_decl, :visibility, :storage_class, :virtual, :inline
  def_initializer :type, :template_decl, :visibility, :storage_class, :virtual, :inline

  def dump(i)
    l = i
    l += "decl [" + self.visibility.to_s + "]: "
    if self.storage_class
      l += self.storage_class.to_s + " "
    end
    if self.virtual
      l += "virtual "
    end
    if self.inline
      l += "inline "
    end
    if self.template_decl
      l += "template<" + self.template_decl.to_s + "> "
    end
    if self.type.respond_to?(:to_s)
      l += self.type.to_s
    else
      l += self.type.dump(i + "  ")
    end
    l += " #" + self.myself.to_s
    l
  end

end

# @brief A namespace
# @attribute name The namespace name (a string)
# @attribute members The content of the namespace: an array of CPPTypedef, CPPNamespace (nested namespaces), CPPStructDeclaration, CPPEnumDeclaration or CPPDeclaration objects
class CPPNamespace < CPPObject

  attr_accessor :name, :members
  def_initializer :name, :members

  def dump(i)
    l = i + "namespace #{self.name} {\n"
    l += (self.members || []).collect { |m| m.dump(i + "  ") }.join("\n")
    l += i + "}"
  end

end

# @brief Describes a single enum constant
# @attribute name The name of the enum constant
# @attribute init The initalizer (not parsed - just a string)
class CPPEnumSpec < CPPObject

  attr_accessor :name, :init
  def_initializer :name, :init

  def to_s
    self.name + (self.init ? "=" + self.init : "")
  end

end

# @brief Describes an enum declaration
# @attribute name The name of the enum (a string)
# @attribute specs the enum members (an array of CPPEnumSpec objects)
class CPPEnum < CPPObject

  attr_accessor :name, :specs, :is_class
  def_initializer :name, :specs, :is_class

  def to_s
    "enum " + (self.name || "")
  end

  def dump(i)
    l = i + self.to_s + (self.is_class ? " class" : "") + " {\n"
    l += (self.specs || []).collect { |s| i + "  " + s.to_s + "\n" }.join("")
    l += i + "}"
  end
  
end

# @brief The root object of the declaration tree
# @attribute decls The content of the module: an array of CPPTypedef, CPPNamespace (nested namespaces), CPPStructDeclaration, CPPEnumDeclaration or CPPDeclaration objects
class CPPModule < CPPObject

  attr_accessor :decls
  def_initializer :decls

  def dump
    (self.decls || []).collect { |d| d.dump("") }.join("\n")
  end

end

