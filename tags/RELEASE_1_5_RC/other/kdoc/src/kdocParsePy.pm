package kdocParsePy;

use kdocAstGen;
use kdocAstUtil;
use File::Basename;

use strict;

use vars qw/$expecteddepth $rootNode $ident $params $class $method $variable
$cSourceNode $striphpath $docexpected $newNode/;

=head2 Parsing python

1 clear buffer
2 read a line and append to buffer
	ends in \ -> 2
syntax:

decl:	"def"
		
=cut

sub recursePairs
{
	my( $ochr, $cchr, $depth ) = @_;

	return "[^$ochr$cchr]" if $depth <= 0;

	my $text = recursePairs( $ochr, $cchr, $depth - 1 );
	my $frep = $depth > 1 ? "" : "+";
	my $rep = $depth > 1 ? "?" : "*";

	return "(?:(?>$text$frep)|$ochr$text$rep$cchr)*";
}

sub BEGIN {
	$ident = "[\\w\\._]+";
	$params = recursePairs( "(", ")", 2 );
	$class= "class\\s+($ident)\\s*((:?\\($params\\))?)\\s*:";
	$method= "def\\s+($ident)\\s*\\(($params)\\)\\s*:";
	$variable = "($ident)\s*=\s*([^#]+)";
	$striphpath = 1;
}

sub parseStream
{
	( $rootNode, $cSourceNode, *INPUT ) = @_;
	my( $currentfile ) = @_;
	$expecteddepth = 0;
	$docexpected = 0;
	$newNode = undef;

	my $buffer = "";
	my $defunfinished = 0;
	
	while( <INPUT> ) {
		chop;
		next if /^\s*(?:#.*)?$/; #blank or comment
		
		if ( /\\$/ ){
			$buffer .= $`;
			next;
		}
		$buffer .= $_;
		parseDecl( $buffer );
		$buffer = "";
	}
}



sub parseDecl
{
	my $decl = shift;
	/^(\s*)/;

	print $decl, "\n" if $main::debug;

	my $ndepth = length( $1 );

	if( $ndepth > $expecteddepth ) {
#		warn "Parse error: depth $ndepth unexpected\n";
	}
	elsif( $ndepth < $expecteddepth ) {
#		my $up = $expecteddepth - ( 1 + $ndepth );
#
#		while( $up > 0  ) {
#			kdocAstGen::popDecl() if $kdocAstGen::cNode->{Compound};
#			$up--;
#		}
#		$expecteddepth = $ndepth;

#		$expecteddepth--;
		while( $expecteddepth > $ndepth ) {
			kdocAstGen::popDecl() if $kdocAstGen::cNode->{Compound};
			$expecteddepth--;
		}
		$docexpected = 0;
	}

	if( $decl =~ /$class/o ) {

		my $rest = $';
		$newNode = kdocAstGen::newClass( "", "class", $1, 0 );
		$newNode->{KidAccess} = "public";
		my $inlist = $2;
		$inlist =~ s/[\(\)\s]//g;
		
		foreach my $in ( split( /\s*,\s*/, $inlist ) ){
			my $in = kdocAstGen::newInherit( $newNode, $in, "public" );
			$in->{Type} = "public";
		}

		postInitNode( $newNode );

		if ( $rest =~ /\s*pass\s*(?:#.*)?$/  ) { # HACK FIXME
			$docexpected = 0;
			kdocAstGen::popDecl()
		}
		elsif ($rest =~ /\s*(?:#.*)?$/ ) {
			$expecteddepth++;
			$docexpected = 1;
		}

		$docexpected = 1;
	}
	elsif( $decl =~ /$method/o ) {
		my $rest = $';
		$newNode = kdocAstGen::newMethod( "", $1, $2, "", "", "" );

		postInitNode( $newNode );

#		if( $rest =~ /\s*(?:#.*)?$/  ){
#			$expecteddepth++;
#		}
		$docexpected = 1;
	}
	elsif( $decl =~ /$variable$/osx ) {
		$newNode = kdocAstGen::newVar( "", $1, "" );
		postInitNode( $newNode );
		print "New Var: $1: $2\n";

		$docexpected = 1;
	}
	elsif( $docexpected && $decl =~ /^\s*(['"]+)/) {
		my $tag = $1;
		my $text = $';
		$docexpected = 0;
		$newNode->{DocNode} = parseDoc( $text, $tag );
	}
	else {
		$docexpected = 0;
	}
}


sub postInitNode
{
	my $newNode = shift;
	$newNode->{Source} = $cSourceNode ;
}

sub parseDoc
{
	my( $text, $endtag  ) = @_;
	my $done = 0;
	my $buffer = "";


	while( !$done ){
		if( $text =~ /$endtag\s*$/ ) {
			$text = $`;
			$done = 1;
		}
		$buffer .= $text;

		if( !$done ) {
			$text = <INPUT>;
		}
	}

	$buffer =~ s/\@arg\s+/\@p /g;

	my $node = Ast::New( "Doc" );
	$node->{NodeType} = "DocNode";

	my $short = "";
	foreach my $para ( split( /\n\s*\n/, $buffer ) ){
		my $tnode = Ast::New( $para );
		$tnode->{NodeType} = "DocText";
		$node->AddPropList( "Text", $tnode );
		$tnode = Ast::New( "\n" );
		$tnode->{NodeType} = "ParaBreak";
		$node->AddPropList( "Text", $tnode );

		$short = $para if $short eq "";
	}

	if ($short ne "" ) {
		$short =~ s/\..*$/./gs;
		$node->{ClassShort} = $short
	}

	return $node;
}

1;
