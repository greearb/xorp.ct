
package kdocCxxDocbook;
use Carp;
use File::Path;
use kdocDocIter;
use kdocAstUtil;
use kdocAstSearch;
use Iter;

use strict;
no strict "subs";

use vars qw/ $lib $rootnode $outputdir $opt $paraOpen/;

=head2 kdocCxxDocBook

	TODO:

	Templates
	Fix tables, add index.
	Global docs
	Groups

=cut

sub writeDoc
{
	( $lib, $rootnode, $outputdir, $opt ) = @_;

	makeReferences( $rootnode );

	$lib = "kdoc-out" if $lib eq "";	# if no library name set

	mkpath( $outputdir) unless -f $outputdir;

	open ( DOC, ">$outputdir/$lib.sgml" ) || die "Couldn't write output.";

	my $time = localtime;

print DOC<<EOF;
<?xml version="1.0" ?>
<!DOCTYPE book PUBLIC "-//KDE//DTD DocBook XML V4.1-Based Variant V1.0//EN" "dtd/kde.dtd" [
]>
<book id="$lib-lib">
	<bookinfo>
		<date>$time</date>
		<title>$lib API Documentation</title>
	</bookinfo>
EOF

	printLibDoc();
	printHierarchy();
	printClassIndex();
	printClassDoc();

print DOC<<EOF;
</book>
EOF

}

sub printLibDoc
{
	return unless kdocAstUtil::hasDoc( $rootnode );
	
	print DOC chapter( "$lib-intro", "Introduction" );
	printDoc( $rootnode->{DocNode}, *DOC, $rootnode );
	print DOC C( "chapter" );
}


sub printHierarchy
{

	print DOC chapter( "class-hierarchy", "$lib Class Hierarchy" );

	my $first = 0;
	my @firststack = ();

	Iter::Hierarchy( $rootnode,
		sub {	# down
			print DOC "<itemizedlist>\n";
			push @firststack, $first;
			$first = 1;
		},
		sub {	# print
			my $node = shift;
			return if $node == $rootnode;

			if( $first ) {
				$first = 0;
			}
			else {
				print DOC "</listitem>";
			}

			my $src = defined $node->{ExtSource} ? " ($node->{ExtSource})":"";
			print DOC "<listitem><para>", refName($node), "$src</para>\n"
		},
		sub {	# up
			$first = pop @firststack;
			if( $first ) {
				print DOC "</itemizedlist>\n";
			}
			else {
				print DOC "</listitem></itemizedlist>\n";
			}
		}, 		# no kids
		sub { print DOC "</listitem>"; }
	);
		
	print DOC C( "chapter" ); 
}

sub printClassIndex
{
	my @clist = ();
	kdocAstUtil::makeClassList( $rootnode, \@clist );

	print DOC chapter( "class-index", "$lib Class Index" ),
		O( "table", "tocentry", 0, "pgwide", 1, "colsep", 0 ),
		tblk( "title", $lib,' classes' ),
		O( "tgroup", "cols", 2  ), O( "tbody" );

	foreach my $kid ( @clist ) {

		# Internal, deprecated, abstract
		my $name = refName( $kid );

		if ( $kid->{Abstract} ) {
			$name = tblk( "emphasis", $name );
		}

		my $extra = "";

		if ( $kid->{Internal} ) {
			$extra .= " internal";
		}

		if ( $kid->{Deprecated} ) {
			$extra .= " ".tblk( "emphasis", "deprecated" );
		}

		$extra = " [$extra]" unless $extra eq "";
		
		# print class entry
		print DOC tblk( 'row', tblk( 'entry', $name )
			. tblk( 'entry', deref( $kid->{DocNode}->{ClassShort},
				$rootnode).$extra ));
	}

	print DOC C( "tbody", "tgroup", "table", "chapter" );
}

sub printClassDoc
{
	print DOC chapter( "class-doc", "$lib Class Documentation" );

	Iter::LocalCompounds( $rootnode, sub { docChildren( shift ); } );

	print DOC C("chapter"), "\n";
}

sub docChildren
{
	my ( $node ) = @_;

	return if $node == $rootnode;
	
	## document self
	printClassInfo( $node );
	
	if ( kdocAstUtil::hasDoc( $node ) ) {
		printDoc( $node->{DocNode}, *DOC{IO}, $rootnode, 1 );
	}

	# First a member index by type
	print DOC O( "sect2" ),
		"Interface Synopsis", C( "sect2" );

	Iter::MembersByType( $node,
		sub {	# start
		print DOC O( 'table', 'pgwide', 1, "colsep", 0,
				"rowsep", 0, "tocentry", 0 ),
			tblk( 'title', fullName($node)," ",$_[0] ),
			O( "tgroup", "cols", 1 ), O( "tbody" );
		},
		\&sumListMember, # print
		sub {	# end
			print DOC C( "tbody", "tgroup", "table" ), "\n";
		},
		sub {	# no kids
			print DOC tblk( "para", "(Empty interface)" );
		}
	);

	print DOC O( "sect2" );
	print DOC "Member Documentation", C( "sect2" );

	# Then long docs for each member
	Iter::MembersByType ( $node, undef, \&printMemberDoc, undef,
		sub { print DOC tblk( "para", "(Empty interface)" ); }
	);
	
	print DOC C( "sect1" );

	return;
}

