
package kdocParseCxx;

use Carp;

use Ast;
use kdocParseDoc;
use kdocAstGen;
use kdocAstUtil;

use strict;
use vars qw/
$ancestor $ancestry $any $array $cSourceNode $classdecl $currentfile
$docNode $docincluded $enum $extern $fnargs $fninit $fnmodifier $fnname
$friend $function $ident $inExtern $inaccess $kcompidl $kcompound
$namespace $operator $rootNode $sident $template $tmpldecl $type
$typetoken $typetoken1 $typetoken2 $value $var $variable $visibility
$xancestor $xvar %stats $qproperty $propfunction $xpropfunction $fnptr 
$mfnptr $throw $inEnum $enumName/;

=head1 C++/IDL Parser

	type: ident ident* [&*]*
		
	classdecl: tmpltype? ckeyword classname ancestry?
		tmpltype: "template" template
		ckeyword:	"struct" | "class" | "namespace" | "union"
		classname:	ident
		ancestry: ":" inaccess* type ("," inaccess* type)*
		inaccess: "public" | "private" | "protected" | "virtual"

	typedef: "typedef" "typename"? type var

	enum: "enum" ident? 

	function: type? fnname "(" fnargs ")" fnmodifier* initializers?
		fnname: var | operator
		operator: "operator" ( "()" | [^\)]+ )
		fnargs: any?
		fnmodifier: "const" | /=.*0/
		intializers: : var "(" initval ")" ("," var "(" initval ")" )*
		initval: any
	
	variables: type var value? ("," var value?)*
		value: "=" any [,;]

	friends:	"friend" friend
		friend:	(ckeyword var) | function

x	function pointers:
	namespace modifier block:	"using"	type
	forward decl:	classdecl
	extern blocks: "extern" \"text\"

x	comments:
x	typedef blocks:
	visibility specifiers:	words ":"
	
x
x	lowlevel
		template: "<" tmplargs? ">";
			tmplargs:	targ ("," targ)*
				targ:	sident ("<" targ2 ("," targ2)* ">")?
				targ2:	sident ("<" targ3 ("," targ3)* ">")?
				targ3:	sident ("<" sident ("," sident)* ">")?

		sident:
		ident: (sident template?)+

		array
x		any
		var: ident template? array?

=cut

sub sp
{
	my $str = shift;
	$str =~ s/\s+/\\s*/g;

	return "(?:$str)";
}

sub recursePairs
{
	my( $ochr, $cchr, $depth ) = @_;

	return "[^$ochr$cchr]" if $depth <= 0;

	my $text = recursePairs( $ochr, $cchr, $depth - 1 );
	my $frep = $depth > 1 ? "" : "+";
	my $rep = $depth > 1 ? "?" : "*";

	return "(?:(?>$text$frep)|$ochr$text$rep$cchr)*";

	
}

=head2 initRegexps

The base regexps used to parse C++ and IDL.

=cut

