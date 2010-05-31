package Iter;

use strict;

=head1 Iterator Module

A set of iterator functions for traversing the various trees and indexes.
Each iterator expects closures that operate on the elements in the iterated
data structure.


=head2 Generic

	Params: $node, &$loopsub, &$skipsub, &$applysub, &$recursesub

Iterate over $node's children. For each iteration:
	
If loopsub( $node, $kid ) returns false, the loop is terminated. 
If skipsub( $node, $kid )  returns true, the element is skipped.

Applysub( $node, $kid ) is called
If recursesub( $node, $kid ) returns true, the function recurses into 
the current node.

=cut

sub Generic
{
	my ( $root, $loopcond, $skipcond, $applysub, $recursecond ) = @_;

	return sub {
		foreach my $node ( @{$root->{Kids}} ) {

			if ( defined  $loopcond ) {
				return 0 unless $loopcond->( $root, $node );
			}

			if ( defined $skipcond ) {
				next if $skipcond->( $root, $node );
			}

			my $ret = $applysub->( $root, $node );
			return $ret if defined $ret && $ret;

			if ( defined $recursecond 
					&& $recursecond->( $root, $node ) ) {
				$ret = Generic( $node, $loopcond, $skipcond,
						$applysub, $recursecond)->();
				if ( $ret ) {
					return $ret;
				}
			}
		}

		return 0;
	};
}

sub Class
{
	my ( $root, $applysub, $recurse ) = @_;

	return Generic( $root, undef,
		sub {
			my ( $root, $node ) = @_;
			return !( $node->{NodeType} eq "class" 
				|| $node->{NodeType} eq "struct" );
		}, 
		$applysub, $recurse );
}

=head2 Tree

	Params: $root, $recurse?, $commonsub, $compoundsub, $membersub,
		$skipsub

Traverse the ast tree starting at $root, skipping if skipsub returns true.

Applying $commonsub( $node, $kid),
then $compoundsub( $node, $kid ) or $membersub( $node, $kid ) depending on
the Compound flag of the node.

=cut

sub Tree
{
	my ( $rootnode, $recurse, $commonsub, $compoundsub, $membersub, 
		 $skipsub ) = @_;

	my $recsub = ($recurse != 0) ? sub { return 1 if $_[1]->{Compound}; } 
				: undef;

	Generic( $rootnode, undef, $skipsub,
		sub { 					# apply
			my ( $root, $node ) = @_;
			my $ret;

			if ( defined $commonsub ) {
				$ret = $commonsub->( $root, $node );
				return $ret if defined $ret;
			}

			if ( $node->{Compound} && defined $compoundsub ) {
				$ret = $compoundsub->( $root, $node );
				return $ret if defined $ret;
			}
			
			if( !$node->{Compound} && defined $membersub ) {
				$ret = $membersub->( $root, $node );
				return $ret if defined $ret;
			}
			return;
		},
		$recsub 				# skip
	)->();
}

=head2 LocalCompounds

Apply $compoundsub( $node ) to all locally defined compound nodes
(ie nodes that are not external to the library being processed).

=cut

sub LocalCompounds
{
		my ( $rootnode, $compoundsub ) = @_;

		return unless defined $rootnode && defined $rootnode->{Kids};

		foreach my $kid ( sort { $a->{astNodeName} cmp $b->{astNodeName} }
								 @{$rootnode->{Kids}} ) {
				next if !defined $kid->{Compound};

				$compoundsub->( $kid ) unless defined $kid->{ExtSource};
				LocalCompounds( $kid, $compoundsub );
		}
}

=head2 GlobalsBySourceFile

	p: rootnode, sS, sE, sFileS, sFileE, sGlobalP

=cut

sub GlobalsBySourceFile
{
	my( $node, $sS, $sE, $sFileS, $sFileE, $sGlobalP ) = @_;

	my $sources = $node->{Sources};
	return if !defined $sources || $#$sources < 0;

	$sS->() if defined $sS;

	foreach my $source ( 
			sort { $a->{astNodeName} cmp $b->{astNodeName} } @$sources )
	{
		next if !defined $source->{Globals};
		
		$sFileS->( $source->{astNodeName}, $source ) if defined $sFileS;

		if ( defined $sGlobalP ) {
			foreach my $global ( @{ $source->{Globals} } ) {
				$sGlobalP->( $global );
			}
		}

		$sFileE->( $source->{astNodeName}, $source ) if defined $sFileE;
	}

	$sE->() if defined $sE;
}

=head2 Hierarchy

	Params: $node, $levelDownSub, $printSub, $levelUpSub

This allows easy hierarchy traversal and printing.

