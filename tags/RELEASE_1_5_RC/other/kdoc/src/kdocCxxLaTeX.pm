package kdocCxxLaTeX;

use File::Path;
use File::Basename;

use Carp;
use Ast;
use kdocAstUtil;
use kdocAstSearch;
use kdocUtil;
use Iter;
use kdocDocIter;
use strict;
no strict qw/subs/;

use vars qw/@clist $gentext $debug $lib $rootnode $outputdir $opt/;

=head1 kdocCxxLaTex

=cut

BEGIN
{


	my $host = kdocUtil::hostName();
	my $who = kdocUtil::userName();
	my $now = localtime;
	chomp $now;

	$gentext = "$who on $host, $now.";

	@clist = ();
}


sub writeDoc
{
	( $lib, $rootnode, $outputdir, $opt ) = @_;
	$debug = $main::debug;

	makeRef( $rootnode );
	kdocAstUtil::makeClassList( $rootnode, \@clist );

	startDoc();

	writeAnnotatedList();
	writeHier( $rootnode );
	writeGlobalDoc();

	Iter::LocalCompounds( $rootnode, sub { writeClassDoc( shift ); } );
#	writeHeaders();
	finishDoc();
}


sub startDoc
{
	mkpath( $outputdir ) unless -f $outputdir;

	open(MAIN, ">$outputdir/main.tex") 
		|| die "Couldn't create $outputdir/main.tex\n";
    
  print MAIN<<EOF;
\\documentclass[a4paper,10pt]{article}

% Geometria strony (twoside ?)
\\usepackage[a4paper,hmargin={2cm,2cm},vmargin={2cm,2cm}]{geometry}

\\usepackage[latin2]{inputenc}

\\usepackage{longtable}
\\fontfamily{cms}

% Specyficzne listingi
\\usepackage{listings}
%\\keywordstyle{\\bfseries\\sffamily}
%\\blankstringtrue
%\\prelisting{\\small\\sffamily}

% Przynajmniej nie bêdzie krzycza³
% \\catcode`\\_=12

% Odstêpy miêdzy akapitami itp
\\setlength{\\parindent}{0cm}
\\addtolength{\\parskip}{1ex}

\\title{$lib Reference Documentation}

\\begin{document}

\\maketitle
\\tableofcontents

\\begin{abstract}
$lib Reference Documentation.

Generated: $gentext.
\\end{abstract}

EOF

}

sub finishDoc
{
	print MAIN "\n\\end{document}\n";
	close MAIN;
}


sub writeGlobalDoc
{
	my $file = "$outputdir/all-globals.tex";


	Iter::GlobalsBySourceFile( $rootnode, 
		sub { # sS
			open( CLASS, ">$file" ) || die "Couldn't create $file";
			print CLASS sectionHeader( "$lib Globals" ),
				"\n\\subsection*{Index by file}\n";

			print MAIN "\\input{all-globals.tex}\n\n";
		},
		undef,
		sub { # sFileS
			print CLASS "\n\\subsubsection*{", esc(shift ), "}\n",
				"\\begin{itemize}\n";
		},
		sub { print CLASS "\n\\end{itemize}\n"; },
		sub { # sGlobalP
			my $node = shift;
			printMemberIndex( $node->{Parent}, $node );
		}
	);

	my $currSource = undef;

	Iter::GlobalsBySourceFile( $rootnode, 
		sub { print CLASS "\n\\subsection*{Detailed Description}\n"; },
		sub { # sE
			close CLASS;
		},
		sub { # sFileS
			$currSource = shift;
		},
		undef,
		sub { # sGlobalP
			my $global = shift;
			return unless kdocAstUtil::hasDoc( $global );

			printMemberName( $global );
			print CLASS "\n\n{\\small\\verb!#include <$currSource>!}\n\n";
			printDoc( $global->{DocNode}, *CLASS, $rootnode );
		}
	);
}

=head2 writeAnnotatedList

	Parameters: rootnode

	Writes out a list of classes with short descriptions to
	index-long.tex.

=cut

sub writeAnnotatedList
{
	my $rootnode = shift;
	my $short;

	open(CLIST, ">$outputdir/index-long.tex") 
		|| die "Couldn't create $outputdir/index-long.tex\n";

	print MAIN "\\input{index-long.tex}\n\n";

	print CLIST sectionHeader( "$lib Class List" ),
			"\n\\begin{longtable}{lp{8cm}}\n";

	foreach my $node ( @clist ) {
		if ( !defined $node ) {
			warn "undef in class list.";
			next;
		}

		my $docnode = $node->{DocNode};
		$short = "";

		if( defined $docnode && exists $docnode->{ClassShort} ) {
			$short = deref($docnode->{ClassShort}, $rootnode );
			if( !defined $short ) {
				print $node->{astNodeName}, "has undef short\n";
				next;
			}
		}

		print CLIST refName($node), ' & ', $short, 
			"\\dotfill\\pageref{", $node->{TeXRef}, "}\\\\\\\\\n";                
	}

	print CLIST "\\end{longtable}\n";
	close CLIST;
}


=head2 writeHier

	p: node

	Writes out the class hierarchy index to hier.tex.

=cut

sub writeHier
{
	my $root = shift;

	open( HIER, ">$outputdir/hier.tex") 
		|| die "Couldn't create $outputdir/hier.tex\n";

	print MAIN "\\input{hier.tex}\n\n";

	print HIER sectionHeader( "$lib Class Hierachy" );

	my $level = 1;

	Iter::Hierarchy( $root,
		sub { #sLdown
			$level = $level + 1;
		},
		sub { #sP
			my $node = shift;
			return if $node == $root;

			my $src = defined $node->{ExtSource} ?
				" {\\small (".$node->{ExtSource}.")}" : " ";
			print HIER "\\verb!", '   ' x $level, "!",
                   refName( $node ),"$src\\\\\n";
		},
		sub { #sLup
			$level = $level - 1;
		},
	);

	close HIER;
}

=head2 writeClassDoc

	p: compound node

=cut

sub writeClassDoc
{
	my $node = shift;
	my $fullname = join( "::", kdocAstUtil::heritage( $node ) );

	my $file = "$outputdir/$fullname.tex";

	open( CLASS, ">$file" ) || die "Couldn't create $file";

	print MAIN  "\\input{$fullname.tex}\n\n";

	my $doc = $node->{DocNode};
	my $short = "";
	my $version;
	my $author;

	if( kdocAstUtil::hasDoc( $node ) ) {
		$short .= esc($doc->{ClassShort}) if defined $doc->{ClassShort};
		$short .= "\n\n\\textbf{Deprecated.}" if defined $node->{Deprecated};
		$short .= "\n\n\\textbf{Internal use only.}" 
				if defined $node->{Internal};
				
		$version = esc($doc->{Version}) if defined $doc->{Version};
		$author = esc($doc->{Author}) if defined $doc->{Author};
	}

	$short .= "\n\n\\textbf{Abstract class.}" if defined $node->{Pure};
	$short .= "\n\n\\verb!#include <".$node->{Source}->{astNodeName}
				.">!\n\n" if $node->{NodeType} ne "namespace";

	if ( exists $node->{Tmpl} ) {
		$short .= "\n\nTemplate form: \\verb!<$node->{Tmpl}> $fullname!\n\n";
	}

	my $comma = "Inherits: ";
	Iter::Ancestors( $node, $rootnode, undef, undef,
		sub { # sAncesP
			my ( $ances, $name, $type, $template ) = @_;
			$short .= $comma.esc($name);
			$short .= esc("<$template>") if defined $template;
			$short .= " {\\small\\[$type\\]}" if $type ne "public";
			$short .= " {\\small($ances->{ExtSource})}" if defined $ances;

			$comma = ", ";
		},
		sub { # sE
			$short .= "\n\n";
		}
	);

	$comma = "Inherited by: ";
	Iter::Descendants( $node, undef, undef,
		sub { # sDescP
			my $in = shift;
			$short .= esc($in->{astNodeName});
			$short .= " {\\small($in->{ExtSource})}" 
					if defined $in->{ExtSource};
		},
		sub { # sE
			$short .= "\n\n";
		}
	);

	print CLASS "\n\\label{",$node->{TeXRef},"}",
		sectionHeader( "$node->{NodeType} $fullname", $short );

	Iter::MembersByType( $node,
		sub { # $sGroupS
			print CLASS "\\subsection*{", shift, "}\n\n\\begin{itemize}\n";
		},
		\&printMemberIndex,
		sub { # $sGroupE
			print CLASS "\\end{itemize}\n";
		},
		sub { # $sNoneP
			print CLASS "\\begin{quote}\nNo members.\n\\end{quote}\n";
		}
	);

	if( kdocAstUtil::hasDoc( $node ) ) {
		print CLASS "\n\\subsection*{Detailed Description}\n\n";
		printDoc( $doc, *CLASS, $rootnode, 1 );
	}

	print CLASS "\n\\subsection*{Member Documentation}\n\n";

	Iter::DocTree( $node, 0, 0,
		sub { # common
			my ( $node, $kid ) = @_;
			printMemberName( $kid );
			printDoc( $kid->{DocNode}, *CLASS, $rootnode );
		},
		undef, # compound
		sub { # other
			my ( $node, $kid ) = @_;
			return if $kid->{NodeType} ne "method";
			my $ref = kdocAstSearch::findOverride( $rootnode, $node, 
						$kid->{astNodeName} );
			if ( defined $ref ) {
				print CLASS "\n\nReimplemented from ",
				refName( $ref->{Parent} ), "\n\n";
			}
		}
	);

	# TODO version and author
	
	close CLASS;
}

sub printMemberIndex
{
	my ( $parent, $m ) = @_;
	my $name;

	if( exists $m->{Compound} ) {
		next if $parent eq $rootnode; 

		$name = refName( $m );
	}
	elsif( exists $m->{DocNode} ) {
# compound nodes have their own section
		$name = refName( $m,  'NumRef' );
	} else {
		$name = esc( $m->{astNodeName} );
	}

	my $type = $m->{NodeType};

	print CLASS "\\item ";

	if( $type eq "var" ) {
		print CLASS esc( $m->{Type}), " \\textbf{", $name,"}\n";
	}
	elsif( $type eq "method" || $type eq "fnptr" ) {
		my $flags = $m->{Flags};

		$name = "\\emph{".esc($name)."}" if $flags =~ /p/;
		my $extra = "";
		$extra .= "virtual " if $flags =~ "v";
		$extra .= "static " if $flags =~ "s";
		my $printname = "\\textbf{$name}";
		$printname = "(*$printname)" if $type eq "fnptr";

		print CLASS $extra, esc($m->{ReturnType}), " $printname (", 
				esc($m->{Params}), ") ", $flags =~ /c/ ? " const\n": "\n";
	}
	elsif( $type eq "enum" ) {
		print CLASS "enum \\textbf{", esc($name), 
				"} \\{", esc($m->{Params}),"\\}\n";
	}
	elsif( $type eq "typedef" ) {
		print CLASS "typedef ", esc($m->{Type}), " \\textbf{", $name,"}\n";
	}
	else { 
# unknown type
		print CLASS esc($type), " \\textbf{", $name,"}\n";
	}

	my $fill = "\\dotfill";

	if ( defined $m->{DocNode} ){
		$fill .= "\\pageref{".$m->{TeXRef}."}";
	}

	print CLASS "$fill\n";
}


sub printMemberName
{
	my $m = shift;

	my $name = esc(	join( "::", kdocAstUtil::heritage( $m )) );
	my $type = $m->{NodeType};
	my $ref;
	my $flags = undef;

#	foreach $ref ( @_ ) {
# ??? Pomy¶leæ o tym
#		print CLASS "\\label{", $ref, "}";
#	}

#	print CLASS "\n\n\\textbf{";
	print CLASS "\n\\label{".$m->{TeXRef}."}\n\\subsubsection*{";

	if( $type eq "var" ) {
		print CLASS textRef($m->{Type}, $rootnode ), 
		" \\texttt{", $name,"} ";
	}
	elsif( $type eq "method" || $type eq "fnptr" ) {
		$flags = $m->{Flags};
		$name = "\\emph{".esc($name)."}" if $flags =~ /p/;
		$name = "(*$name)" if $type eq "fnptr";

		print CLASS textRef($m->{ReturnType}, $rootnode ),
		" \\texttt{", $name, "} (", 
		textRef($m->{Params}, $rootnode ), ") ";
	}
	elsif( $type eq "enum" ) {
		print CLASS "enum \\texttt{", $name, "} \\{",
		esc($m->{Params}),"\\} ";
	}
	elsif( $type eq "typedef" ) {
		print CLASS "typedef ", 
		textRef($m->{Type}, $rootnode ), " \\texttt{", $name,"} ";
	}
	else {
		print CLASS $name, " {\\small (", 
			esc($type), ")} ";
	}

# extra attributes
	my @extra = ();

	if( !exists $m->{Access} ) {
		print "Member without access:\n";
		kdocAstUtil::dumpAst( $m );
	}

	($ref = $m->{Access}) =~ s/_slots//g;

	push @extra, $ref
		unless $ref =~ /public/
		|| $ref =~ /signal/;

	if ( defined $flags ) {
		my $f;
		my $n;
		foreach $f ( split( "", $flags ) ) {
			$n = $main::flagnames{ $f };
			warn "flag $f has no long name.\n" if !defined $n;
			push @extra, $n;
		}
	}

	if ( $#extra >= 0 ) {
		print CLASS " {\\small [", join( " ", @extra ), "]}";
	}

	print CLASS "}"; # Po subsubsection
	print CLASS "\n\n";
	
# finis
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

#	p: docNode, sS, sE, sTextP, sRefP, sSectP, sPreP, sImageP,
#                sParaP, sListS, sListE, sListP

	kdocDocIter::TextIter( $docNode,
		sub { print CLASS "\n\n"; },
		sub { print CLASS "\n\n"; },
		sub { print CLASS "", deref( $_[1], $rootnode ); },
		sub { 
			my ( $tnode, $name, $nref ) = @_;
			print CLASS "", defined $nref ? refName( $nref ) : $name;
		},
		undef, # TODO sect
		sub {
			print CLASS "\n\\begin{verbatim}\n",$_[1],"\n\\end{verbatim}\n";
		},
		undef, # TODO image: do we convert to eps here?
		sub { print CLASS "\n\n"; },
		sub { print CLASS "\n\\begin{itemize}\n"; },
		sub { print CLASS "\\end{itemize}\n" },
		sub { print CLASS "\\item ", deref( $_[1], $rootnode ), "\n"; }
	);

	# p: node, sS, sE, sParamP

	kdocDocIter::ParamIter( $docNode,
		sub { 
			print CLASS "\n\n\\textbf{Parameters}:\n\n", 
					"\\begin{longtable}{lp{8cm}}\n";
		},
		sub {
			print CLASS "\n\\end{longtable}\n";
		},
		sub {
			my ( $name, $text ) = @_;
			$text =~ s/\n+//g;
			print CLASS "\\emph{", esc($name), '} & ', 
						deref( $text, $rootnode ), "\\\\\n";
		}
	);

	printTextItem( $docNode, CLASS, "Returns", "Return" );

	# p:  node, nonesub, startsub, printsub, endsub
	my $comma = "\n\n\\textbf{Throws}: ";
	kdocDocIter::Throws ( $docNode, undef, undef,
		sub {
			print CLASS $comma, deref( shift, $rootnode );
			$comma = ", ";
		},
		sub { print CLASS "\n\n"; }
	);


	# p:  node, nonesub, startsub, printsub, endsub
	$comma = "\n\n\\textbf{See also}: ";
	kdocDocIter::SeeAlso ( $docNode, undef, undef,
		sub {
			my ( $name, $node ) = @_;
			print CLASS $comma, defined $node ? refName($node) : esc($name);
			$comma = ", ";
		},
		sub { print CLASS "\n\n"; }
	);

	printTextItem( $docNode, CLASS, "Since", "Since" );
	printTextItem( $docNode, CLASS, "Version", "Version" );
	printTextItem( $docNode, CLASS, "Id", "Id" );
	printTextItem( $docNode, CLASS, "Author", "Author" );
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
	
#	print CLASS "\n\n\\textbf{", $label, "}: ", deref( $text, $rootnode  ), "\n\n";
#	print CLASS "\n\n\\textbf{$label}:\n\n", deref( $text, $rootnode  ), "\n\n";
        my $txt = deref($text, $rootnode);
	print CLASS <<EOF

\\textbf{$label}:

\\begin{longtable}{lp{10cm}}
 & $txt \\\\
\\end{longtable}
EOF
}

sub esc
{
	my $str = shift;

	return "" if !defined $str || $str eq "";

        # Trzeba zrobiæ sztuczkê by nie zamieniaæ w³asnych klamerek
        # lub backslashy
	$str =~ s/{/\\lbrace/g;
	$str =~ s/}/\\rbrace/g;
        $str =~ s/\\/\\ensuremath{\\backslash}/g;
#	$str =~ s/{/\\{/g;
#	$str =~ s/}/\\}/g;

        $str =~ s/</\\ensuremath{<}/g;
        $str =~ s/>/\\ensuremath{>}/g;
	$str =~ s/#/\\#/g;
	$str =~ s/%/\\%/g;
	$str =~ s/&/\\&/g;
	$str =~ s/\$/\\\$/g;
	$str =~ s/_/\\_/g;
        $str =~ s/~/\\ensuremath{\\sim}/g;
#        $str =~ s/\^/{\\ensuremath{^}}/g;

	return $str;
}

sub sectionHeader
{
	return "\n\\section{". shift() ." \\hrulefill}". shift();
}

sub deref
{
	my( $str, $node ) = @_;
	my $out = "";

	kdocDocIter::Deref( $str, $node, sub { $out .= esc( shift ); } );

	return $out;
}

sub textRef
{
	my ( $str, $node ) = @_;
	my $out = "";

	kdocDocIter::TextRef( $str, $node, sub {$out .= esc( shift );});

	return $out;
}

sub refName
{
	my $node = shift;
	my $out = esc($node->{astNodeName});
	$out = "\\emph{$out}" if exists $node->{Pure};
	return $out;
}

sub makeRef
{
	my $node = shift;
	my $ref = 1;

	Iter::Tree( $node, 1, 
		sub { 
			my ( $root, $node ) = @_;
			$node->{TeXRef} = "fn:$ref" ; 
			$ref++;

			return;
		}
	);
}

1;
