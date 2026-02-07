#!/usr/bin/env ruby

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

$:.push(File.dirname($0))

require 'oj'
require 'cpp_classes.rb'
require 'reader_ext.rb'

input_file = "all.db"
conf_file = "mkqtdecl.conf"
cls_list = nil
excl_list = {} 
modn = "Qt"
$gen_dir = "generated"

while ARGV.size > 0
  o = ARGV.shift
  if o == "-i"
    input_file = ARGV.shift
  elsif o == "-c"
    cls_list = ARGV.shift
  elsif o == "-x"
    excl_file = ARGV.shift
    File.open(excl_file, "r") do |file|
      file.each_line { |l| excl_list[l.chop] = true }
    end
  elsif o == "-s"
    conf_file = ARGV.shift
  elsif o == "-m"
    modn = ARGV.shift
  else
    raise("Invalid option #{o} - usage is 'produce.rb -s mkqtdecl.conf -i all.db -c QWidget,QSizePolicy'")
  end
end

# Objects of this type require special treatment when passing values between GSI and Qt
# (a converter is instantiated)
SpecialClasses = [ "QChar", "WId", "Q_PID" ]

# These typedefs are maintained to provide some abstraction
MaintainTypedefs = [ "Q_PID", "WId", "HANDLE", "Qt::HANDLE", 
                     "qulonglong", "qlonglong", 
                     "qint8", "qint16", "qint32", "qint64",
                     "quint8", "quint16", "quint32", "quint64",
                     "quintptr" ]

# The substitutions for signal/slot signatures to maintain the signature
SignalSubstitutions = {
  "QList<QModelIndex>" => "QModelIndexList"
}

class CPPDeclaration

  def is_const?
    self.type.func.cv == :const
  end

  def ref
    self.type.func.ref
  end

  def hash_str

    # TODO: this is a hack for making the hash values unique:
    $unique ||= {}

    # (weak) backward compatibility for hash computation
    func = self.type.func
    nmax = func.max_args

    hk = (self.is_const? ? "c" : "")
    if self.ref
      if self.ref == "&"
        hk += "r"
      elsif self.ref == "&&"
        hk += "rr"
      end
    end

    if nmax > 0

      args = nmax.times.collect { |ia| func.args[ia].anonymous_type.to_s }
      sig = args.join(",")
      hk += args.collect { |a| a.split(/\s+/) }.inspect.sum.to_s

      hku = 0
      hk_basic = hk
      while ($unique[self.type.name + "-" + hk] ||= sig) != sig
        hku += 1
        hk = hk_basic + "u" + hku.to_s
      end

    else
      hk += "0"
    end

    hk

  end

  def call_sig

    func = self.type.func
    nmax = func.max_args
    args = nmax.times.collect { |ia| func.args[ia].anonymous_type.to_s }.join(", ")

    res = "(" + args + ")"
    if self.is_const?
      res += " const"
    end
    if self.ref
      res += " " + self.ref
    end

    res

  end

  def sig(cls)
    self.type.name_substituted_type(CPPQualifiedId::new(false, [ cls, self.type.func.func_name ])).to_s
  end

  def raw_sig(cls)
    # backward compatibility for signature computation
    s = self.type.name_substituted_type(CPPQualifiedId::new(false, [ cls, self.type.func.func_name ])).to_s
    if self.ref
      s = s.sub(/\s+&+$/, "")
    end
    if self.is_const?
      s = s.sub(/\s+const$/, "")
    end
    s
  end

end

class CPPNamespace

  def collect_enum_decls(map, &filter)

    self.members.each do |bd|
      if bd.is_a?(CPPEnumDeclaration) 
        bd.enum && filter.call(bd) && (map[bd.enum.name] ||= bd)
      end
    end

  end

end

class CPPModule

  def collect_enum_decls(map, &filter)

    self.decls.each do |bd|
      if bd.is_a?(CPPEnumDeclaration) 
        bd.enum && filter.call(bd) && (map[bd.enum.name] ||= bd)
      end
    end

  end

end

class CPPEnum

  def resolve_typedefs(scope)
  end

  def rescope(prev_scope, other_scope, idpath)
  end

  def each_qid(&block)
  end

end

class CPPStruct

  def resolve_typedefs(scope)
  end

  def rescope(prev_scope, other_scope, idpath)
  end

  def each_qid(&block)
  end

  def is_qobject?(conf)

    if self.id.to_s == "QObject"
      return true
    end

    self.each_base_class(conf) do |bc| 
      if bc.struct.is_qobject?(conf)
        return true
      end 
    end

    return false

  end

  def needs_adaptor(conf)

    cls = self.id.to_s 

    if conf.is_final_class?(cls)
      return false
    end
      
    all_methods = {}
    self.collect_all_methods(all_methods, conf)

    # If there is a private destructor, we cannot create an adaptor
    dtor = self.get_dtor
    if dtor && dtor.visibility == :private
      return false
    end

    # we generate an adaptor if there is a virtual destructor
    # Note: an adaptor can only be generated if the class has a vtable since
    # it will get a vtable through QtObjectBase and because of this a pure
    # cast to the base class will fail (base class = without vtable, derived class
    # = with vtable). But that cast is the basis of the void * representation of
    # the objects.
    needs_adaptor = dtor && dtor.virtual

    all_methods.each do |mn,m| 
      m.each do |bd| 
        if bd.virtual && conf.target_name(cls, bd, mn) 
          needs_adaptor = true
        elsif bd.virtual && bd.type.init == "0"
          # an abstract method was dismissed -> cannot generate an adaptor
          return false
        end
      end
    end

    # QObject will always require an adaptor to produce event emitters
    return is_qobject?(conf) || needs_adaptor

  end

  def collect_used_enums(map, conf)

    cls = self.id.to_s

    # collect used enums from inner classes
    (self.body_decl || []).each do |bd|
      decl = nil
      if bd.is_a?(CPPStructDeclaration) && bd.visibility == :public && bd.struct.body_decl && bd.myself != "" && !conf.is_class_dropped?(cls, bd.myself) 
        bd.struct.collect_used_enums(map, conf)
      end
    end

    # collect used enums from base classes
    (self.base_classes || []).each do |bc|
      bc_obj = self.parent.resolve_qid(bc.class_id)
      if bc_obj.is_a?(CPPStructDeclaration) && bc_obj.visibility == :public && bc_obj.struct.body_decl && bc_obj.myself != "" && !conf.is_class_dropped?(cls, bc_obj.myself) 
        bc_obj.struct.collect_used_enums(map, conf)
      end
    end

    methods = {}
    self.collect_all_methods(methods, conf)

    methods[cls] = self.collect_ctors

    needs_adaptor = self.needs_adaptor(conf)

    methods.each do |mn,m|

      m.each do |bd|

        vis = bd.visibility
        if vis == :public || (vis == :protected && needs_adaptor)

          # don't consider dropped methods
          conf.target_name(cls, bd, mn) || next

          bd.type.each_qid do |qid|
            obj = self.parent.resolve_qid(qid)
            if obj.is_a?(CPPEnumDeclaration) && obj.visibility == :public
              map[obj.object_id] = obj
            end
          end

        end

      end

    end

  end
  
  def collect_enum_decls(map, &filter)

    (self.body_decl || []).each do |bd|
      if bd.is_a?(CPPEnumDeclaration) && bd.visibility == :public
        bd.enum && filter.call(bd) && (map[bd.enum.name] ||= bd)
      end
    end

  end

  def collect_methods(map, weak = false)

    mmap = {} 

    (self.body_decl || []).each do |bd|

      decl = nil

      if bd.is_a?(CPPDeclaration) && ! bd.template_decl
        vis = bd.visibility
        decl = bd
      elsif bd.is_a?(CPPUsingSpec)
        # resolve using specs
        vis = bd.visibility
        tbd = self.parent.resolve_qid(bd.qid)
        if tbd && tbd.is_a?(CPPDeclaration) && ! tbd.template_decl
          decl = tbd
        end
      end

      if decl 

        func = decl.type.func
        if func && decl.type.concrete != nil

          mn = func.func_name
          cls = self.id.to_s
          decl_new = decl.dup
          decl_new.visibility = vis
          decl_new.type.resolve_typedefs(self.parent)
          decl_new.type.rescope(self.parent, self.global_scope, false)
          (mmap[mn] ||= []) << decl_new

        end
      
      end

    end

    #  take non-duplicates (by call signature) for the map
    #  weak ones do not redefine methods
    mmap.each do |mn,decls|

      seen = {}
      decls.each do |d|
        s = d.call_sig
        if !seen[s]
          seen[s] = true
          if !weak || !map[mn]
            (map[mn] ||= []) << d
          end
        end
      end

    end

  end

  def get_dtor

    (self.body_decl || []).each do |bd|

      if bd.is_a?(CPPDeclaration) 

        func = bd.type.func
        if func

          mn = func.func_name
          cls = self.id.to_s
          if mn == "~" + cls

            if ! bd.virtual

              # inherit the virtual flag from the base classes
              (self.base_classes || []).each do |bc|
                bc_obj = self.parent.resolve_qid(bc.class_id)
                if bc_obj.is_a?(CPPStructDeclaration)
                  bc_dtor = bc_obj.struct.get_dtor
                  if bc_dtor && bc_dtor.virtual
                    bd.virtual = true
                    return bd
                  end
                end
              end

            end
                
            return bd

          end

        end

      end

    end

    nil

  end

  def collect_friend_definitions

    friend_decl = []

    (self.body_decl || []).each do |bd|
      if bd.is_a?(CPPFriendDecl)
        bd.decl.each do |decl|
          if ! decl.template_decl && decl.respond_to?(:is_definition) && decl.is_definition
            friend_decl << decl
          end
        end
      end
    end

    friend_decl

  end

  def collect_ctors

    ctors = []

    (self.body_decl || []).each do |bd|

      if bd.is_a?(CPPDeclaration) && ! bd.template_decl

        # only public ctors are considered - currently protected ones are not used
        bd.visibility == :public || next

        func = bd.type.func
        if func

          mn = func.func_name
          cls = self.id.to_s
          if mn == cls
            bd_new = bd.dup
            bd_new.type.resolve_typedefs(self.parent)
            bd_new.type.rescope(self.parent, self.global_scope, false)
            ctors << bd_new
          end

        end

      end

    end

    ctors

  end

  def collect_all_methods(map, conf)

    self.collect_methods(map)

    (self.base_classes || []).select { |b| conf.imported?(self.id.to_s, b.class_id.to_s) }.each do |bc|

      bc_obj = self.parent.resolve_qid(bc.class_id)
      if bc_obj.is_a?(CPPStructDeclaration)

        base_map = {}
        bc_obj.struct.collect_all_methods(base_map, conf)

        # derived classes override base class declarations
        base_map.each do |mn, d|

          # virtual is inherited to the derived class
          map[mn] ||= d
          d.each do |bd_base|
            if bd_base.virtual
              sig = bd_base.call_sig
              map[mn].each do |bd|
                bd.call_sig == sig && (bd.virtual = true)
              end
            end
          end

        end

      elsif bc_obj
        puts("Warning: #{bc.class_id.to_s} is not a base class in #{self.id.to_s}")
      else
        puts("Cannot find base class: #{bc.class_id.to_s} of #{self.parent.myself} - declaration ignored")
      end

    end

  end

  def each_base_class(conf, &block)

    (self.base_classes || []).select { |b| conf.imported?(self.id.to_s, b.class_id.to_s) }.each do |bc|

      bc_obj = self.parent.resolve_qid(bc.class_id)
      if bc_obj.is_a?(CPPStructDeclaration)
        block.call(bc_obj)
        bc_obj.struct.each_base_class(conf, &block)
      end

    end

  end

end

class CPPEllipsis

  def resolve_typedefs(scope)
  end

  def rescope(prev_scope, other_scope, idpath)
  end

  def each_qid(&block)
  end

end

class CPPPOD

  def resolve_typedefs(scope)
  end

  def rescope(prev_scope, other_scope, idpath)
  end

  def each_qid(&block)
  end

end

class CPPAnonymousId

  def resolve_typedefs(scope)
  end

  def rescope(prev_scope, other_scope, idpath)
  end

  def each_qid(&block)
  end

end

class CPPOuterType

  def resolve_typedefs(scope)
    self.inner.resolve_typedefs(scope)
  end

  def rescope(prev_scope, other_scope, idpath)
    self.inner.rescope(prev_scope, other_scope, idpath)
  end

  def each_qid(&block)
    self.inner.each_qid(&block)
  end

end

