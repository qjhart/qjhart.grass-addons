#! /usr/bin/perl -w

use CGI  qw/:standard -oldstyle_urls/;
use XML::Writer;

my $q=new CGI;
my $TIME=$q->param('TIME');
$q->delete('TIME');

my $map={};

$map->{height}||=$q->param('HEIGHT');
$map->{width}||=$q->param('WIDTH');

if ($q->param('BBOX')) {
  my ($w,$s,$e,$n)=split(/,/,$q->param('BBOX'));
  $map->{extent}->{minx}=$w;  
  $map->{extent}->{miny}=$s; 
  $map->{extent}->{maxx}=$e;  
  $map->{extent}->{maxy}=$n;
}  

if (lc(param('REQUEST')) eq 'getfeatureinfo') {
# Setup up GRASS
    $ENV{GISBASE}='/usr/local/grass5';
    $ENV{PATH}="/home/groups/goes/bin:$ENV{GISBASE}/bin:$ENV{GISBASE}/scripts:$ENV{PATH}";
    $ENV{LD_LIBRARY_PATH}='/usr/local/grass5/lib';
    $ENV{GISRC}="/home/groups/goes/htdocs/wms/grassrc5";

    my $X=$q->param('X');
    my $Y=$q->param('Y');
    my $bbox=$map->{extent};
    my $x=$bbox->{minx}+($bbox->{maxx}-$bbox->{minx})*$X/$map->{width};
    my $y=$bbox->{maxy}-($bbox->{maxy}-$bbox->{miny})*$Y/$map->{height};

    my %layer=(G=>'G',K=>'K',clouds=>'clouds',et0=>'et0',
	       Tx=>'max_at',Tn=>'min_at',ws=>'ws');
	       
    my @qlm=grep({($n,$d)=split(/\@/);$d||=$TIME; if ($layer{$n}){$_=$layer{$n}."\@$d"} else {$_=undef}} split /,/,$q->param('QUERY_LAYERS'));
    my $qlm=join(',',@qlm);

    my $rwhat = `echo $x $y | r.what $qlm`;
    chomp $rwhat;
    my ($xx,$yy,$z,@qvals) = split '\|',$rwhat;

    my $writer = new XML::Writer();
    $writer->startTag("cimis_data");
    $writer->startTag("daily_data");
    $writer->startTag("grid_data",X=>$xx,Y=>$yy);
    $writer->startTag("date",val=>"$TIME");
    $writer->startTag("eto",q=>'');
    $writer->characters($qvals[0]);
    $writer->endTag("eto");
    $writer->endTag("date");
    $writer->endTag("grid_data");
    $writer->endTag("daily_data");
    $writer->endTag("cimis_data");
    $writer->end();

  }
