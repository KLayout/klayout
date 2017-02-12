#!/usr/bin/tclsh

if { [ llength $argv ] != 2 } {
  puts "mkoasis <input> <output>"
  exit 1
}

set infile [ lindex $argv 0 ]
set outfile [ lindex $argv 1 ]

set input [ open $infile "r" ]
set txt [ read $input ]
close $input

set output [ open $outfile "w" ]


# produce a byte
proc byte { b } {
  global output
  puts -nonewline $output [ binary format {c} $b ]
}
 
# produce an unsigned int 
proc uint { ui } {
  if { $ui == 0 } {
    byte 0
  } else {
    while { $ui > 0 } {
      if { $ui < 128 } {
        byte $ui
      } else {
        byte [ expr 128+($ui%128) ]
      }
      set ui [ expr $ui>>7 ]
    }
  }
}

# produce an int
proc int { ui } {
  if { $ui == 0 } {
    byte 0
  } else {
    set ui [ expr $ui<0 ? (-2*$ui+1) : ($ui*2) ]
    while { $ui > 0 } {
      if { $ui < 128 } {
        byte $ui
      } else {
        byte [ expr 128+($ui%128) ]
      }
      set ui [ expr $ui>>7 ]
    }
  }
}

# produce a bit field
proc bits { value } {
  global output
  puts -nonewline $output [ binary format B8 $value ]
}

# produce a real
proc real { type value { value2 1.0 } } {
  global output

  uint $type
  if { $type == 0 } {
    uint [ expr int($value+0.5) ]
  } elseif { $type == 1 } {
    uint [ expr int(-$value+0.5) ]
  } elseif { $type == 2 } {
    uint [ expr int(1.0/$value+0.5) ]
  } elseif { $type == 3 } {
    uint [ expr int(-1.0/$value+0.5) ]
  } elseif { $type == 4 } {
    uint [ expr int($value+0.5) ]
    uint [ expr int($value2+0.5) ]
  } elseif { $type == 5 } {
    uint [ expr int(-$value+0.5) ]
    uint [ expr int($value2+0.5) ]
  } elseif { $type == 6 } {
    if { [ string compare [ binary format f 1.0 ] "\x00\x00\x80\x3f" ] == 0 } {
      # big endian
      puts -nonewline $output [ binary format f $value ]
    } else {
      # little endian
      set v 0
      binary scan [ binary format f $value ] I v
      puts -nonewline $output [ binary format i $v ]
    }
  } elseif { $type == 7 } {
    if { [ string compare [ binary format d 1.0 ] "\x00\x00\x00\x00\x00\x00\xf0\x3f" ] == 0 } {
      # big endian
      puts -nonewline $output [ binary format d $value ]
    } else {
      # little endian
      set v 0
      binary scan [ binary format d $value ] W v
      puts -nonewline $output [ binary format w $v ]
    }
  } else {
    error "Invalid real type $type"
  }

}

# produce a string
proc str { value } {
  global output
  uint [ string length $value ]
  puts -nonewline $output [ binary format {a*} $value ]
}
 
set code(PAD)            0
set code(START)          1
set code(END)            2
set code(CELLNAME)       3
set code(CELLNAME_ID)    4
set code(TEXTSTRING)     5
set code(TEXTSTRING_ID)  6
set code(PROPNAME)       7
set code(PROPNAME_ID)    8
set code(PROPSTRING)     9
set code(PROPSTRING_ID)  10
set code(LAYERNAME)      11
set code(LAYERNAME_TXT)  12
set code(CELL_ID)        13
set code(CELL_STR)       14
set code(XYABSOLUTE)     15
set code(XYRELATIVE)     16
set code(PLACEMENT)      17
set code(PLACEMENT_MAG)  18
set code(TEXT)           19
set code(RECTANGLE)      20
set code(POLYGON)        21
set code(PATH)           22
set code(TRAPEZOID_AB)   23
set code(TRAPEZOID_A)    24
set code(TRAPEZOID_B)    25
set code(CTRAPEZOID)     26
set code(CIRCLE)         27
set code(PROPERTY)       28
set code(PROPERTY_REP)   29
set code(XNAME)          30
set code(XELEMENT)       31
set code(XELEMENT_ID)    32
set code(XGEOMETRY)      33
set code(CBLOCK)         34


# produce a certain record
proc record { rec } {
  global code
  byte $code($rec)
}

# produce header
proc header { } {
  global output
  puts -nonewline $output [ binary format {a*h2h2} "%SEMI-OASIS" d a ]
  record START
  str "1.0" 
}

# produce tail
proc tail {} {
  global output
  record END
  puts -nonewline $output [ binary format {x255} ]
}


# utility functions

namespace eval soa {

  proc header {dbu} {
    header 
    real 0 [ expr 1.0/$dbu ]
    uint 0 ;# offset table is in start record
    for { set i 0 } { $i < 12 } { incr i } { uint 0 }
  }

  proc cell {name} {
    record CELL_STR
    str $name
  }

  proc path {layer datatype width bgnext endext points} {

    if { [llength $points] < 1 } {
      error ".PATH with empty points"
    }
    if { $width < 0 } {
      error ".PATH with negative width"
    }

    record PATH
    bits 11111011  ;# EWPXYRDL
    uint $layer    ;# layer
    uint $datatype ;# datatype
    uint [ expr $width/2 ]  ;# half-width

    bits 00001111  ;# extension-scheme SSEE
    int $bgnext
    int $endext

    uint 4         ;# pointlist: 1-delta (hor. first)
    uint [ expr [llength $points]-1 ] 
    set p0 [ lindex $points 0 ]
    set xlast [ lindex $p0 0 ]
    set ylast [ lindex $p0 1 ]
    for { set n 1 } { $n < [llength $points] } { incr n } {
      set p [ lindex $points $n ]
      set dx [ expr [lindex $p 0]-$xlast ]
      set dy [ expr [lindex $p 1]-$ylast ]
      if { $dx < 0 } {
        uint [ expr -$dx*4+3 ]
      } else {
        uint [ expr $dx*4+1 ]
      }
      set xlast [ lindex $p 0 ]
      set ylast [ lindex $p 1 ]
      int $dy
    }

    int [ lindex $p0 0 ]   ;# geometry-x (absolute)
    int [ lindex $p0 1 ]   ;# geometry-y (absolute)

  }

  #  box list is { left bottom right top }
  proc rect { layer datatype text box } {
    record TEXT
    bits 01111011  ;# SWHXYRDL
    uint $layer    ;# layer
    uint $datatype ;# datatype
    uint [ expr [lindex $box 2]-[lindex $box 0] ]   ;# width 
    uint [ expr [lindex $box 3]-[lindex $box 1] ]   ;# height
    int [ lindex $box 0 ]   ;# x-pos 
    int [ lindex $box 1 ]   ;# y-pos
  }

  proc text { layer datatype text pos } {
    record TEXT
    bits 01011011  ;# 0CNXYRTL
    str $text      ;# text-string
    uint $layer    ;# text-layer
    uint $datatype ;# text-datatype
    int [ lindex $pos 0 ]   ;# text-x (relative)
    int [ lindex $pos 1 ]   ;# text-y (relative)
  }

  proc tail {} {
    tail
  }

}

# evaluate the script

eval $txt


