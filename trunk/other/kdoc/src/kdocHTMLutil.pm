
=head1 kdocHTMLutil - Common HTML routines.

=cut

package kdocHTMLutil;

use kdocAstUtil;
use Carp;
use Iter;
use kdocDocIter;
use strict;
no strict qw/ subs/;
 
use vars qw( $VERSION @ISA @EXPORT $rcount $docNode $rootnode $comp *CLASS 
	$allowhtml %escmap %urlescmap);

BEGIN {
	$VERSION = '$Revision$';
	@ISA = qw( Exporter );
	@EXPORT = qw( makeReferences refName refNameFull refNameEvery hyper 
			esc printDoc printTextItem wordRef textRef deref 
			encodeURL newPgHeader tabRow makeHeader 
			HeaderPathToHTML writeTable makeSourceReferences B);

	$rcount = 0;
	$allowhtml = 0;

	%escmap = ( '<' => '&lt;', '>' => '&gt;', '&' => '&amp;' );
	%urlescmap = ( ':' => '%3A', '<' => '%3C', '>' => '%3E',
			' ' => '%20', '%' => '%25' );

}

sub allowHTML
{
	$allowhtml = shift;
}

## generic HTML generator routines

sub newPgHeader
{
	my ( $html, $heading, $desc, $rest, $toclist ) = @_; 
	my $bw=0;
	my $cspan = defined $main::options{"html-logo"} ? 2 : 1;
	my $stylesheet = $main::options{"html-css"};

	if ( defined $stylesheet ) {
		$stylesheet = '<LINK REL="STYLESHEET" HREF="'.$stylesheet.'">'
	}
	else {
		$stylesheet = "";
	}

	if ( $rest == "" ) {
	    $rest = "<tr><td>&nbsp;</td></tr>"
	}

	print $html <<EOF;
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
    "http://www.w3.org/TR/html4/loose.dtd">
<HTML>
<HEAD>
<TITLE>$heading</TITLE>
$stylesheet
<META NAME="Generator" CONTENT="KDOC $main::version">
<META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=ISO-8859-1">
</HEAD>
<BODY>
<DIV id="kdoc-titlebox">
<TABLE WIDTH="100%" BORDER="$bw">
<TR>
<TD>
	<TABLE BORDER="$bw">
		<TR><TD valign="top" align="left">
		<h1>$heading</h1>
		</TD>
		</TR>
                <TR>
		<TD>$desc</TD></TR>
	</TABLE>
	<HR>
	<TABLE BORDER="$bw">
		$rest
	</TABLE>
	</TD>
EOF

#	print $html '<TABLE BORDER="',$bw,'"><TR><TD>';
	my @klist = keys %$toclist;

	print $html '<TD align="right"><TABLE BORDER="',$bw,'">';

	my ($ll, $le) = ( "", "");
	if (defined $main::options{"html-logo-link"}) {
	    $ll = '<a href="' . $main::options{"html-logo-link"} . '">';
	    $le = '</a>'
	}
	# image
	print $html '<TR><TD align="right" colspan="', ($#klist)+2, '">',
	    $ll, '<IMG SRC="', $main::options{"html-logo"},
	         '" ALT="LOGO" BORDER="0">', $le, '</TD></TR>'
		if defined $main::options{"html-logo"};

	# TOC
	print $html '<TR>';
	foreach my $item ( sort @klist ) {
	    my $tocitem = ${item};
	    $tocitem =~ s/[ \t]/&nbsp;/;
	    print $html '<TD>', '<small>&nbsp;<A HREF="', $toclist->{$item},
	                '">', $tocitem, "</A></small></TD>\n";
	}
	print $html '</TR>';

	print $html "</TABLE></TD></TR></TABLE>\n";
	print $html "</DIV>\n";
}

sub writeTable
{
	my ( $file, $list, $columns ) = @_;

	my ( $ctr, $size ) = ( 0, int(($#$list+1)/$columns) );
	$size = 1 if $size < 1;

# spread out unallocated items across columns.
# The old behaviour was to dump them in the last column.
	my $s = $size * $columns;
	$size++ if $s < ($#$list+1);

	if ($#$list <= 0) {
	    return;
	}

	print $file '<DIV CLASS="listing-table">';
	print $file '<TABLE WIDTH="100%" BORDER="0"><TR>';

	while ( $ctr <= $#$list ) {
		print $file '<TD VALIGN="top" WIDTH="33%">';
		$s = $ctr+$size-1;

		if ( $s > $#$list ) {
			$s = $#$list;
		}
		elsif ( ($#$list - $s) < $columns) {
			$s = $#$list;
		}

		writeListPart( $file, $list, $ctr, $s );
		print $file "</TD>";
		$ctr = $s+1;
	}

	print $file '</TR></TABLE>';
	print $file '</DIV>';
}

=head2

	Parameters: fd, list, start index, end index

	Helper for writeClassList.  Prints a table containing a
	hyperlinked list of all nodes in the list from start index to
	end index. A table header is also printed.

=cut

sub writeListPart
{
	my( $file, $list, $start, $stop ) = @_;

	print $file "<DIV class=\"listing-subtable\">";
	print $file "<TABLE BORDER=\"0\" WIDTH=\"100%\">";

	print $file '<TR><TH>', $list->[ $start ]->{astNodeName};
	print $file "&nbsp;-&nbsp;<br>", $list->[ $stop ]->{astNodeName}, 
		"</TH></TR>";

	my $col = 0;
	my $colour = "";

	for my $ctr ( $start..$stop ) {
		$col = $col ? 0 : 1;
		$colour = $col ? 'class="lodd"' : 'class="leven"';

		print $file "<TR $colour><TD $colour>", refNameFull( $list->[ $ctr ] ),
			"</TD></TR>\n";
	}

	print $file "</TABLE>";
	print $file "</DIV> <!--\"listing-subtable\"-->";
}


=head2 makeReferences

	Parameters: rootnode

	Recursively traverses the Kids of the root node, setting
	the "Ref" property for each. This is the HTML reference for
	the node. 

	A "NumRef" property is also set for non-compound members,
	which is used for on-page links.

=cut

sub makeReferences
{
	my ( $rootnode ) = @_;

	$rootnode->{rcount} = 0 ;

	return Iter::Tree ( $rootnode, 1, 
		sub { 					# common
			my ( $root, $node ) = @_;

			$root->{rcount}++; 
			$node->AddProp( 'NumRef', "#ref".$root->{rcount} ); 

			return;
		},
		sub { 					# compound
			my ( $root, $node ) = @_;
			return if defined $node->{ExtSource};

			if ( $node == $rootnode ) {
				$node->{Ref} = "all-globals.html";
			}
			else {
				my @heritage = kdocAstUtil::heritage( $node );
				foreach my $n ( @heritage ) { $n = encodeURL( $n );	}
				$node->{Ref} = join( "__", @heritage ).".html";
			}
			$node->{rcount} = 0 ;

			return;
		},
		sub {					# member
			my ( $root, $node ) = @_;
			$node->{Ref} =  $root->{Ref}."#".encodeURL($node->{astNodeName})
				unless defined $node->{ExtSource};

			return;
		}
	);
}

sub makeSourceReferences
{
	my( $rootnode ) = shift;

	return if !exists $rootnode->{Sources};

	# Set up references

	foreach my $header ( @{$rootnode->{Sources}} ) {
		my $htmlname = HeaderPathToHTML( $header->{astNodeName} );
		$header->{Ref} = $htmlname ;
	}


}


=head2 refName

	Parameters: node, refprop?

	Returns a hyperlinked name of the node if a reference exists,
	or just returns the name otherwise. Useful for printing node names.

	If refprop is specified, it is used as the reference property
	instead of 'Ref'.

=cut

sub refName
{
	my ( $node, $ref ) = @_;
	confess "refName called with undef" unless defined $node->{astNodeName};

	$ref = 'Ref' unless defined $ref;

	$ref = $node->{ $ref };

	my $out;

	if ( !defined $ref ) {
		$out =  $node->{astNodeName};
	} else {
		$out = hyper( $ref, $node->{astNodeName} );
	}

	$out = "<i>".$out."</i>" if exists $node->{Pure};

	return $out;

}

=head2 refNameFull

	Parameters: node, rootnode, refprop?

	Returns a hyperlinked, fully qualified (ie including parents)
	name of the node if a reference exists, or just returns the name
	otherwise. Useful for printing node names.

	If refprop is specified, it is used as the reference property
	instead of 'Ref'.

=cut

sub refNameFull
{
	my ( $node, $rootnode, $refprop ) = @_;

	my $ref = defined $refprop ? $refprop : 'Ref';
	$ref = $node->{ $ref };
	my $name = join( "::", kdocAstUtil::heritage( $node ) );

	my $out;

	if ( !defined $ref ) {
		$out = $name;
	} else {
		$out = hyper( $ref, $name );
	}

	$out = "<i>".$out."</i>" if exists $node->{Pure};

	return $out;
}


=head2 refNameEvery

	Parameters: node

	Like refNameFull, but every separate link in the chain is
	referenced.

=cut

sub refNameEvery
{
	my ( $node, $rootnode ) = @_;



	# make full name
	my $name = $node->{astNodeName};

	my $parent = $node->{Parent};

	while ( $parent != $rootnode ) {
		$name = refName($parent)."::".$name;
		$parent = $parent->{Parent};
	}

	return $name;
}

=head2 hyper

	Parameters: hyperlink, text

	Returns an HTML hyperlink. The text is escaped.

=cut

sub hyper
{
	confess "hyper: undefed parameter $_[0], $_[1]"
		unless defined $_[0] && defined $_[1];
	return "<A HREF=\"$_[0]\">".esc($_[1])."</A>";
}


sub B
{
		my $tag = shift;

		return "<$tag>". join( "", @_). "</$tag>";
}

=head2 esc

	Escape special HTML characters.

=cut

sub esc
{
	return $_[0] if $allowhtml;

	my $str = $_[0];

	return "" if !defined $str || $str eq "";

	$str =~ s/([<>&])/$escmap{$1}/g;

#	$str =~ s/&/&amp;/g;
#	$str =~ s/</&lt;/g;
#	$str =~ s/>/&gt;/g;

	return $str;
}


=head2 printDoc

	Parameters: docnode, *filehandle, rootnode, compound

	Print a doc node. If compound is specified and non-zero, various
	compound node properties are not printed.

=cut

sub printDoc
{
	my $docNode = shift;
	local ( *CLASS, $rootnode ) = @_;
        my ( $comp ) = @_;

        my $type;
	my $lasttype = "none";

	$comp = defined $comp? $comp : 0;

	if ( defined $docNode->{Main} ) {
		print CLASS "<H2>", 
			deref( $docNode->{Main}, $rootnode ), "</H2>\n";
	}

	kdocDocIter::TextIter( $docNode,
		sub { #start
			print CLASS "<p>";
		},
		sub { #end
			print CLASS "\n";
		},
		sub { #text
			print CLASS "",deref( $_[1], $rootnode );
		},
		sub { #ref
			my ( $node, $name, $ref ) = @_;
			print CLASS defined $ref ? refName( $ref ) : $name;
		},
		sub { #sect
			print CLASS "</p>\n\n<H3>",
					deref( $_[1], $rootnode),"</H3>\n<p>\n";
		},
		sub { #pre
			my ( $node, $name, $desc ) = @_;
			$name = textRef( $name, $rootnode );
			$desc = defined $desc ?
				"<tr><td><small>$desc</desc></td></tr>" : "";

			print CLASS<<EOF;
</p>
<pre>
$name
</pre>

<p>
$desc
<p>
EOF
		},
		sub { #image
			print CLASS "</p>\n<img src=\"", $_[2], "\">\n<p>\n";
		},
		sub { #para
			print CLASS "</p>\n<p>";
		},
		sub { #list start
			print CLASS "</p>\n\n<ul>\n";
		},
		sub { #list end
			print CLASS "</ul>\n\n<p>\n";
		},
		sub { #item
			print CLASS "<li>", deref( $_[1], $rootnode ), "</li>\n";

		}
	);

# Params
		kdocDocIter::ParamIter( $docNode,
			sub {
				print CLASS "<p><b>Parameters</b>:",
					"<TABLE BORDER=\"0\">\n";
			},
			sub {
				print CLASS "</TABLE>\n";
			},
			sub {
				my ( $name, $text ) = @_;
				print CLASS "<TR><TD align=\"left\" valign=\"top\"><i>",
				$name, "</i></TD><TD align=\"left\" valign=\"top\">",
				deref($text, $rootnode ), "</TD></TR>\n";
			}
		);

	# Return
	printTextItem( $docNode, *CLASS, "Returns" );

	# Exceptions
	my $text = $docNode->{Throws};

	if ( defined $text ) {
		my $comma = "<p><b>Throws</b>: ";

		foreach my $tosee ( @$text ) {
			print CLASS $comma, esc( $tosee );
			$comma = ", ";
		}
		print CLASS "</p>\n";
	}

	# See
	my $comma = "";

	kdocDocIter::SeeAlso ( $docNode, undef,
		sub { # start
			print CLASS "<p><b>See also</b>: ";
		},
		sub { # print
			my ( $label, $ref ) = @_;
			$label = refName( $ref ) if defined $ref;

			print CLASS $comma, $label;
			$comma = ", ";
		},
		sub { # end
			print CLASS "</p>\n";
		}
	);
	return if $comp;

	printTextItem( $docNode, *CLASS, "Since" );
	printTextItem( $docNode, *CLASS, "Version" );
	printTextItem( $docNode, *CLASS, "Id" );
	printTextItem( $docNode, *CLASS, "Author" );
}

=head2 printTextItem

	Parameters: node, *filehandle, prop, label

	If prop is set, it prints the label and the prop value deref()ed.

=cut

sub printTextItem
{
	my $node = shift;
	local *CLASS = shift;
	my ( $prop, $label ) = @_;

	my $text = $node->{ $prop };
	
	return unless defined $text;
	$label = $prop unless defined $label;
	
	print CLASS "<p><b>", $label, "</b>: ", 
			deref( $text, $rootnode  ), "</p>\n";
}


=head2 wordRef

	Parameters: word

	Prints a hyperlink to the word's reference if found, otherwise
	just prints the word. Good for @refs etc.

=cut

sub wordRef
{
	my ( $str, $rootnode ) = @_;
	my $out;

	confess "rootnode is undef" if !defined $rootnode;

	return "" if $str eq "";

	my $ref = kdocAstSearch::findRef( $rootnode, $str );

	return esc($str) if !defined $ref;

	if ( defined $ref->{Ref} ) {
		$out = hyper( $ref->{Ref}, $str );
	}
	else {
		warn fullName( $ref ). " hasn't a reference.";
		$out = $str;
	}

	return $out;
}

=head2 textRef

	Parameters: string
	Returns: hyperlinked, escaped text.

	Tries to find a reference for EVERY WORD in the string, replacing it
	with a hyperlink where possible. All non-hyper text is escaped.

	Needless to say, this is quite SLOW.

=cut

sub textRef
{
	my ( $str, $rootnode ) = @_;

	$str = esc( $str );
	$str =~ s/([\w:]+)/&wordRef($1,$rootnode)/ge;
	
	return $str;
}

=head2 deref

	Parameters: text
	returns text

	dereferences all @refs in the text and returns it.

=cut

sub deref
{
	my ( $str, $rootnode ) = @_;
	confess "rootnode is null" if !defined $rootnode;
	my $out = "";
	my $text;

# escape @x commands
	foreach $text ( split (/(\@\w+(?:\s+(?:[#\w:~_]+)|\{[^}]+\}))/, $str ) ) {
# check whether $text is an @command or the text between
# @commands
		if (  $text =~ /\@(\w+)(?:\s+([#\w:~_]+)|\s*\{([^}]+)\})/ )   {
			my $command = $1;
			my $content = $2 . $3;

# @ref -- cross reference
			if ( $command eq "ref" ) {
				$content =~ s/\s*#//g;
				$out .= wordRef( $content, $rootnode );
			}

# @p  -- typewriter
			elsif ( $command eq "p" ) {
				$out .= "<code>".esc($content)."</code>";
			}

# @em -- emphasized
			elsif ( $command eq "em" ) {
				$out .= "<em>".esc($content)."</em>";
			}

# @sect1-4 -- section header
			elsif ( $command =~ /sect([1-4])$/ ) {
				$out .= "<h$1>".esc($content)."</h$1>";
			}

# unknown command. warn and copy command
			else {
				warn "Unknown inline tag @", $command, "\n";
				$out .= esc($text);
			}
		}
# no @x command, thus regular text
		else {
			$out .= esc($text);
		}
	}

	return $out;
}

=head2 encodeURL

	Parameters: url

	Returns: encoded URL

=cut

sub encodeURL
{
	my $url = shift;
	my $pfx = "";

	if ( $url =~ /^\s*(\w+:)/ ) {
		$pfx = $1;
		$url =~ s/^$pfx//;
	}

	$url =~ s/([:<> %])/$urlescmap{$1}/g;

	return $pfx.$url;
}

=head2 tabRow

	Returns a table row with each element in the arg list as
	one cell.
	
=cut

sub tabRow
{
	return "<TR><TH>$_[0]</TH><TD>$_[1]</TD></TR>\n";
}

=head2 makeHeader

	Writes an HTML version of a file.

=cut

sub makeHeader
{
	my ( $out, $filename ) = @_;

	open ( SOURCE, "$filename" ) || die "Couldn't read $filename\n";

	print $out "<pre>\n";

	while ( <SOURCE> ) {
		print $out esc( $_ );
	}

	print $out "</pre>\n";
}

=head2 HeaderPathToHTML

	Takes the path to a header file and returns an html file name.

=cut

sub HeaderPathToHTML
{
	my ( $path ) = @_;

	$path =~ s/_/__/g;
	$path =~ s/\//___/g;
	$path =~ s/\./_/g;
	$path =~ s/:/____/g;

	return $path.".html";
}

# for printing debug node.

sub fullName
{
		return join( "::", kdocAstUtil::heritage( shift ) );
}

1;