sub printClassInfo
{
	my $node = shift;

	print DOC O( "sect1", "id", $node->{DbRef},
				"xreflabel", fullName( $node, "::" ) ), "\n",
		tblk( "title", esc($node->{NodeType})," ",fullName( $node ) );


	if ( defined $node->{DocNode} &&  $node->{DocNode}->{ClassShort} ) {
		printClassInfoField( "Description", 
			deref( $node->{DocNode}->{ClassShort}, $rootnode ) );
	}

	printClassInfoField( "Header", tblk( "literal", 
			$node->{Source}->{astNodeName} ));

	my ($text, $comma ) = ("", "");

	Iter::Ancestors( $node, $rootnode, undef, undef,
		sub { # print
			my ( $node, $name, $type ) = @_;
			$name = refName( $node ) if defined $node;
			$name .= " (".$node->{ExtSource}.")" if defined $node->{ExtSource};
			$text .= $comma.$name;
			$text .= " [$type]" unless $type eq "public";

			$comma = ", ";
		},
		sub { # end
			printClassInfoField( "Inherits", $text );
		}
	);
	
	$text = $comma = "";

	Iter::Descendants( $node, undef, undef,
		sub { # print
			my $desc = shift;
			$text .= $comma.refName( $desc );
			$text .= " (".$desc->{ExtSource}.")" if defined $desc->{ExtSource};

			$comma = ", ";
		},
		sub { # end
			printClassInfoField( "Inherited By", $text );
		}
	);

	if ( $node->{Internal} ) {
		printClassInfoField( "note", "Internal use only." );
	}

	if ( $node->{Deprecated} ) {
		printClassInfoField( "note", "Deprecated, to be removed." );
	}

	return;
}

sub printClassInfoField
{
	my ( $label, $text ) = @_;

	print DOC tblk( "formalpara", tblk( "title", $label ),
			tblk( "para", " ".$text ) );
}

sub sumListMember
{
	my( $class, $m ) = @_;

	print DOC O( "row"), O( "entry" );


	my $type = $m->{NodeType};
	my $name = esc( $m->{astNodeName} );
	my $pname = tblk( "emphasis", $name );

	if( $type eq "var" ) {
		print DOC tblk( "literal", esc($m->{Type}) ), " $pname\n";
	}
	elsif( $type eq "method" || $type eq "fnptr" ) {
		my $flags = $m->{Flags};

		if ( !defined $flags ) {
			warn "Method ".$m->{astNodeName}.
				" has no flags";
		}

		my $extra = "";
		$extra .= "virtual " if $flags =~ "v";
		$extra .= "static " if $flags =~ "s";

		my $params = esc( $m->{Params} );
		$params =~ s/^\s+//g;
		$params =~ s/\s+$//g;
		$params = " $params " unless $params eq "";
		my $c = $flags =~ /c/ ? " const" : "";
		my $p = $flags =~ /p/ ? " [pure]" : "";

		$pname = "(*$pname)" if $type eq "fnptr";


		print DOC esc($m->{ReturnType}), "  $pname\($params\)$c$p\n";
	}
	elsif( $type eq "enum" ) {
		my $n = $name eq "" ? "" : $pname." ";

		print DOC tblk("literal","enum"),
			" $n\{ ",tblk("literal",esc($m->{Params}))," }";
	}
	elsif( $type eq "typedef" ) { 
		print DOC tblk( "literal", "typedef " ), esc($m->{Type}), 
				tblk( "emphasis", $name );
	}
	else { 
		# unknown type
		print DOC tblk( "literal", esc( $type ) )," $pname\n";
	}

	print DOC C( "entry" ), C( "row" ),"\n";

	return;
}

=head2 printMemberDoc

	params: classnode, membernode

	Prints title and long documentation for one class member.

=cut

