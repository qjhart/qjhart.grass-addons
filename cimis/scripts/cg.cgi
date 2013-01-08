#! /usr/bin/perl -w
use XML::Writer;
use JSON;

my %units = 
  (
   ETo=>"[mm]",
   et0=>"[mm]",
   Rs=>"[W/m^2]",
   Rso=>"[W/m^2]",
   K=>undef,
   Rnl=>"[W/m^2]",
   Tx=>"[C]",
   Tn=>"[C]",
   U2=>"[m/s]",
  );


#%Module
#%  description: Get weather information for specific dates and times.
#%  keywords: CIMIS,etxml,evapotranspiration
#%End
#%flag
#% key: xml
#% description: Output XML
#%end
#%option
#% key: date
#% type: string
#% description: Date(s) of interest 
#% multiple: yes
#% required : no
#%end
#%option
#% key: srid
#% type: string
#% description: EPSG SRID 
#% multiple: no
#% answer:3310
#% required : yes
#%end
#%option
#% key: item
#% type: string
#% description: Items of interest from (et0,Rs,K,Rnl,Tx,Tn,U2)
#% multiple: yes
#% answer: et0,et0,Rs,K,Rnl,Tx,Tn,U2,Rso
#% required : yes
#%end
#%option
#% key: point
#% type: string
#% description: Point(s) to access
#% multiple: yes
#% required : no
#%end

#%option
#% key: zipcode
#% type: string
#% description: Zipcode(s) of interest
#% multiple: yes
#% required : no
#%end

#%option
#% key: X
#% type: string
#% description: Box Pointer X
#% multiple: yes
#% required : no
#%end
#%option
#% key: Y
#% type: string
#% description: Box Pointer Y
#% multiple: yes
#% required : no
#%end
#%option
#% key: BBOX
#% type: string
#% description: Bounding Box (e,s,w,n)
#% multiple: no
#% answer:-400000,-650000,600000,450000
#% required : yes
#%end
#%option
#% key: HEIGHT
#% type: string
#% description: Box Height
#% multiple: no
#% answer:550
#% required : yes
#%end
#%option
#% key: WIDTH
#% type: string
#% description: Box width
#% multiple: no
#% answer:500
#% required : yes
#%end

if ( !$ENV{'GISBASE'} ) {
    die "You must be in GRASS GIS to run this program.\n";
}

if (!defined($ARGV[0]) or ($ARGV[0] ne '@ARGS_PARSED@')) {
    my $arg = "";
    for (my $i=0; $i < @ARGV;$i++) {
        $arg .= " $ARGV[$i] ";
    }
    system("$ENV{GISBASE}/bin/g.parser $0 $arg");
    exit;
}

my $yesterday=`date --date=yesterday --iso`;
chomp $yesterday;

# Dates have special ':' processing
my $date=$ENV{GIS_OPT_DATE} || $yesterday;
my @date=map { split /,/ } $date;
@date = map {
  if (/\:/) {
    my ($s,$e) = split /\:/;
    my @t;
    for(my $date=$s; $date ne $e; $date=`date --date='$date +1 day' --iso`,chomp $date) {
      push @t,$date;
    }
  (@t,$e);
  } else {
    $_;
  }
} @date;

@date=sort {$a cmp $b} @date;

my @item=split(/,/,$ENV{GIS_OPT_ITEM});

my @zipcode;

my $cmd.=sprintf("cg.cgi item='%s' date='%s'",join(',',@item),join(',',@date));

my $json = JSON->new->allow_nonref;

my $points;
my $in_points=[];

my $srid=$ENV{GIS_OPT_SRID};

if ($ENV{GIS_OPT_ZIPCODE}) {
    @zipcode=split(/,/,$ENV{GIS_OPT_ZIPCODE});
    $cmd.=sprintf(" zipcode=%s",join(',',@zipcode));
} elsif ($ENV{GIS_OPT_POINT}) {
    my $point=$ENV{GIS_OPT_POINT};
    $in_points=$json->decode(sprintf('[%s]',$ENV{GIS_OPT_POINT}));
    $cmd.=sprintf(" srid=%s,point=%s",$srid,$point);

} else { #Old Style
    my $bbox=$ENV{GIS_OPT_BBOX};
    my ($w,$s,$e,$n)=split(/,/,$bbox);
    my $height=$ENV{GIS_OPT_HEIGHT};
    my $width=$ENV{GIS_OPT_WIDTH};
    my @X = map { split /,/ } $ENV{GIS_OPT_X};
    my @Y = map { split /,/ } $ENV{GIS_OPT_Y};

    # Do for each input point;
    for (my $k=0; $k<= $#X; $k++) {
	my $x= $w+($e-$w)*$X[$k]/$width;
	my $y = $n-($n-$s)*$Y[$k]/$height;
	push @{$in_points},[$x,$y];
    }


    $cmd.=sprintf(" srid=%s BBOX='%s' WIDTH=%d HEIGHT=%d X='%s' Y='%s'",
		  $srid,$bbox,$width,$height,join(',',@X),join(',',@Y));
}


my $in_points_3310=[];
if ($srid ==3310) {
    $in_points_3310=$in_points;
} else {
    my $i=join("\n",map("$$_[0] $$_[1]",@$in_points));
    foreach (split(/\n/,`echo '$i' | cs2cs +from +init=epsg:$srid +to +init=epsg:3310`)) {
	my @o=split(/\s+/,$_);
	push @$in_points_3310,[$o[0],$o[1]];
    }
}

