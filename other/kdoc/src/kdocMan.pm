
package kdocMan;

use File::Path;
use File::Basename;

use Carp;
use Ast;
use kdocAstUtil;
use kdocUtil;
use Iter;
use kdocDocIter;

use strict;

use vars qw/ @clist $lib $root $outdir $opt $debug $datestamp /;

BEGIN
{
}

sub writeDoc
{
	( $lib, $root, $outdir, $opt ) = @_;
	$debug = $main::debug;
	$datestamp = localtime;

	mkpath( $outdir ) unless -f $outdir;

	@clist = ();
	kdocAstUtil::makeClassList( $root, \@clist );

	writeIndex();

	foreach my $class ( @clist ) {
		writeClass( $class );
	}
}

sub writeIndex
{
# header
	open( MANPAGE, ">$outdir/$lib.3$lib" ) 
		|| die "Couldn't write to $outdir/$lib.3$lib\n";

print MANPAGE<<EOF;
.TH $lib 3$lib "$datestamp" "KDOC"
.SH NAME
$lib - Class Library
EOF
	my $doc = $root->{DocNode};

	if ( defined $doc ) {
		print MANPAGE ".SH DESCRIPTION\n"; 
		printDoc( $root, $root->{DocNode} ) if defined $root->{DocNode};
	}

	print MANPAGE "\n.SH HIERARCHY\n";
	writeHier();

	# list and short description
	print MANPAGE "\n.SH INDEX\n";
	foreach my $class ( @clist ) {
		my $short = reform(deref( $class->{DocNode}->{ClassShort} ));
		my $name = fullName( $class );

		print MANPAGE "\n.PP\n.B \"$name\"\n", reform(deref($short));
	}

	if ( defined $doc ) {
		my $sep = "";

		kdocDocIter::SeeAlso( $doc, undef,
			sub { # start
				print MANPAGE "\n.SH \"SEE ALSO\"\n.PP\n";
			},
			sub { # print
				print MANPAGE $sep, shift;
				$sep = ", ";
			}
		 );

		if ( defined $doc->{Author} ) {
			print MANPAGE "\n.SH \"AUTHOR\"\n.PP\n", 
				deref( $doc->{Author}),"\n";
		}
		if ( defined $doc->{Version} ) {
			print MANPAGE "\n.SH \"VERSION\"\n.PP\n", 
				deref( $doc->{Version}),"\n";
		}
	}
	close MANPAGE;
}

sub writeHier
{
	Iter::Hierarchy( $root,
		sub {
			print MANPAGE ".in +1c\n";
		},
		sub {
			my $node = shift;

			if ( $node != $root ) {
				my $src = $node->{ExtSource};
				$src = defined $src ? " ($src)" : "";
				print MANPAGE ".br\n",fullName( $node ),"$src\n";
			}
		},
		sub {
			print MANPAGE ".in -1c\n";
		}
	);
}