class CPPFunc

  def func_name
    self.inner.is_a?(CPPQualifiedId) && self.inner.parts[-1].to_s
  end

  def min_args
    n = 0
    self.args.each do |a|
      if a.is_a?(CPPType) && !a.init
        n += 1
      else
        break
      end
    end
    n
  end

  def max_args
    n = 0
    self.args.each do |a|
      if a.is_a?(CPPType)
        n += 1
      else
        break
      end
    end
    n
  end

  def resolve_typedefs(scope)
    self.args && self.args.each { |a| a.resolve_typedefs(scope) }
  end

  def rescope(prev_scope, other_scope, idpath)
    self.inner && self.inner.rescope(prev_scope, other_scope, idpath)
    self.args && self.args.each { |a| a.rescope(prev_scope, other_scope, false) }
  end

  def each_qid(&block)
    self.inner && self.inner.each_qid(&block)
    self.args && self.args.each { |a| a.each_qid(&block) }
  end

end

class CPPConst
  def resolve_typedefs(scope)
  end
  def rescope(prev_scope, other_scope, idpath)
  end
  def each_qid(&block)
  end
end

class CPPTemplateArgs

  def resolve_typedefs(scope)
    self.args && self.args.each { |a| a.resolve_typedefs(scope) }
  end

  def rescope(prev_scope, other_scope, idpath)
    self.args && self.args.each { |a| a.rescope(prev_scope, other_scope, false) }
  end

  def each_qid(&block)
    self.args && self.args.each { |a| a.each_qid(&block) }
  end

end

class CPPQualifiedId

  def resolve_typedefs(scope)
    self.parts.each do |p|
      p.template_args && p.template_args.resolve_typedefs(scope)
    end
  end

  def each_qid(&block)
    block.call(self)
    self.parts.each do |p|
      p.template_args && p.template_args.each_qid(&block)
    end
  end

  def rescope(prev_scope, other_scope, idpath)

    # don't rescope identifiers
    idpath && return

    self.parts.each do |p|
      p.template_args && p.template_args.rescope(prev_scope, other_scope, false)
    end

    if !self.global && prev_scope != other_scope

      parents = {} 
      o = other_scope
      while o
        parents[o.parent] = true
        o = o.parent
      end

      obj = prev_scope.resolve_qid(self)
      if obj 

        self.parts = [ self.parts[-1] ] 

        s = obj.parent
        while s && !parents[s]
          s.myself && self.parts.unshift(CPPId::new(s.myself, nil))
          s = s.parent
        end

        parents[s] || raise("Unable to rescope from #{obj.parent.myself || '::'} to #{other_scope.myself || '::'}")

      end

    end

  end

end

class CPPType

  def each_qid(&block)
    self.inner && self.inner.each_qid(&block)
    self.concrete && self.concrete.each_qid(&block)
  end

  # @brief Returns the CPPQualifiedId inside self.concrete or nil if there is none
  def concrete_qid
    it = self.concrete
    while it.respond_to?(:inner)
      it = it.inner
    end
    it.is_a?(CPPQualifiedId) && it
  end

  # @brief Replaces the CPPQualifiedId inside self.concrete 
  def concrete_qid=(qid)
    it = self.concrete
    if it.is_a?(CPPQualifiedId)
      self.concrete = qid
    else
      itt = nil
      while it.respond_to?(:inner)
        itt = it
        it = it.inner
      end
      itt && (itt.inner = qid)
    end
  end

  def resolve_typedefs(scope)

    while self.concrete_qid

      qid = self.concrete_qid

      # keep some typedefs since they provide some abstraction
      if MaintainTypedefs.index(qid.to_s) 
        break
      end

      obj = scope.resolve_qid(qid)
      if obj && obj.is_a?(CPPTypedef)

        new_type = obj.type.dup
        it = new_type
        while it.respond_to?(:inner)
          if it.inner.is_a?(CPPAnonymousId) || it.inner.is_a?(CPPQualifiedId)
            it.inner = self.inner
            break
          else
            it = it.inner
          end
        end

        self.inner = new_type.inner
        self.concrete_qid = new_type.concrete 

        self.rescope(obj.parent, scope, false)

      else
        # resolve template arguments
        qid.resolve_typedefs(scope)
        break
      end
      
    end

    self.inner.resolve_typedefs(scope)

  end

  def rescope(prev_scope, other_scope, idpath)
    self.inner && self.inner.rescope(prev_scope, other_scope, true)
    self.concrete && self.concrete.rescope(prev_scope, other_scope, false)
  end

  def is_enum(decl_obj)
    if self.concrete_qid
      obj = decl_obj.resolve_qid(self.concrete_qid)
      # replace protected enum's by unsigned int (TODO: is that necessary?)
      return obj.is_a?(CPPEnumDeclaration) && obj.visibility == :protected 
    end
    false
  end

  def is_special(decl_obj)
    if self.concrete_qid
      obj = decl_obj.resolve_qid(self.concrete_qid)
      if obj.is_a?(CPPEnumDeclaration)
        return true
      elsif obj.is_a?(CPPTypedef) || obj.is_a?(CPPStructDeclaration)
        name = obj.myself
        return SpecialClasses.index(name) != nil
      end
    end
    false
  end

  def access_qt_arg(decl_obj, expr = nil)
    expr ||= self.name
    expr || raise("access_qt called with nil expression")
    if self.is_enum(decl_obj)
      # enums currently use unsigned int for the target name
      return "#{self.anonymous_type.to_s}(" + expr + ")"
    elsif self.is_special(decl_obj)
      ta = self.anonymous_type.to_s
      if ta =~ /^const (.*) &&$/
        ta = $1
        acc = ".cmove()"
      elsif ta =~ /^(.*) &&$/
        ta = $1
        acc = ".move()"
      elsif ta =~ /^const (.*) &$/
        ta = $1
        acc = ".cref()"
      elsif ta =~ /^(.*) &$/
        ta = $1
        acc = ".ref()"
      elsif ta =~ /^const (.*) \*$/
        ta = $1
        acc = ".cptr()"
      elsif ta =~ /^(.*) \*$/
        ta = $1
        acc = ".ptr()"
      else
        ta = ta.sub(/^const /, "")
        acc = ".cref()"
      end
      ta = ta.sub(/>$/, "> ")
      return "qt_gsi::QtToCppAdaptor<#{ta}>(" + expr + ")#{acc}"
    else
      return expr
    end
  end
      
  def access_qt_return(decl_obj, expr = nil)
    expr ||= self.name
    expr || raise("access_qt called with nil expression")
    if self.is_enum(decl_obj)
      # enums currently use unsigned int for the target name
      return "#{self.anonymous_type.to_s}(" + expr + ")"
    elsif self.is_special(decl_obj)
      ta = self.anonymous_type.to_s
      acc = ""
      racc = ""
      # Handle references and pointers in returns as copies - we loose the ability to write,
      # but can at least read them.
      if ta =~ /^const (.*) &$/ || ta =~ /^(.*) &$/
        ta = $1
      elsif ta =~ /^const (.*) &&$/ || ta =~ /^(.*) &&$/
        ta = $1
      elsif ta =~ /^const (.*) \*$/ || ta =~ /^(.*) \*$/
        ta = $1
        acc = "*"
      else
        # TODO: this is not required but for backward compatibility - questionable!
        racc = ".cref()" 
      end
      ta = ta.sub(/^const /, "")
      ta = ta.sub(/>$/, "> ")
      return "qt_gsi::QtToCppAdaptor<#{ta}>(" + acc + expr + ")#{racc}"
    else
      return expr
    end
  end
      
  def access_gsi_arg(decl_obj, expr = nil)
    if self.is_enum(decl_obj)
      # enums currently use unsigned int for the target name
      return "(unsigned int)(" + (expr || self.name) + ")"
    elsif self.is_special(decl_obj)
      ta = self.anonymous_type.to_s
      if ta =~ /^const (.*) &$/ || ta =~ /^(.*) &$/
        acc = ""
        ta = $1
      elsif ta =~ /^const (.*) \*$/ || ta =~ /^(.*) \*$/
        acc = "*"
        ta = $1
      else
        ta = ta.sub(/^const /, "")
        # TODO: should be, but not backward compatible: acc = ".cref()"
        acc = ""
      end
      ta = ta.sub(/>$/, "> ")
      if expr
        # Used to derivce init values dynamically - need to store the value on the heap since the 
        # read adaptor's lifetime is very limited and references are kept.
        return "qt_gsi::CppToQtReadAdaptor<#{ta}>(%HEAP%, " + acc + expr + ")"
      else
        return "qt_gsi::CppToQtAdaptor<#{ta}>(" + acc + self.name + ")"
      end
    else
      return expr || self.name
    end
  end
      
  def access_gsi_return(decl_obj, expr = nil)
    expr ||= self.name
    expr || raise("access_gsi called with nil expression")
    if self.is_enum(decl_obj)
      # enums currently use unsigned int for the target name
      return "(unsigned int)(" + expr + ")"
    elsif self.is_special(decl_obj)
      ta = self.anonymous_type.to_s
      acc = ""
      # Handle references and pointers in returns as copies - we loose the ability to write,
      # but can at least read them.
      if ta =~ /^const (.*) &$/ || ta =~ /^(.*) &$/
        ta = $1
      elsif ta =~ /^const (.*) \*$/ || ta =~ /^(.*) \*$/
        ta = $1
        acc = "*"
      end
      ta = ta.sub(/^const /, "")
      ta = ta.sub(/>$/, "> ")
      return "qt_gsi::CppToQtAdaptor<#{ta}>(" + acc + expr + ")"
    else
      return expr
    end
  end
      
  def gsi_decl_return(decl_obj)
    if self.is_enum(decl_obj)
      # enums currently use unsigned int for the target name
      return "unsigned int" + (self.name ? " " + self.name : "")
    elsif self.is_special(decl_obj)
      ta = (self.name ? self.anonymous_type.to_s : self.to_s)
      # return values of references or pointers are converted to plain copies - we cannot
      # convert them on the fly - a temporary object would not be sufficient
      if ta =~ /^const (.*) &$/ || ta =~ /^(.*) &$/ || ta =~ /^const (.*) \*$/ || ta =~ /^(.*) \*$/
        ta = $1
      end
      ta = ta.sub(/^const /, "")
      ta = ta.sub(/>$/, "> ")
      return "qt_gsi::Converter<#{ta}>::target_type" + (self.name ? " " + self.name : "")
    else
      return self.to_s
    end
  end

  def gsi_decl_arg(decl_obj)
    if self.is_enum(decl_obj)
      # enums currently use unsigned int for the target name
      return "unsigned int" + (self.name ? " " + self.name : "")
    elsif self.is_special(decl_obj)
      ta = (self.name ? self.anonymous_type.to_s : self.to_s)
      if ta =~ /^const (.*) &$/
        ta = $1
        const = "const "
        acc = " &"
      elsif ta =~ /^(.*) &$/
        ta = $1
        const = ""
        acc = " &"
      elsif ta =~ /^const (.*) \*$/
        ta = $1
        const = "const "
        acc = " *"
      elsif ta =~ /^(.*) \*$/
        ta = $1
        const = ""
        acc = " *"
      else
        ta = ta.sub(/^const /, "")
        acc = " &"
        const = "const "
      end
      ta = ta.sub(/>$/, "> ")
      return "#{const}qt_gsi::Converter<#{ta}>::target_type#{acc}" + (self.name ? " " + self.name : "")
    else
      return self.to_s
    end
  end

end