my $in_points_4269=[];
if ($srid==4269) {
    $in_points_4269=$in_points;
} else {
    my $i=join("\n",map("$$_[0] $$_[1]",@$in_points));
    foreach (split(/\n/,`echo '$i' | cs2cs -f "%.4f" +from +init=epsg:$srid +to +init=epsg:4269`)) {
	my @o=split(/\s+/,$_);
	push @$in_points_4269,[$o[0],$o[1]];
    }
}

#print STDERR $cmd,"\n";
#printf STDERR "in_points: %s\n",$json->encode($in_points);
#printf STDERR "in_points_3310: %s\n",$json->encode($in_points_3310);
#printf STDERR "in_points_4269: %s\n",$json->encode($in_points_4269);
#exit 1;

# Send XML document;
my $xml=new XML::Writer(NEWLINES=>0,DATA_MODE=>0);
$xml->comment("Draft Spec for input");
$xml->comment($cmd);
$xml->startTag("data",dates=>join(',',@date),first_date=>$date[0],last_date=>$date[-1]);
$xml->characters("\n");

# Do zipcodes
foreach my $zip (@zipcode) {
    foreach my $date (@date) {
	my @err;
	# First check we have this date
	my $ans=`g.findfile mapset=$date file=zipcode element=vector | grep ^name`;
	chomp $ans;
	if ("name='zipcode'" ne $ans) {
	    $xml->emptyTag("DataPoint",zipcode=>$zip,date=>$date,err=>'date_not_found');
	} else {
	    my %item;
	    foreach my $col (split /\n/,`v.info --q -c zipcode\@$date`) {
		my ($t,$n) = split('\|',$col);
		if ($n =~ s/_mean//) {
		    $item{$n}++;
		}
	    }
	    my @have;
	    foreach(@item) {
		push @have,$_ if ($item{$_});
	    }
	    my $ans;
	    if ($#have > 0) {
		my $c=join(',',map("${_}_mean",@have));
		$ans=`v.db.select --q -c fs=, columns=$c map=zipcode\@$date where="zipcode='$zip'"`;
	    } else {
		$ans=`v.db.select --q -c fs=, columns=zipcode map=zipcode\@$date where="zipcode='$zip'"`;
	    }
	    chomp $ans;
	    my $etoFlag;
	    if ($ans) {
		$xml->startTag("DataPoint",zipcode=>$zip,date=>$date,err=>'');
		$xml->characters("\n");
		my @a = split(',', $ans);
		for (my $i=0; $i<=$#have;$i++) {
		    $item{$have[$i]}=$a[$i];
		}
		for (my $i=0; $i<=$#item;$i++) {
		    my %attr;
		    my $val;
		    if ($item{$item[$i]}) {
			$val=$item{$item[$i]};
		    } else {
			$val='';
			$attr{'err'}='not_found';
		    }
		    # Quick fix for Carlos
		    my $name=$item[$i];
		    if ($name eq 'et0' and ! $etoFlag ) {
			$name = 'ETo';
			$etoFlag++;
		    }
		    $attr{units}=$units{$name} if $units{$name};
		    # Quick Fix for Radiance
		    $val *= 11.574074 if ($val and 
					  ($name eq 'Rn' or $name eq 'Rnl' or 
					   $name eq 'Rs' or $name eq 'Rso'));
		    $xml->dataElement($name,$val,%attr);
		    $xml->characters("\n");
		}
		$xml->endTag;
	    } else {
		$xml->emptyTag("DataPoint",zipcode=>$zip,date=>$date,err=>'zipcode_not_found');
	    }
	    $xml->characters("\n");
	}
    }
}
# Do for each input point;
for (my $k=0; $k<= $#$in_points_3310; $k++) {
    foreach my $date (@date) {
	my @err;
	# First check we have this date
	my $ans=`g.findfile mapset=$date file=sretr element=cellhd | grep ^name`;
	chomp $ans;
	push @err,"date_not_found" if ("name='sretr'" ne $ans);
	
	# Check we're in CA
	my ($x,$y)=@{$$in_points_3310[$k]};
	my ($lon,$lat)=@{$$in_points_4269[$k]};
	$ans=`echo $x $y | r.what state\@2km`;
	chomp $ans;
	(undef,undef,undef,$ans)=split(/\|/,$ans);
	push @err,"not_in_CA" unless ($ans eq 1);
	
	if (@err) {
	    $xml->emptyTag("DataPoint",input_point=>$k,x=>$x,y=>$y,lon=>$lon,lat=>$lat,date=>$date,err=>join(',',@err));
	} else {
	    $xml->startTag("DataPoint",input_point=>$k,x=>$x,y=>$y,lon=>$lon,lat=>$lat,date=>$date,err=>'');
	    $xml->characters("\n");
	    my @qlm=map({my ($name,$d)=split(/\@/);$d||=$date; "$name\@$d"} @item);
	    my $qlm=join(',',@qlm);
	    $r_what=`echo $x $y | r.what $qlm`;
	    chomp $r_what;
	    my @r_what=split(/\|/,$r_what);
	    my $etoFlag;
	    for (my $i=0; $i<=$#qlm;$i++) {
		my %attr;
		my $val=$r_what[$i+3];
		undef $val if $val eq '*';
		# Quick fix for Carlos
		my $name=$item[$i];
		if ($name eq 'et0' and ! $etoFlag ) {
		    $name = 'ETo';
		    $etoFlag++;
		}
		$attr{units}=$units{$name} if $units{$name};
		# Quick Fix for Radiance
		$val *= 11.574074 if ($val and 
				      ($name eq 'Rn' or $name eq 'Rnl' or 
				       $name eq 'Rs' or $name eq 'Rso'));
		$xml->dataElement($name,$val,%attr);
		$xml->characters("\n");
	    }
	    $xml->endTag;
	}
	$xml->characters("\n");
    }
}
$xml->endTag;