sub writeClass
{
	my $class = shift;
	my $name = fullName( $class );

	my $manfile = "$outdir/$name.3$lib";
	open( MANPAGE, ">$manfile" ) || die "Couldn't write to $manfile";

	my $desc = "$name";

	if ( defined $class->{DocNode} 
			&& defined $class->{DocNode}->{ClassShort} ) {
		$desc = deref( $name ." - ". $class->{DocNode}->{ClassShort} );
	}

	my $header = $class->{Source}->{astNodeName};
	my $type = $class->{NodeType};

	print MANPAGE<<EOF;
.TH $name 3$lib "$datestamp" "KDOC"
.SH NAME
$type $desc
.SH SYNOPSIS
.PP
#include<$header>
EOF

	# warnings
	if ( defined $class->{Abstract} ) {
		print MANPAGE ".PP\nAbstract - cannot be instantiated\n";
	}
	if ( defined $class->{Internal} ) {
		print MANPAGE ".PP\nInternal use only.\n";
	}
	if ( defined $class->{Deprecated} ) {
		print MANPAGE ".PP\nDeprecated - do not use in new code.\n";
	}




	# inheritance
	
	my $sep = ":";

	Iter::Ancestors( $class, $root, undef,
		sub { #start
			print MANPAGE ".PP\nInherits";
		},
		sub { #print
			my ( $annode, $anname, $intype, $tmpltype ) = @_;
			$anname .= "<$tmpltype>" if defined $tmpltype;
			$anname .= " ($intype)" unless $intype eq "public";
			my $src = "";

			if ( defined $annode && defined $annode->{ExtSource} ) {
				$src = "(".$annode->{ExtSource}.")";
			}

			print MANPAGE "$sep $anname $src";
			$sep = ",";
		},
		sub { #end
			print MANPAGE ".\n";
		}
	);


	$sep = ":";
	Iter::Descendants( $class, undef,
		sub { #start
			print MANPAGE ".PP\nInherited by";
		},
		sub { #print
			my $descnode = shift;
			my $descname = fullName( $descnode );

			print MANPAGE "$sep $descname";
			$sep = ",";
		},
		sub { #end
			print MANPAGE ".\n";
		}
	);

	
	# member list
	Iter::MembersByType ( $class,
		sub { # group start
			my $group = shift;
			print MANPAGE<<EOF;
.PP
.SS "$group"
.in +1c
EOF
		},
		sub { # member
			listMember( @_ );
		},
		sub { # group end
			print MANPAGE ".in -1c\n";
		},
		undef
	);
	

	if ( defined $class->{DocNode} ) {
		print MANPAGE "\n.SH DESCRIPTION\n";

		printDoc( $class, $class->{DocNode} );
	}

	# member docs
	print MANPAGE "\n.SH MEMBER DOCUMENTATION\n";
	documentMembers( $class );

	# annotations
	if ( defined $class->{DocNode} ) {
		my $doc = $class->{DocNode};
		$sep = "";

		kdocDocIter::SeeAlso( $doc, undef,
			sub { # start
				print MANPAGE "\n.SH \"SEE ALSO\"\n.PP\n";
			},
			sub { # print
				print MANPAGE $sep, shift;
				$sep = ", ";
			}
		 );

		if ( defined $doc->{Author} ) {
			print MANPAGE "\n.SH \"AUTHOR\"\n.PP\n", 
				deref( $doc->{Author}),"\n";
		}
		if ( defined $doc->{Version} ) {
			print MANPAGE "\n.SH \"VERSION\"\n.PP\n", 
				deref( $doc->{Version}),"\n";
		}

	}

	print MANPAGE "\n";
	close MANPAGE;
}

sub listMember
{
	my ( $class, $m ) = @_;
	my $name = $m->{astNodeName};
	my $type = $m->{NodeType};

	print MANPAGE ".ti -1c\n.BI \"";
	if ( $type eq "var" ) {
		print MANPAGE $m->{Type}, $name;
	}
	elsif ( $type eq "method" || $type eq "fnptr" ) {
		my $flags = $m->{Flags};
		my $ret = $m->{ReturnType};
		$ret = "virtual ".$ret if $flags =~ /v/;
		$ret = "static ".$ret if $flags =~ /s/;
		$name = "(*$name)" if $type eq "fnptr";

		print MANPAGE "$ret$name (", $m->{Params},")",
				$flags =~ /c/ ? " const" : "";

	}
	elsif ( $type eq "enum" ) {
		print MANPAGE "enum $name {", $m->{Params},"}";
	}
	elsif ( $type eq "typedef" ) {
		print MANPAGE "typedef ", $m->{Type}," $name";
	}
	else {
		print MANPAGE "$type $name";
	}

	print MANPAGE "\"\n.br\n";
}

sub documentMembers
{
	Iter::DocTree( shift, 0, 0,
		sub { # common
			my ( $class, $kid ) = @_;

			if ( kdocAstUtil::hasDoc( $kid ) ) {
				documentMember( $class, $kid );
			}
		}, 
		undef, undef
	);
}

