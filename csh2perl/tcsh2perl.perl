#!/usr/bin/perl
#
# tcsh2perl
#
#	Program to help convert csh programs to perl.
#
# Description
#
#	This program is used to help convert csh scripts into
#	perl scripts. (Much editing is still necessary)
#
#
use POSIX qw(strftime);

$userflag = 0;
$pdate = strftime("%m/%d/%Y", localtime);

while(<>)
{
	chomp;
	$line = $_;

#
# Handle converting bits inside of lines
#
restart:
	#
	# Handle continued lines by merging them together
	#
	if (($line =~ /(.*)(\\)$/) && !($line =~ /^#/))
	{
		#
		# Lose continuation character
		#
		chop($line);
		$old = $line;
		$new = <>;
		chomp($new);
		$new =~ s/(^[ ]*)(.*)$/ $2/;
		$line = $old . $new;
		goto restart;
	}

	#
	# Strip whitespace from end of line
	#
	$line =~ s/\[ \t]*$//;

	#
	# Change `printenv XXX` to '$ENV{'XXX'}'
	#
	while ($line =~ /(^.*)(`printenv )(.*)(`)(.*)/)
	{
		if (substr($3, 0, 1) ne '"' && substr($3, 0, 1) ne "'")
		{
			$line = $1 . '$ENV{"' . $3 . '"}' . $5;
		}
		else
		{
			$line = $1 . '$ENV{' . $3 . '}' . $5;
		}
	}

	#
	# Change `argv` to 'ARGV'
	#
	$line =~ s/argv/ARGV/g;

	#
	# Change `ARGV[x]` to 'ARGV[x-1]'
	#
	if ($line =~ /(^.*)(ARGV\[)(.*)(\].*$)/)
	{
		my $m1 = $1;
		my $m3 = $3;
		my $m4 = $4;

		if ($m3 =~ /^[0-9]*$/)
		{
			$mysum = $m3 - 1;
			$line = $m1 . 'ARGV[' . $mysum . $m4;
		}
		else
		{
			$line = $m1 . 'ARGV[' . $m3 . ' - 1' . $m4;
		}
	}
	#
	# Change `$#ARGV` to '($#ARGV+1)'
	#
	if ($line =~ /(^.*)(\$\#ARGV)(.*$)/)
	{
		$line = $1 . '($#ARGV+1)' . $3;
	}

	#
	# Change `$status` to '$? >> 8'
	#
	$line =~ s/\$status/(\$? >> 8)/g;
	$line =~ s/\$\{status\}/(\$? >> 8)/g;

	#
	# Change "${xxx}" to $xxx
	#
	while ($line =~ /(.*)(\"\$\{)([a-z0-9_]*)(\}\")(.*)/)
	{
		$line = $1 . '$' . $3 . $5;
	}

	#
	# Change >! to >
	#	This usually causes a file named '!' to be created
	#
	while ($line =~ /(.*)(\>!)(.*)/)
	{
		$line = $1 . '>' . $3;
	}

	#
	# Change '== "xxx"' to 'eq "xxx"'
	#
	while ($line =~ /(.*)(\=\= \")(.*)/)
	{
		$line = $1 . 'eq "' . $3;
	}
	while ($line =~ /(.*)(\!\= \")(.*)/)
	{
		$line = $1 . 'ne "' . $3;
	}

	#
	# Change '$0' to '$9' to '\$0' to '\$9'
	#
#	$line =~ s/([^\\])(\$[0-9])/$1\\$2/g;

	#
	# Change "set var = `echo ${var} | tr a-z A-Z`" to "$var = uc($var)"
	#
	if ($line =~ /(^[ \t]*)(set )([a-z0-9_]*)(\ = `echo ["]?\$[{]?)([a-z0-9_]*)([}]?["]? \| tr a-z A-Z`)/)
	{
		$line = $1 . '$' . $3 . ' = uc($' . $5 . ')';
	}

	#
	# Change "set $var = `echo ${var} | tr A-Z a-z`" to "$var = lc($var)"
	#
	if ($line =~ /(^[ \t]*)(set )([a-z0-9_]*)(\ = `echo ["]?\$[{]?)([a-z0-9_]*)([}]?["]? \| tr A-Z a-z`)/)
	{
		$line = $1 . '$' . $3 . ' = lc($' . $5 . ')';
	}

	#
	# Change "set $var = `echo ${var} | tr -d A-z`" to "$var =~ s/\D//g"
	#
	if ($line =~ /(^[ \t]*)(set )([a-z0-9_]*)(\ = `echo \$\{)([a-z0-9_]*)(\} \| tr -d A-z`)/)
	{
		if ($3 eq $5)
		{
			$line = $1 . '$' . $3 . ' =~ tr/\D//g';
		}
	}

	#
	# Change `ls *` to <*>
	#
	if ($line =~ /(.*)(\`ls )([a-zA-z0-9\/\{\}\$\*\.]+)(\`)(.*)/)
	{
		$line = $1 . '<' . $3 . '>' . $5;
	}

	#
	# Change (-z xxx) to (-z "xxx")
	#
	if ($line =~ /(.*)(\(\-[rwxeozfd][\t ]*)([^"]+)(\))(.*)/)
	{
		$line = $1 . $2 . '"' . $3 . '"' . $4 . $5;
	}

	#
	# Change '`echo xxx | cut -fn -d:`' to something slightly better
	#
	if ($line =~ /(^.*)(\`echo \$)([\{\}a-z0-9_]*)( \| cut \-f)([0-9]+)( \-d\"\:\"\`)(.*$)/)
	{
		if (substr($3, 0, 1) eq '{')
		{
			$tempvar = substr($3, 1, -1);
		}
		else
		{
			$tempvar = $3;
		}
		$line = $1 . '&cutout($' . $tempvar . ', ' . $5 . ')' . $7;
		$needcutout++;
	}
 	#
	# Change '`grep xxx xxx | cut -fn -d:`' to something slightly better
	# There should probaly be a chomp there somewhere.
	#
	if ($line =~ /(^.*)(\`grep .*)( \| cut \-f)([0-9]+)( \-d\"\:\"\`)(.*$)/)
	{
		$tempvar = $2;
		$line = $1 . '&cutout(' . $tempvar . '`, ' . $4 . ')' . $6;
		$needcutout++;
	}

	#
	# Modify "foreach $xxx (`echo $xxx`)
	#
	if ($line =~ /(^.*)(foreach )([a-z_]+)( \(\`echo \$[{]?)([a-z0-9_]+)([}]?\`\))(.*$)/)
	{
		$line = $1 . 'foreach $' . $3 . " (split(/[ \\t\\n]/, \$" . $5 . '))' . $7;
		$needcutout++;
	}

	#
	# Change '(-e xxxxx)' to '(-e "xxxxx")'
	#
	if ($line =~ /(^.*\(\-[ed][\t ]+)([^")]+)(\).*$)/)
	{
		$line = $1 . '"' . $2 . '"' . $3;
	}

	#
	# Handle `hostname -s` in strings
	#
	if ($line =~ /(^[\t ]*)(.*)(\`hostname \-s\`)(.*)/)
	{
		print $1, "use Sys::Hostname;\n";
		print $1, "\$hostname = hostname;\n";
		$line = $1 . $2 . '${hostname}' . $4;
	}

	#
	# Handle `date 'xxx'`
	#
	if ($line =~ /(^[\t ]*)(.*)(\`date \'\+)([^']*)(\'\`)(.*$)/)
	{
		print "use POSIX qw(strftime);\n" if (!$xstrftime);
		$xstrftime = 1;
		print $1, "\$pdate = strftime(\"", $4, "\", localtime);\n";
		$line = $1 . $2 . "\${pdate}" . $6;
	}
	elsif ($line =~ /(^[\t ]*)(.*)(\`date .*\`)(.*$)/)
	{
		print $1, "chomp(\$pdate = ", $3, ");\n";
		$line = $1 . $2 . "\${pdate}" . $4;
	}

	#
	# Handle $USER variable
	#
	if ($userflag == 0 && ($line =~ /\$USER/ || $line =~ /\$\{USER\}/))
	{
		$userflag = 1;
		print "\$USER = \$ENV{\"USER\"};\n";
	}

	#
	#  Change "xxxxx"$yyy to "xxxx" . $yyy
	#
	if ((!($line =~ /\`/)) && (!($line =~ /\'/)))
	{
		while ($line =~ /^([^"]*\")([^"]*\"[^"]*\")*([^"]*\")(\$)(.*)$/)
		{
			$line = $1 . $2 . $3 . ' . ' .$4 . $5;
		}
	}

	#
	# Change "${xxx}" and ${xxx} to $xxx when easy
	#
	if ((!($line =~ /\`/)) && (!($line =~ /\'/)))
	{
		while ($line =~ /^(([^"]*\"[^"]*\")*)([^"]*)(\$\{)([^}]*)(\})(.*)$/)
		{
			$line = $1 . $3 . '$' . $5 . $7;
		}
		while ($line =~ /^(([^"]*\"[^"]*\")*)([^"]*)(\"\$\{)([^}]*)(\}\")(.*)$/)
		{
			$line = $1 . $3 . '$' . $5 . $7;
		}
	}


	######################################################################
	# Process specific comands
	# Here we actually output code.
	######################################################################

	if (($. == 1) && ((substr($line, 0, 10) eq '#!/bin/csh') || (substr($line, 0, 11) eq '#!/bin/tcsh')))
	{
		#
		# First line in the file?
		#
		print "#!/usr/bin/perl\n";
		print "# Converted from csh to perl on ${pdate}\n";
	}
	elsif (($. == 1) && ($line =~ /^\#\!/))
	{
		print STDERR "This isn't a CSH program\n";
		print "This isn't a CSH program! BAD THINGS PROBABLY HAPPEN!\n";
		print $_, "\n";
	}
	elsif ($line =~ /^[ \t]*#/)
	{
		print "$line\n";
	}
	#
	# Change 'set xxx = ' to '$xxx ='
	#
	elsif ($line =~ /(^[ \t]*)(set )(.*= `)(.*$)/)
	{
		$clean1 = &cleanup($4);
		print $1, 'chomp($' . $3 . $clean1 . ");\n";
	}
	elsif ($line =~ /(^[ \t]*)(set )(.*$)/)
	{
		$line = $1 . '$' . $3;

		#
		# Change input 'set xxx = $<' to 'chomp($xxx = <STDIN>)'
		#
		if ($line =~ /(^[ \t]*)(.*)(\$\<$)/)
		{
			$line = $1 . "chomp(" . $2 . "<STDIN>)";
		}
		elsif ($line =~ /(^[ \t]*)(.*)(\"\$\<\"$)/)
		{
			$line = $1 . "chomp(" . $2 . "<STDIN>)";
		}
		print $line, ";\n";
	}
	#
	# If there is a pipe in the stream, ...
	#
	elsif ($line =~ /(^[ \t]*)(.*)( \| )(.*$)/)
	{
		print $1, 'system("', cleanup($2 . $3 . $4), '");', "\n";
	}
	elsif ($line =~ /(^[\t ]*)(\$\{script_path\}\/)(.*)$/)
	{
		print $1, 'system("', cleanup($2 . $3), '");', "\n";
	}
	elsif ($line =~ /(^[\t ]*)(\$script_path\/)(.*)$/)
	{
		print $1, 'system("', cleanup($2 . $3), '");', "\n";
	}
	elsif ($line =~ /(^[\t ]*)(sed)(.*)$/)
	{
		print $1, 'system("', cleanup($2 . $3), '");', "\n";
	}
	#
	# Change touch to system("touch")
	#
	elsif ($line =~ /(^[ \t]*)(touch \")(.*)(\"$)/)
	{
		print $1, "# $line\n";
		print $1, "utime time, time, \"$3\";\n";
	}
	elsif ($line =~ /(^[ \t]*)(touch )(.*$)/)
	{
		print $1, "# $line\n";
		print $1, "utime time, time, \"$3\";\n";
	}
	#
	# Change 'echo xxx' to 'print xxx'
	#
	elsif ($line =~ /(^[ \t]*)(echo [^\<]*\>.*)/)
	{
		print $1, 'system("', cleanup($2), '");', "\n";
	}
	elsif ($line =~ /(^[ \t]*)(echo \-n)(.*\>\>)(.*$)/)
	{
		print $1, 'system("', cleanup($2 . $3 . $4), '");', "\n";
	}
	elsif ($line =~ /(^[ \t]*)(echo \-n)(.*$)/)
	{
		print $1, 'print ', $3, ";\n";
	}
	elsif ($line =~ /(^[ \t]*)(echo )(.*\>\>)(.*$)/)
	{
		print $1, 'system("', cleanup($2 . $3 . $4), '");', "\n";
	}
	elsif ($line =~ /(^[ \t]*)(sort )(.*)$/)
	{
		print $1, 'system("', $2 . $3, '");', "\n";
	}
	elsif ($line =~ /(^[ \t]*)(echo \")(.*)(\"$)/)
	{
		print $1, 'print "', $3, "\\n\";\n";
	}
	elsif ($line =~ /(^[ \t]*)(echo )(.*$)/)
	{
		print $1, 'print ', $3, ', "\n"', ";\n";
	}
	elsif ($line eq "")
	{
		print "\n";
	}
	elsif ($line =~ /(^[ \t]*)(foreach[ \t]+)(.*$)/)
	{
		if (substr($3, 0, 1) eq '$')
		{
			print $1, $2, $3, "\n", $1, "{\n";
		}
		else
		{
			print $1, $2, '$', $3, "\n", $1, "{\n";
		}
	}
	elsif ($line =~ /(^[ \t]*)(while)(.*$)/)
	{
		print $1, $2, $3, "\n", $1, "{\n";
	}
	elsif ($line =~ /(^[ \t]*)(end$)/)
	{
		print $1, "}\n";
	}
	elsif ($line =~ /(^[ \t]*)(if \!)(.*)( then$)/)
	{
		print $1, "if (!", $3, ")", "\n" . $1, "{\n";
	}
	elsif ($line =~ /(^[ \t]*)(else if )(.*)( then$)/)
	{
		print $1, "}\n", $1, "elsif ", $3, "\n", $1, "{\n";
	}
	elsif ($line =~ /(^[ \t]*)(if.*)( then$)/)
	{
		print $1, $2, "\n" . $1, "{\n";
	}
	elsif ($line =~ /^([\t ]*)(rm )(.*\*.*)$/)
	{
		print $1, 'unlink <', $3, '>;', "\n";
	}
	elsif ($line =~ /(^[ \t]*)(rm )(.*$)/)
	{
		print $1 . 'unlink "' . $3 . "\";\n";
	}
	elsif ($line =~ /(^[ \t]*)(chmod )(.*$)/)
	{
		print $1 . 'chmod(' . $3 . ");\n";
	}
	elsif ($line =~ /(^[ \t]*)(else[\t ]*$)/)
	{
		print $1 . "}\n" . $1 . "else\n" . $1 . "{\n";
	}
	elsif ($line =~ /(^[ \t]*)(endif$)/)
	{
		print $1 . "}\n";
	}
	elsif ($line =~ /^([\t ]*)(onintr[\t ])([A-Za-z0-9]*)/)
	{
		print "#FIXME: ", $line, "\n";
		print $1, "\$SIG{'INT'} = sub { goto $3; };\n";
	}
	elsif ($line =~ /(^[\t ]*onintr[\t ])/)
	{
		print "#FIXME: ", $line, "\n";
	}
	elsif ($line =~ /^([\t ]*)(clear)$/)
	{
		print $1, "system(\"clear\");\n";
	}
	elsif ($line =~ /^([\t ]*)(.*script_path.*resetlock )([a-z]+)$/)
	{
		print $1, "resetlock(\"", $3, "\");\n";
	}
	elsif ($line =~ /^([\t ]*)(.*script_path.*setlock )([a-z]+)([\t ]+)(.*)$/)
	{
		print $1, "setlock(\"", $3, "\", ", $5, ");\n";
	}
	elsif ($line =~ /^([\t ]*)breaksw[\t ]*/)
	{
		$switchclose = 0;
		print $1, "}\n";
	}
	elsif ($line =~ /^([\t ]*)(switch[\t ]*\([\t ]*)([^\)]*)[\t ]*\)$/)
	{
		print "# ", $line, "\n";
		if ($switcher ne "")
		{
			push(@switchstack, $switcher);
		}
		$switcher = $3;
		$switchcnt = 0;
	}
	elsif ($line =~ /^([\t ]*)endsw[\t ]*$/)
	{
		print "# ", $line, "\n";
		print $1, "}\n" if $switchclose;
		$switchclose = 0;
		$switcher = pop(@switchstack);
	}
	elsif ($line =~ /^([\t ]*)(case[\t ]*)([^:]*)[\t ]*:$/)
	{
		if (substr($3, 0, 1) eq '"')
		{
			$quote = '';
		}
		else
		{
			$quote = '"';
		}
		if ($switchcnt == 0)
		{
			print $1, "if (", $switcher, " eq ", $quote, $3, $quote, ")\n",
				$1, "{\n";
		}
		else
		{
			print $1, "elsif (", $switcher, " eq ", $quote, $3, $quote, ")\n",
				$1, "{\n";
		}
		$switchcnt++;
		$switchclose = 1;
	}
	elsif ($line =~ /^([\t ]*)default[\t ]*:[\t ]*$/)
	{
		print "#", $line, "\n";
		print $1, "else\n", $1, "{\n";
		$switchclose = 1;
	}
	elsif ($line =~ /^([\t ]*)(mv[\t ]+)([^ ]+)([\t ]+)([^ ]+)$/)
	{
		print $1, "rename \"$3\", \"$5\";\n";
	}
	else
	{
		#
		# Keep whatever is left (and slap on a semi-colon to
		# end the statement)
		#
#		print "system(\"" . &cleanup($line) . "\");\n";
		print $line, ";\n";
	}
}

if ($needcutout)
{
	print "\n\nsub cutout\n{\n" .
		"\treturn((split(/:/, \$_[0]))[\$_[1] - 1]);\n" .
		"}\n\n";
}

sub cleanup
{
    my $ptr;
    my $string = $_[0];

    $ptr = 0;
    while (($ptr = index($string, '\\', $ptr)) != -1)
    {
        $string = substr($string, 0, $ptr) . '\\' .
            substr($string, $ptr);
        $ptr += 2;
    }

    $ptr = 0;
    while (($ptr = index($string, '"', $ptr)) != -1)
    {
        $string = substr($string, 0, $ptr) . '\\' .
            substr($string, $ptr);
        $ptr += 2;
    }

    $string =~ s/([^\\])(\$[0-9])/$1\\$2/g;

    return $string;
}