sub initRegexps
{
# parser regexp init
# NOTE: spaces in regexp strings are converted by sp(), so
# DON'T DELETE SPACES IN THE REGEXPS!

	$kcompound = sp( 'struct|union|class|namespace' );
	$kcompidl = sp( 'module|interface|exception' );



	$sident = sp("(?:(?>\\:\\:)|(?:[_\\w~]+))+");
#	$sident = '(?:(?:(?:\\:\\:\\s*)?[_\w~]+)+)';

	$template = sp( "<".recursePairs( "<", ">", 2 )."?>" );
	$tmpldecl = sp( "template $template" );

	my $sarray = sp( '\[[^\[\]]*\]' );
	my $array = sp( "$sarray(?: $sarray)*" );
	$ident = sp( "(?:$sident|(?:\\s*$template))+" );
	$any = ".*";

	$var = sp( "(?>$ident) $array?" );
	$xvar = sp( "((?>$ident)) ($array?)");
	my $typetok = sp("$ident"."[*&]*");
	$type = sp( "$typetok(?:\\s+(?:[*&]+|$typetok))*" );

	$inaccess = sp("public|private|protected|virtual");
	$ancestor = sp( "(?:$inaccess\\s+)*$type" );
	$xancestor = sp( "((?:$inaccess\\s+)*)\\s*($type)" );
	$ancestry = sp( ": $ancestor(?: , $ancestor)*" );

	$classdecl = sp( "($tmpldecl?) ((?>$kcompound|$kcompidl))\\s+"
			."(?:[\\w_]+\\s+)?($ident) ($ancestry?)" );

	$enum = "enum(\\s+$ident)?";

	# FIXME to resolve namespace for operator too :(
	$operator = 
		sp( "operator (?:(?:\\( \\))|(?:[^\\(]+))" );
#		print "$operator\n";
	$fnname = sp( "$ident|$operator" );

	$fnargs = recursePairs( "\\(", "\\)", 2 );

	$fninit = sp( ":[^{;]+" );
	$throw = sp( "throw \\($fnargs\\)" );

	$function = sp( "((?:$type\\s+[*&]*)?)($fnname) "
			."\\(($fnargs?)\\) ((?>const)?) ($throw?) ((?>= 0)?)"
			." $fninit?" );

	$fnptr = sp( "((?:$type\\s+[*&]*)?)\\(([*\\s]+)($fnname) \\) "
			."\\(($fnargs?)\\) ((?>const)?)");

	$mfnptr = sp( "((?:$type\\s+[*&]*)?)"
		        ."\\((\\w+::)?([*\\s]+)($fnname) \\) "
			."\\(($fnargs?)\\) ((?>const)?)");

	$value = sp( "(?>[:=])[^,;]*" );
	$variable = sp( "($type\\s+[&*]*)($var) ($value?)");

	$friend	= sp( "friend\\s+((?:$kcompound\\s+$var)|$function)" );

	my $viskey = sp("public|private|protected|signals|k_dcop");
	$visibility	= "($viskey(?:\\s+[\\w_]+)*)\\s*\:";

	$extern = sp( "extern\\s+\"[\\w+]+\"" );

	$namespace = sp( "using\\s+($ident)" );

	$propfunction = sp( "(?:READ|WRITE|DESIGNABLE)\\s+(?:$ident)" );
	$xpropfunction = sp( "(READ|WRITE|DESIGNABLE)\\s+($ident)" );
	$qproperty = sp( "Q_PROPERTY \\( ($ident)\\s+($ident)((?:\\s+$propfunction)+) \\)" );

}

sub BEGIN 
{
# a lot of these need to be set externally.

	%stats = ();
	$inExtern = 0;
	$docNode = undef;
	$docincluded = 0;
	$rootNode = undef;

	initRegexps();
}


##########################################################3
##########################################################3
##########################################################3
##########################################################3

# Module entry point

sub parseStream
{
	( $rootNode, $cSourceNode, *INPUT ) = @_;
	my $k = undef;
	$inExtern = 0;
	$inEnum = 0;

	readDecls();

}



# Stream parser

