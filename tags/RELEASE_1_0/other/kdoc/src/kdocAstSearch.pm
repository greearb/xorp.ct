
package kdocAstSearch;

use strict;
use Carp;

use vars qw/ $depth $refcalls $refiters @noreflist %noref /;

sub BEGIN {
# statistics for findRef

	$depth = 0;
	$refcalls = 0;
	$refiters = 0;

# findRef will ignore these words

	@noreflist = qw( const int char long double template 
		unsigned signed float void bool true false uint 
		uint32 uint64 extern static inline virtual operator );

	foreach my $r ( @noreflist ) {
		$noref{ $r } = 1;
	}
}

=head2 NODE SEARCH ROUTINES

=over 2

=item findRef

	Parameters: root, ident, report-on-fail
	Returns: node, or undef

	Given a root node and a fully qualified identifier (:: separated),
	this function will try to find a child of the root node that matches
	the identifier.

=cut

sub findRef
{
	my( $root, $name, $r ) = @_;

	confess "findRef: no name" if !defined $name || $name eq "";

	$name =~ s/\s+//g;	
	return undef if exists $noref{ $name };

	$name =~ s/^#//g;
	$name =~ s/$kdocParseCxx::template//gxso;

	my ($iter, @tree) = split /(?:\:\:|#)/, $name;
	my $kid;

	$refcalls++;

	# Upward search for the first token
	return undef if !defined $iter;

	while ( !defined findIn( $root, $iter ) ) {
		return undef if !defined $root->{Parent};
		$root = $root->{Parent};
	}
	$root = $root->{KidHash}->{$iter};
	carp if !defined $root;

	# first token found, resolve the rest of the tree downwards
	foreach $iter ( @tree ) {
		confess "iter in $name is undefined\n" if !defined $iter;
		next if $iter =~ /^\s*$/;

		unless ( defined findIn( $root, $iter ) ) {
			confess "findRef: failed on '$name' at '$iter'\n"
				if defined $r;
			return undef;
		}

		$root = $root->{KidHash}->{ $iter };	
		carp if !defined $root;
	}

	return $root;
}

=item inheritName

	pr: $inheritance node.

	Returns the name of the inherited node. This checks for existence
	of a linked node and will use the "raw" name if it is not found.

=cut

sub inheritName
{
	my $innode = shift;

	return defined $innode->{Node} ? 
		$innode->{Node}->{astNodeName}
		: $innode->{astNodeName};
}

=item inheritedBy

	Parameters: out listref, node

	Recursively searches for nodes that inherit from this one, returning
	a list of inheriting nodes in the list ref.

=cut

sub inheritedBy
{
	my ( $list, $node ) = @_;

	return unless exists $node->{InBy};

	foreach my $kid ( @{ $node->{InBy} } ) {
		push @$list, $kid;
		inheritedBy( $list, $kid );
	}
}

=item findOverride

	Parameters: root, node, name

	Looks for nodes of the same name as the parameter, in its parent
	and the parent's ancestors. It returns a node if it finds one.

=cut

sub findOverride
{
	my ( $root, $node, $name ) = @_;
	return undef if !exists $node->{InList};

	foreach my $in ( @{$node->{InList}} ) {
		my $n = $in->{Node};
		next unless defined $n && $n != $root && exists $n->{KidHash};

		my $ref  = $n->{KidHash}->{ $name };
		
		return $ref if defined $ref && $ref->{NodeType} eq "method";

		if ( exists $n->{InList} ) {
			$ref = findOverride( $root, $n, $name );
			return $ref if defined $ref;
		}
	}

	return undef;
}

=item findIn

	node, name: search for a child

=cut

sub findIn( $$ )
{
	return undef unless defined $_[0]->{KidHash};

	my $ret =  $_[0]->{KidHash}->{ $_[1] };

	return $ret;
}
1;
