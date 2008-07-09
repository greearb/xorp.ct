package kdoctexi;

# kdoctexi.pm -- C++ TexInfo output module for KDOC.
#		Copyright (C) 1998, Bernd Gehrmann
# $Id$

# The KDOC package is distributed under the GNU Public License.

use Ast;
use kdocAstUtil;
use kdocAstSearch;
use File::Path;
use File::Basename;
use Iter;
use kdocDocIter;

use strict;

use vars qw/ $lib $rootnode $outputdir @clist $depth /;

=head1 kdoctexi

	TexInfo output module.

=cut
    
sub writeDoc 
{
        ( $lib, $rootnode, $outputdir ) = @_;
  
        print "Generating texinfo documentation.\n" unless $main::quiet;

	mkpath( $outputdir ) unless -f $outputdir;

        makeClassList( $rootnode );
        writeMain();
        writeHierarchy();
        writeOverview();
        writeClasses();

}



sub makeClassList
{
	my ( $rootnode ) = @_;

	@clist = ();

	foreach my $node ( @ {$rootnode->{Kids}} ) {
		if ( !defined $node ) {
			print "makeClassList: undefined child in rootnode!\n";
			next;
		}

		push( @clist, $node ) unless exists $node->{ExtSource}
				|| !exists $node->{Compound};
	}

	@clist = sort { $a->{astNodeName} cmp $b->{astNodeName}  }
			@clist;
}



sub writeMain {

        open( TEXMAIN, ">$outputdir/$lib.texi" ) 
	  || die "Couldn't write to $outputdir/$lib-main.texi\n";

        print TEXMAIN<<EOF;
\\input texinfo
\@c %**start of header
\@comment ----- automatically generated - do not edit! -----
\@afourpaper
\@setfilename $lib.info
\@settitle $lib documentation
\@headings on
\@setchapternewpage on
\@c %**end of header

\@include $lib-hier.tex
\@include $lib-overvw.tex
\@include $lib-inc.tex

\@page
\@comment \@printindex fn
\@shortcontents
\@bye
EOF

        close( TEXMAIN );
}



sub writeHierarchy 
{
	my @bullets = ( '@bullet', '*', '+', '-' );
	my $depth = 0;

        open( TEX, ">$outputdir/$lib-hier.tex" ) 
	  || die "Couldn't write to $outputdir/$lib-hier.tex\n";
	
	print TEX "\@comment ----- automatically generated - do not edit! -----\n";
	print TEX "\@unnumbered $lib Class Hierarchy\n\n";

	Iter::Hierarchy( $rootnode,
		sub { # down
			my $bullet = $bullets[ $depth ];
			$bullet = "-" unless defined $bullet;
			++$depth;

			print TEX "\@itemize $bullet\n";
		},
		sub { # print
			my ($node) = @_;
			return if $node == $rootnode;

			my $src = defined $node->{ExtSource} ?
				" (from $node->{ExtSource})" : "";
			my $item = $depth > 0 ? "\@item\n" : "";

			print TEX "$item\@code{",$node->{astNodeName},"$src}\n";
		},
		sub { # up
			print TEX "\@end itemize\n";
			--$depth;
		}
	);
		
	close TEX;
}


sub writeOverview 
{
	open( TEX, ">$outputdir/$lib-overvw.tex" ) 
	  || die "Couldn't write to $outputdir/$lib-classes.tex\n";
	
	print TEX "\@comment ----- automatically generated - do not edit! -----\n";
	print TEX "\@unnumbered $lib Library Overview\n\n";

	if ( defined $rootnode->{DocNode} ) {
		writeDescription( $rootnode->{DocNode} );
	}


	print TEX "\@table \@asis\n";
	
	foreach my $node ( @clist ) {

                my $docnode = $node->{DocNode};

		my $short = "";
		if ( defined $docnode && exists $docnode->{ClassShort} ) {
		        $short = formatText($docnode->{ClassShort});
		}

		my $className = $node->{astNodeName};

                # The following is really a dirty hack
                # Find something better!
		print TEX "\@item \@code{$className ";
	        print TEX "@ @ @ @ @ @ @ @ @ @ @ @ @ }", "\n";
		print TEX escape($short), "\n";
	}
						  
	print TEX "\@end table\n";
	close TEX;
}