Traverses the inheritance hierarchy starting at $node, calling printsub
for each node. When recursing downward into the tree, $levelDownSub($node) is
called, the recursion takes place, and $levelUpSub is called when the
recursion call is completed. 

=cut

sub Hierarchy
{
	my ( $node, $ldownsub, $printsub, $lupsub, $nokidssub ) = @_;

	return if defined $node->{ExtSource}
		&& (!defined $node->{InBy} 
			|| !kdocAstUtil::hasLocalInheritor( $node ));

        return if exists $node->{DocNode}->{Internal} && $main::skipInternal;

	$printsub->( $node );

	if ( defined $node->{InBy} ) {
		$ldownsub->( $node );

		foreach my $kid ( 
				sort {$a->{astNodeName} cmp $b->{astNodeName}}
				@{ $node->{InBy} } ) {
			Hierarchy( $kid, $ldownsub, $printsub, $lupsub );
		}

		$lupsub->( $node );
	}
	elsif ( defined $nokidssub ) {
		$nokidssub->( $node );
	}

	return;
}


sub Ancestors
{
	my ( $node, $rootnode, $noancessub, $startsub, $printsub,
		$endsub ) = @_;
	my @anlist = ();

	return if $node eq $rootnode;

	if ( !exists $node->{InList} ) {
		$noancessub->( $node ) unless !defined $noancessub;
		return;
	}
	
	foreach my $innode ( @{ $node->{InList} } ) {
		my $nref = $innode->{Node};	# real ancestor
		next if defined $nref && $nref == $rootnode;

		push @anlist, $innode;
	}

	if ( $#anlist < 0 ) {
		$noancessub->( $node ) unless !defined $noancessub;
		return;
	}

	$startsub->( $node ) unless !defined $startsub;

	foreach my $innode ( sort { $a->{astNodeName} cmp $b->{astNodeName} }
				@anlist ) {

		# print
		$printsub->( $innode->{Node}, $innode->{astNodeName},
			$innode->{Type}, $innode->{TmplType} ) 
			unless !defined $printsub;
	}

	$endsub->( $node ) unless !defined $endsub;

	return;

}

sub Descendants
{
	my ( $node, $nodescsub, $startsub, $printsub, $endsub ) = @_;

	if ( !exists $node->{InBy} ) {
		$nodescsub->( $node ) unless !defined $nodescsub;
		return;
	}

	
	my @desclist = ();
	DescendantList( \@desclist, $node );
	
	if ( $#desclist < 0 ) {
		$nodescsub->( $node ) unless !defined $nodescsub;
		return;
	}

	$startsub->( $node ) unless !defined $startsub;

	foreach my $innode ( sort { $a->{astNodeName} cmp $b->{astNodeName} }
				@desclist ) {

		$printsub->( $innode) 
			unless !defined $printsub;
	}

	$endsub->( $node ) unless !defined $endsub;

	return;

}

sub DescendantList
{
	my ( $list, $node ) = @_;

	return unless exists $node->{InBy};

	foreach my $kid ( @{ $node->{InBy} } ) {
		push @$list, $kid;
		DescendantList( $list, $kid );
	}
}

=head2 DocTree

=cut

sub DocTree
{
	my ( $rootnode, $allowforward, $recurse, 
		$commonsub, $compoundsub, $membersub ) = @_;
		
	Generic( $rootnode, undef,
		sub {				# skip
			my( $node, $kid ) = @_;

			unless (!(defined $kid->{ExtSource}) 
					&& ($allowforward || $kid->{NodeType} ne "Forward")
					&& ($main::doPrivate || !($kid->{Access} =~ /private/))
					&& exists $kid->{DocNode} ) {

				return 1;
			}

			return;
		},
		sub { 				# apply
			my ( $root, $node ) = @_;

			my $ret;

			if ( defined $commonsub ) {
				$ret = $commonsub->( $root, $node );
				return $ret if defined $ret;
			}

			if ( $node->{Compound} && defined $compoundsub ) {
				$ret = $compoundsub->( $root, $node );
				return $ret if defined $ret;
			}
			elsif( defined $membersub ) {
				$ret = $membersub->( $root, $node );
				return $ret if defined $ret;
			}

			return;
		},
		sub { return 1 if $recurse; return; }	# recurse
		)->();

}

=head2 MembersByType

	p: node, $sGroupS, $sItemP, $sGroupE, $sOnNone

Iterates over node's members sorted by type.

What we have to cover, in order

public (types/data/methods/slots/statics)
signals
k_dcop stuff
protected (types/data/methods/slots/statics)
private (types/data/methods/slots/statics) (depending on doPrivate)

Collection criteria:

