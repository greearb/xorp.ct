=head1 kdocAstUtil

	Utilities for syntax trees.

=cut

package kdocAstUtil;

use Ast;
use Carp;
use kdocUtil;
use kdocAstSearch;
use kdocParseCxx;
use Iter;
use strict;

use vars qw/$depth/;

sub BEGIN {
	$depth = 0;
}


=head2 TREE POST PROCESSING

=over 2

=item makeInherit

	Parameter: $rootnode, $parentnode

	Make an inheritance graph from the parse tree that begins
	at rootnode. parentnode is the node that is the parent of
	all base class nodes.

=cut

sub makeInherit
{
	my( $rnode, $parent ) = @_;

	foreach my $node ( @{ $rnode->{Kids} } ) {
		next if !defined $node->{Compound};

		# set parent to root if no inheritance

		if ( !exists $node->{InList} ) {
#			newInherit( $node, "Global", "", $parent );
			$parent->AddPropList( 'InBy', $node );

			makeInherit( $node, $parent );
			next;
		}

		# link each ancestor
		my $acount = 0;
ANITER:
		foreach my $in ( @{ $node->{InList} } ) {
			unless ( defined $in ) {
				Carp::cluck "warning: $node->{astNodeName} "
					." has undef in InList.";
				next ANITER;
			}

			my $ref = kdocAstSearch::findRef( $rnode, 
					$in->{astNodeName} );

			if( !defined $ref ) {
				# ancestor undefined
				warn "warning: ", $node->{astNodeName},
					" inherits unknown class '",
						$in->{astNodeName},"'\n";

				# create a new (fake) class
				$ref = Ast::New( $in->{astNodeName} ); # FIXME namespace?
				$ref->{NodeType} = $node->{NodeType};
				$ref->{ExtSource} = "unknown";
				$ref->{Compound} = 1;
				kdocAstGen::attachChild( $parent, $ref );
			}

			# found ancestor
			$in->{Node} = $ref ;
			$ref->AddPropList( 'InBy', $node );
			$acount++;
		}

		if ( $acount == 0 ) {
			# inherits no known class: just parent it to global
#FIXME		newInherit( $node, "Global", "", $parent );
			$parent->AddPropList( 'InBy', $node );
		}
		makeInherit( $node, $parent );
	}
}

=item linkReferences

	Parameters: root, node

	Recursively links references in the documentation for each node
	to real nodes if they can be found.  This should be called once
	the entire parse tree is filled.

=cut

sub linkReferences
{
	my( $root, $node ) = @_;

	if ( exists $node->{DocNode} ) {
		linkDocRefs( $root, $node, $node->{DocNode} );

		if( exists $node->{Compound} ) {
			linkSee( $root, $node, $node->{DocNode} );
		}
	}

	my $kids = $node->{Kids};
	return unless defined $kids;

	foreach my $kid ( @$kids ) {
		# only continue in a leaf node if it has documentation.
		next if !exists $kid->{Kids} && !exists $kid->{DocNode};
		if( !exists $kid->{Compound} ) {
			linkSee( $root, $node, $kid->{DocNode} );
		}
		linkReferences( $root, $kid );
	}
}


=item linkNamespaces

	p: node

	link all nodes to their namespaces via the ExtNames property.

=cut

sub linkNamespaces
{
	my $node = shift;

	if ( defined $node->{ImpNames} ) {
		foreach my $space ( @{$node->{ImpNames}} ) {
			my $spnode = kdocAstSearch::findRef( $node, $space );

			if( defined $spnode ) {
				$node->AddPropList( "ExtNames", $spnode );
			}
			else {
				warn "namespace not found: $space\n";
			}
		}
	}

	return unless defined $node->{Compound} || !defined $node->{Kids};

	foreach my $kid ( @{$node->{Kids}} ) {
		linkNamespaces( $kid ) if localComp( $kid );
	}
}

=item linkGlobalsToSources

	p: rootnode

	Links all globals for a file node into its "Globals" property.

=cut

sub linkGlobalsToSources
{
	my $node = shift;

	foreach my $kid ( @{$node->{Kids}} ) {
		next if exists $kid->{ExtSource} 
			|| exists $kid->{Compound}
			|| (!$main::doPrivate && 
				$kid->{Access} =~ /private/)
			|| !defined $kid->{Source};
		
		$kid->{Source}->AddPropList( "Globals", $kid );
	}
}

=item linkDocRefs

	Parameters: root, node, docnode

	Link references in the docs if they can be found.  This should
	be called once the entire parse tree is filled.

=cut