sub writeClasses 
{
	open( TEXINC, ">$outputdir/$lib-inc.tex" ) 
	  || die "Couldn't write to $outputdir/$lib-inc.tex\n";
	print TEXINC "\@comment ----- automatically generated - do not edit! -----\n";
	
	foreach my $node ( @clist ) {

		next if !defined $node->{Compound} 
		  || defined $node->{ExtSource};

		my $className = $node->{astNodeName};

		print TEXINC "\@include $className.tex\n";
		open( TEX, ">$outputdir/$className.tex" ) 
		  || die "Couldn't write to $outputdir/$className.tex\n";
		print TEX "\@comment ----- automatically generated - do not edit! -----\n";

		writeClass( $node );
		close TEX;
	}
	
	close TEXINC;
}



sub writeClass 
{
	my ( $node ) = @_;

	my $docnode = $node->{DocNode};
	my $header = $node->{Source}->{astNodeName};
	my $className = $node->{astNodeName};
	
	my $classType = (exists $node->{Tmpl} )? "Template" : "Class";
	
	print TEX<<EOF;
\@unnumbered \@code\{$className\} $classType Reference \@i\{($header)\}
\@findex $className
EOF
	
	if ( exists $docnode->{Deprecated} ) {
		print TEX "\@noindent\n\@b{Deprecated Class}\n";
	}
	
	if ( exists $docnode->{Internal} ) {
		print TEX "\@noindent\n\@b{For internal use only}\n";
	}

	if ( exists $node->{Pure} ) {
		print TEX "\@noindent\n\@b{Contains pure virtuals}\n";
	}
	
	if ( $classType eq "template" ) {
		print TEX "\@table \@asis\n\@item Template form:\n",
		"\@code{template <", escape( $node->{Tmpl} ),
		"> ", $className, "}\n\@end table\n";
	}
	
	my $comma = "";

	Iter::Ancestors( $node, $rootnode, undef,
		sub { # start
			print TEX "\@table \@asis\n\@item Inherits:\n"
		},
		sub { # print
			my ( $n, $name, $type, $template ) = @_;
			my $source;

			if ( defined $n ) {
				$source = defined $n->{ExtSource} ?
				" ($n->{ExtSource})" : "";
				$name = $n->{astNodeName};
			}

			print TEX $comma, escape( $name ), $source, "\n";
		},
		sub { #end
			print TEX "\@end table\n";
		}
	);
	
	Iter::Descendants( $node, undef,
		sub { #start
			print TEX "\@table \@asis\n\@item Inherited by:\n";
			$comma = "";
		},
		sub { #print
			my ($in) = @_;
			my $source;
			$source = defined $in->{ExtSource} ?
				" ($in->{ExtSource})\n" :"\n";

			print TEX $comma, escape( $in->{astNodeName} ),
				$source;
		},
		sub { #end
			print TEX "\n\@end table\n";
		}
	);

        my $author   = $docnode->{Author};
        my $version  = $docnode->{Version};
        my $classSee = $docnode->{See};

	if( $author ne "" || $version ne "" || $classSee ne "" ) {
		my $escVersion = escape( $version );
		my $escAuthor = $author;
		$escAuthor =~ s/<(.+\@\@.+)>/\@email{$1}/mg;
		$escAuthor =~ s/\((.+\@\@.+)\)/\@email{$1}/mg;
		$escAuthor = formatText( $escAuthor );

		my $escSee = escape( $classSee );
		print TEX "\@table \@asis\n";
		print TEX "\@item Version:\n$escVersion\n" unless $escVersion eq "";
		print TEX "\@item Author:\n$escAuthor\n" unless $escAuthor eq "";		
		print TEX "\@item See Also:\n$escSee\n" unless $escSee eq "";		
		print TEX "\@end table\n";
	}
	
	writeDescription( $docnode );
	
	Iter::MembersByType( $node,
		sub {	#startgroup
			print TEX "\@unnumberedsec $_[0]\n";
		},
		sub {	# member
			my ( $node, $kid ) = @_;

			writeMember( $node, $kid );
		}
	);
}

