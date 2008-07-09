
=head1 kdocLib

Writes out a library file.

NOTES ON THE NEW FORMAT

	Stores: class name, members, hierarchy
	node types are not stored


	File Format Spec
	----------------

	header
	zero or more members, each of
		method
		member
		class, each of
			inheritance
			zero or more members



	Unrecognized lines  ignored.

	Sample
	------

	<! KDOC Library HTML Reference File>
	<VERSION="2.0">
	<BASE URL="http://www.kde.org/API/kdecore/">

	<C NAME="KApplication" REF="KApplication.html">
		<IN NAME="QObject">
		<ME NAME="getConfig" REF="KApplication.html#getConfig">
		<M NAME="" REF="">
	</C>

=cut

package kdocLib;
use strict;

use Carp;
use File::Path;
use File::Basename;

use Ast;
use kdocAstUtil;
use kdocUtil;
use kdocAstGen;


use vars qw/ $exe $lib $root $plang $outputdir $docpath $url $compress /;

BEGIN {
	$exe = basename $0;
}

sub writeDoc
{
	( $lib, $root, $plang, $outputdir, $docpath, $url, 
		$compress ) = @_;
	my $outfile = "$outputdir/$lib.kdoc";
	$url = $docpath unless defined $url;

	mkpath( $outputdir ) unless -f $outputdir;

	if( $compress ) {
		open( LIB, "| gzip -9 > \"$outfile.gz\"" ) 
			|| die "$exe: couldn't write to $outfile.gz\n";

	}
	else {
		open( LIB, ">$outfile" ) 
			|| die "$exe: couldn't write to $outfile\n";
	}

	my $libdesc = "";
	if ( defined $root->{LibDoc} ) {
			$libdesc="<LIBDESC>".$root->{LibDoc}->{astNodeName}."</LIBDESC>";
	}
	
	print LIB<<LTEXT;
<! KDOC Library HTML Reference File>
<VERSION="$main::Version">
<BASE URL="$url">
<PLANG="$plang">
<LIBNAME>$lib</LIBNAME>
$libdesc

LTEXT

	writeNode( $root, "" );
	close LIB;
}

sub writeNode
{
	my ( $n, $prefix ) = @_;
	return if !exists $n->{Compound};
	return if exists $n->{Forward} && !exists $n->{KidAccess};

	if( $n != $root ) {
		$prefix .= $n->{astNodeName};
		print LIB "<C NAME=\"", $n->{astNodeName},
			"\" REF=\"$prefix.html\">\n";
	}
	else {
		print LIB "<STATS>\n";
		my $stats = $root->{Stats};
		foreach my $stat ( keys %$stats ) {
			print LIB "<STAT NAME=\"$stat\">",
				$stats->{$stat},"</STAT>\n";
		}
		print LIB "</STATS>\n";
	}

	if( exists $n->{Ancestors} ) {
		my $in;
		foreach $in ( @{$n->{Ancestors}} ) {
			$in =~ s/\s+//g;
			print LIB "<IN NAME=\"",$in,"\">\n";
		}
	}

	return if !exists $n->{Kids};
	my $kid;
	my $type;

	foreach $kid ( @{$n->{Kids}} ) {
		next if exists $kid->{ExtSource}
			|| $kid->{Access} eq "private";

		if ( exists $kid->{Compound} ) {
			if( $n != $root ) {
				writeNode( $kid, $prefix."__" );
			}
			else {
				writeNode( $kid, "" );
			}
			next;
		}

		$type = $kid->{NodeType} eq "method" ? 
			"ME" : "M";

		print LIB "<$type NAME=\"", $kid->{astNodeName},
			"\" REF=\"$prefix.html#", $kid->{astNodeName}, "\">\n";
	}

	if( $n != $root ) {
		print LIB "</C>\n";
	}
}

