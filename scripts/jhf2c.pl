#!/usr/bin/perl 

use strict;

my $name;
while ($name = shift) {

  my $num; 

  my $ch = 32;
  my @EDG;
  my @INFO;

  open JHF, "<$name.jhf" or die "cannot open $name.jhf";
  my $yymax = 0; my $yymin = 0;

  LP: while (read (JHF,$num,5) > 0) {
    my $n;
    if (read (JHF,$n,3) < 3) {
      last LP;
    }
    my $starti = @EDG;
    my $c1; 
    my $c2;
    read (JHF,$c1,1);
    read (JHF,$c2,1);
    my $x1 = ord ($c1);
    my $x2 = ord ($c2);
    my $w = $x2-$x1;
    my $ymin = 0;
    my $ymax = 0;
    my $last = undef;
    while ($n > 1) {
      read(JHF,$c1,1);
      while (ord ($c1) == 13 || ord ($c1) == 10) {
        read(JHF,$c1,1);
      }
      read(JHF,$c2,1);
      my $x = ord($c1)-$x1;
      my $y = ord("Z")+1-ord($c2);
      if ($y < $ymin) { $ymin = $y }
      if ($y > $ymax) { $ymax = $y }
      if ($c1 eq " ") {
        $last = undef;
      } else {
        if (defined($last)) {
          push @EDG, "$last, $x, $y";
        }
        $last = "$x, $y";
      }
      $n--;
    }
    my $endi = @EDG;
    push @INFO, [ $starti, $endi, $w, $ymin, $ymax ];
    read(JHF,$c1,1);
    if (ord ($c1) == 13) {
      read(JHF,$c1,1);
    }

    if ($ymin < $yymin) { $yymin = $ymin }
    if ($ymax > $yymax) { $yymax = $ymax }

    $ch++;
  }

  close JHF;

  print "short ${name}_edges [][4] = {\n    { ", join(" },\n    { ", @EDG), " }\n};\n\n";

  print "HersheyCharInfo ${name}_info [] = {\n    HersheyCharInfo (";
  my @INFOS = map { join (", ", @$_) } @INFO;
  print join ("),\n    HersheyCharInfo (", @INFOS);
  print ")\n};\n";

  print "\nstatic HersheyFont $name (${name}_edges, ${name}_info, 32, ",$ch,", ", $yymax, ", ", $yymin, ");\n\n";
  
}