sub linkDocRefs
{
	my ( $root, $node, $docNode ) = @_;
	return unless exists $docNode->{Text};

	my ($text, $ref, $item, $tosearch);

	foreach $item ( @{$docNode->{Text}} ) {
		next if $item->{NodeType} ne 'Ref';

		$text = $item->{astNodeName};

		if ( $text =~ /^(?:#|::)/ ) {
			$text = $';
			$tosearch = $node;
		}
		else {
			$tosearch = $root;
		}

		$ref = kdocAstSearch::findRef( $tosearch, $text );
		$item->{Ref} = $ref if defined $ref;

		confess "Ref failed for ", $item->{astNodeName},
		"\n" unless defined $ref;
	}
}

sub linkSee
{
	my ( $root, $node, $docNode ) = @_;
	return unless exists $docNode->{See};

	my ( $text, $tosearch, $ref );

	foreach $text ( @{$docNode->{See}} ) {
		if ( $text =~ /^\s*(?:#|::)/ ) {
			$text = $';
			$tosearch = $node;
		}
		else {
			$tosearch = $root;
		}

		$ref = kdocAstSearch::findRef( $tosearch, $text );
		$docNode->AddPropList( 'SeeRef', $ref )
			if defined $ref;
	}
}



=back 

=head2 NODE AGGREGATION

=over 2

=item allTypes

	Parameters: node list ref
	returns: list

	Returns a sorted list of all distinct "NodeType"s in the nodes 
	in the list.

=cut

sub allTypes( $ )
{
	my ( $lref ) = @_;

	my %types = ();
	foreach my $node ( @{$lref} ) {
		$types{ $node->{NodeType} } = 1;
	}

	return sort keys %types;
}



=item findNodes

	Parameters: outlist ref, full list ref, key, value

	Find all nodes in full list that have property "key=value".
	All resulting nodes are stored in outlist.

=cut

sub findNodes( $$$$ )
{
	my( $rOutList, $rInList, $key, $value ) = @_;

	my $node;

	foreach $node ( @{$rInList} ) {
		next if !exists $node->{ $key };
		if ( $node->{ $key } eq $value ) {
			push @$rOutList, $node;
		}
	}
}

=item allMembers

	Parameters: hashref outlist, node, $type

	Fills the outlist hashref with all the methods of outlist,
	recursively traversing the inheritance tree.

	If type is not specified, it is assumed to be "method"

=cut

sub allMembers
{
	my ( $outlist, $n, $root, $type ) = @_;
	my $in;
	$type = "method" if !defined $type;

	if ( exists $n->{InList} ) {

		foreach $in ( @{$n->{InList}} ) {
			next if !defined $in->{Node};
			my $i = $in->{Node};

			allMembers( $outlist, $i, $root, $type ) 
				unless $i == $root;
		}
	}

	return unless exists $n->{Kids};

	foreach $in ( @{$n->{Kids}} ) {
		next if $in->{NodeType} ne $type;

		$outlist->{ $in->{astNodeName} } = $in;
	}
}


=item makeClassList

	Parameters: node, outlist ref

	fills outlist with a sorted list of all direct, non-external
	compound children of node.

=cut

sub makeClassList( $ $ )
{
	my ( $rootnode, $list ) = @_;

	@$list = ();

	Iter::LocalCompounds( $rootnode,
		sub { 
				my $node = shift;

				my $her = join ( "::", heritage( $node ) );
				$node->{FullName} = $her ;

				if ( !exists $node->{DocNode}->{Internal} ||
				     !$main::skipInternal ) {
					push @$list, $node;
				}
	} );

	@$list = sort { $a->{FullName} cmp $b->{FullName} } @$list;
}

=item heritage

	p: node

	returns names of ancestors from closest to furthest.

=cut

sub heritage( $ )
{
		my $node = shift;
		my @heritage;

		while( 1 ) {
				push @heritage, $node->{astNodeName};

				last unless defined $node->{Parent};
				$node = $node->{Parent};
				last unless defined $node->{Parent};
		}

		return reverse @heritage;
}

=item refHeritage

	p: node

	Returns ancestor nodes from closest to furthest.

=cut

sub refHeritage( $ )
{
		my $node = shift;
		my @heritage;

		while( 1 ) {
				push @heritage, $node;

				last unless defined $node->{Parent};
				$node = $node->{Parent};
				last unless defined $node->{Parent};
		}

		return reverse @heritage;

}

=back

=head2 NODE PROPERTIES

=over 2

=item External

	p: node
	
	Returns true if the node is externally defined, ie
	in a library linked with '-l'.

=cut

sub External( $ )
{
	return defined $_[0]->{ExtSource};
}

=item Compound

	p: node

	returns true if the node is compound.

=cut

sub Compound( $ )
{
	return defined $_[0]->{Compound};
}

=item localComp

	p: node

	Returns true if the node is a locally defined compound node.

=cut

sub localComp( $ )
{
	my ( $node ) = $_[0];
	return defined $node->{Compound} 
		&& !defined $node->{ExtSource} 
		&& $node->{NodeType} ne "Forward";
}

=item hasDoc

	p: node

	Returns true if the node has attached documentation.

=cut

sub hasDoc( $ )
{
	return defined $_[0]->{DocNode};
}


=item hasLocalInheritor

	Parameter: node
	Returns: 0 on fail

Checks if the node has an inheritor that is defined within the
current library. This is useful for drawing the class hierarchy,
since you don't want to display classes that have no relationship
with classes within this library.

NOTE: perhaps we should cache the value to reduce recursion on 
subsequent calls.

=cut

sub hasLocalInheritor
{
	my $node = shift;

	return 0 if !exists $node->{InBy};

	my $in;
	foreach $in ( @{$node->{InBy}} ) {
		return 1 if hasLocalInheritor( $in );
                return 1 if !exists $in->{ExtSource}
                  && !( exists $in->{DocNode}->{Internal} && $main::skipInternal );
	}

	return 0;
}

=back

=head2 DEBUGGING AND TESTING

=over 2

=cut

sub calcStats
{
		my ( $stats, $root, $node ) = @_;
# stats:
# num types
# num nested
# num global funcs
# num methods


		my $type = $node->{NodeType};

		if ( $node eq $root ) {
			# global methods
			if ( defined $node->{Kids} ) {
				foreach my $kid ( @{$node->{Kids}} ) {
						$stats->{Global}++ if $kid->{NodeType} eq "method";
				}
			}

			$node->{Stats} = $stats ;
		}
		elsif ( kdocAstUtil::localComp( $node ) 
					|| $type eq "enum" || $type eq "typedef" ) {
				$stats->{Types}++;
				$stats->{Nested}++ if $node->{Parent} ne $root;
		}
		elsif( $type eq "method" ) {
				$stats->{Methods}++;
		}

		return unless defined $node->{Compound} || !defined $node->{Kids};

		foreach my $kid ( @{$node->{Kids}} ) {
				next if defined $kid->{ExtSource};
				calcStats( $stats, $root, $kid );
		}
}

=item dumpAst

	Parameters: node, deep
	Returns: none

	Does a recursive dump of the node and its children.
	If deep is set, it is used as the recursion property, otherwise
	"Kids" is used.

=cut

sub dumpAst
{
	my ( $node, $deep ) = @_;

	$deep = "Kids" if !defined $deep;

	print "\t" x $depth, $node->{astNodeName}, 
		" (", $node->{NodeType}, ")\n";

	my $kid;

	foreach $kid ( $node->GetProps() ) {
		print "\t" x $depth, "  -\t", $kid, " -> ", $node->{$kid},"\n"
			unless $kid =~ /^(astNodeName|NodeType|$deep)$/;
	}
	if ( exists  $node->{Ancestors} ) {
		print "\t" x $depth, "Ancestors:\t",
			join( ",", @{$node->{Ancestors}}),"\n";
	}


	$depth++;
	foreach $kid ( @{$node->{ $deep }} ) {
		dumpAst( $kid );
	}

	print "\t" x $depth, "Documentation nodes:\n" if defined 
		@{ $node->{Doc}->{ "Text" }};

	foreach $kid ( @{ $node->{Doc}->{ "Text" }} ) {
		dumpAst( $kid );
	}

	$depth--;
}

=item testRef

	Parameters: rootnode

	Interactive testing of referencing system. Calling this
	will use the readline library to allow interactive entering of
	identifiers. If a matching node is found, its node name will be
	printed.

=cut

sub testRef {
	require Term::ReadLine;

	my $rootNode = $_[ 0 ];

	my $term = new Term::ReadLine 'Testing findRef';

	my $OUT = $term->OUT || *STDOUT{IO};
	my $prompt = "Identifier: ";

	while( defined ($_ = $term->readline($prompt)) ) {

		my $node = kdocAstSearch::findRef( $rootNode, $_ );

		if( defined $node ) {
			print $OUT "Reference: '", $node->{astNodeName}, 
			"', Type: '", $node->{NodeType},"'\n";
		}
		else {
			print $OUT "No reference found.\n";
		}

		$term->addhistory( $_ ) if /\S/;
	}
}

1;