class Configurator

  def initialize
    @include = {}
    @no_copy_ctor = {}
    @no_default_ctor = {}
    @final_classes = {}
    @dropped_methods = {}
    @dropped_classes = {}
    @dropped_enums = {}
    @included_enums = {}
    @renamed_methods = {}
    @kept_args = {}
    @owner_args = {}
    @return_new = {}
    @aliased_methods = {}
    @dropped_enum_consts = {}
    @renamed_enum_consts = {}
    @aliased_enum_consts = {}
    @events = {}
    @property_readers = {}
    @property_writers = {}
    @no_imports = {}
    @native_impl = {}
  end

  def include(cls, files)
    @include[cls] ||= []
    @include[cls] += files
  end

  def no_copy_ctor(cls)
    @no_copy_ctor[cls] = true
  end

  def has_copy_ctor?(cls)
    !@no_copy_ctor[cls]
  end

  def no_default_ctor(cls)
    @no_default_ctor[cls] = true
  end

  def has_default_ctor?(cls)
    !@no_default_ctor[cls]
  end

  def final_class(cls)
    @final_classes[cls] = true
  end

  def is_final_class?(cls)
    @final_classes[cls]
  end

  def includes(cls)
    @include[cls]
  end

  def no_imports(cls, base = :every_base)
    @no_imports[cls] ||= {}
    @no_imports[cls][base] = true
  end

  def imported?(cls, base)
    !(@no_imports[cls] && (@no_imports[cls][:every_base] || @no_imports[cls][base]))
  end

  def drop_enum_const(cls, sig)
    @dropped_enum_consts[cls] ||= []
    @dropped_enum_consts[cls] << sig
  end

  def rename_enum_const(cls, sig, to)
    @renamed_enum_consts[cls] ||= []
    @renamed_enum_consts[cls] << [ sig, to ]
  end

  def def_enum_const_alias(cls, sig, to)
    @aliased_enum_consts[cls] ||= []
    @aliased_enum_consts[cls] << [ sig, to ]
  end

  def is_enum_const_dropped?(cls, sig)
    dm = (@dropped_enum_consts[:all_classes] || []) + (@dropped_enum_consts[cls] || [])
    dm.find { |d| sig =~ d } != nil
  end

  def drop_class(cls, inner_cls = :whole_class)
    @dropped_classes[cls] ||= []
    @dropped_classes[cls] << inner_cls
  end

  def drop_method(cls, sig)
    @dropped_methods[cls] ||= []
    @dropped_methods[cls] << sig
  end

  def return_new(cls, sig)
    @return_new[cls] ||= []
    @return_new[cls] << sig
  end

  def owner_arg(cls, sig, nth)
    @owner_args[cls] ||= []
    @owner_args[cls] << [ sig, nth ]
  end

  def keep_arg(cls, sig, nth)
    @kept_args[cls] ||= []
    @kept_args[cls] << [ sig, nth ]
  end

  def rename(cls, sig, to)
    @renamed_methods[cls] ||= []
    @renamed_methods[cls] << [ sig, to ]
  end

  def def_alias(cls, sig, to)
    @aliased_methods[cls] ||= []
    @aliased_methods[cls] << [ sig, to ]
  end

  def is_dropped?(cls, sig)
    dm = (@dropped_methods[:all_classes] || []) + (@dropped_methods[cls] || [])
    dm.find { |d| sig =~ d } != nil
  end

  def is_class_dropped?(cls, sig = :whole_class)
    if cls != :all_classes && cls !~ /^Q\w+$/
      # don't consider classes which are not plain Qt classes (i.e. templates, internal ones etc.)
      return true
    elsif cls != :all_classes && cls =~ /^QPrivateSignal$/
      # drop QPrivateSignal because that's just a marker
      return true
    else
      dc = (@dropped_classes[:all_classes] || []) + (@dropped_classes[cls] || [])
      if sig != :whole_class
        return dc.find { |d| d == :whole_class || sig =~ d } != nil
      else
        return dc.find { |d| d == :whole_class || sig == d } != nil
      end
    end
  end

  # @brief Produce an enum target name

  def target_name_for_enum_const(cls, sig, mid, mm = nil, decl = nil)

    if is_enum_const_dropped?(cls, sig)
      return nil
    else

      name = mid

      rn = (@renamed_enum_consts[:all_classes] || []) + (@renamed_enum_consts[cls] || [])
      rt = rn.find { |d| sig =~ d[0] } 
      if rt
        name = rt[1]
      end

      al = (@aliased_enum_consts[:all_classes] || []) + (@aliased_enum_consts[cls] || [])
      at = al.select { |d| sig =~ d[0] } 
      if !at.empty?
        name += "|" + at.collect { |a| a[1] }.join("|")
      end
        
      name

    end

  end

  # @brief Gets a list of owner arguments which will keep the object is non-null

  def owner_args(bd)

    cls = bd.parent.myself || "::"

    kn = (@owner_args[:all_classes] || []) + (@owner_args[cls] || [])
    if !kn.empty?
      sig = bd.sig(bd.parent.myself || "")
      return kn.select { |d| sig =~ d[0] }.collect { |d| d[1] }
    else
      return []
    end

  end

  # @brief Gets a list of arguments which need to be kept

  def kept_args(bd)

    cls = bd.parent.myself || "::"

    kn = (@kept_args[:all_classes] || []) + (@kept_args[cls] || [])
    if !kn.empty?
      sig = bd.sig(bd.parent.myself || "")
      return kn.select { |d| sig =~ d[0] }.collect { |d| d[1] }
    else
      return []
    end

  end

  # @brief Gets a value indicating whether a method returns a new object 
  # The lifetime of an object returned by such a method is managed by the 
  # script client

  def returns_new(bd)

    cls = bd.parent.myself || "::"

    rn = (@return_new[:all_classes] || []) + (@return_new[cls] || [])
    if !rn.empty?
      sig = bd.sig(bd.parent.myself || "")
      return rn.find { |d| sig =~ d } != nil
    else
      return false 
    end

  end

  # @brief Produce the target name for a method
  # Produces the target name string or nil if the method is supposed
  # to be dropped.

  def target_name(cls, bd, mid, mm = nil, decl = nil)

    sig = bd.sig(cls)

    # we also test for the parent of bd which may be different from cls if 
    # the method is imported from a base class or through "using"
    cls2 = (bd.parent && bd.parent.myself) || ""
    sig2 = bd.sig(cls2)

    # the drop test includes the static attribute so we can distinguish between
    # static and non-static methods
    if bd.storage_class == :static
      sig = "static " + sig 
      sig2 = "static " + sig2
    end

    if is_dropped?(cls, sig) || is_dropped?(cls2, sig2)
      return nil
    end

    # strip operator
    name = mid.sub(/^operator\s*/, "")
    
    # replace assignment operator
    if name == "="
      # drop assignment if the class does not have a copy ctor ("no copy semantics")
      # (required because otherwise operator= is exposed for adaptor classes)
      if !has_copy_ctor?(cls)
        return nil
      end
      name = "assign"
    end

    rn = (@renamed_methods[:all_classes] || []) + (@renamed_methods[cls] || []) + (@renamed_methods[cls2] || [])
    rt = rn.find { |d| sig =~ d[0] } 
    if rt
      name = rt[1]
    end

    al = (@aliased_methods[:all_classes] || []) + (@aliased_methods[cls] || []) + (@aliased_methods[cls2] || [])
    at = al.select { |d| sig =~ d[0] } 
    if !at.empty?
      name += "|" + at.collect { |a| a[1] }.uniq.join("|")
    elsif mm && decl && decl.type.func

      rn = property_reader_name(cls, sig) || property_reader_name(cls2, sig2)
      wn = property_writer_name(cls, sig) || property_writer_name(cls2, sig2)

      if rn && (decl.type.func.args || []).size == 0 

        # property readers become 
        #   isX -> isX?|:x
        #   x -> :x
        # the colon hints that this has to be a getter (for Python)

        if name =~ /^is([A-Z])(.*)/
          name += "?"
        end
        if name == rn
          name = ":" + rn
        else
          name += "|:" + rn
        end

      elsif wn && (decl.type.func.args || []).size == 1

        # property writers become setX -> setX|x=
        if name == wn
          name = wn + "="
        else
          name += "|" + wn + "="
        end
        
      elsif name =~ /^is([A-Z])(.*)/
        # implicit alias: isX -> isX?
        name += "?"
      end

    end
      
    name

  end

  # @brief Returns a string for building identifers from a method id
  
  def mid2str(mid)
    mid.gsub(/\(\)/, "_func_").
        gsub(/\[\]/, "_index_").
        gsub(/\//, "_slash_").
        gsub(/\*/, "_star_").
        gsub(/\^/, "_acute_").
        gsub(/=/, "_eq_").
        gsub(/-/, "_minus_").
        gsub(/~/, "_tilde_").
        gsub(/\+/, "_plus_").
        gsub(/</, "_lt_").
        gsub(/>/, "_gt_").
        gsub(/!/, "_excl_").
        gsub(/&/, "_amp_").
        gsub(/\|/, "_pipe_").
        gsub(/\s+/, "")
  end

  def include_enum(cls, sig)
    @included_enums[cls] ||= []
    @included_enums[cls] << sig
  end

  def is_enum_included?(cls, sig)
    dm = (@included_enums[:all_classes] || []) + (@included_enums[cls] || [])
    dm.find { |d| sig =~ d } != nil
  end

  def drop_enum(cls, sig)
    @dropped_enums[cls] ||= []
    @dropped_enums[cls] << sig
  end

  def is_enum_dropped?(cls, sig)
    dm = (@dropped_enums[:all_classes] || []) + (@dropped_enums[cls] || [])
    dm.find { |d| sig =~ d } != nil
  end

  def add_native_impl(cls, code, decl)
    @native_impl[cls] ||= []
    @native_impl[cls] << [ code, decl ]
  end

  def native_impl(cls)
    @native_impl[cls]
  end

  def property_writer(cls, sig, name)
    @property_writers[cls] ||= []
    @property_writers[cls] << [ sig, name ]
  end

  def property_writer_name(cls, sig)
    pm = (@property_writers[:all_classes] || []) + (@property_writers[cls] || [])
    p = pm.find { |d| sig =~ d[0] }
    p && p[1]
  end

  def property_reader(cls, sig, name)
    @property_readers[cls] ||= []
    @property_readers[cls] << [ sig, name ]
  end

  def property_reader_name(cls, sig)
    pm = (@property_readers[:all_classes] || []) + (@property_readers[cls] || [])
    p = pm.find { |d| sig =~ d[0] }
    p && p[1]
  end

  def event(cls, sig, args)
    @events[cls] ||= []
    @events[cls] << [ sig, args ]
  end

  # @brief Returns the event arguments or nil if the method is not an event
  def event_args(cls, sig)
    dm = (@events[:all_classes] || []) + (@events[cls] || [])
    e = dm.find { |d| sig =~ d[0] }
    e && e[1]
  end

  def load(filename)
    # with Ruby 1.9
    # found = ([ File.dirname($0) ] + $:).each do |d|
    #   ap = File.absolute_path(filename, d)
    #   File.readable?(ap) && ap
    # end
    # found = found.select { |f| f }
    # found.is_empty? && raise("Cannot load file #{filename} - file not found or not readable")
    # File.open(found[0], "r") do |file|
    #   self.instance_eval(file.read, found[0])
    # end

    # with Ruby 1.8
    File.open(filename, "r") do |file|
      self.instance_eval(file.read, filename)
    end
  end

end

class BindingProducer

  attr_accessor :modn, :root

  # @brief Read the input file (JSON)
  # 
  # This method will set up the binding producer for generating
  # the class declarations

  def read(input_file)

    @source_files = nil
    @classes = nil
    @ext_decls = []

    File.open(input_file, "r") do |file|

      json = file.read

      @root = Oj.load(json)
      puts "Reading done."

      # replace default visibility by the real one
      @root.set_visibility

      # collect the children and create the lookup tables. Also establish
      # the parent relationships
      @root.set_parent

      # move cross-scoped items such as "class A::B { ... };" to the right place
      @root.inject_scoped

      # and regenerate the lookup tables plus new parent relationships
      @root.set_parent

      @used_enums = {}
      
    end

  end

  # @brief Gets a list of objects to produce in the root namespace 

  def prod_list(conf)

    pl = []
    @root.ids.each do |cls|
      # only take plain (non-template) classes which are not dropped
      if !conf.is_class_dropped?(cls)
        pl << @root.id2obj(cls)
      end
    end

    return pl

  end

  # @brief Collects the used enum declarations

  def collect_used_enums(conf, decl_obj)

    if decl_obj.is_a?(CPPStructDeclaration)
      decl_obj.struct.collect_used_enums(@used_enums, conf)
    end

  end

  # @brief Substitute words from the context of a declaration

  def rescope_expr(expr, decl_obj)

    expr.gsub(/(::)?\w+/) do |s|
      if s[0] == ":"
        s
      else
        res = decl_obj.resolve_qid(CPPQualifiedId::new(false, [ CPPId::new(s, nil) ]))
        (res && res.parent && res.parent.myself) ? (res.parent.myself + "::" + s) : s
      end
    end

  end

  # @brief Produce the main declaration file for the given cls inside the given module

  def produce_cpp(conf, cls)

    qid = CPPQualifiedId::new(false, [ CPPId::new(cls, nil) ])
    decl_obj = @root.resolve_qid(qid)
    decl_obj || raise("Class not found: #{cls}")

    produce_cpp_from_decl(conf, decl_obj)

  end

  def produce_cpp_from_decl(conf, decl_obj)

    if !decl_obj.is_a?(CPPStructDeclaration) && !decl_obj.is_a?(CPPNamespace) && !decl_obj.is_a?(CPPEnumDeclaration)
      return
    end

    index = 0

    cont = true
    while cont

      (cls, clsn) = make_cls_names(decl_obj)
      @classes ||= []
      @classes << clsn

      if index > 0
        ofile_name = "gsiDecl#{clsn}_#{index}.cc"
      else
        ofile_name = "gsiDecl#{clsn}.cc"
      end
      ofile_path = $gen_dir + "/" + ofile_name
        
      File.open(ofile_path, "w") do |ofile|
      
        @source_files ||= []
        @source_files << ofile_name

        ofile.puts(<<"END");

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

/**
*  @file #{ofile_name} 
*
*  DO NOT EDIT THIS FILE. 
*  This file has been created automatically
*/

END

        if decl_obj.is_a?(CPPStructDeclaration)
          cont = produce_class(conf, decl_obj, ofile, index)
        elsif decl_obj.is_a?(CPPNamespace)
          cont = produce_namespace(conf, decl_obj, ofile, index)
        elsif decl_obj.is_a?(CPPEnumDeclaration)
          cont = produce_enum(conf, decl_obj, ofile, index)
        end

        puts("#{ofile_name} written.")

        index += 1

      end
        
    end

  end

  def is_enum_used?(bd)
    @used_enums[bd.object_id]
  end

  def make_child_cls_name(s)
    s[0].upcase + s[1..-1]
  end

  def make_cls_names(decl_obj)

    cls = decl_obj.myself
    clsn = make_child_cls_name(decl_obj.myself)

    o = decl_obj
    while o.parent && o.parent.myself
      o = o.parent
      cls = o.myself + "::" + cls
      clsn = make_child_cls_name(o.myself) + "_" + clsn
    end

    return [ cls, clsn ]

  end

  def make_cls_name(cls)
    return cls.split("::").collect { |s| make_child_cls_name(s) }.join("_")
  end

  def produce_class_include(conf, decl_obj, ofile)

    ( cls, clsn ) = make_cls_names(decl_obj)

    if !conf.includes(cls)

      if cls =~ /^(.*?)::/
        ofile.puts("#include <#{$1}>")
      else
        ofile.puts("#include <#{cls}>")
      end

      # derive dependencies where possible
      if decl_obj.is_a?(CPPStructDeclaration)

        struct = decl_obj.struct

        methods = {}
        struct.collect_all_methods(methods, conf)
        methods[cls] = struct.collect_ctors
        needs_adaptor = struct.needs_adaptor(conf)

        used_classes = {}

        methods.each do |mn,m|

          m.each do |bd|

            vis = bd.visibility
            is_signal = conf.event_args(cls, bd.sig(cls))

            if vis == :public || (vis == :protected && needs_adaptor) || is_signal

              # don't consider dropped methods
              conf.target_name(cls, bd, mn) || next
              bd.type.each_qid do |qid|
                obj = decl_obj.parent.resolve_qid(qid)
                if obj.is_a?(CPPStructDeclaration) 
                  used_classes[obj.object_id] = obj
                end
              end

            end

          end

        end

        used_classes.values.map { |uc| uc.myself || uc.myself_weak }.select { |uc| !conf.is_class_dropped?(uc) && uc != cls }.sort.each do |uc|
          ofile.puts("#include <#{uc}>")
        end

      end

    else

      conf.includes(cls).each do |f|
        ofile.puts("#include #{f}")
      end

    end

    ofile.puts("#include \"gsiQt.h\"")
    ofile.puts("#include \"gsi#{modn}Common.h\"")

  end

  def produce_enum(conf, decl_obj, ofile, index)

    ( cls, clsn ) = make_cls_names(decl_obj)

    produce_class_include(conf, decl_obj, ofile)
    ofile.puts("#include <memory>")

    ofile.puts("")
    ofile.puts("// -----------------------------------------------------------------------")
    ofile.puts("// enum #{cls}")

    ofile.puts("")

    # emit enum wrapper classes (top level, hence container class is nil)
    produce_enum_wrapper_class(ofile, conf, nil, cls, decl_obj) 

    return false

  end

  def produce_namespace(conf, decl_obj, ofile, index)

    ( cls, clsn ) = make_cls_names(decl_obj)

    enum_decls_by_name = {}
    decl_obj.collect_enum_decls(enum_decls_by_name) { true }

    produce_class_include(conf, decl_obj, ofile)
    ofile.puts("#include <memory>")

    ofile.puts("")
    ofile.puts("// -----------------------------------------------------------------------")
    ofile.puts("// namespace #{cls}")

    ofile.puts("")
    ofile.puts("class #{cls}_Namespace { };")
    ofile.puts("")

    if index == 0
      ofile.puts("namespace gsi")
      ofile.puts("{")
      ofile.puts("gsi::Class<#{cls}_Namespace> decl_#{cls}_Namespace (\"#{modn}\", \"#{clsn}\",")
      ofile.puts("  gsi::Methods(),")
      ofile.puts("  \"@qt\\n@brief This class represents the #{cls} namespace\");")
      ofile.puts("}")
      ofile.puts("")
    end

    # emit enum wrapper classes

    enums_per_file = 20

    en_names = enum_decls_by_name.keys.sort
  
    en_names.each_with_index do |en,en_index|
      if en_index < (index + 1) * enums_per_file && en_index >= index * enums_per_file
        ed = enum_decls_by_name[en]
        produce_enum_wrapper_class(ofile, conf, cls, en, ed) 
      end
    end

    # produce more files if required
    return en_names.size > (index + 1) * enums_per_file

  end

  def produce_enum_wrapper_class(ofile, conf, cls, en, ed) 

    clsn = cls && make_cls_name(cls)

    # emit enum wrapper classes
  
    ofile.puts("")
    ofile.puts("//  Implementation of the enum wrapper class for #{cls}::#{en}")
    ofile.puts("namespace qt_gsi")
    ofile.puts("{")
    ofile.puts("")
    if cls
      ofile.puts("static gsi::Enum<#{cls}::#{en}> decl_#{clsn}_#{en}_Enum (\"#{modn}\", \"#{clsn}_#{en}\",")
    else
      ofile.puts("static gsi::Enum<#{en}> decl_#{en}_Enum (\"#{modn}\", \"#{en}\",")
    end

    edecl = []

    ec = ed.enum.specs.collect { |s| s.name }
    ec.each_with_index do |ei,i|

      ei_name = conf.target_name_for_enum_const(cls ? cls : "::", "#{en}::#{ei}", ei)
      if ! ei_name
        # enum dropped
        next
      end

      if cls
        if ed.enum.is_class
          edecl << "  gsi::enum_const (\"#{ei_name}\", #{cls}::#{en}::#{ei}, \"@brief Enum constant #{cls}::#{en}::#{ei}\")"
        else
          edecl << "  gsi::enum_const (\"#{ei_name}\", #{cls}::#{ei}, \"@brief Enum constant #{cls}::#{ei}\")"
        end
      else
        if ed.enum.is_class
          edecl << "  gsi::enum_const (\"#{ei_name}\", #{en}::#{ei}, \"@brief Enum constant #{en}::#{ei}\")"
        else
          edecl << "  gsi::enum_const (\"#{ei_name}\", #{ei}, \"@brief Enum constant #{ei}\")"
        end
      end

    end

    ofile.puts("  " + edecl.join(" +\n  ") + ",\n")
    if cls
      ofile.puts("  \"@qt\\n@brief This class represents the #{cls}::#{en} enum\");")
    else
      ofile.puts("  \"@qt\\n@brief This class represents the #{en} enum\");")
    end
    ofile.puts("")

    if cls
      ofile.puts("static gsi::QFlagsClass<#{cls}::#{en} > decl_#{clsn}_#{en}_Enums (\"#{modn}\", \"#{clsn}_QFlags_#{en}\",")
      ofile.puts("  \"@qt\\n@brief This class represents the QFlags<#{cls}::#{en}> flag set\");")
      ofile.puts("")
    else
      ofile.puts("static gsi::QFlagsClass<#{en} > decl_#{en}_Enums (\"#{modn}\", \"QFlags_#{en}\",")
      ofile.puts("  \"@qt\\n@brief This class represents the QFlags<#{en}> flag set\");")
      ofile.puts("")
    end

    if cls

      # inject the declarations into the parent namespace or class
      pdecl_obj = ed.parent

      pcls = pdecl_obj.myself
      o = pdecl_obj
      while o.parent && o.parent.myself
        o = o.parent
        pcls = o.myself + "::" + pcls
      end

      pname = pcls
      if pdecl_obj.is_a?(CPPNamespace)
        pname = pcls + "_Namespace"
      end

      if ! ed.enum.is_class
        ofile.puts("//  Inject the declarations into the parent")
        ofile.puts("static gsi::ClassExt<#{pname}> inject_#{clsn}_#{en}_Enum_in_parent (decl_#{clsn}_#{en}_Enum.defs ());")
      end

      ofile.puts("static gsi::ClassExt<#{pname}> decl_#{clsn}_#{en}_Enum_as_child (decl_#{clsn}_#{en}_Enum, \"#{en}\");")
      ofile.puts("static gsi::ClassExt<#{pname}> decl_#{clsn}_#{en}_Enums_as_child (decl_#{clsn}_#{en}_Enums, \"QFlags_#{en}\");")

      ofile.puts("")

    end

    ofile.puts("}")
    ofile.puts("")

  end

  def produce_keep_self(ofile, alist, obj, owner_args = [])

    owner_args.each do |a|
      ofile.puts("  if (#{alist[a]}) {");
      ofile.puts("    qt_gsi::qt_keep (#{obj});")
      ofile.puts("  } else {");
      ofile.puts("    qt_gsi::qt_release (#{obj});")
      ofile.puts("  }");
    end
  
  end

  def produce_arg_read(ofile, decl_obj, func, alist, kept_args = [])

    n_args = func.max_args
    n_min_args = func.min_args
    has_heap = false

    n_args.times do |ia|

      t = func.args[ia]
      argname = alist[ia]

      at = t.anonymous_type
      ta = at.gsi_decl_arg(decl_obj)

      nt = t.renamed_type(argname)
      tn = nt.gsi_decl_arg(decl_obj)

      if !has_heap
        ofile.puts("  tl::Heap heap;")
        has_heap = true
      end

      if ia < n_min_args || !t.init

        ofile.puts("  #{tn} = gsi::arg_reader<#{ta} >() (args, heap);")

      else

        init = rescope_expr(t.init.to_s, decl_obj)
        init_expr = at.access_gsi_arg(decl_obj, init)
  
        if init_expr =~ /%HEAP%/
          init_expr = init_expr.gsub("%HEAP%", "heap")
        end

        ofile.puts("  #{tn} = args ? gsi::arg_reader<#{ta} >() (args, heap) : gsi::arg_maker<#{ta} >() (#{init_expr}, heap);")

      end

      if kept_args.index(ia)
        ofile.puts("  qt_gsi::qt_keep (#{argname});")
      end

    end

  end

  def produce_arg_init(ofile, decl_obj, func)

    n_min_args = func.min_args

    func.max_args.times do |ia|

      t = func.args[ia]
      argname = t.name || "arg#{ia + 1}"

      at = t.anonymous_type
      ta = at.gsi_decl_arg(decl_obj);

      if ia < n_min_args || !t.init
        ofile.puts("  static gsi::ArgSpecBase argspec_#{ia} (\"#{argname}\");")
      else
        init = rescope_expr(t.init.to_s, decl_obj)
        istr = init.gsub(/"/, "\\\"")
        ofile.puts("  static gsi::ArgSpecBase argspec_#{ia} (\"#{argname}\", true, \"#{istr}\");")
      end

      ofile.puts("  decl->add_arg<#{ta} > (argspec_#{ia});")

    end

  end

  def produce_class(conf, decl_obj, ofile, index)

    if index > 0
      raise "Internal error: no splitting of class definitions yet."
    end

    struct = decl_obj.struct

    (cls, clsn) = make_cls_names(decl_obj)
    cclsn = make_child_cls_name(decl_obj.myself)

    # produce public subclasses
    (struct.body_decl || []).each do |bd|
      if bd.is_a?(CPPStructDeclaration) && bd.visibility == :public && bd.struct.body_decl && bd.myself != "" && !conf.is_class_dropped?(cls, bd.myself) 
        produce_cpp_from_decl(conf, bd)
      end
    end

    base_classes = (struct.base_classes || []).select { |b| conf.imported?(cls, b.class_id.to_s) }
    base_cls = base_classes[0] && base_classes[0].class_id.to_s
    base_clsn = base_cls && make_cls_name(base_cls)

    # as we only support single base classes (a tribute to Ruby), we treat all other base classes as
    # mixins
    mixin_base_classes = base_classes[1..] || []

    methods_by_name = {}
    all_methods_by_name = {}
    enum_decls_by_name = {}

    bc_methods_by_name = {}
    base_classes.each do |bc|
      bc_decl_obj = decl_obj.resolve_qid(bc.class_id)
      bc_decl_obj && bc_decl_obj.respond_to?(:struct) && bc_decl_obj.struct.collect_all_methods(bc_methods_by_name, conf)
    end

    mmap = {}
    struct.collect_all_methods(all_methods_by_name, conf)
    struct.collect_methods(methods_by_name)
    struct.collect_enum_decls(enum_decls_by_name) { |bd| self.is_enum_used?(bd) || conf.is_enum_included?(cls, bd.myself) }

    # if one method is abstract, omit ctors for example
    is_abstract = all_methods_by_name.values.find do |m|
      m.find { |bd| bd.virtual && bd.type.init == "0" } != nil
    end

    # gets the operator= if there is one
    eq_op = (all_methods_by_name["operator="] || [])[0]

    # collect used enums in order to generate forward converter declarations
    # ("used" implies argument types and defined enums)
    used_ed = {}
    enum_decls_by_name.each do |en,ed|
      used_ed[ed.object_id] = ed
    end
    struct.collect_used_enums(used_ed, conf)
    used_enums_by_name = {}
    used_ed.each do |id,ed|
      id = CPPQualifiedId::new(false, [ CPPId::new(ed.enum.name, nil) ])
      id.rescope(ed.parent, ed.global_scope, false)
      used_enums_by_name[id.to_s] = ed
    end

    # needs_adaptor is true if there is any virtual method which can potentially
    # be reimplemented in script
    needs_adaptor = struct.needs_adaptor(conf) 

    # is_qobject is true, if the class is derived from QObject
    is_qobject = struct.is_qobject?(conf)

    # provide constructors only if we really can create an object
    # (we can't, if the class is abstract and there is no adaptor)
    if !is_abstract || needs_adaptor

      # collect ctors
      ctors = struct.collect_ctors

      # create a default ctor if there is no ctor at all and we can have one
      if ctors.empty? && conf.has_default_ctor?(cls)
        func = CPPFunc::new(CPPQualifiedId::new(false, [ CPPId::new(decl_obj.myself, nil) ]), [], nil, nil)
        type = CPPType::new(nil, func, nil)
        def_ctor = CPPDeclaration::new(type, nil, :public, nil, false, false, false)
        def_ctor.parent = decl_obj
        ctors << def_ctor
      end

    else
      ctors = []
    end

    global_operators = []

    # collect global operators with the given class as the first argument
    # Note that operators can also implicitly be declared as friends
    (@root.decls + struct.collect_friend_definitions).each do |bd|

      if bd.is_a?(CPPDeclaration) && bd.type.func && bd.type.name =~ /^operator/
        op_func = bd.type.func
        if op_func.args.size >= 1
          a1 = op_func.args[0].anonymous_type.to_s
          if a1 =~ /^(const\s+)?#{cls}(\s*&)?$/
            global_operators << bd
          end
        end
      end

    end

    native_impl = conf.native_impl(cls)

    has_metaobject = ((struct.body_decl || []).find { |bd| bd.is_a?(CPPDeclaration) && bd.type.name == "metaObject" } != nil)
    has_metaobject = has_metaobject && !conf.is_dropped?(cls, cls + "::staticMetaObject")

    mdecl = []
    mdecl_ctors = []

    produce_class_include(conf, decl_obj, ofile)

    ofile.puts("#include <memory>")

    ofile.puts("")
    ofile.puts("// -----------------------------------------------------------------------")
    ofile.puts("// #{struct.kind.to_s} #{cls}")

    if has_metaobject

      ofile.puts("")
      ofile.puts("//  get static meta object")
      ofile.puts("")
      ofile.puts("static void _init_smo (qt_gsi::GenericStaticMethod *decl)")
      ofile.puts("{")
      ofile.puts("  decl->set_return<const QMetaObject &> ();")
      ofile.puts("}")
      ofile.puts("")
      ofile.puts("static void _call_smo (const qt_gsi::GenericStaticMethod *, gsi::SerialArgs &, gsi::SerialArgs &ret) ")
      ofile.puts("{")
      ofile.puts("  ret.write<const QMetaObject &> (#{cls}::staticMetaObject);")
      ofile.puts("}")
      ofile.puts("")

      mdecl << "new qt_gsi::GenericStaticMethod (\"staticMetaObject\", \"@brief Obtains the static MetaObject for this class.\", &_init_smo, &_call_smo);"

    end

    native_impl && native_impl.each { |n| n[0] && ofile.puts(n[0]) }

    if ! needs_adaptor

      # expose ctors here (with virtual functions, the adaptor will expose the ctors)
      ctors.each do |bd|

        bd.visibility == :public || next

        func = bd.type.func
        hk = bd.hash_str
        sig = bd.sig(cls)
        rsig = bd.raw_sig(cls)
        mn = decl_obj.myself # ctor!

        mn_name = conf.target_name(cls, bd, mn)
        if ! mn_name
          # method dropped
          next
        elsif mn_name == mn
          mn_name = "new"
        end

        n_args = func.max_args
        n_min_args = func.min_args

        ant      = n_args.times.collect { |ia| func.args[ia].anonymous_type }
        alist    = n_args.times.collect { |ia| "arg#{ia + 1}" }
        qt_alist = n_args.times.collect { |ia| func.args[ia].renamed_type(alist[ia]).access_qt_arg(decl_obj) }

        ofile.puts("")
        ofile.puts("//  Constructor #{rsig}")
        ofile.puts("")

        ofile.puts("")
        ofile.puts("static void _init_ctor_#{clsn}_#{hk} (qt_gsi::GenericStaticMethod *decl)")
        ofile.puts("{")
        produce_arg_init(ofile, decl_obj, func)
        ofile.puts("  decl->set_return_new<#{cls}> ();")
        ofile.puts("}")
        ofile.puts("")
        ofile.puts("static void _call_ctor_#{clsn}_#{hk} (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) ")
        ofile.puts("{")
        ofile.puts("  __SUPPRESS_UNUSED_WARNING(args);")
        produce_arg_read(ofile, decl_obj, func, alist, conf.kept_args(bd))
        if conf.owner_args(bd).size > 0
          ofile.puts("  #{cls} *obj = new #{cls} (#{qt_alist.join(', ')});")
          conf.owner_args(bd).each do |a|
            ofile.puts("  if (#{alist[a]}) {");
            ofile.puts("    qt_gsi::qt_keep (obj);")
            ofile.puts("  }");
          end
          ofile.puts("  ret.write<#{cls} *> (obj);")
        else
          ofile.puts("  ret.write<#{cls} *> (new #{cls} (#{qt_alist.join(', ')}));")
        end
        ofile.puts("}")
        ofile.puts("")

        mdecl_ctors << "new qt_gsi::GenericStaticMethod (\"#{mn_name}\", \"@brief Constructor #{rsig}\\nThis method creates an object of class #{cls}.\", &_init_ctor_#{clsn}_#{hk}, &_call_ctor_#{clsn}_#{hk});"

      end

    end

    # produce public, non-static methods

    methods_by_name.keys.sort.each do |mid|

      methods_by_name[mid].each do |bd|

        if bd.visibility != :public || bd.storage_class == :static
          next
        end

        mn = conf.mid2str(mid)
        mn_name = conf.target_name(cls, bd, mid, all_methods_by_name, bd)
        if ! mn_name
          # method dropped
          next
        end

        func = bd.type.func
        const = bd.is_const?
        hk = bd.hash_str
        sig = bd.sig(cls)
        rsig = bd.raw_sig(cls)

        if conf.event_args(cls, sig) && bd.type.return_type.is_void?
          # don't produce bindings for signals (which are public in Qt5)
          next
        end

        is_reimp = nil
        if bc_methods_by_name[mid]
          call_sig = bd.call_sig
          bc_methods_by_name[mid].each do |bd_base|
            if bd_base.call_sig == call_sig
              bd_base.virtual && (is_reimp = bd_base)
              break
            end
          end
        end
          
        rt = bd.type.return_type

        n_args = func.max_args
        n_min_args = func.min_args

        ant      = n_args.times.collect { |ia| func.args[ia].anonymous_type }
        alist    = n_args.times.collect { |ia| "arg#{ia + 1}" }
        qt_alist = n_args.times.collect { |ia| func.args[ia].renamed_type(alist[ia]).access_qt_arg(decl_obj) }

        ofile.puts("")
        ofile.puts("// #{rsig}")
        ofile.puts("")

        ofile.puts("")
        ofile.puts("static void _init_f_#{mn}_#{hk} (qt_gsi::GenericMethod *decl)")
        ofile.puts("{")
        produce_arg_init(ofile, decl_obj, func)
        rpf = (conf.returns_new(bd) ? "_new" : "")
        ofile.puts("  decl->set_return#{rpf}<#{rt.gsi_decl_return(decl_obj)} > ();")
        ofile.puts("}")
        ofile.puts("")
        ofile.puts("static void _call_f_#{mn}_#{hk} (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) ")
        ofile.puts("{")
        ofile.puts("  __SUPPRESS_UNUSED_WARNING(args);")
        produce_arg_read(ofile, decl_obj, func, alist, conf.kept_args(bd))
        produce_keep_self(ofile, alist, "(#{cls} *)cls", conf.owner_args(bd))
        if !rt.is_void?
          ofile.puts("  ret.write<#{rt.gsi_decl_return(decl_obj)} > ((#{rt.gsi_decl_return(decl_obj)})" + rt.access_gsi_return(decl_obj, "((#{cls} *)cls)->#{mid} (#{qt_alist.join(', ')})") + ");")
        else
          ofile.puts("  __SUPPRESS_UNUSED_WARNING(ret);")
          ofile.puts("  ((#{cls} *)cls)->#{mid} (#{qt_alist.join(', ')});")
        end
        ofile.puts("}")
        ofile.puts("")

        mdecl << "new qt_gsi::GenericMethod (\"#{mn_name}\", \"@brief Method #{rsig}\\n" + (is_reimp ? "This is a reimplementation of #{is_reimp.parent.myself}::#{mid}" : "") + "\", #{const.to_s}, &_init_f_#{mn}_#{hk}, &_call_f_#{mn}_#{hk});"

      end

    end

    # handle events

    if is_qobject

      all_methods_by_name.keys.sort.each do |mid|

        all_methods_by_name[mid].each do |bd|

          if bd.visibility == :private || bd.storage_class == :static || !bd.type.return_type.is_void?
            next
          end

          mn = conf.mid2str(mid)
          mn_name = conf.target_name(cls, bd, mid, all_methods_by_name, bd)
          if ! mn_name
            # method dropped
            next
          end

          if conf.event_args(cls, bd.sig(cls))

            # strip QPrivateSignal argument if present
            bd_short = bd.dup
            func = bd_short.type.func
            if func.args.size > 0 && func.args[-1].anonymous_type.to_s =~ /QPrivateSignal/
              func.args.pop
            end
              
            sig = bd_short.sig(cls)
            rsig = bd_short.raw_sig(cls)

            hk = bd_short.hash_str
            n_args = func.max_args
            argnames = n_args.times.collect { |ia| (func.args[ia].name || "arg#{ia + 1}") }
            ant      = n_args.times.collect { |ia| func.args[ia].anonymous_type }
            ren_args = n_args.times.collect { |ia| func.args[ia].renamed_type(argnames[ia]) }
            gsi_args = ant.collect { |a| a.gsi_decl_arg(decl_obj) }
            callargs = ren_args.collect { |a| a.access_gsi_arg(decl_obj) }

            event_al = gsi_args.join(", ")
            al       = ant.collect { |a| a.to_s }.join(", ")
            aln      = ren_args.collect { |a| a.to_s }.join(", ")

            rsig_wo_void = rsig.sub(/^void /, "")

            al_subst = al
            SignalSubstitutions.each do |t,s|
              al_subst = al_subst.gsub(t, s)
            end

            argspecs = argnames.collect { |a| "gsi::arg(\"#{a}\"), " }.join("")
            if gsi_args.empty?
              mdecl << "gsi::qt_signal (\"#{mid}(#{al_subst})\", \"#{mn_name}\", \"@brief Signal declaration for #{rsig_wo_void}\\nYou can bind a procedure to this signal.\");"
            else
              mdecl << "gsi::qt_signal<#{event_al} > (\"#{mid}(#{al_subst})\", \"#{mn_name}\", #{argspecs}\"@brief Signal declaration for #{rsig_wo_void}\\nYou can bind a procedure to this signal.\");"
            end

          end

        end

      end

    end

    # produce public, static methods

    methods_by_name.keys.sort.each do |mid|

      methods_by_name[mid].each do |bd|

        if bd.visibility != :public || bd.storage_class != :static
          next
        end

        func = bd.type.func
        const = bd.is_const?
        hk = bd.hash_str
        sig = bd.sig(cls)
        rsig = bd.raw_sig(cls)

        mn = conf.mid2str(mid)
        mn_name = conf.target_name(cls, bd, mid, all_methods_by_name, bd)
        if ! mn_name
          # method dropped
          next
        end

        rt = bd.type.return_type

        n_args = func.max_args
        n_min_args = func.min_args

        ant      = n_args.times.collect { |ia| func.args[ia].anonymous_type }
        alist    = n_args.times.collect { |ia| "arg#{ia + 1}" }
        qt_alist = n_args.times.collect { |ia| func.args[ia].renamed_type(alist[ia]).access_qt_arg(decl_obj) }

        ofile.puts("")
        ofile.puts("// static #{rsig}")
        ofile.puts("")

        ofile.puts("")
        ofile.puts("static void _init_f_#{mn}_#{hk} (qt_gsi::GenericStaticMethod *decl)")
        ofile.puts("{")
        produce_arg_init(ofile, decl_obj, func)
        rpf = (conf.returns_new(bd) ? "_new" : "")
        ofile.puts("  decl->set_return#{rpf}<#{rt.gsi_decl_return(decl_obj)} > ();")
        ofile.puts("}")
        ofile.puts("")
        ofile.puts("static void _call_f_#{mn}_#{hk} (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) ")
        ofile.puts("{")
        ofile.puts("  __SUPPRESS_UNUSED_WARNING(args);")
        produce_arg_read(ofile, decl_obj, func, alist, conf.kept_args(bd))
        if !rt.is_void?
          ofile.puts("  ret.write<#{rt.gsi_decl_return(decl_obj)} > ((#{rt.gsi_decl_return(decl_obj)})" + rt.access_gsi_return(decl_obj, "#{cls}::#{mid} (#{qt_alist.join(', ')})") + ");")
        else
          ofile.puts("  __SUPPRESS_UNUSED_WARNING(ret);")
          ofile.puts("  #{cls}::#{mid} (#{qt_alist.join(', ')});")
        end
        ofile.puts("}")
        ofile.puts("")

        mdecl << "new qt_gsi::GenericStaticMethod (\"#{mn_name}\", \"@brief Static method #{rsig}\\nThis method is static and can be called without an instance.\", &_init_f_#{mn}_#{hk}, &_call_f_#{mn}_#{hk});"

      end

    end

    # produce global operators

    seen_sig = {}
  
    global_operators.each do |bd|
      
      func = bd.type.func
      mid = bd.type.name
      const = bd.is_const?
      hk = bd.hash_str
      sig = bd.sig("")
      rsig = bd.raw_sig("")

      # operators may be present twice with the same signature
      # (here: same hash key)
      hash_sig = mid + "-" + hk
      seen_sig[hash_sig] && next
      seen_sig[hash_sig] = true

      mn = conf.mid2str(mid)
      mn_name = conf.target_name("", bd, mid)
      if ! mn_name
        # operator dropped
        next
      end

      rt = bd.type.return_type

      # modify first argument (reference, value -> pointer)
      func = func.dup
      it = func.args[0].inner
      if it.is_a?(CPPReference) 
        func.args[0].inner = CPPPointer::new(it.inner)
      else
        func.args[0].inner = CPPPointer::new(it)
      end

      n_args = func.max_args
      argnames = n_args.times.collect { |ia| (func.args[ia].name || "arg#{ia + 1}") }
      argnames[0] = "_self"
      qt_alist = n_args.times.collect { |ia| func.args[ia].renamed_type(argnames[ia]).access_qt_arg(decl_obj) }
      rnt      = n_args.times.collect { |ia| func.args[ia].renamed_type(argnames[ia]) }
      args     = rnt.collect { |t| t.gsi_decl_arg(decl_obj) }.join(", ")

      ofile.puts("")
      ofile.puts("//  #{rsig}")
      ofile.puts("static #{rt.gsi_decl_return(decl_obj)} op_#{clsn}_#{mn}_#{hk}(#{args}) {")

      if !rt.is_void?
        ofile.puts("  return " + rt.access_gsi_return(decl_obj, "#{mid}(*#{qt_alist.join(', ')});"))
      else
        ofile.puts("  #{mid}(*#{qt_alist.join(', ')});")
      end

      ofile.puts("}")

      argspecs = argnames[1..-1].collect { |a| "gsi::arg (\"#{a}\"), " }.join("")

      mdecl << "gsi::method_ext(\"#{mn_name}\", &::op_#{clsn}_#{mn}_#{hk}, #{argspecs}\"@brief Operator #{rsig}\\nThis is the mapping of the global operator to the instance method.\");"

    end

    mdecl_bcc = []

    if base_classes.size > 1

      ofile.puts("")

      base_classes.each do |bc|

        bc_name = bc.class_id.to_s

        ofile.puts("//  base class cast for #{bc_name}")
        ofile.puts("")
        ofile.puts("static void _init_f_#{clsn}_as_#{bc_name} (qt_gsi::GenericMethod *decl)")
        ofile.puts("{")
        ofile.puts("  decl->set_return<#{bc_name} *> ();")
        ofile.puts("}")
        ofile.puts("")
        ofile.puts("static void _call_f_#{clsn}_as_#{bc_name} (const qt_gsi::GenericMethod *, void *cls, gsi::SerialArgs &, gsi::SerialArgs &ret) ")
        ofile.puts("{")
        ofile.puts("  ret.write<#{bc_name} *> ((#{bc_name} *)(#{cls} *)cls);")
        ofile.puts("}")
        ofile.puts("")

        mdecl_bcc << "new qt_gsi::GenericMethod (\"as#{bc_name}\", \"@brief Delivers the base class interface #{bc_name} of #{cls}\\nClass #{cls} is derived from multiple base classes. This method delivers the #{bc_name} base class aspect.\", false, &_init_f_#{clsn}_as_#{bc_name}, &_call_f_#{clsn}_as_#{bc_name});"

        ofile.puts("static void _init_f_#{clsn}_as_const_#{bc_name} (qt_gsi::GenericMethod *decl)")
        ofile.puts("{")
        ofile.puts("  decl->set_return<const #{bc_name} *> ();")
        ofile.puts("}")
        ofile.puts("")
        ofile.puts("static void _call_f_#{clsn}_as_const_#{bc_name} (const qt_gsi::GenericMethod *, void *cls, gsi::SerialArgs &, gsi::SerialArgs &ret) ")
        ofile.puts("{")
        ofile.puts("  ret.write<const #{bc_name} *> ((const #{bc_name} *)(const #{cls} *)cls);")
        ofile.puts("}")
        ofile.puts("")

        mdecl_bcc << "new qt_gsi::GenericMethod (\"asConst#{bc_name}\", \"@brief Delivers the base class interface #{bc_name} of #{cls}\\nClass #{cls} is derived from multiple base classes. This method delivers the #{bc_name} base class aspect.\\n\\nUse this version if you have a const reference.\", true, &_init_f_#{clsn}_as_const_#{bc_name}, &_call_f_#{clsn}_as_const_#{bc_name});"

      end

    end

    mdecl = mdecl_ctors + mdecl + mdecl_bcc

    if ! needs_adaptor
      ofile.puts("")
    end

    ofile.puts("")
    ofile.puts("namespace gsi")
    ofile.puts("{")

    ofile.puts("")
    ofile.puts("static gsi::Methods methods_#{clsn} () {")
    ofile.puts("  gsi::Methods methods;")
    mdecl.each do |s|
      ofile.puts("  methods += #{s}")
    end
    ofile.puts("  return methods;")
    ofile.puts("}")
    ofile.puts("")

    if is_qobject
      decl_type = "qt_gsi::QtNativeClass<#{cls}>"
    else
      decl_type = "gsi::Class<#{cls}>"
    end

    if base_cls
      ofile.puts("gsi::Class<#{base_cls}> &qtdecl_#{base_clsn} ();")
      ofile.puts("")
      ofile.puts("#{decl_type} decl_#{clsn} (" + "qtdecl_#{base_clsn} (), \"#{modn}\", \"#{clsn}" + (needs_adaptor ? "_Native" : "") + "\",")
    else
      ofile.puts("#{decl_type} decl_#{clsn} (\"#{modn}\", \"#{clsn}" + (needs_adaptor ? "_Native" : "") + "\",")
    end
    if native_impl
      native_impl.each { |n| n[1] && ofile.puts(n[1] + "+") }
    end
    ofile.puts("  methods_#{clsn} (),")

    is_child_class = (decl_obj.parent && decl_obj.parent.myself)

    if needs_adaptor
      ofile.puts("  \"@hide\\n@alias #{clsn}\");")
    else

      ofile.puts("  \"@qt\\n@brief Binding of #{cls}\");")
      ofile.puts("")

      if is_child_class

        # Produce the child class declaration if applicable
        pdecl_obj = decl_obj.parent

        pcls = pdecl_obj.myself
        o = pdecl_obj
        while o.parent && o.parent.myself
          o = o.parent
          pcls = o.myself + "::" + pcls
        end

        ofile.puts("gsi::ClassExt<#{pcls}> decl_#{clsn}_as_child (decl_#{clsn}, \"#{cclsn}\");")

      end

    end

    if !is_child_class

      # forward decl
      @ext_decls << "#{struct.kind.to_s} #{cls};\n\n"

      # only for top-level classes external declarations are produced currently
      @ext_decls << "namespace gsi { GSI_#{modn.upcase}_PUBLIC gsi::Class<#{cls}> &qtdecl_#{clsn} (); }\n\n"

    end

    # Produce the mixin base classes

    if ! mixin_base_classes.empty?
      ofile.puts("")
      ofile.puts("//  Additional base classes")
      ofile.puts("")
      mixin_base_classes.each do |bc|
        bc_name = bc.class_id.to_s
        ofile.puts("gsi::Class<#{bc_name}> &qtdecl_#{bc_name} ();")
      end
      ofile.puts("")
    end

    mixin_base_classes.each do |bc|
      bc_name = bc.class_id.to_s
      ofile.puts("gsi::ClassExt<#{cls}> base_class_#{bc_name}_in_#{clsn} (qtdecl_#{bc_name} ());")
    end

    ofile.puts("")
    ofile.puts("GSI_#{modn.upcase}_PUBLIC gsi::Class<#{cls}> &qtdecl_#{clsn} () { return decl_#{clsn}; }")
    ofile.puts("")

    ofile.puts("}")
    ofile.puts("")

    if needs_adaptor

      # need to produce an adaptor

      native_impl = conf.native_impl(clsn + "_Adaptor")

      ofile.puts("")
      ofile.puts("class #{clsn}_Adaptor : public #{cls}, public qt_gsi::QtObjectBase")
      ofile.puts("{")
      ofile.puts("public:")
      native_impl && native_impl.each { |n| n[0] && ofile.puts(n[0]) }
      ofile.puts("")
      ofile.puts("  virtual ~#{clsn}_Adaptor();")

      # expose ctors here
      ctors.each do |bd|

        bd.visibility == :public || next

        func = bd.type.func
        hk = bd.hash_str
        sig = bd.sig(cls)
        rsig = bd.raw_sig(cls)
        mn = decl_obj.myself # ctor!

        mn_name = conf.target_name(cls, bd, mn)
        if ! mn_name
          # method dropped
          next
        elsif mn_name == mn
          mn_name = "new"
        end

        rt = bd.type.return_type

        i_var = 0

        #  TODO: provide initializer instead of multiple implementations?
        (func.min_args .. func.max_args).each do |n_args|

          argnames = n_args.times.collect { |ia| (func.args[ia].name || "arg#{ia + 1}") }
          rnt      = n_args.times.collect { |ia| func.args[ia].renamed_type(argnames[ia]) }

          args = rnt.collect { |t| t.to_s }.join(", ")

          ofile.puts("")
          ofile.puts("  //  [adaptor ctor] #{rsig}")
          ofile.puts("  #{clsn}_Adaptor(#{args}) : #{cls}(#{argnames.join(', ')})")
          ofile.puts("  {")
          ofile.puts("    qt_gsi::QtObjectBase::init (this);")
          ofile.puts("  }")

          i_var += 1

        end

      end

      # expose all protected, non-virtual methods so we can call them from an implementation
      all_methods_by_name.keys.sort.each do |mid|

        all_methods_by_name[mid].each do |bd|

          bd.virtual && next

          bd.visibility == :protected || next

          func = bd.type.func
          hk = bd.hash_str
          sig = bd.sig(cls)
          rsig = bd.raw_sig(cls)
          const = bd.is_const?

          # exclude events
          conf.event_args(cls, sig) && next

          mn = conf.mid2str(mid)
          mn_name = conf.target_name(cls, bd, mid)
          if ! mn_name
            # method dropped
            next
          end

          rt = bd.type.return_type

          n_args = func.max_args

          argnames = n_args.times.collect { |ia| (func.args[ia].name || "arg#{ia + 1}") }
          rnt      = n_args.times.collect { |ia| func.args[ia].renamed_type(argnames[ia]) }

          args     = rnt.collect { |t| t.gsi_decl_arg(decl_obj) }.join(", ")
          argexpr  = rnt.collect { |t| t.access_qt_arg(decl_obj) }.join(", ")

          ofile.puts("")
          ofile.puts("  //  [expose] #{rsig}")
          ofile.puts("  " + (bd.storage_class == :static ? "static " : "") + "#{rt.gsi_decl_return(decl_obj)} fp_#{clsn}_#{mn}_#{hk} (#{args}) " + (const ? "const " : "") + "{")
          if rt.is_void?
            ofile.puts("    #{cls}::#{mid}(#{argexpr});")
          else
            ofile.puts("    return " + rt.access_gsi_return(decl_obj, "#{cls}::#{mid}(#{argexpr})") + ";")
          end
          ofile.puts("  }")

        end

      end

      callbacks = []

      # expose implementation hooks for a virtual methods
      # first public, then protected for backward compatibility
      [ :public, :protected ].each do |vis|

        all_methods_by_name.keys.sort.each do |mid|

          all_methods_by_name[mid].each do |bd|

            bd.visibility == vis || next

            func = bd.type.func
            hk = bd.hash_str
            sig = bd.sig(cls)
            rsig = bd.raw_sig(cls)
            const = bd.is_const?
            rt = bd.type.return_type

            mn = conf.mid2str(mid)
            mn_name = conf.target_name(cls, bd, mid)
            if ! mn_name
              # method dropped
              next
            end

            if is_qobject && conf.event_args(cls, sig) && bd.storage_class != :static && rt.is_void?

              # if the last argument is a QPrivateSignal we cannot emit

              is_private = false
              bd_short = bd.dup
              func = bd_short.type.func
              if func.args.size > 0 && func.args[-1].anonymous_type.to_s =~ /QPrivateSignal/
                func.args.pop
                is_private = true
              end

              sig = bd_short.sig(cls)
              rsig = bd_short.raw_sig(cls)

              # for events produce an emitter function

              i_var = func.max_args - func.min_args   # for backward compatibility
              n_args = func.max_args

              argnames = n_args.times.collect { |ia| (func.args[ia].name || "arg#{ia + 1}") }.collect { |a| a == mn ? "_" + a : a }
              rnt      = n_args.times.collect { |ia| func.args[ia].renamed_type(argnames[ia]) }

              raw_args = rnt.collect { |t| t.to_s }.join(", ")
              call_args = argnames.join(", ")

              ofile.puts("")
              ofile.puts("  //  [emitter impl] #{rsig}")
              ofile.puts("  #{rt.to_s} emitter_#{clsn}_#{mn}_#{hk}(#{raw_args})")
              ofile.puts("  {")
              if is_private
                argnames.each do |a|
                  ofile.puts("    __SUPPRESS_UNUSED_WARNING (#{a});")
                end
                ofile.puts("    throw tl::Exception (\"Can't emit private signal '#{sig}'\");")
              else
                ofile.puts("    emit #{cls}::#{mid}(#{call_args});")
              end
              ofile.puts("  }")

            elsif bd.virtual

              # for virtual functions produce a callback and a virtual reimplementation

              abstract = (bd.virtual && bd.type.init == "0")

              i_var = func.max_args - func.min_args   # for backward compatibility
              n_args = func.max_args

              argnames = n_args.times.collect { |ia| (func.args[ia].name || "arg#{ia + 1}") }.collect { |a| a == mn ? "_" + a : a }
              rnt      = n_args.times.collect { |ia| func.args[ia].renamed_type(argnames[ia]) }

              args     = rnt.collect { |t| t.gsi_decl_arg(decl_obj) }.join(", ")
              argexpr  = rnt.collect { |t| t.access_qt_arg(decl_obj) }.join(", ")
              argexprr = rnt.collect { |t| ", " + t.access_gsi_arg(decl_obj) }.join("")

              ta = [ "#{clsn}_Adaptor" ]
              if !rt.is_void?
                ta << rt.gsi_decl_return(decl_obj)
              end
              ta += rnt.collect { |t| t.anonymous_type.gsi_decl_arg(decl_obj) }
              ta_str = ta.join(", ")
              if ta_str =~ />$/
                ta_str += " "
              end

              raw_args = rnt.collect { |t| t.to_s }.join(", ")
              call_args = argnames.join(", ")

              ofile.puts("")
              ofile.puts("  //  [adaptor impl] #{rsig}")
              ofile.puts("  #{rt.gsi_decl_return(decl_obj)} cbs_#{mn}_#{hk}_#{i_var}(#{args})" + (const ? " const" : ""))
              ofile.puts("  {")
              if abstract
                argnames.each do |a|
                  ofile.puts("    __SUPPRESS_UNUSED_WARNING (#{a});")
                end
                ofile.puts("    throw qt_gsi::AbstractMethodCalledException(\"#{mn}\");")
              elsif rt.is_void?
                ofile.puts("    #{cls}::#{mid}(#{argexpr});")
              else
                ofile.puts("    return " + rt.access_gsi_return(decl_obj, "#{cls}::#{mid}(#{argexpr})") + ";")
              end
              ofile.puts("  }")
              ofile.puts("")
              ofile.puts("  virtual #{rt.to_s} #{mid}(#{raw_args})" + (const ? " const" : ""))
              ofile.puts("  {")
              if rt.is_void?
                ofile.puts("    if (cb_#{mn}_#{hk}_#{i_var}.can_issue()) {");
                ofile.puts("      cb_#{mn}_#{hk}_#{i_var}.issue<#{ta_str}>(&#{clsn}_Adaptor::cbs_#{mn}_#{hk}_#{i_var}#{argexprr});");
                ofile.puts("    } else {");
                if abstract
                  ofile.puts("      throw qt_gsi::AbstractMethodCalledException(\"#{mn}\");")
                else
                  ofile.puts("      #{cls}::#{mid}(#{call_args});");
                end
                ofile.puts("    }");
              else
                ofile.puts("    if (cb_#{mn}_#{hk}_#{i_var}.can_issue()) {");
                ofile.puts("      return " + rt.access_qt_return(decl_obj, "cb_#{mn}_#{hk}_#{i_var}.issue<#{ta_str}>(&#{clsn}_Adaptor::cbs_#{mn}_#{hk}_#{i_var}#{argexprr})") + ";");
                ofile.puts("    } else {");
                if abstract
                  ofile.puts("      throw qt_gsi::AbstractMethodCalledException(\"#{mn}\");")
                else
                  ofile.puts("      return #{cls}::#{mid}(#{call_args});");
                end
                ofile.puts("    }");
              end
              ofile.puts("  }")

              callbacks << "gsi::Callback cb_#{mn}_#{hk}_#{i_var};";

            end
            
          end

        end

      end

      ofile.puts("")
      ofile.puts("  " + callbacks.join("\n  ") + "\n")
      ofile.puts("};")
      ofile.puts("")
      ofile.puts("#{clsn}_Adaptor::~#{clsn}_Adaptor() { }");

      mdecl = []

      ctors.each do |bd|

        bd.visibility == :public || next

        func = bd.type.func
        hk = bd.hash_str
        sig = bd.sig(cls)
        rsig = bd.raw_sig(cls)
        mn = decl_obj.myself # ctor!

        mn_name = conf.target_name(cls, bd, mn)
        if ! mn_name
          # method dropped
          next
        elsif mn_name == mn
          mn_name = "new"
        end

        n_args = func.max_args
        n_min_args = func.min_args

        alist    = n_args.times.collect { |ia| "arg#{ia + 1}" }
        qt_alist = n_args.times.collect { |ia| func.args[ia].renamed_type(alist[ia]).access_qt_arg(decl_obj) }

        ofile.puts("")
        ofile.puts("//  Constructor #{rsig} (adaptor class)")
        ofile.puts("")
        ofile.puts("static void _init_ctor_#{clsn}_Adaptor_#{hk} (qt_gsi::GenericStaticMethod *decl)")
        ofile.puts("{")
        produce_arg_init(ofile, decl_obj, func)
        ofile.puts("  decl->set_return_new<#{clsn}_Adaptor> ();")
        ofile.puts("}")
        ofile.puts("")
        ofile.puts("static void _call_ctor_#{clsn}_Adaptor_#{hk} (const qt_gsi::GenericStaticMethod * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) ")
        ofile.puts("{")
        ofile.puts("  __SUPPRESS_UNUSED_WARNING(args);")
        produce_arg_read(ofile, decl_obj, func, alist, conf.kept_args(bd))
        if conf.owner_args(bd).size > 0
          ofile.puts("  #{clsn}_Adaptor *obj = new #{clsn}_Adaptor (#{qt_alist.join(', ')});")
          produce_keep_self(ofile, alist, "obj", conf.owner_args(bd))
          ofile.puts("  ret.write<#{clsn}_Adaptor *> (obj);")
        else
          ofile.puts("  ret.write<#{clsn}_Adaptor *> (new #{clsn}_Adaptor (#{qt_alist.join(', ')}));")
        end
        ofile.puts("}")
        ofile.puts("")

        mdecl << "new qt_gsi::GenericStaticMethod (\"#{mn_name}\", \"@brief Constructor #{rsig}\\nThis method creates an object of class #{cls}.\", &_init_ctor_#{clsn}_Adaptor_#{hk}, &_call_ctor_#{clsn}_Adaptor_#{hk});"

      end

      all_methods_by_name.keys.sort.each do |mid|

        all_methods_by_name[mid].each do |bd|

          bd.visibility == :public || bd.visibility == :protected || next

          func = bd.type.func
          hk = bd.hash_str
          sig = bd.sig(cls)
          rsig = bd.raw_sig(cls)
          const = bd.is_const?
          rt = bd.type.return_type

          mn = conf.mid2str(mid)
          mn_name = conf.target_name(cls, bd, mid)
          if ! mn_name
            # method dropped
            next
          end

          if is_qobject && conf.event_args(cls, sig) && bd.storage_class != :static && rt.is_void?

            # for events produce the emitter

            bd_short = bd.dup
            func = bd_short.type.func
            if func.args.size > 0 && func.args[-1].anonymous_type.to_s =~ /QPrivateSignal/
              func.args.pop
            end

            sig = bd_short.sig(cls)
            rsig = bd_short.raw_sig(cls)

            n_args = func.max_args
            n_min_args = func.min_args

            alist    = n_args.times.collect { |ia| "arg#{ia + 1}" }

            ifc_obj = "GenericMethod"

            ofile.puts("")
            ofile.puts("// emitter #{rsig}")
            ofile.puts("")
            ofile.puts("static void _init_emitter_#{mn}_#{hk} (qt_gsi::#{ifc_obj} *decl)")
            ofile.puts("{")
            produce_arg_init(ofile, decl_obj, func)
            rpf = (conf.returns_new(bd_short) ? "_new" : "")
            ofile.puts("  decl->set_return#{rpf}<#{rt.gsi_decl_return(decl_obj)} > ();")
            ofile.puts("}")
            ofile.puts("")
            ofile.puts("static void _call_emitter_#{mn}_#{hk} (const qt_gsi::#{ifc_obj} * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs & /*ret*/) ")
            ofile.puts("{")
            ofile.puts("  __SUPPRESS_UNUSED_WARNING(args);")
            produce_arg_read(ofile, decl_obj, func, alist, conf.kept_args(bd_short))
            ofile.puts("  ((#{clsn}_Adaptor *)cls)->emitter_#{clsn}_#{mn}_#{hk} (#{alist.join(', ')});")
            ofile.puts("}")
            ofile.puts("")

            const_flag = ""
            if bd_short.storage_class != :static
              const_flag = ", " + const.to_s
            end
            mdecl << "new qt_gsi::#{ifc_obj} (\"emit_#{mn_name}\", \"@brief Emitter for signal #{rsig}\\nCall this method to emit this signal.\"#{const_flag}, &_init_emitter_#{mn}_#{hk}, &_call_emitter_#{mn}_#{hk});"

          elsif !bd.virtual && bd.visibility == :protected

            # expose all protected, non-virtual methods and the signals so we can call them from an implementation

            n_args = func.max_args
            n_min_args = func.min_args

            alist    = n_args.times.collect { |ia| "arg#{ia + 1}" }

            ifc_obj = bd.storage_class == :static ? "GenericStaticMethod" : "GenericMethod"

            ofile.puts("")
            ofile.puts("// exposed #{rsig}")
            ofile.puts("")
            ofile.puts("static void _init_fp_#{mn}_#{hk} (qt_gsi::#{ifc_obj} *decl)")
            ofile.puts("{")
            produce_arg_init(ofile, decl_obj, func)
            rpf = (conf.returns_new(bd) ? "_new" : "")
            ofile.puts("  decl->set_return#{rpf}<#{rt.gsi_decl_return(decl_obj)} > ();")
            ofile.puts("}")
            ofile.puts("")
            if bd.storage_class == :static
              ofile.puts("static void _call_fp_#{mn}_#{hk} (const qt_gsi::#{ifc_obj} * /*decl*/, gsi::SerialArgs &args, gsi::SerialArgs &ret) ")
            else
              ofile.puts("static void _call_fp_#{mn}_#{hk} (const qt_gsi::#{ifc_obj} * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) ")
            end
            ofile.puts("{")
            ofile.puts("  __SUPPRESS_UNUSED_WARNING(args);")
            produce_arg_read(ofile, decl_obj, func, alist, conf.kept_args(bd))
            if bd.storage_class == :static
              if !rt.is_void?
                ofile.puts("  ret.write<#{rt.gsi_decl_return(decl_obj)} > ((#{rt.gsi_decl_return(decl_obj)})#{cls}_Adaptor::fp_#{clsn}_#{mn}_#{hk} (#{alist.join(', ')}));")
              else
                ofile.puts("  __SUPPRESS_UNUSED_WARNING(ret);")
                ofile.puts("  #{cls}_Adaptor::fp_#{clsn}_#{mn}_#{hk} (#{alist.join(', ')});")
              end
            else
              if !rt.is_void?
                ofile.puts("  ret.write<#{rt.gsi_decl_return(decl_obj)} > ((#{rt.gsi_decl_return(decl_obj)})((#{clsn}_Adaptor *)cls)->fp_#{clsn}_#{mn}_#{hk} (#{alist.join(', ')}));")
              else
                ofile.puts("  __SUPPRESS_UNUSED_WARNING(ret);")
                ofile.puts("  ((#{clsn}_Adaptor *)cls)->fp_#{clsn}_#{mn}_#{hk} (#{alist.join(', ')});")
              end
            end
            ofile.puts("}")
            ofile.puts("")

            const_flag = ""
            if bd.storage_class != :static
              const_flag = ", " + const.to_s
            end
            mdecl << "new qt_gsi::#{ifc_obj} (\"*#{mn_name}\", \"@brief Method #{rsig}\\nThis method is protected and can only be called from inside a derived class.\"#{const_flag}, &_init_fp_#{mn}_#{hk}, &_call_fp_#{mn}_#{hk});"

          elsif bd.virtual

            # produce call wrappers for the virtual methods 

            pp = (bd.visibility == :protected ? "*" : "")

            abstract = bd.type.init == "0"

            i_var = func.max_args - func.min_args  # backward compatibility
            n_args = func.max_args

            ant      = n_args.times.collect { |ia| func.args[ia].anonymous_type }
            alist    = n_args.times.collect { |ia| "arg#{ia + 1}" }

            ofile.puts("")
            ofile.puts("// #{rsig}")
            ofile.puts("")
            ofile.puts("static void _init_cbs_#{mn}_#{hk}_#{i_var} (qt_gsi::GenericMethod *decl)")
            ofile.puts("{")
            ant.each_with_index do |at,ia|
              ta = at.gsi_decl_arg(decl_obj);
              argname = func.args[ia].name || "arg#{ia + 1}"
              ofile.puts("  static gsi::ArgSpecBase argspec_#{ia} (\"#{argname}\");")
              ofile.puts("  decl->add_arg<#{ta} > (argspec_#{ia});")
            end
            ofile.puts("  decl->set_return<#{rt.gsi_decl_return(decl_obj)} > ();")
            ofile.puts("}")
            ofile.puts("")
            ofile.puts("static void _call_cbs_#{mn}_#{hk}_#{i_var} (const qt_gsi::GenericMethod * /*decl*/, void *cls, gsi::SerialArgs &args, gsi::SerialArgs &ret) ")
            ofile.puts("{")
            ofile.puts("  __SUPPRESS_UNUSED_WARNING(args);")
            if !ant.empty?
              ofile.puts("  tl::Heap heap;")
            end
            ant.each_with_index do |at,ia|
              ofile.puts("  #{at.renamed_type(alist[ia]).gsi_decl_arg(decl_obj)} = args.read<#{at.gsi_decl_arg(decl_obj)} > (heap);")
            end
            if !rt.is_void?
              ofile.puts("  ret.write<#{rt.gsi_decl_return(decl_obj)} > ((#{rt.gsi_decl_return(decl_obj)})((#{clsn}_Adaptor *)cls)->cbs_#{mn}_#{hk}_#{i_var} (#{alist.join(', ')}));")
            else
              ofile.puts("  __SUPPRESS_UNUSED_WARNING(ret);")
              ofile.puts("  ((#{clsn}_Adaptor *)cls)->cbs_#{mn}_#{hk}_#{i_var} (#{alist.join(', ')});")
            end
            ofile.puts("}")
            ofile.puts("")
            ofile.puts("static void _set_callback_cbs_#{mn}_#{hk}_#{i_var} (void *cls, const gsi::Callback &cb)")
            ofile.puts("{")
            ofile.puts("  ((#{clsn}_Adaptor *)cls)->cb_#{mn}_#{hk}_#{i_var} = cb;")
            ofile.puts("}")
            ofile.puts("")

            mdecl << "new qt_gsi::GenericMethod (\"#{pp}#{mn_name}\", \"@brief Virtual method #{rsig}\\nThis method can be reimplemented in a derived class.\", #{const.to_s}, &_init_cbs_#{mn}_#{hk}_#{i_var}, &_call_cbs_#{mn}_#{hk}_#{i_var});"
            mdecl << "new qt_gsi::GenericMethod (\"#{pp}#{mn_name}\", \"@hide\", #{const.to_s}, &_init_cbs_#{mn}_#{hk}_#{i_var}, &_call_cbs_#{mn}_#{hk}_#{i_var}, &_set_callback_cbs_#{mn}_#{hk}_#{i_var});"

          end

        end

      end

      # produce the main declaration

      ofile.puts("")
      ofile.puts("namespace gsi")
      ofile.puts("{")
      ofile.puts("")
      ofile.puts("gsi::Class<#{cls}> &qtdecl_#{clsn} ();")
      ofile.puts("")
      ofile.puts("static gsi::Methods methods_#{clsn}_Adaptor () {")
      ofile.puts("  gsi::Methods methods;")
      mdecl.each do |s|
        ofile.puts("  methods += #{s}")
      end
      ofile.puts("  return methods;")
      ofile.puts("}")
      ofile.puts("")
      ofile.puts("gsi::Class<#{clsn}_Adaptor> decl_#{clsn}_Adaptor (qtdecl_#{clsn} (), \"#{modn}\", \"#{clsn}\",")
      if native_impl
        native_impl.each { |n| n[1] && ofile.puts(n[1] + "+") }
      end
      ofile.puts("  methods_#{clsn}_Adaptor (),")
      ofile.puts("  \"@qt\\n@brief Binding of #{cls}\");")
      ofile.puts("")

      if decl_obj.parent && decl_obj.parent.myself

        # Produce the child class declaration if applicable
        pdecl_obj = decl_obj.parent

        pcls = pdecl_obj.myself
        o = pdecl_obj
        while o.parent && o.parent.myself
          o = o.parent
          pcls = o.myself + "::" + pcls
        end

        ofile.puts("gsi::ClassExt<#{pcls}> decl_#{clsn}_as_child (decl_#{clsn}_Adaptor, \"#{cclsn}\");")

      end

      ofile.puts("}")
      ofile.puts("")

    end

    # emit enum wrapper classes
  
    enum_decls_by_name.keys.sort.each do |en|
      ed = enum_decls_by_name[en]
      produce_enum_wrapper_class(ofile, conf, cls, en, ed) 
    end

    # don't continue
    return false

  end

  def produce_externals

    externals_name = "gsiQtExternals.h"
    externals_path = $gen_dir + "/" + externals_name

    File.open(externals_path, "w") do |extfile|

      extfile.puts(<<"END");

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

END

      extfile.puts("/*")
      extfile.puts("  External declarations for for Qt bindings")
      extfile.puts("")
      extfile.puts("  DO NOT EDIT THIS FILE. ")
      extfile.puts("  This file has been created automatically")
      extfile.puts("*/")
      extfile.puts("")

      extfile.puts("#if !defined(HDR_gsi#{modn}Externals)")
      extfile.puts("#define HDR_gsi#{modn}Externals")
      extfile.puts("")

      extfile.puts("#include \"gsiClass.h\"")
      extfile.puts("#include \"gsi#{modn}Common.h\"")
      extfile.puts("")

      @ext_decls.each do |ed|
        extfile.puts(ed)
      end

      extfile.puts("")
      extfile.puts("#define QT_EXTERNAL_BASE(X) gsi::qtdecl_##X(),")
      extfile.puts("")
      extfile.puts("#endif")
      extfile.puts("")

    end

  end

  def produce_common_header

    src_name = "gsi#{modn}Common.h"
    src_path = $gen_dir + "/" + src_name

    File.open(src_path, "w") do |src|

      src.puts("/**")
      src.puts(" *  Common header for Qt binding definition library")
      src.puts(" *")
      src.puts(" *  DO NOT EDIT THIS FILE. ")
      src.puts(" *  This file has been created automatically")
      src.puts(" */")

      src.puts("")
      src.puts("#include \"tlDefs.h\"")
      src.puts("")
      src.puts("#if !defined(HDR_gsi#{modn}Common_h)")
      src.puts("# define HDR_gsi#{modn}Common_h")
      src.puts("")
      src.puts("# ifdef MAKE_GSI_#{modn.upcase}_LIBRARY")
      src.puts("#   define GSI_#{modn.upcase}_PUBLIC           DEF_INSIDE_PUBLIC")
      src.puts("#   define GSI_#{modn.upcase}_PUBLIC_TEMPLATE  DEF_INSIDE_PUBLIC_TEMPLATE")
      src.puts("#   define GSI_#{modn.upcase}_LOCAL            DEF_INSIDE_LOCAL")
      src.puts("# else")
      src.puts("#   define GSI_#{modn.upcase}_PUBLIC           DEF_OUTSIDE_PUBLIC")
      src.puts("#   define GSI_#{modn.upcase}_PUBLIC_TEMPLATE  DEF_OUTSIDE_PUBLIC_TEMPLATE")
      src.puts("#   define GSI_#{modn.upcase}_LOCAL            DEF_OUTSIDE_LOCAL")
      src.puts("# endif")
      src.puts("")
      src.puts("#define FORCE_LINK_GSI_#{modn.upcase} GSI_#{modn.upcase}_PUBLIC int _force_link_gsi#{modn}_f (); int _force_link_gsi#{modn} = _force_link_gsi#{modn}_f ();")
      src.puts("")
      src.puts("#endif")

      puts("#{src_name} written.")

    end

  end

  def produce_main_source

    src_name = "gsi#{modn}Main.cc"
    src_path = $gen_dir + "/" + src_name

    File.open(src_path, "w") do |src|

      src.puts("/**")
      src.puts(" *  Main source file for Qt binding definition library")
      src.puts(" *")
      src.puts(" *  DO NOT EDIT THIS FILE. ")
      src.puts(" *  This file has been created automatically")
      src.puts(" */")

      src.puts("")
      src.puts("#include \"gsi#{modn}Common.h\"")
      src.puts("")
      src.puts("GSI_#{modn.upcase}_PUBLIC int _force_link_gsi#{modn}_f () { return 0; }")
      src.puts("")

      puts("#{src_name} written.")

    end

  end

  def produce_pri

    makefile_name = modn + ".pri"
    makefile_path = $gen_dir + "/" + makefile_name

    File.open(makefile_path, "w") do |makefile|

      makefile.puts("#")
      makefile.puts("#  Partial QMAKE project file for Qt bindings")
      makefile.puts("#")
      makefile.puts("#  DO NOT EDIT THIS FILE. ")
      makefile.puts("#  This file has been created automatically")
      makefile.puts("#")

      makefile.puts("")
      makefile.puts("SOURCES += \\")
      makefile.puts("  gsi#{modn}Main.cc \\")
      if @source_files
        makefile.puts(@source_files.collect { |s| "  $$PWD/" + s }.join(" \\\n"))
      end

      makefile.puts("")
      makefile.puts("HEADERS += gsi#{modn}Common.h")
      makefile.puts("")

      puts("#{makefile_name} written.")

    end

  end

  def produce_class_list

    File.open("class_list.txt", "w") do |list|
      (@classes || []).each do |c|
        list.puts(c)
      end
    end

  end

end

# ---------------------------------------------------------------------

conf = Configurator::new

File.open(conf_file, "r") do |file|
  conf.instance_eval(file.read, conf_file)
end

bp = BindingProducer::new
bp.modn = modn
bp.read(input_file)

puts("Collecting used enums ..")
l = bp.prod_list(conf)
l.each_with_index do |decl_obj,i|
  decl_obj.myself && puts("#{decl_obj.myself}: #{i+1}/#{l.size}")
  bp.collect_used_enums(conf, decl_obj)
end

puts("Producing ..")

if cls_list
  cls_list.split(",").each do |cs|
    bp.produce_cpp(conf, cs)
  end
else
  bp.prod_list(conf).each do |decl_obj|
    if decl_obj.myself && !excl_list[decl_obj.myself] 
      bp.produce_cpp_from_decl(conf, decl_obj)
    end
  end
end

puts("Producing class list")
bp.produce_class_list

puts("Producing main source file ..")
bp.produce_main_source

puts("Producing common header file ..")
bp.produce_common_header

puts("Producing .pri file ..")
bp.produce_pri

puts("Producing external declarations ..")
bp.produce_externals

