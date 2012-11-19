#! /usr/bin/perl -w
use strict qw (vars refs);
use Getopt::Long qw[:config prefix_pattern=(--|-|) ];
use Pod::Usage;

our $ssha=undef;
our $tl=undef;
our $k=undef;
our $Bc=undef;
our $Dc=undef;
our $Gc=undef;

GetOptions(
	   "ssha=s"=>\$ssha,
	   "tl=s"=>\$tl,
	   "k=s"=>\$k,
	   "Bc=s"=>\Bc,
	   "Dc=s"=>\Dc,
	   "Gc=s"=>\Gc,
	  );


sub g_cmd {
  my ($cmd,%p)=@_;
  my @cmd = map("$_=$p{$_}",keys %p);
  print STDERR join(" ",$cmd,@cmd)."\n";
  system $cmd,@cmd;
}

sub r_mapcalc {
  my (%p)=@_;
  g_cmd('r.mapcalc',%p);
}

my $date=`g.gisenv MAPSET`;
chomp $date;

my @k=split(/ /,$k);

my @interval;
my $p=$k[0];
foreach my $n (@k[1 .. $#k ]) {
    my ($pm,$nm);
    $p =~ /k(..)(..)/ and $pm=$1*60+$2;
    $n =~ /k(..)(..)/ and $nm=$1*60+$2;
    my $d=($nm+$pm)/2;
    my $h = int($d/60);
    my $m = $d % 60;
    push @interval, sprintf "%2.2d:%2.2d",$h,$m;
    $p=$n;
}

# Calclate Instant Integrals
foreach my $i (@interval) {
    g_cmd("r.heliosat --prefix='_' --instant --date=${date}T${i}:00 elevin=Z\@2km linkein=$tl lat=latitude_deg\@2km ssha=ssha");
 }
  
my $pB="(-Bc/2.0)";
my $pD="(-Dc/2.0)";
for (my $i=0; $i<$#k; $i++) {
    my $k=$k[$i];
    my $time=$interval[$i];
    $time=~s/://;
    my $cB="_Bci$time";
    my $cD="_Dci$time";
    r_mapcalc("B$k"=>"$k*($cB-$pB)");
    r_mapcalc("D$k"=>"$k*($cD-$pD)");
    $pB=$cB;
    $pD=$cD;
}
# Last ones
r_mapcalc("B$k[-1]"=>"$k[-1]*(Bc/2-$pB)");
r_mapcalc("D$k[-1]"=>"$k[-1]*(Dc/2-$pD)");
# And Sum
r_mapcalc(B=>join('+',map("\"B$_\"",@k)));
r_mapcalc(D=>join('+',map("\"D$_\"",@k)));
r_mapcalc(G=>"B+D");

# Calculate Daily K
r_mapcalc(Bk=>'B/Bc');
r_mapcalc(Dk=>'D/Dc');
r_mapcalc(K=>'G/Gc');
