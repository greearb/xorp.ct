
package kdocAstGen;

use Ast;
use kdocAstSearch;
use Carp;

use strict;

use vars qw/@classStack $cNode $root $lastParentNode/;

BEGIN
{
	@classStack = ();
	$cNode = undef;
	$root = undef;
}

# parse tree node stack

sub setRoot( $ )
{
	$root = $cNode = shift;
}

sub pushDecl( $ )
{
	push @classStack, $cNode;
	$cNode = shift;
#	warn "Gen: Entering $cNode->{astNodeName}\n";

	return $cNode;
}

sub popDecl()
{
	if ( $#classStack < 0 ) {
		warn "Mismatched block close";
		return;
	}
	
	my $node = $cNode;
#	warn "Gen: Leaving $node->{astNodeName}\n";
	$cNode = pop @classStack;

	return $cNode;
}

sub resetStack()
{
	@classStack = ();
	$cNode = $root;
	$lastParentNode = undef;
}

# Code node factories

sub newClass( $ $ $ $ )
{
	my( $tmplArgs, $cNodeType, $name, $forward ) = @_;

	my $access = "private";
	$access = "public" if $cNodeType ne "class";

	# try to find an exisiting node, or create a new one
	my $oldnode = kdocAstSearch::findRef( $cNode, $name );
	my $node = defined $oldnode ? $oldnode : Ast::New( $name );

	if ( $forward ) {
		# forward
		if ( !defined $oldnode ) {
			# new forward node
			$node->{NodeType} = "Forward" ;
			$node->{KidAccess} = $access ;
			attachChild( $cNode, $node );
		}
		return $node;
	}

	# this is a class declaration
	$node->{NodeType} = $cNodeType ;
	$node->{Compound} = 1 ;
# FIXME	$node->{Source} = $cSourceNode ;

	$node->{KidAccess} = $access ;
	$node->{Tmpl} = $tmplArgs  unless $tmplArgs eq "";

	if ( !defined $oldnode ) {
		attachChild( $cNode, $node );
	}

	return pushDecl( $node );
}


sub newEnum( $ $ $ )
{
	my ( $name, $end, $params ) = @_;

	$name = $end if $name eq "" && $end ne "";

	my ( $node ) = Ast::New( $name );
	$node->{NodeType} = "enum" ;
	$node->{Params} = $params ;
	attachChild( $cNode, $node );

	$lastParentNode = $node;

	return $node;

}

=head2 newTypedef

	Parameters: realtype, name

	Handles a type definition.

=cut

sub newTypedef( $ $ )
{
	my ( $realtype, $name ) = @_;

	my ( $node ) = Ast::New( $name );

	$node->{NodeType} = "typedef" ;
	$node->{Type} = $realtype ;

	attachChild( $cNode, $node );

	return $node;
}

=head2 newTypedefComp

	Params: realtype, name, compound?

	Creates a new compound type definition.

=cut

sub newTypedefComp( $ $ $ )
{
	my ( $realtype, $name, $compound ) = @_;

	my ( $node ) = Ast::New( $name );

	$node->{NodeType} = "typedef" ;
	$node->{Type} = $realtype ;

	attachChild( $cNode, $node );

	if ( $compound ) {
		print "newTypedefComp: Pushing $cNode->{astNodeName}\n" if $main::debug;
		push ( @classStack, $cNode );
		$cNode = $node;
	}

	return $node;
}

=head2 newVar

	Parameters: type, name, value

	New variable. Value is ignored if undef

=cut

sub newVar
{
	my ( $type, $name, $val ) = @_;

	my $node = Ast::New( $name );
	$node->{NodeType} = "var" ;

	my $static = 0;
	if ( $type =~ /static/ ) {
	#	$type =~ s/static//;
		$static = 1;
	}

	$node->{Type} = $type ;
	$node->{Flags} = 's'  if $static;
	$node->{Value} = $val  if defined $val;
	attachChild( $cNode, $node );

	return $node;
}

=head2 newAccess

	Parameters: access

	Sets the default "Access" specifier for the current class node. If
	the access is a "slot" type, "_slots" is appended to the access
	string.

=cut

sub newAccess( $ )
{
	$cNode->{KidAccess} = shift ;

	#print "for $cNode->{astNodeName} set KidAccess to $cNode->{KidAccess}\n";

	return $cNode;
}

sub closeBlock()
{
	$lastParentNode = $cNode;
	popDecl();
}

sub assignToLastParent
{
	my $name = shift;

	if( defined $name && $cNode->{astNodeName} eq "--" ) {
# structure typedefs should have no name preassigned.
# If they do, then the name in 
# "typedef struct <name> { ..." is kept instead.
# TODO: Buglet. You should fix YOUR code dammit. ;)
# 		What we could do instead is create a new subtype as closed,
#		then create vars using this type

		$cNode->{astNodeName} = $1;
		my $siblings = $cNode->{Parent}->{KidHash};
		undef $siblings->{"--"};
		$siblings->{ $name } = $cNode;
	}
}

sub lastParentDone
{
	$lastParentNode = undef;
}

=head2 newNamespace

	Param: namespace name.
	Returns nothing.

	Imports a namespace into the current node, for ref searches etc.
	Triggered by "using namespace ..."

=cut

sub newNamespace( $ )
{
	$cNode->AddPropList( "ImpNames", shift );
}