sub printMemberDoc
{
	my ( $class, $mem ) = @_;

	return unless kdocAstUtil::hasDoc( $mem );

	print DOC O( "sect3", "id",	$mem->{DbRef},
			"xreflabel", fullName( $mem, "::" ) );

	# title
	my $type = $mem->{NodeType};
	my $name = esc( $mem->{astNodeName} );
	my $pname = tblk( "emphasis", $name );

	if( $type eq "var" ) {
		print DOC tblk( "literal", esc($mem->{Type}) ), " $pname\n";
	}
	elsif( $type eq "method" || $type eq "fnptr" ) {
		my $flags = $mem->{Flags};

		if ( !defined $flags ) {
			warn "Method ".$mem->{astNodeName}.
				" has no flags";
		}

		my $extra = "";
		$extra .= "virtual " if $flags =~ "v";
		$extra .= "static " if $flags =~ "s";

		my $params = $mem->{Params};
		$params =~ s/^\s+//g;
		$params =~ s/\s+$//g;
		$params = " $params " unless $params eq "";
		my $c = $flags =~ /c/ ? " const" : "";
		my $p = $flags =~ /p/ ? " [pure]" : "";

		$pname = "(*$pname)" if $type eq "fnptr";

		print DOC deref($mem->{ReturnType}, $rootnode), "  $pname\(".
			deref( $params, $rootnode )."\)$c$p\n";
	}
	elsif( $type eq "enum" ) {
		my $n = $name eq "" ? "" : $pname." ";

		print DOC tblk("literal","enum"),
			" $n\{ ",tblk("literal",esc($mem->{Params}))," }";
	}
	elsif( $type eq "typedef" ) { 
		print DOC tblk( "literal", "typedef" ), " ", esc($mem->{Type}), $pname;
	}
		# TODO nested compounds
	else { 
		# unknown type
		print DOC tblk( "literal", esc( $type ) )," $pname\n";
	}

	print DOC C( "sect3" );

	# documentation
	printDoc( $mem->{DocNode}, *DOC, $rootnode );

	if ( $type eq "method" ) {
		my $ref = kdocAstSearch::findOverride( $rootnode,
				$class, $mem->{astNodeName} );
		if ( defined $ref ) {
			print DOC tblk( "formalpara",
				tblk( "title", "Reimplemented from" ),
				tblk( "para", fullName( $ref->{Parent}, "::" )) ), "\n";
		}
	}
}

=head2 printDoc

Parameters: docnode, *filehandle, rootnode, compound

Print a doc node. If compound is specified and non-zero, various
compound node properties are not printed.

=cut

sub printDoc
{
	my $docNode = shift;
	local *CLASS = shift;

        my ( $rootnode, $comp ) = @_;
        my $node;
        my $type;
	my $text;
	my $lasttype = "none";

	$comp = 0 if !defined $comp;

	# Main body
	kdocDocIter::TextIter( $docNode,
		sub { # start
			$paraOpen = 0;
		},
		sub { # end
			print CLASS "", pc();
		},
		sub { # text
			print CLASS "", pc(), po(), deref( $_[1], $rootnode );
		},
		sub { # ref
			my ( $node, $name, $ref ) = @_;
			print CLASS defined $ref ? refName($ref) : $name;
		}, 
		sub { # sect
			print CLASS "",pc(), O(  "sect4" ),
					deref( $_[1], $rootnode ), C( "sect4" ),"\n";
		},
		sub { # pre
			print CLASS "",pc(),tblk( "programlisting", esc( $_[1] ) );
		},
		sub { # img
			print CLASS "", pc(),"<graphic fileref=\"$_[2]\"></graphic>";

		},
		undef, #para
		sub { # list start
			print CLASS "", pc(),"<itemizedlist>\n";
		},
		sub { # list end
			print CLASS "", pc(),"</itemizedlist>\n";
		},
		sub { # item
			print CLASS "", tblk( "listitem",
					tblk("para", deref($_[1], $rootnode )));
		}
	);

	# Params
	kdocDocIter::ParamIter( $docNode,
		sub { # start
			print CLASS "", O( 'table', 'pgwide', 0, "colsep", 0,
				"frame", "none", "tocentry", 0 ),
				tblk( 'title', 'Parameters' ),
				O( "tgroup", "cols", "2" ), O( "tbody" );
		},
		sub { # end
			print CLASS "", C( "tbody", "tgroup", "table" );
		},
		sub { # print
			print CLASS "<row><entry>", esc( shift ),
				"</entry><entry>", deref( shift, $rootnode ),
				"</entry></row>\n";
		}
	);

	# Return
	printTextItem( $docNode, *CLASS, "Returns" );

	# Exceptions
	$text = $docNode->{Throws};

	if ( defined $text ) {
		my $comma = "<formalpara><title>Exceptions</title><para>";

		foreach my $tosee ( @$text ) {
			print CLASS $comma, esc( $tosee );
			$comma = ", ";
		}
		print CLASS C( "para", "formalpara" );
	}

	# See
	my $comma = "";
	
	kdocDocIter::SeeAlso ( $docNode, undef,
		sub { # start
			print CLASS "", O( "tip" ),tblk( "title", "See Also"),O( "para" );
		},
		sub { # print
			my ( $label, $ref ) = @_;
			$label = defined $ref ? refName( $ref ):esc( $label );

			print CLASS $comma, $label;
			$comma = ", ";
		},
		sub { # end
			print CLASS "", C( "para", "tip" );
		}
	);
	
	printTextItem( $docNode, *CLASS, "Since" );
	printTextItem( $docNode, *CLASS, "Version" );
	printTextItem( $docNode, *CLASS, "Id" );
	printTextItem( $docNode, *CLASS, "Author" );
}