sub readDecls {
	local *INPUT = shift;

	my $full = "";
	my $done = 0;

	while( !$done ) {
		if ( $full =~ /([;{}]|\/\*)/xs ) {
			# decl ( C++ or comment )
			my $char = $1;
			my $ctext = $';

			if ( $char eq "/*" ) {
				# start of comment

				if ( $ctext =~ /^\*+/  ){
					# doc comment
					my $buffer = "";
					$full =~ s/(?:Q_OBJECT|Q_ENUMS|K_SYCOCATYPE|K_DCOP|Q_ENUMS|Q_CLASSINFO|K_SYCOCAFACTORY|Q_OVERRIDE)(?:\s*\([^\)]*\))*//gs;
					identifyDecl( \$buffer, $full );
					$buffer = "" if $buffer =~ /^\s*\n$/s;
					$full = $buffer;
				}
				else {
					# normal comment, skip it

					while( 1 ) {
						if ( !defined $ctext  ) {
							$done = 1;
							last;
						}
						elsif( $ctext =~ /\*\//  ) {
							$full = $';
							$full = "" if $full =~ /^\s*\n$/s;
							last;
						}
						$ctext = readCxxLine();
					}
				}
			}
			else {
				# C/C++ decl
				$full = $`.$char;
				$full =~ s/(?:Q_OBJECT|Q_ENUMS|K_SYCOCATYPE|K_DCOP|Q_ENUMS|K_SYCOCAFACTORY|Q_CLASSINFO|Q_OVERRIDE)(?:\s*\([^\)]*\))*//gs;
				my $rest = "";
				my $toskip = identifyDecl( \$rest, $full );

				# if $rest is set it means that identifyDecl didn't do anything with that part of the text.
				# we need to prepend( ? ) it to the buffer for processing again.
				# BUT: what happens if a whole set of new lines has been read since then? We will need to
				# throw away the old "rest" text.

				if( $toskip ) {
					# decl asked for a C++ code block to be skipped.
					# what do we do with rest here?
					print "Calling skip with '$char' '$ctext'\n" if $main::debug;
					$full = skipCxxCodeBlock( $char.$ctext );
				}
				else {
					# decl fully consumed, now for next one
					$full = $rest.$ctext;
				}

				$full = "" if $full =~ /^\s*\n$/s;
			}
		}
		else {
			# nope, no decl found this time.
			last if $done;

			my $ntext;
			while( 1 ) {
				$ntext = readCxxLine();
				if ( defined $ntext  ) {
					last unless $ntext =~ /^\s*$/s;
				}
				else {
					# finished
					$ntext = "";
					$done = 1;
					last;
				}
			}
			$full .= $ntext;
		}
	}
}

sub skipCxxCodeBlockNew
{
	local *INPUT = shift;
	my $l= shift;

	my $open = kdocUtil::countReg( $l, "{" );
	my $close = kdocUtil::countReg( $l, "}" );

	my $count = $open - $close;

	return if $count == 0 && $open == 1;

	# find opening brace
	if ( $count == 0 ) {
		while( $count == 0 ) {
			$l = readCxxLine();
			return undef if !defined $l;
			$l =~ s/\\.//g;
			$l =~ s/'.?'//g;
			$l =~ s/".*?"//g;

			$count += kdocUtil::countReg( $l, "{" );
			print "c ", $count, " at '$l'" if $main::debug;
		}
		$count -= kdocUtil::countReg( $l, "}" );
	}
}

sub readCxxLine
{
	my $line;
	my $done = 0;

	while( !$done ){
		$line = <INPUT>;
		return undef unless defined $line;

		$line =~ s,/\*.*?\*/,,g;
		$line =~ s,//.*$,,g;
		if( $line =~ /^\s*#\s*\w+/ ) {
			while( $line =~ m,\\\s*$, ) {
				$line = <INPUT>;
			}
			next;
		}

		$done = 1;
	}
# TODO: preprocessor file context

	return $line;
}


####################################################################################
####################################################################################




=head2 skipCxxCodeBlock

	Reads a C++ code block (recursive curlies), returning the last line
	or undef on error.

	Parameters: none

=cut


sub skipCxxCodeBlock
{
# Code: begins in a {, ends in }\s*;?
# In between: cxx source, including {}

	my $l = shift;
	my ( $count ) = 0;
	
	print "Skip block start at: $l\n" if $main::debug;;
	my $open = kdocUtil::countReg( $l, "{" );
	my $close = kdocUtil::countReg( $l, "}" );
	$count = $open - $close;
	print "Skip: after first line:( $open, $close, $count )\n" if $main::debug;

	# block opens and closes on same line
	if ( $count == 0 && $open > 0  ) {
		print "Skip block end ( zero length ) at: $l\n" if $main::debug;
		return "";
	}

	# find opening brace
	if ( $count == 0 ) {
		while( $count == 0 ) {
			$l = <INPUT>;
			croak "Confused by unmatched braces" if !defined $l;
			print "Skip block open search at: $l\n" if $main::debug;
			$l =~ s/\\[{}]//g;
			$l =~ s/'[^']*'//g;
			$l =~ s/"[^"]*"//g;

			$count += kdocUtil::countReg( $l, "{" );
			print "c ", $count, " at '$l'" if $main::debug;
		}
		$count -= kdocUtil::countReg( $l, "}" );
	}

	# find associated closing brace
	while ( $count > 0 ) {
		$l = <INPUT>;
		croak "Confused by unmatched braces" if !defined $l;
		$l =~ s/\\.//g;
		$l =~ s/'.?'//g;
		$l =~ s/".*?"//g;

		my $add = kdocUtil::countReg( $l, "{" );
		my $sub = kdocUtil::countReg( $l, "}" );
		$count += $add - $sub;

		print "TEXT($add,$sub,$count): $l\n" if $main::debug;
	}

	print "Skip block end at: $l\n" if $main::debug;
	return "";
}

##########################################################3
##########################################################3
##########################################################3
##########################################################3



=head2 identifyDecl

	Parameters: decl

	Identifies a declaration returned by readDecl. If a code block
	needs to be skipped, this subroutine returns a 1, or 0 otherwise.

=cut

sub identifyDecl
{
	my( $srest, $decl ) = @_;
	print "Decl: <$decl>\n" if $main::debug;

	# Remove heading __BEGIN_DECLS or __END_DECLS
	if ($decl =~ /^\s*(__BEGIN_DECLS|__END_DECLS)(\s|$)+/) {
		$decl =~ s/^\s*(__BEGIN_DECLS|__END_DECLS)//;
		print "Decl2: <$decl>\n" if $main::debug;
	}

	my $newNode = undef;
	my $skipBlock = 0;

	# Access specifier
	if( $decl =~ /^\s*$visibility/os ){
		$$srest = $';
		kdocAstGen::newAccess( $1 );
		return 0;
	}

	# doc comment
	if ( $decl =~ m:^\s*/\*\*+:s ){
		$docNode = kdocParseDoc::newDocComment( $', *INPUT, $srest );

		# if it's the main doc, it is attached to the root node
		if ( defined $docNode->{LibDoc} ) {
			kdocParseDoc::attachDoc( $rootNode, $docNode,
					$rootNode );
			undef $docNode;
		}

		return 0;
	}

	# we are current parsing an enum and we have all the parameters now
	if( $inEnum ){
		$inEnum = 0;
		$decl =~ s/}.*//g;
		print "Enum complete: params $decl\n" if $main::debug;
		postInitNode( kdocAstGen::newEnum( $enumName, "", $decl ) );
		return 0;
	}
	
	# End of block TODO work out context to work out if var decls or ; are expected.
	if ( $decl =~ /^\s*}/s ){
		print "Block End: $decl\n" if $main::debug;
		if( $inExtern ){
			$inExtern = 0;
		}
		else {
			kdocAstGen::closeBlock();
		}
		return 0;

	}
	if ( $decl =~ /^\s*;/s ){
		kdocAstGen::lastParentDone();
		return 0;

	}

	# Qt Property
	if( $decl =~ /^\s*$qproperty/os ){
		my( $ptype, $pname, $fnlist ) = ( $1, $2, $3 );
		$$srest = $'; # since it doesn't end on a "normal" decl boundary
		my $read = undef;
		my $write = undef;
		my $designable = undef;

		while( $fnlist =~ /^\s*$xpropfunction/os ){
			my ( $mode, $func ) = ( $1, $2 );
			$fnlist = $';
			if( $mode eq "READ"  ){
				$read = $func;
			}
			elsif( $mode eq "WRITE" ){
				$write = $func;
			}
			elsif( $mode eq "DESIGNABLE" ){
				$designable = $func;
			}
			else {
				warn "unknown property function mode \"$mode\" in '$decl'";
			}
		}

		postInitNode( kdocAstGen::newQProperty( $pname, $ptype, $read, $write, $designable ) );
		return 0;
	}

	# Typedef struct/class
	if ( $decl =~ /^\s*typedef
			\s+($kcompound|enum)
			\s*($ident?)
			\s*([;{]) 
			/xso ) {
		my ($type, $name, $endtag) = ($1, $2, $3 );
		my $rest = $';
		$name = "--" if $name eq "";

		warn "typedef '$type' n:'$name'\n" if $main::debug;

		if ( $rest =~ /}\s*([\w_]+(?:::[\w_])*)\s*;/ ) {
			# TODO: Doesn't parse members yet!
			$endtag = ";";
			$name = $1;
			$rest = $';
		}

		my ($iscomp) = $endtag eq "{" ? 1 : 0;

		# Check if "typedef enum ..."
		if ( $decl =~ /^\s*typedef
			\s+(enum)
			\s*($ident?)
			\s*([;{]) 
			/xso ) {
			print "Enum: <$name>\n" if $main::debug;
			my $enumname = $name;
			$enumName = $enumname;	# keep for later, when we've
						# finished parsing this.
			$inEnum = 1;
			$iscomp = 0;
		}

		$newNode = kdocAstGen::newTypedefComp( $type, $name, $iscomp );
		postInitNode( $newNode );
		return 0;
	}

	# Typedef
	if ( $decl =~ /^\s*typedef\s+
			(?:typename\s+)?        	# `typename' keyword
			(.*?\s*[\*&]?)			# type
			\s+($ident)			# name
			\s*($array?)			# array
			\s*[{;]\s*$/xso  ) {

		print "Typedef: <$1 $3> <$2>\n" if $main::debug;
		$newNode = kdocAstGen::newTypedef( $1." ".$3, $2 );
		postInitNode( $newNode );
		return 0;
	}

	# Enum
	if ( $decl =~ /^\s*enum((?:\s+$ident)?)\s*{/xso  ) {
		print "Enum: <$1>\n" if $main::debug;
		my $enumname = $1;
		$enumname =~ s/^\s+//g;
		$enumname =~ s/\s+$//g;
		$enumName = $enumname;	# keep for later, when we've finished
								# parsing this.
		$inEnum = 1;
		return 0;
	}
	# Using: import namespace
	if ( $decl =~ /^\s*$namespace/xso ) {
		kdocAstGen::newNamespace( $1 );
		return 0;
	}

	# extern block
	if ( $decl =~ /^\s*$extern\s*{/xso ) {
		$inExtern = 1;
		return 0;
	}

	print "Check class\n" if $main::debug;
	# Class/Struct
	if ( $decl =~ /^\s*$classdecl\s*([;{])/xso ) {
		print "Class: [$1]\n\t[$2]\n\t[$3]\n\t[$4]\n\t[$5]\n" if $main::debug;
		my ( $tmpl, $ntype, $name, $rest, $endtag ) =
			( $1, $2, $3, $4, $5 );

		my $isCompound = 1;

		$tmpl =~ s/.*?<(.*)>/$1/ if $tmpl ne "";

		unless(  $rest =~ /^\s*$/ || $rest =~ /^\s*:\s*/ ) { # inheritance 
			my $val = "";

			if ( $rest =~ /^((?:\s*[&*])+)/ ) {	# real name
				$name .= $1;
				$rest = $';
			}
			if ( $rest =~ /=\s*(.+)/ ) { # value
				$rest = $`;
				$val = $1;
			}

			$newNode = kdocAstGen::newVar( $ntype." ".$name, $rest, $val );
			$isCompound = 0;
		}

		if ( $isCompound ) {
			$newNode = kdocAstGen::newClass( $tmpl, $ntype, $name, 
					$endtag eq "{" ? 0 : 1);
			parseInheritance( $rest,
				sub {
					my ( $type, $class, $tmplargs ) = @_;
					if ( $type eq "" ) {
						$type = $ntype eq "class" ? "private" : "public";
					}
					
					my $in = kdocAstGen::newInherit( $newNode, $class,
							$tmplargs );
					$in->{Type} = $type;
				}
			);
		}

		postInitNode( $newNode) if defined $newNode;
		return 0;
	}

	print "Check method\n" if $main::debug;
	# Method
	if ( $decl =~ /^\s*$function\s*([;{])/xso ) {
		print "Method: T:[$1] N:[$2]\tP:[$3]\n\t[$4]\n" if $main::debug;

		my ( $type, $name, $params, $const, $throws, $pure, $end ) =
			( $1, $2, $3, $4 ne "", $5, $6 ne "", $7 );
			
		$newNode = kdocAstGen::newMethod( $type, $name, $params, 
				$const, $throws, $pure );
		postInitNode( $newNode );

		return $end eq "{" ? 1 : 0;
	}

	print "Check tmplvar\n" if $main::debug;
	# templated Variable
	if ( $decl =~ /^\s*($tmpldecl)\s*$variable\s*([;{,])/xso ) {
		print "Tmpl Var: T:[$2] N:[$3]\tP:[$4]\n\t[$5]\n" if $main::debug;

		my $tmpl = $1;
		my $type = $2;
		my $name = $3; # type + name
		my $val = $4;
		my $end = $5;

		$newNode = kdocAstGen::newVar( $type, $name, $val );

		if ( $end eq "," ) { # multivar
			my $rest = $';
			my $doc = $docNode;
			$rest =~ s/([;{]).*$//;
			$end = $1;
			$type =~ s/[&*]+\s*$//g;

			foreach my $svar ( split( ",", $rest ) ) {
				next unless $svar =~ 
								/^\s*([*\s&]*)\s*($var)((?:\s*$value)?)/xso;
				my $name = $2;
				my $val = $3;
				print "MULTI: ($type $1) ($name)\n" if $main::debug;
				my $node = kdocAstGen::newVar( $type.$1, $name, $val );
				$docNode = $doc;
				postInitNode( $node );
			}

			$docNode = $doc;
		}

		return $end eq "{" ? 1 : 0;


	}

	print "Check tmplmethod\n" if $main::debug;
	# templated Method
	if ( $decl =~ /^\s*($tmpldecl)\s*$function\s*([;{])/xso ) {
		print "Tmpl Method: T:[$2] N:[$3]\tP:[$4]\n\t[$5]\n" if $main::debug;

		my $type = $2;
		my $name = $3; # type + name
		my $params = $4;
		my $const = $5 eq "" ? 0 : 1;
		my $pure = $6 eq "" ? 0 : 1;
		my $end = $7;

		$newNode = kdocAstGen::newMethod( $type, $name, $params, 
				$const, $pure );

		return $end eq "{" ? 1 : 0;
	}


	print "Check var\n" if $main::debug;
	# Variable
	if ( $decl =~ /^\s*$variable\s*([;{,])/xso ) {
		my $type = $1;
		my $name = $2;
		my $val	 = $3;
		my $end	 = $4;

		print "Var: [$name] type: [$type] val: [$val]\n" if $main::debug;

		$newNode = kdocAstGen::newVar( $type, $name, $val );

		if ( $end eq "," ) { # multivar
			my $rest = $';
			my $doc = $docNode;
			$rest =~ s/([;{]).*$//;
			$end = $1;
			$type =~ s/[&*]+\s*$//g;

			foreach my $svar ( split( ",", $rest ) ) {
				next unless $svar =~ 
								/^\s*([*\s&]*)\s*($var)((?:\s*$value)?)/xso;
				my $name = $2;
				my $val = $3;
				print "MULTI: ($type $1) ($name)\n" if $main::debug;
				my $node = kdocAstGen::newVar( $type.$1, $name, $val );
				$docNode = $doc;
				postInitNode( $node );
			}

			$docNode = $doc;
		}

		return $end eq "{" ? 1 : 0;

	}


	# Pointer to Function
	if ( $decl =~ /^\s*$fnptr\s*([;{])/xso ) {
		print "FnPtr: T:[$1] N:[$3]\tP:[$4]\n\t[$5]\n" if $main::debug;

		my ( $type, $ptr, $name, $params, $const ) 
			= ( $1, $2, $3, $4, $5 );

		$newNode = kdocAstGen::newFnPtr( $type, $name, $params, $const );
		postInitNode( $newNode );
		return 0;
	}

	# Pointer to Member Function
	if ( $decl =~ /^\s*$mfnptr\s*([;{])/xso ) {
		print "FnPtr: T:[$1] N:[$2$4]\tP:[$3]\n\t[$5]\noh and 2=$2\n" if $main::debug;

		my ( $type, $ptr, $name, $params, $const ) 
			= ( $1, $3, $2 . $4, $5, $6 );

		$newNode = kdocAstGen::newFnPtr( $type, $name, $params, $const );
		postInitNode( $newNode );
		return 0;
	}

	# identifiers following a block definition, need to create and attach
	# one or more var or typedef nodes to the previous parent
	if( defined $kdocAstGen::lastParentNode ) {
		my $lastParent = $kdocAstGen::lastParentNode;

# TODO TODO TODO
#		foreach my $var ( split( /[\s,]+/, $decl )) {
#			print "Variable: '$var' attaches to ", 
#					"$kdocAstGen::lastParentNode->{astNodeName}\n";
#		}
#		my $real = $kdocAstGen::lastParentNode;
	}

	# unidentified block start
	elsif ( $decl =~ /{/ ) {
		print "Unidentified block start: $decl\n" if $main::debug;
		$skipBlock = 1;
	}
	else {

		## decl is unidentified.
		warn "Unidentified decl: '$decl' at\n";
	}

	# once we get here, the last doc node is already used.
	# postInitNode should NOT be called for forward decls
	postInitNode( $newNode ) unless !defined $newNode;

	return $skipBlock;
}

sub postInitNode
{
	my $newNode = shift;

	carp "Cannot postinit undef node." if !defined $newNode;

	# The reasoning here:
	# Forward decls never get a source node.
	# Once a source node is defined, don't assign another one.

	if ( $newNode->{NodeType} ne "Forward" && !defined $newNode->{Source}) {
		$newNode->{Source} = $cSourceNode ;
	}
	elsif ($main::debug) {
		print "postInit: skipping fwd: $newNode->{astNodeName}\n";
	}

	if( defined $docNode ) {
		kdocParseDoc::attachDoc( $newNode, $docNode, $rootNode );
		undef $docNode;
	}
}


=head2 parseInheritance

	Param: inheritance decl string, subref( $type, $class, $tmplargs ) 

=cut

sub parseInheritance( $$ )
{
	my ($instring, $sIn ) = @_;

	$instring =~ s/^\s+//;
	$instring =~ s/\s+$//;

	while( $instring =~ /^\s*[:,]*\s*$xancestor/o ) {
		my $type = $1;
		my $class = $2;
		my $tmpl = "";
		$instring = $';

		if ( $class =~ /($template)\s*$/xso ) {
			$class = $`;
			$tmpl = $1;
		}
		$sIn->( $type, $class, $tmpl );

	}
}

1;
