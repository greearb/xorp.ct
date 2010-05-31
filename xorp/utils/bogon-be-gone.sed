#
# $XORP: xorp/utils/bogon-be-gone.sed,v 1.2 2002/12/19 01:13:17 hodson Exp $
#

#
# This is a script to coerce code into a more xorp coding style friendly
# manner.  It targets some of the common xorp developer bogons that
# everyone makes and is not an indent replacement or a magic format 
# fixer.
#
# When you use this script check the diffs and run compilation and
# tests before committing the changes back to the repository.  You will
# get eye strain looking at the diffs :-)
#

# Remove Trailing whitespaces from lines
s/[ 	]*$//

# Ignore pre-processor directives
/^#/ b

# Ignore comments at top of file
/^\/\// b

# Dont process text within C comments
/^\/\*/,/\*\/$/ b

# Dont futz with enum assignments since aligned values are okay
/enum.*{/,/};/ b

# Dont futz with const assignments since aligned values are okay
/const[^=]*=.*;/ b

# Attempt to fix missing spaces around operators with parenthetical expressions, ie if (), for(), etc...
# The list of operators is incomplete
/^[^/]/ s/\([{(;]\{0,\}\)\([A-Za-z0-9_.()]\{1,\}\)[ 	]*\([-=*&^!+<>]\{0,2\}=\)[ 	]*\([A-Za-z0-9_.()]\{1,\}\)\([});]\)/\1\2 \3 \4\5/g
/^[^/]/ s/\([{(;]\{1,\}\)\([A-Za-z0-9_.()]\{1,\}\)[ 	]*\([<>]\)[ 	]*\([A-Za-z0-9_.()]\{1,\}\)\([});]\)/\1\2 \3 \4\5/g

# Fix things that look like assignments from start of line, eg "s+= x;" -> "s += x;",
# The list of operators is incomplete
s/^\([ 	]*\)\([A-Za-z0-9_.()]\{1,\}\)[ 	]*\([-=*&^!+<>]\{0,2\}=\)[ 	]*/\1\2 \3 /g

s/\([A-z0-9)]\)==\([A-z0-9]\)/\1 == \2/g

# Put space between semi-colon/comman and the following character
s/\([;,]\)\([-+_A-Za-z0-9]\)/\1 \2/g

# Map "{a" -> "{ a"
s/{\([A-Za-z_0-9]\)/{ \1/g

# Map "a}" -> "a }"
s/\([A-Za-z_0-9]\)}/\1 }/g

# Map ";}" -> "; }"
s/;}/; }/g

# Map ";i" -> "; i"
s/;\([A-Za-z_0-9]\)/; \1/g

# Map "if(" -> "if ("
s/\([ 	]\{1,\}\)if(/\1if (/

# Map "switch(" -> "switch ("
s/\([ 	]\{1,\}\)switch(/\1switch (/

# Map "for(" -> "for ("
s/\([ 	]\{1,\}\)for(/\1for (/

# Map "while(" -> "while ("
s/\([ 	]\{1,\}\)while(/\1while (/

# Map "; };" -> "; }"
s/;\([ 	]\{1,\}\)}[ 	]*;[ 	]*$/;\1}/

# Put a space between a c-plus-plus comment and following text "//foo" -> "// foo"
s/\([ 	]*\/\/\)\([^ 	]\)/\1 \2/

# Put a newline between name of function and starting bracket.  Per style guide
# and heated debate, ie
#
#  int foo() {      -> int foo()
#      ....            {
#      ....                ....   
#  }                   }
#
# But be careful not to futz with opening brace after some common decls, eg
# class, struct, etc

/^struct/ b
/^typedef/ b
/^class/ b
/^enum/ b
/^extern/ b
/^[A-Za-z].*{[ 	]*$/ {
    s/[ 	]*{[ 	]*$//
    a\
{
}