sub writeMember 
{
	my ( $classnode, $member ) = @_;
	my $className = $classnode->{astNodeName};
	my $memberName = $member->{astNodeName};
        my $escName = escape( $memberName );
	my $type = $member->{NodeType};
        $_ = $type;
        SWITCH: {
		
		if( /^method/ ) {
			my $escRetType = escape( $member->{ReturnType} );
			my $escParams = escape( $member->{Params} );
			my $flags = $member->{Flags};
			my $docNode = $member->{DocNode};

			if ( $memberName eq $className ) {
				print TEX "\@deftypefn Constructor {} $escName {($escParams)}\n";
			}
			elsif ( $memberName eq "~$className" ) {
				print TEX "\@deftypefn Destructor {} $escName {($escParams)}\n";
			}
                        elsif ( $flags =~ /s/ ) {
				print TEX "\@deftypefn {Static Method} {$escRetType} $escName ",
				"{($escParams)}\n";
			}
			else {
				print TEX "\@deftypefn Method {$escRetType} $escName ",
				"{($escParams)}", $flags =~ /c/? " const\n" : "\n";
			}
			
			my $ref = kdocAstSearch::findOverride( $rootnode, 
					$classnode, $member->{astNodeName} );
			if ( defined $ref ) {
			        print TEX "Reimplemented from ",
								fullName($ref->{Parent}),"\n";
			}

			writeMemberInfo( $docNode ) if defined $docNode;
			
			kdocDocIter::ParamIter( $docNode,
				sub { # start
					print TEX "\@noindent\nParameters:\n",
						"\@multitable \@columnfractions .1 .15 .75\n";
				},
				sub { # end
					print TEX "\@end multitable\n";
				},
				sub { # print
					print TEX "\@item \@tab ",escape(shift)," \@tab ",
							escape(shift), "\n";
				}
			);
				
			my $returns = $docNode->{Returns};
			print TEX "\@noindent\nReturns: ", 
			escape($returns), "\n" unless $returns eq "";

			my $exceptions = $docNode->{Throws};
			if ( $exceptions ne "" ) {
				print TEX "\@noindent\n\@table ",
				"\@asis\n\@item Throws:\n",
				escape($exceptions), "\n\@end table\n";
			}

			print TEX "\@end deftypefn\n\n";
			last SWITCH;
		} # /^method/
		
		if ( /^var/ ) {
			my $type = $member->{Type};
			print TEX "\@deftypevar ", escape($type), " $escName\n";
			writeMemberInfo( $member->{DocNode} );
			print TEX "\@end deftypevar\n";
			last SWITCH;
		} # /^var/
		
		if( /^typedef/ ) {
			my $escType = escape( $member->{Type} );
			print TEX "\@deftp {typedef} {$escType} $escName\n";
			writeMemberInfo( $member->{DocNode});
			print TEX "\@end deftp\n";
			last SWITCH;
		} # /^typedef/
		
		if ( /^enum/ ) {
			# I'm not really convinced that it makes sense to list all constants.
			# Normally, I would expect that all constants are explained in the
			# doc comment, embedded in a <ul>..</ul> list.
			my $escConstants = escape( $member->{Params} );
			$escConstants =~ s/\n+/\n/g;
                        my $enumName = ($escName =~ /^\s*$/)? 
				"(Anonymous)" : $escName;
			print TEX "\@deftp Enumeration \@code{enum} $enumName\n";
			print TEX "\@example\nenum $enumName \@{\n",
			"$escConstants\@};\n\@end example\n";
			writeMemberInfo( $member->{DocNode} );
			print TEX "\@end deftp\n";
			last SWITCH;
		} # /^enum/
		
	} # SWITCH
	
}



sub writeDescription 
{
	kdocDocIter::TextIter( shift,
		undef,#start
		sub { #end
			print TEX "\n";
		},
		sub { #text
			print TEX formatText( $_[1] );
		},
		sub { #ref
			print TEX formatText( $_[1] );
		},
		sub { #sect
		},
		sub { #pre
			print TEX "\@example\n",escape($_[1]),"\n\@end example\n";
		},
		undef, #image
		sub { #para
			print TEX "\n\n";
		},
		sub { #list start
			print TEX "\@itemize \@bullet\n";
		},
		sub { #list end
			print TEX "\@end itemize\n";
		},
		sub { #item
			print TEX "\@item ", formatText($_[1]), "\n";
		}
	);
}



sub writeMemberInfo 
{

        my ( $docnode ) = @_;

	writeDescription( $docnode );
	
#	if( $MethDeprecated ) {
#		print TEX "\@noindent\n\@b{Deprecated Member}\n";
#	}
#	
#	if( $MethInternal ) {
#		print TEX "\@noindent\n\@b{For internal use only}\n";
#	}
	
}


# This is for conversion of general, non-preformatted text

