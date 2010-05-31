
package kdocDocIter;

use strict;
use kdocAstUtil;
use kdocAstSearch;

=head1 kdocDocIter -- Iteration over DocNode children

=head2 TextIter -- Iterate over doc data (non-attribute children).

	p: docNode, sS, sE, sTextP, sRefP, sSectP, sPreP, sImageP,
		sParaP, sListS, sListE, sListP

"$s..." means subref.

$..S and $...E means start and end, P means "print".
eg $sS means doc start, $sTextP means print text.

All subrefs receive ( node, text ) as params (text should be ignored
if it is not pertinent to the type).

It is safe to leave any subref undefined.

=cut

sub TextIter
{
	my ( $docNode, $sS, $sE, $sTextP, $sRefP, $sSectP, $sPreP, $sImageP,
		$sParaP, $sListS, $sListE, $sListP ) = @_;
	return if !defined $docNode;

	my $text = $docNode->{Text};
	return if !defined $text || $#$text < 0;

	my $lastType = "";

	$sS->( $docNode ) if defined $sS;

	foreach my $tnode ( @$text ) {
		my $type = $tnode->{NodeType};
		my $name = $tnode->{astNodeName};
		
		if ( $lastType eq "ListItem" && $type ne $lastType ) {
			$sListE->( $tnode, $name ) if defined $sListE;
		}

		if ( $type eq "DocText" ) {
			$sTextP->( $tnode, $name ) if defined $sTextP;
		}
		elsif ( $type eq "Pre" ) {
			$sPreP->( $tnode, $name, $tnode->{Desc} ) if defined $sPreP;
		}
		elsif ( $type eq "Ref" ) {
			$sRefP->( $tnode, $name, $tnode->{Ref} ) if defined $sRefP;
		}
		elsif ( $type eq "DocSection" ) {
			$sSectP->( $tnode, $name ) if defined $sSectP;
		}
		elsif ( $type eq "Image" ) {
			$sImageP->( $tnode, $name, $tnode->{Path} ) if defined $sImageP;
		}
		elsif( $type eq "ParaBreak" ) {
			$sParaP->( $tnode, $name ) if defined $sParaP;
		}
		elsif( $type eq "ListItem" ) {
			if ( $lastType ne "ListItem" ) {
				$sListS->( $tnode, $name ) if defined $sListS;
			}
			$sListP->( $tnode, $name ) if defined $sListP;
		}
		else {
#			warn "Unknown DocNode:Text node: $name ($type)\n";
		}

		$lastType = $type;
	}

	if ( $lastType eq "ListItem" ) {
		$sListE->( undef, undef ) if defined $sListE;
	}

	$sE->( $docNode ) if defined $sE;
}

=head2 ParamIter

	p: docNode, sS, sE, sP

Iterate over @params in the doc node.

=cut

sub ParamIter
{
	my ( $docNode, $sS, $sE, $sP ) = @_;
	return unless defined $docNode && defined $docNode->{Text};

	my @params = ();
	kdocAstUtil::findNodes( \@params, $docNode->{Text}, "NodeType", "Param" );
	return unless $#params >= 0;

	$sS->() if defined $sS;

	foreach my $param ( @params ) {
		$sP->( $param->{Name}, $param->{astNodeName} ) if defined $sP;
	}

	$sE->() if defined $sE;
}

sub SeeAlso
{
	my ( $node, $nonesub, $startsub, $printsub, $endsub ) = @_;

	if( !defined $node ) {
		$nonesub->() if defined $nonesub;
		return;
	}

	my $doc = $node;

	if ( $node->{NodeType} ne "DocNode" ) {
		$doc = $node->{DocNode};
		if ( !defined $doc ) {
			$nonesub->() if defined $nonesub;
			return;
		}
	}

	if ( !defined $doc->{See} ) {
		$nonesub->() if defined $nonesub;
		return;
	}

	my $see = $doc->{See};
	my $ref = $doc->{SeeRef};

	if ( $#$see < 0 ) {
		$nonesub->() if defined $nonesub;
		return;
	}

	$startsub->( $node ) if defined $startsub;

	for my $i ( 0..$#$see ) {
		my $seelabel = $see->[ $i ];
		my $seenode = undef;
		if ( defined $ref ) {
			$seenode = $ref->[ $i ]; 
		}

		$printsub->( $seelabel, $seenode ) if defined $printsub;
	}

	$endsub->( $node ) if defined $endsub;

	return;
}

sub Throws
{
	my ( $node, $nonesub, $startsub, $printsub, $endsub ) = @_;

	if( !defined $node ) {
		$nonesub->() if defined $nonesub;
		return;
	}

	my $doc = $node;

	if ( $node->{NodeType} ne "DocNode" ) {
		$doc = $node->{DocNode};
		if ( !defined $doc ) {
			$nonesub->() if defined $nonesub;
			return;
		}
	}

	if ( !defined $doc->{Throws} ) {
		$nonesub->() if defined $nonesub;
		return;
	}

	my $see = $doc->{Throws};

	if ( $#$see < 0 ) {
		$nonesub->() if defined $nonesub;
		return;
	}

	$startsub->( $node ) if defined $startsub;

	if ( defined $printsub ) {
		foreach my $throwtext ( @$see ) {
			$printsub->( $throwtext );
		}
	}

	$endsub->( $node ) if defined $endsub;

	return;
}


=head2 Deref

	p: text, node, sTextP, sRefP, sParamP

	Iterates over text and @ref parts of a text string.

=cut

sub Deref
{
	my ( $text, $node, $sTextP, $sRefP, $sParamP ) = @_;

	foreach my $part ( split (/(\@(?:ref|p)\s+[^\s\.]+)/, $text ) ) {
		if ( $part =~ /\@(ref|p)\s+([^\s\.]+)/ ) {
			$part = $2;
			my $call = $1;
			if ( defined $sRefP && $call eq "ref" ) {
				my $ref = kdocAstSearch::findRef( $node, $part );

				if ( defined $ref ) {
					$sRefP->( $ref );
					next;
				}
			}
			elsif( defined $sParamP && $call eq "p" ) {
				$sParamP->( $part );
				next;
			}
		}

		$sTextP->( $part ) if defined $sTextP;
	}
}


=head2 TextRef

	p: text, node, sTextP, sRefP

Iterate over every word in the text, calling $sRefP for every reference
match and $sTextP for everything else (or if $sRefP is undefined).

=cut

sub TextRef
{
	my ( $text, $node, $sTextP, $sRefP ) = @_;

	foreach my $part ( split( /([^\w:]+)/, $text ) ) {
		if ( $part =~ /^[\w:]/ ) {
			if ( defined $sRefP ) {
				my $ref = kdocAstSearch::findRef( $node, $part );

				if ( defined $ref ) {
					$sRefP->( $ref );
					next;
				}
			}
		}

		$sTextP->( $part ) if defined $sTextP;
	}
}

1;
