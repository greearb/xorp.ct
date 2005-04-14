package kdocDocHelper;

use Carp;
use Ast;
use kdocAstUtil;
use kdocUtil;

use strict;
use vars qw/ @undoc_class @undoc_func @no_short 
	$lib $rootnode $outputdir $opt /;

=head1 kdocDocCheck

	Check source files for documentation statistics.

	Undocumented globals

=cut

BEGIN {
	@undoc_class = ();
	@undoc_func = ();
	@no_short = ();
}

sub writeDoc
{
	( $lib, $rootnode, $outputdir, $opt ) = @_;

	foreach my $node ( @{$rootnode->{Kids}} ) {
		next if defined $node->{ExtSource} 
			|| defined $node->{Forward}
			|| $node->{NodeType} eq "Forward";

		if ( !defined $node->{Compound} ) {
			push @undoc_func, $node 
				unless defined $node->{DocNode};

			next;
		}

		if ( !defined $node->{DocNode} ) {
			push @undoc_class, $node;
		}
		elsif ( !defined $node->{DocNode}->{ClassShort} ) {
			push @no_short, $node;
		}
	}

	listOffenders( "Undocumented classes", \@undoc_class );
	listOffenders( "No short description", \@no_short );
	listOffenders( "Undocumented functions", \@undoc_func );
}

sub listOffenders
{
	my ( $reason, $list ) = @_;

	return if $#{$list} < 0;

	my $source = undef;

	print "$reason:\n";
	foreach my $node ( 
			sort { $a->{Source} cmp $b->{Source} } @{$list} ) {

		my $newsource = $node->{Source};
		if ( $source != $newsource ) {
			print "\t", $newsource->{astNodeName},":\n";
			$source = $newsource;
		}
			
		print "\t\t(", $node->{NodeType}, ") ", 
			$node->{astNodeName}, "\n";
	}
}

1;