sub documentMember
{
	my ( $class, $m ) = @_;
	my $name = $m->{astNodeName};
	my $type = $m->{NodeType};
	my $qual = "";

	# full name

	print MANPAGE "\n.SS \"";
	if ( $type eq "var" ) {
		print MANPAGE $m->{Type}, $name;
	}
	elsif ( $type eq "method" || $type eq "fnptr" ) {
		my $flags = $m->{Flags};
		my $ret = $m->{ReturnType};
		$qual = "pure ".$qual if $flags =~ /p/;
		$qual = "virtual ".$qual if $flags =~ /v/;
		$qual = "static ".$qual if $flags =~ /s/;
		$name = "(*$name)" if $type eq "fnptr";

		print MANPAGE "$ret$name (", reform($m->{Params}),")",
				$flags =~ /c/ ? " const" : "";

		if ( defined $m->{Throws} ) {
			print MANPAGE " ", reform( $m->{Throws} );
		}

	}
	elsif ( $type eq "enum" ) {
		print MANPAGE "enum $name {", reform($m->{Params}),"}";
	}
	elsif ( $type eq "typedef" ) {
		print MANPAGE "typedef ", $m->{Type}," $name";
	}
	else {
		print MANPAGE "$type $name";
	}

	# qualified
	if ( $m->{Access} ne "public" ) {
		$qual .= $m->{Access};
	}

	if ( $qual ne "" ) {
		$qual =~ s/^\s+//g;
		$qual =~ s/\s+$//g;
		$qual = " [$qual]";
	}

	print MANPAGE "$qual\"\n.br\n";
	
	# warnings
	if( $m->{Internal} ) {
		print MANPAGE "Internal Use Only.\n.br\n";
	}
	if( $m->{Deprecated} ) {
		print MANPAGE "Deprecated - do not use in new code.\n.br\n";
	}

	# docs
	return unless defined $m->{DocNode};

	printDoc( $m, $m->{DocNode} );

	# extra

	my $sep = "";
	kdocDocIter::SeeAlso( $m->{DocNode}, undef,
		sub { # start
			print MANPAGE "\n.PP\n.B \"See Also:\"\n";
		},
		sub { # print
			print MANPAGE $sep, shift;
			$sep = ", ";
		}
	);
}

sub printDoc
{
	my ( $node, $docNode ) = @_;

	kdocDocIter::TextIter( $docNode,
		sub { # start
#			print MANPAGE "\n.PP\n";
		},
		undef, # end
		sub { # text
			my ( $node, $text ) = @_;
			print MANPAGE "\n.PP\n",reform( deref($text) )
				unless $text =~ /^\s$/;
		},
		sub { # ref
			my ( $node, $text ) = @_;
			print MANPAGE " \\fB".reform(deref($text))."\\fP ";
		},
		sub { # sect
			shift;
			print MANPAGE "\n.PP\n.B ", shift, "\n";
		},
		sub { # pre
			shift;
			print MANPAGE "\n.br\n.nf\n", shift, "\n.fi\n.br\n";
		},
		undef, # image
		sub { # para
#			print MANPAGE "\n.PP\n";
		},
		undef, # list start
		undef, # list end
		sub { # list
			shift;
			print MANPAGE "\n.TP\n.B o\n",reform( deref( shift ) );
		}
	);

	kdocDocIter::ParamIter( $docNode, 
		sub {
			print MANPAGE "\n.PP\n.B Parameters:";
		}, 
		undef,
		sub {
			print MANPAGE "\n.TP\n.B ",shift,"\n", reform( deref(shift) );
		} 
	);

	if( defined $docNode->{Returns} ) {
		print MANPAGE "\n.PP\n.B \"Returns:\"\n",
			reform( deref( $docNode->{Returns} ) );
	}

	my $sep = "";
	kdocDocIter::Throws( $docNode, undef,
		sub { # start
			print MANPAGE "\n.PP\n.B \"Throws:\"\n";
		},
		sub { # print
			print MANPAGE $sep, shift;
			$sep = ", ";
		}
	);
}

=head2 Utilities

=cut

sub fullName
{
	return join( "::", kdocAstUtil::heritage( shift ) );
}

sub deref
{
    my( $str ) = @_;

    $str =~ s/\@(?:ref|em|p|sect.)\s+(\S+)/\\fB$1\\fP/g;

    return $str;
}

sub reform
{
	my $width = 0;
	my @accum = ();

	foreach my $word ( split( /\s+/s, shift ) ) {
		next if $word =~ /^\s*$/s;
		if ( $width >= 70 ) {
			push @accum, "\\c\n";
			$width = 0;
		}

		push @accum, $word;
		$width = $width + length( $word ) + 1;
	}

	return join( " ", @accum );
}

1;