sub formatText 
{
	my ( $text ) = @_;
	
	$text = escape($text);
	# After escape, @ is @@
	$text =~ s/\@\@ref\s+([\w\d_~]+)(#|::)([\w\d_~]+)/$1::$3/g;
        $text =~ s/\@\@ref\s+(#|::)?([\w\d_~]+)/$2/g;
	$text =~ s/\@\@see\s+([\w\d_~]+)(#|::)([\w\d_~]+)/$1::$3/g;
        $text =~ s/\@\@see\s+(#|::)?([\w\d_~]+)/$2/g;
	$text =~ s/<em>/\@emph\{/ig;
	$text =~ s/<\/em>/\}/ig;
	$text =~ s/<strong>/\@strong\{/ig;
        $text =~ s/<\/strong>/\}/ig;
        $text =~ s/<dfn>/\@dfn\{/ig;
        $text =~ s/<\/dfn>/\}/ig;
        $text =~ s/<code>/\@code\{/ig;
        $text =~ s/<\/code>/\}/ig;
        $text =~ s/<samp>/\@samp\{/ig;
        $text =~ s/<\/samp>/\}/ig;
        $text =~ s/<kbd>/\@kbd\{/ig;
        $text =~ s/<\/kbd>/\}/ig;
        $text =~ s/<var>/\@var\{/ig;
        $text =~ s/<\/var>/\}/ig;
        $text =~ s/<cite>/\@cite\{/ig;
        $text =~ s/<\/cite>/\}/ig;
        $text =~ s/<i>/\@i\{/ig;
        $text =~ s/<\/i>/\}/ig;
        $text =~ s/<b>/\@b\{/ig;
        $text =~ s/<\/b>/\}/ig;
        $text =~ s/<tt>/\@t\{/ig;
        $text =~ s/<\/tt>/\}/ig;

        $text =~ s/<br>/\@*/ig;
        $text =~ s/<p>/\n/ig;
        $text =~ s/<\/p>//ig;
			 
        $text =~ s/&lt;/</g;
        $text =~ s/&gt;/>/g;
        $text =~ s/&amp;/&/g;
        $text =~ s/&nbsp;/\@ /g;
        $text =~ s/&copy;/\@copyright{}/g;

# Support for other languages?
        $text =~ s/&auml;/\@"a/g;
        $text =~ s/&ouml;/\@"o/g;
        $text =~ s/&uuml;/\@"u/g;
        $text =~ s/&Auml;/\@"A/g;
        $text =~ s/&Ouml;/\@"O/g;
        $text =~ s/&Uuml;/\@"U/g;
        $text =~ s/&szlig;/\@ss/g;

        $text =~ s/^\s*<blockquote>\s*$/\@quotation/mig;
        $text =~ s/^\s*<\/blockquote>\s*$/\@end quotation/mig;
        $text =~ s/^\s*<ul>\s*$/\@itemize \@bullet/mig;
        $text =~ s/^\s*<\/ul>\s*$/\@end itemize/mig;
        $text =~ s/^\s*<ol>\s*$/\@enumerate/mig;
        $text =~ s/^\s*<\/ol>\s*$/\@end enumerate/mig;
        $text =~ s/^\s*<dl>\s*$/\@table \@asis/mig;
        $text =~ s/^\s*<\/dl>\s*$/\@end table/mig;
        $text =~ s/^\s*<li>/\@item /mig;
        $text =~ s/^\s*<\/li>//mig;
        $text =~ s/^\s*<dt>/\@item /mig;
        $text =~ s/^\s*<\/dt>//mig;
        $text =~ s/^\s*<dd>//mig;
        $text =~ s/^\s*<\/dd>//mig;

        return $text;
}

sub fullName
{
	return join( "::", kdocAstUtil::heritage( shift ) );
}

# This is for preformatted or similar text
sub escape 
{
	my( $text ) = @_;
	
	$text =~ s/([\@\{\}])/\@$1/g;

# i18n support
#  $text =~ s/ä/\@"a/g;
#  $text =~ s/ö/\@"o/g;
#  $text =~ s/ü/\@"u/g;
#  $text =~ s/Ä/\@"A/g;
#  $text =~ s/Ö/\@"O/g;
#  $text =~ s/Ü/\@"U/g;
#  $text =+ s/ß/\@ss{}/g;

	return $text;
}

#

sub deref
{
	my ( $str ) = @_;
	my $out = "";
	my $text;

	$str =~ s/\@\@ref\s+([\w\d_~]+)(#|::)([\w\d_~]+)/$1::$3/g;
        $str =~ s/\@\@ref\s+(#|::)?([\w\d_~]+)/$2/g;
        $out = $str;

#	foreach $text ( split (/(\@ref\s+[\w:#]+)/, $str ) ) {
#		if ( $text =~ /\@ref\s+([\w:#]+)/ ) {
#			$out .= $1;
#		}
#		else {
#			$out .= $text;
#		}
#	}
	
	return $out;
}

1;
