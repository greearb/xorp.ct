package kdocParser;

use strict;

use kdocParseCxx;
use kdocParsePy;
use Carp;
use Ast;
use File::Basename;

use vars qw/$currentfile $cpp $cppcmd @includes $striphpath %rootNodes
$cSourceNode/;


sub BEGIN {
	$cpp = 0;
	$cppcmd = "";
	@includes = ();
	$striphpath = 0;
 	%rootNodes = ();
	$currentfile = undef;
	$cSourceNode = undef;
}

sub parseFiles
{
	foreach $currentfile ( @ARGV ) {
		$main::currfile = $currentfile;
		my $lang = "CXX";

		if ( $currentfile =~ /\.idl\s*$/i ) {
# IDL file
			$lang = "IDL";
		}
		elsif ( $currentfile =~ /\.py\s*$/i ) {
			$lang = "PY";
		}

# assume cxx file
		if( $cpp ) {
# pass through preprocessor
			my $cmd = $cppcmd;
			foreach my $dir ( @includes ) {
				$cmd .= " -I $dir ";
			}

			$cmd .= " -DQOBJECTDEFS_H $currentfile";

			open( INPUT, "$cmd |" ) || croak "Can't preprocess $currentfile";
		}
		else {
			open( INPUT, "$currentfile" ) || croak "Can't read from $currentfile";
		}

		print $main::exe.": processing $currentfile\n" unless $main::quiet;

# reset vars
		my $rootNode = getRoot( $lang );
		kdocAstGen::setRoot( $rootNode );
		kdocAstGen::resetStack();

# add to file lookup table
		my $showname = $striphpath ? basename( $currentfile ) : $currentfile;
		$cSourceNode = Ast::New( $showname );
		$cSourceNode->{NodeType} = "source" ;
		$cSourceNode->{Path} = $currentfile ;
		$rootNode->AddPropList( "Sources", $cSourceNode );

# parse
		if( $lang eq "PY" ){
			kdocParsePy::parseStream( $rootNode, $cSourceNode, *INPUT );
		}
		else {
			kdocParseCxx::parseStream( $rootNode, $cSourceNode, *INPUT );
		}

		close INPUT;
	}

	return allRoots();
}

=head2 getRoot

	Return a root node for the given type of input file.

=cut

sub getRoot
{
	my $type = shift;
	carp "getRoot called without type" unless defined $type;

	if ( !exists $rootNodes{ $type } ) {
		my $node = Ast::New( "Global" );	# parent of all nodes
		$node->{NodeType} = "root" ;
		$node->{RootType} = $type ;
		$node->{Compound} = 1 ;
		$node->{KidAccess} = "public" ;

		$rootNodes{ $type } = $node;
	}
	print "getRoot: call for $type\n" if $main::debug;

	return $rootNodes{ $type };
}

=head2 allRoots

	Return ref to a hash of all root nodes.

=cut

sub allRoots
{
	return \%rootNodes;
}

1;
