#!/usr/bin/perl
$flag = 0;
while (<>) {
  if (/^%%Page:/) {
    if ($flag) {
      print "grestore\n";
    }
    $flag = 1;
    print $_;
    print "gsave\n";
    print ".85 setgray\n";
    print "/Helvetica-Bold findfont 96 scalefont setfont\n";
    print "306 400 moveto\n";
    print "45 rotate\n";
    print "(Draft) dup\n";
    print "stringwidth pop 2 div neg 0 rmoveto show\n";
    print "grestore\n";
  }
  else
  {
    print;
  }
}

