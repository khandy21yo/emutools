#!/usr/bin/perl
# world.perl
#
# This program generates a world map from the CIA World Database II.
# The code is just a quick hack, and is quite ugly, but it works Ok.
#
# Kevin Handy, May 2016
#
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
$fulllength = 1800;	# Full height of image file
$woffset = 20;		# Offset width
$hoffset = 20;		# Offset height
$wmax = $fullwidth - 2 * $woffset;	# Width of map area
$hmax = $fulllength - 2 * $hoffset;	# Height of mp area
$max_rank = 2;		# Anything ranked higher is not displayed
			#  The larger the number, the more detail included.
			#  but too high and the map is very messy.

#
# These functios map the latitude and longitude to pixel positions
# on the png.
#
sub remapx
{
	return int(((90 - $_[0]) / 180.0 * $hmax)) + $hoffset;
}
sub remapy
{
	return  int(((180 + $_[0]) / 360.0 * $wmax)) + $woffset;
}

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
# Loop through list of files
#
for $file (@filenames)
{
	print STDERR $file, "\n";
	&process_file($file);
}

#
# Output map to a .png file
#  Name hardcoded for now.
#
open(OUT, ">world.png") || die;;
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
			#
			# Segment header.
			#   Start a new line.
			#
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
			# Ignore data that is outside of our desired ranking
		}
		else
		{
			#
			# Split out the lat/long from the data line.
			# The 1st method should always work, nut just in case.
			# The 2nd method sometimes failes.
			#   character pPositions are not fixed.
			#
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

			#
			# Debugging information.
			# Looks mor max/min lat/long
			#
			$maxx = $maxx > $xr ? $maxx : $xr;
			$minx = $minx < $xr ? $minx : $xr;
			$maxy = $maxy > $yr ? $maxy : $yr;
			$miny = $miny > $yr ? $miny : $yr;

			#
			# Convert lat/long to pixel positions
			#
			$x = remapx($xr);
			$y = remapy($yr);

			if ($lastx == -1)
			{
				# Don't hav a starting point for the line
				# segment yet.
				# This point will become the starting point.
			}
			elsif (abs($yr - $lastyr) > 80)
			{
				# This is to drop lines that cross the +/-
				# barrier. Makes nasty lines across the map.
				# Should split into two segments,
				# but there's only a couple of these, so
				# ignoring them seems to work out ok.
			}
			elsif ($lastx != $x || $lasty != $y)
			{
				# Plot a line segment
#print STDERR "$x $y\n";
				$im->line($lasty, $lastx, $y, $x, $black);
			}

			#
			# Mark this point as the new start of the next
			# line segment.
			#
			$lastx = $x;
			$lasty = $y;
			$lastxr = $xr;
			$lastyr = $yr;
		}
	}
}

#
# Print out max/min lat/long.
#
print stderr "x=($minx,$maxx) y=($miny,$maxy)\n";

