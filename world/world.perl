#!/usr/bin/perl

use GD;

# @filenames = ('namer-cil.txt');
#@filenames = ('africa-cil.txt', 'asia-cil.txt',  'europe-cil.txt',
#	'namer-cil.txt',  'samer-cil.txt');
@filenames = ('africa-cil.txt', 'asia-cil.txt',  'europe-cil.txt',
	'namer-cil.txt',  'samer-cil.txt',
	'africa-bdy.txt', 'asia-bdy.txt',  'europe-bdy.txt',
	'namer-bdy.txt',  'samer-bdy.txt');

#
# Define size of the thing
#
$fullwidth = 2400;	# Full width of image file
$fulllength = 1400;	# Full height of image file
$woffset = 20;		# Offset width
$hoffset = 20;		# Offset height
$wmax = 2360;		# Width of map area (< fullwidt - woffset)
$hmax = 1340;		# Height of mp area (< fullheight - hoffset)
$max_rank = 2;		# Anything ranked higher is not displayed

#
# create a new image
#
$im = new GD::Image($fullwidth, $fulllength);

#
# allocate some colors
#
$white = $im->colorAllocate(255,255,255);
$black = $im->colorAllocate(0,0,0);
$red = $im->colorAllocate(255,0,0);
$blue = $im->colorAllocate(0,0,255);

#
# make the background transparent and interlaced
#
#$im->transparent($white);
$im->interlaced('true');

#
# Put a black frame around the picture
#
$im->rectangle($woffset,$hoffset,$woffset+$wmax,$hoffset+$hmax,$black);

#
# Draw a blue oval
#
#$im->arc(50,50,95,75,0,360,$blue);

#
# And fill it with red
#
#$im->fill(50,50,$red);

#
# Loop through list of files
#
for $file (@filenames)
{
	print STDERR $file, "\n";
	&process_file($file);
}

# make sure we are writing to a binary stream
open(OUT, ">world.png") || die;;

# Convert the image to PNG and print it on standard output
print OUT $im->png;
close OUT;


#
# Process one file
#
sub process_file
{
	$fn = $_[0];
	$lastx = -1;
	$lasty = -1;
	$rank = 0;

	open(FN, $fn) || die $fn;

	while(<FN>)
	{
		chomp;
		if (substr($_, 0, 7) eq 'segment')
		{
			$lastx = -1;
			$lasty = -1;
			if ($_ =~ /(rank )([0-9]+)/)
			{
				$rank = $2;
			}
#print STDERR $_, "\n";
		}
		elsif ($rank >= $max_rank)
		{
		}
		else
		{
			if ($_ =~ /([\-0-9\.]+)([ ]+)([\-0-9\.]+)/)
			{
				$xr = $1;
				$yr = $3;
			}
			else
			{
				$xr = substr($_, 1, 9);
				$yr = substr($_, 11);
			}

$maxx = $maxx > $xr ? $maxx : $xr;
$minx = $minx < $xr ? $minx : $xr;
$maxy = $maxy > $yr ? $maxy : $yr;
$miny = $miny > $yr ? $miny : $yr;

			$x = int(((90 - $xr) / 180.0 * $hmax));
			$y = int(((180 + $yr) / 360.0 * $wmax));

			if ($lastx == -1)
			{
			}
			elsif (abs($yr - $lastyr) > 80)
			{
			}
			elsif ($lastx != $x || $lasty != $y)
			{
#print STDERR "$x $y\n";
				$im->line($lasty, $lastx, $y, $x, $black);
			}

			$lastx = $x;
			$lasty = $y;
			$lastxr = $xr;
			$lastyr = $yr;
		}
	}
}

print stderr "x=($minx,$maxx) y=($miny,$maxy)\n";