sub makeReferences
{
	my $root = shift;
	my $idcount = 0;

	return Iter::Tree( $root, 1,
		sub {
				my ( $parent, $node ) = @_;
				return if $node->{ExtSource};

				$idcount++;

				$node->{DbRef} = "docid-$idcount" ;

#				print fullName( $node ), " = ", $node->{DbRef},"\n";

				return;
		}
	);
}

sub printTextItem
{
	my $node = shift;
	local *CLASS = shift;
	my ( $prop, $label ) = @_;

	my $text = $node->{ $prop };
	
	return unless defined $text;
	$label = $prop unless defined $label;
	
	print CLASS "<formalpara><title>", $label, "</title><para> ", 
		deref( $text, $rootnode  ), "</para></formalpara>\n";
}


# utilities


sub refName
{
	my( $node ) = @_;

	return fullName( $node ) if defined $node->{ExtSource}
		|| !kdocAstUtil::hasDoc( $node );

	return '<xref linkend="'.$node->{DbRef}.'"/>';
}

sub fullName
{
	my( $node, $sep ) = @_;

	$sep = "::" unless defined $sep;

	my @heritage = kdocAstUtil::heritage( $node );
	foreach my $n ( @heritage ) { $n = esc( $n ); }

	return join( $sep, @heritage );
}

=head2 deref

	Parameters: text, rootnode
	returns text

	dereferences all @refs in the text and returns it.

=cut

sub deref
{
	my ( $str, $rootnode ) = @_;
	confess "rootnode is null" if !defined $rootnode;
	my $out = "";
	my $text;

	foreach $text ( split (/(\@\w+\s+[\w:#]+)/, $str ) ) {
		if ( $text =~ /\@ref\s+([\w:#]+)/ ) {
			my $name = $1;
			$name =~ s/^\s*#//g;
			$out .= wordRef( $name, $rootnode );
		}
		elsif ( $text =~ /\@p\s+([\w:]+)/ ) {
			$out .= tblk( "literal", esc($1) );
		}
		else {
			$out .= esc($text);
		}
	}

	return $out;
}



=head2 wordRef

	Parameters: word

	Prints a hyperlink to the word's reference if found, otherwise
	just prints the word. Good for @refs etc.

=cut

sub wordRef
{
	my ( $str, $rootnode ) = @_;
	confess "rootnode is undef" if !defined $rootnode;

	return "" if $str eq "";

	my $ref = kdocAstSearch::findRef( $rootnode, $str );
	return esc($str) if !defined $ref;

	return refName( $ref );
}


=head2 esc

	Escape special SGML characters for normal text.

=cut

sub esc
{
	my $str = $_[ 0 ];

	return "" if !defined $str || $str eq "";

	$str =~ s/&/&amp;/g;
	$str =~ s/</&lt;/g;
	$str =~ s/>/&gt;/g;

	return $str;
}


sub chapter
{
	my ( $id, $title ) = @_;

	return "<chapter id=\"$id\"><title>$title</title>";
}


=head2 tblk

	Params: tagname, text..

	Inserts text in a block tag of type tagname (ie both
	opening and closing tags)
=cut

sub tblk
{
	my $tag = shift;
	return "<$tag>".join( "", @_ )."</$tag>";
}


=head2 O

	Params: tagname, list of id and idtext interspersed.

=cut

sub O
{
	my $tag = shift;

	carp "mismatched ids and tags" if ($#_+1) % 2 != 0;

	my $out = "<$tag";

	while ( $#_ >= 0 ) {
		$out .= " ".shift( @_ ).'="'.shift( @_ ).'"';
	}

	$out .= ">";

	return $out;
}

=head2 C

	Params: list of tagnames

	returns a list of close tags in the order specified in the
	params.

=cut

sub C
{
	my $out = "";

	foreach my $tag ( @_ ) {
		$out .= "</$tag>";
	}

	return $out;
}

sub po
{
	my $text = $paraOpen ? "" : "<para>";
	$paraOpen = 1;

	return $text;
}

sub pc
{
	my $text = $paraOpen ? "</para>" : "";
	$paraOpen = 0;

	return $text;
}
1;