Access contains word (public, private, protected)
Access is word (signals, interface, module)

=cut

sub MembersByType
{
	
	my ( $node, $startgrpsub, $methodsub, $endgrpsub, $nokidssub ) = @_;

	# per vis
	my @types = ();
	my @data = ();
	my @methods = ();
	my @slots = ();
	my @static = ();

	my @interfaces = ();
	my @modules = ();
	my @signals = ();

	# build list for public, protected types, data members and methods

	if ( !defined $node->{Kids} ) {
			$nokidssub->( $node ) if defined $nokidssub;
			return;
	}

	my $kids = $node->{Kids};

	foreach my $kid ( @$kids ) {
		my $type = $kid->{NodeType};

		if ( $type eq "method" ) {
			if ( $kid->{Flags} =~ /s/ ) {
				push @static, $kid;
			}
			elsif ( $kid->{Flags} =~ /l/ ) {
				push @slots, $kid;
			}
			elsif ( $kid->{Flags} =~ /n/ ) {
				push @signals, $kid;
			}
			else {
				push @methods, $kid;
			}
		}
		elsif ( $kid->{Compound} ) {
			if ( $type eq "module" ) {
				push @modules, $kid;
			}
			elsif ( $type eq "interface" ) {
				push @interfaces, $kid;
			}
			else {
				push @types, $kid;
			}
		}
		elsif ( $type eq "typedef" || $type eq "enum" ) {
			push @types, $kid;
		}
		else {
			push @data, $kid;
		}
	}

	doGroup( "Modules", $node, \@modules, $startgrpsub,
			$methodsub, $endgrpsub);
	doGroup( "Interfaces", $node, \@interfaces, $startgrpsub,
			$methodsub, $endgrpsub);

	# select into visibility lists
	my @vislist = qw/public protected/;
	push @vislist, "private" if $main::doPrivate;

	foreach my $access ( @vislist ) {
		my $uc_access = ucfirst( $access );

		doGroup( "$uc_access Types", $node, \@types, $startgrpsub,
				$methodsub, $endgrpsub, $access );

#		if ( $access eq "public" ) {
#			doGroup( "Modules", $node, \@modules, $startgrpsub,
#					$methodsub, $endgrpsub);
#			doGroup( "Interfaces", $node, \@interfaces, $startgrpsub,
#					$methodsub, $endgrpsub);
#		}

		doGroup( "$uc_access Methods", $node, \@methods, $startgrpsub,
				$methodsub, $endgrpsub, $access );
		doGroup( "$uc_access Slots", $node, \@slots, $startgrpsub,
				$methodsub, $endgrpsub, $access );

		if ( $access eq "public" ) {
			doGroup( "Signals", $node, \@signals, $startgrpsub,
					$methodsub, $endgrpsub);
		}

		doGroup( "$uc_access Static Methods", $node, \@static, 
				$startgrpsub, $methodsub, $endgrpsub, $access);
		doGroup( "$uc_access Members", $node, \@data, $startgrpsub,
				$methodsub, $endgrpsub, $access );
	}
}

sub doGroup
{
	my ($name, $node, $list, $startgrpsub, $methodsub, $endgrpsub, $vis)=@_;

	my ( $hasMembers ) = 0;
	foreach my $kid ( @$list ) {
		next if defined $vis && !($kid->{Access} =~ /$vis/);

		if ( !exists $kid->{DocNode}->{Reimplemented} ) {
			$hasMembers = 1;
			last;
		}
	}
	return if !$hasMembers;

	$startgrpsub->( $name ) if defined $startgrpsub;

	if ( defined $methodsub ) {
		foreach my $kid ( @$list ) {
			next if defined $vis && !($kid->{Access} =~ /$vis/)
					|| exists $kid->{DocNode}->{Reimplemented};
			$methodsub->( $node, $kid );
		}
	}

	$endgrpsub->( $name ) if defined $endgrpsub;
}

sub ByGroupLogical
{
	my ( $root, $startgrpsub, $itemsub, $endgrpsub ) = @_;

	return 0 unless defined $root->{Groups};

	foreach my $groupname ( sort keys %{$root->{Groups}} ) {
		next if $groupname eq "astNodeName"||$groupname eq "NodeType";

		my $group = $root->{Groups}->{ $groupname };
		next unless $group->{Kids};
		
		$startgrpsub->( $group->{astNodeName}, $group->{Desc} );

		foreach my $kid (sort {$a->{astNodeName} cmp $b->{astNodeName}}
					$group->{Kids} ) {
			$itemsub->( $root, $kid );
		}
		$endgrpsub->( $group->{Desc} );	
	}

	return 1;
}


1;