sub newQProperty( $$$$$ )
{
	my( $name, $type, $read, $write, $designable ) = @_;
	my $node = Ast::New( $name );
	$node->{NodeType} = "QProperty";
	$node->{Type} = $type;
	$node->{Read} = $read;
	$node->{Write} = $write;
	$node->{Designable} = $designable;

	attachChild( $cNode, $node );

	return $node;
}

=head2 newMethod

	Parameters: retType, name, params, const, throws, pure?

	Handles a new method declaration or definition.

=cut

sub newMethod
{
	my ( $retType, $name, $params, $const, $throws, $pure ) = @_;
	my $parent = $kdocAstGen::cNode;
	my $class;
    if ( $retType =~ /([\w\s_<>,]+)\s*::\s*$/ ) {
		# check if stuff before :: got into rettype by mistake.
		$retType = $`;
		($name = $1."::".$name);
		$name =~ s/\s+/ /g;
		print "New name = \"$name\" and type = '$retType'\n" if $main::debug;
	}

	if( $name =~ /^\s*(.*?)\s*::\s*(.*?)\s*$/ ) {
		# Fully qualified method name.
		$name = $2;
		$class = $1;

		if( $class =~ /^\s*$/ ) {
			$parent = $root;
		}
		elsif ( $class eq $cNode->{astNodeName} ) {
			$parent = $cNode;
		}
		else {
			my $node = kdocAstSearch::findRef( $cNode, $class );

			if ( !defined $node ) {
				# if we couldn't find the name, try again with
				# all template parameters stripped off:
				my $strippedClass = $class;
				$strippedClass =~ s/<[^<>]*>//g;

				$node = kdocAstSearch::findRef( $cNode, $strippedClass );

				# if still not found: give up
				if ( !defined $node ) {
						warn $main::exe.": Unidentified class: $class\n";
						return undef;
				}
			}

			$parent = $node;
		}
	}
	else {
		# Within current class/global
	}


	# flags

	my $flags = "";

	if( $retType =~ /static/ ) {
		$flags .= "s";
		$retType =~ s/static//g;
	}

	if( $const ) {
		$flags .= "c";
	}

	if( $pure ) {
		$flags .= "p";
	}

	if( defined $throws ) {
		$flags .= "e";
	}

	if( $retType =~ /virtual/ ) {
		$flags .= "v";
		$retType =~ s/virtual//g;
	}

	if ( !defined $parent->{KidAccess} ) {
		warn "'", $parent->{astNodeName}, "' has no KidAccess ",
		exists $parent->{Forward} ? "(forward)\n" :"\n";
	}

	if ( $parent->{KidAccess} =~ /slot/ ) {
		$flags .= "l";
	}
	elsif ( $parent->{KidAccess} =~ /signal/ ) {
		$flags .= "n";
	}

	# node
	
	my $node = Ast::New( $name );
	$node->{NodeType} = "method" ;
	$node->{Flags} = $flags ;
	$node->{ReturnType} = $retType ;
	$node->{Params} = $params ;
	$node->{Throws} = $throws if defined $throws;

	$parent->{Pure} = 1  if $pure;
	attachChild( $parent, $node );

	return $node;
}

sub newFnPtr( $$$$ )
{
	my ( $retType, $name, $params, $const ) = @_;
	my $parent = $kdocAstGen::cNode;
	my $class;

	# flags

	my $flags = "";

	if( $retType =~ /static/ ) {
		$flags .= "s";
		$retType =~ s/static//g;
	}

	if( $const ) {
		$flags .= "c";
	}

	if ( !defined $parent->{KidAccess} ) {
		warn "'", $parent->{astNodeName}, "' has no KidAccess ",
		exists $parent->{Forward} ? "(forward)\n" :"\n";
	}

	if ( $parent->{KidAccess} =~ /slot/ ) {
		$flags .= "l";
	}
	elsif ( $parent->{KidAccess} =~ /signal/ ) {
		$flags .= "n";
	}

	# node
	
	my $node = Ast::New( $name );
	$node->{NodeType} = "fnptr" ;
	$node->{Flags} = $flags ;
	$node->{ReturnType} = $retType ;
	$node->{Params} = $params ;

	attachChild( $parent, $node );

	return $node;
}


=head2 AST NODE CREATION

=over 2

=item attachChild

	Parameters: parent, child

	Attaches child to the parent, setting Access, Kids
	and KidHash of respective nodes.

=cut

sub attachChild
{
	my ( $parent, $child ) = @_;
	confess "Attempt to attach ".$child->{astNodeName}." to an ".
		"undefined parent\n" if !defined $parent;

	$child->{Access} = $parent->{KidAccess} ;
	$child->{Parent} = $parent ;

	$parent->AddPropList( "Kids", $child );

	my $kh = $parent->{KidHash};

	if( !defined $kh ) {
		$kh = Ast::New( "LookupTable" );
		$parent->{KidHash} = $kh ;
	}

	$kh->{$child->{astNodeName}} = $child; 

#	print "attach $child->{astNodeName} to $parent->{astNodeName} as", 
#	"$child->{Access}\n";
}

=item newInherit

	p: $node, $name, $tmplargs, $lnode?

	Add a new ancestor to $node with raw name = $name and
	node = lnode.

=cut

sub newInherit
{
	my ( $node, $name, $tmplargs, $link ) = @_;

	my $n = Ast::New( $name );
	$n->{NodeType} = "Inheritance";
	$n->{Node} = $link  unless !defined $link;
	$n->{TmplArgs} = $tmplargs;

	$node->AddPropList( "InList", $n );
	return $n;
}
1;