sub readLibrary( \& $ \@ $ )
{
	my( $rootsub, $name, $libdirs, $relurl ) = @_;
	my $real = "";
	my $compressed = 0;

	# first find it
	foreach my $libdir ( @$libdirs ) {
		next unless -d $libdir;
		my $findit = "$libdir/$name.kdoc";

		if ( -r $findit ) {
			$real = $findit;
			$compressed = 0;
			last;
		}
		elsif ( -r "$findit.gz" ) {
			$real = "$findit.gz";
			$compressed = 1;

			last;
		}
	}

	if ( $real eq "" ) {
		warn  "$main::exe: Cannot find readable $name.kdoc or "
				."$name.kdoc.gz, skipping...";
		return;
	}

	$real =~ s#/+/#/#g;
	
	my $url = ".";
	my @stack = ();
	my $version = "2.0";
	my $new;
	
	if ( $compressed ) {
		unless ( open( LIB, "gunzip < \"$real\"|" ) ) {
			warn "Can't read pipe gunzip < \"$real\": $?. skipping...\n";
			return;
		}

	}
	else {
		unless( open( LIB, "$real" ) ) {
			warn "$main::exe: Can't read lib $real, skipping...\n";
			return;
		}
	}

	kdocAstGen::setRoot( $rootsub->( "CXX" ) ); # C++ by default

	while( <LIB> ) {
		next if /^\s*$/;
		if ( !/^\s*</ ) {
			close LIB;
			readOldLibrary( $rootsub->( "CXX" ), $name, $real );
			return;
		}

		if( /<VER\w+\s+([\d\.]+)>/ ) {
			# TODO: what do we do with the version number?
			$version = $1;
		}
		elsif ( /<BASE\s*URL\s*=\s*"(.*?)"/ ) {
			$url = $1;
			$url .= "/" unless $url =~ m#/$#;

			my $test = kdocUtil::makeRelativePath( $relurl, $url );
			$url = $test;
		}
		elsif( /<PLANG\s*=\s*"(.*?)">/ ) {
			kdocAstGen::setRoot( $rootsub->( $1 ) );
			kdocAstGen::resetStack();
		}
		elsif ( /<C\s*NAME="(.*?)"\s*REF="(.*?)"\s*>/  ) {
			$new = kdocAstGen::newClass( "", "class", $1, 0 );
			$new->{Compound} = 1 ;
			$new->{ExtSource} = $name ;

			# already escaped at this point!
			$new->{Ref} = $url.$2 ;
		}
		elsif ( m#<IN\s*NAME\s*=\s*"(.*?)"\s*># ) {
			# ancestor
			kdocAstGen::newInherit( $kdocAstGen::root, $1 );
		}
		elsif ( m#</C># ) {
			# end class
			kdocAstGen::popDecl();
		}
		elsif ( m#<(M\w*)\s+NAME="(.*?)"\s+REF="(.*?)"\s*># ) {
			my ($type, $name, $ref ) = ( $1, $2, $3 );
			if ( $type eq "ME" ) {
				$new = kdocAstGen::newMethod( "", $name, "", 0, 0 );
			}
			else {
				$new = kdocAstGen::newVar( "", $ref, "" );
			}
			$new->{ExtSource} = $name ;
			$new->{Ref} = $url.$ref ; #FIXME relative?
		}
	}
}

=head2 readLibrary

	Parameters: rootnode, libname.

	Read a kdoc 1.0 library into the node tree. Each external class
	will have its "ExtSource" property set to the library name.

=cut

sub readOldLibrary
{
	my ( $root, $libname, $libdir ) = @_;

	my @nodeStack = ();
	my $cnode = $root;
	my $fullpath = $libdir."/".$libname.".kdoc";
	my $liburl = "";
	my $newNode;
	my $newMem;
	
	open( LIB,  $fullpath) || die "$exe: Can't read library $fullpath\n"; 

	$liburl = <LIB>;
	carp "Empty libfile: $fullpath\n" if !defined $liburl;
	$liburl =~ s/\s+//g;

	while( <LIB> ) {
		# class url
		next if !/^([^=]+)=/;
		my $src = $1;
		my $target = $';
		if ( $src =~ /::/ ) {
			# member
			next if !defined $newNode 
				|| $newNode->{astNodeName} ne $src;
			$newMem = Ast::New( $' );
			$newMem->{NodeType} = "Anon" ;
			$newMem->{Ref} = $liburl."/".$target ;
			kdocAstGen::attachChild( $newNode, $newMem );
		}
		else {
			# class
			$src =~ s/^\s*(.*?)\s*$/$1/g;
			$newNode = Ast::New(  $src );
			$newNode->{NodeType} = "class" ;
			$newNode->{ExtSource} = $libname ;
			$newNode->{Compound} = 1 ;
			$newNode->{KidAccess} = "public" ;
			$newNode->{Ref} = $liburl."/".$target ;
			kdocAstGen::attachChild( $root, $newNode );
		}
	}

	close( LIB );
}

1;
