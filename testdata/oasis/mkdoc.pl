#!/usr/bin/perl -w

use strict;

my %TESTS;
my %TEST_INTENTION;

my @MAJOR_TESTS = (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14);
my %MAJOR_DESCRIPTIONS = (
  1 => "Empty file. Various ways to specify a float (database unit).",
  2 => "Cells. Various ways to specify cell names (id, string) and refer to them.",
  3 => "Texts. Various ways to specify text strings and to refer to them.",
  4 => "Rectangles",
  5 => "Polygons",
  6 => "Paths",
  7 => "Trapezoids",
  8 => "Placements",
  9 => "Ctrapezoids",
  10 => "Modal variables",
  11 => "Properties",
  12 => "Circles",
  13 => "Layer names",
  14 => "CBLOCK compression"
);

my $file;
while ($file = shift @ARGV) {

  open SRC, "<$file" or die "Unable to open $file\n";

  my $major = 0;
  my $minor = 0;
  my $content_description = "";
  my @test_intention = ();
  my @content = ();

  my $in_content = 0;

  while (<SRC>) {
    if (/^#\s*<name>(.*)<\/name>/) { 
      $major = $1; 
      $minor = $1; 
      $major =~ s/t(\d+)\.(\d+)\.ot/$1/; 
      $minor =~ s/t(\d+)\.(\d+)\.ot/$2/; 
    } elsif (/^#\s*<content-description>(.*)<\/content-description>/) { 
      $content_description = $1;
    } elsif (/^#\s*<test-intention>(.*)<\/test-intention>/) { 
      push @test_intention, $1; 
    } elsif (/^#\s*<content>/) { 
      $in_content = 1; 
    } elsif (/^#\s*<\/content>/) { 
      $in_content = 0; 
    } elsif ($in_content && /^#\s*(.*)$/) { 
      push @content, $1; 
    }
  }

  close SRC;

  if (!defined $TESTS{$major}) {
    $TESTS{$major} = [];
  }
  push @{$TESTS{$major}}, [ $minor, $content_description, [@test_intention], [@content] ];
  
  my $i;
  foreach $i (@test_intention) {
    if (!defined $TEST_INTENTION{$i}) {
      $TEST_INTENTION{$i} = [];
    }
    push @{$TEST_INTENTION{$i}}, [$major, $minor];
  }

}


print "<html>\n";
print "<body>\n";

print "<h1>OASIS Tests By Cathegory</h1>\n";

my $m = 0;

foreach $m (@MAJOR_TESTS) {
  print "<h4>Cathegory $m: $MAJOR_DESCRIPTIONS{$m}</h4>\n";
  print "  <ul>\n";
  my $mm;
  foreach $mm (@{$TESTS{$m}}) { 
    print "  <li><a href=\"#${m}_$mm->[0]\">$m.$mm->[0] $mm->[1]</a></li>\n";
  }
  print "  </ul>\n";
}

print "<h1>OASIS Tests By Target</h1>\n";

print "<table>\n";
my $k;
foreach $k (sort keys %TEST_INTENTION) {
  my @T = ();
  foreach $m (sort {$a->[0]==$b->[0]?$a->[1]<=>$b->[1]:$a->[0]<=>$b->[0]} @{$TEST_INTENTION{$k}}) {
    push @T, "<a href=\"#$m->[0]_$m->[1]\">$m->[0].$m->[1]</a>";
  }
  print "  <tr><td>$k</td><td>" . join ("\n", @T) . "</td>";
}
print "</table>\n";

print "<h1>OASIS Test Descriptions</h1>\n";

foreach $m (@MAJOR_TESTS) {
  my $mm;
  print "<h2>$m $MAJOR_DESCRIPTIONS{$m}</h2>\n";
  foreach $mm (@{$TESTS{$m}}) { 
    print "<h3>$m.$mm->[0] $mm->[1]</h3>\n";
    print "  <a name=\"${m}_$mm->[0]\"/>\n";
    print "  <p><a href=\"#top\">Back to top</a></p>\n";
    print "  <p>Test targets:</p>\n";
    print "  <ul>\n";
    my $t;
    foreach $t (@{$mm->[2]}) {
      print "    <li>$t</li>\n";
    }

    print "  </ul>\n";
    if (@{$mm->[3]} > 0) {
      print "  <p>Normalized content:</p>\n";
      print "  <pre>" . join ("\n", @{$mm->[3]}) . "</pre>\n";
    }
  }
}


print "</body>\n";
print "</html>\n";

