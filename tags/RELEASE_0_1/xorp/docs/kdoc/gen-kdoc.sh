#!/bin/sh
#
# $XORP: xorp/docs/kdoc/gen-kdoc.sh,v 1.9 2002/12/11 19:09:53 hodson Exp $
#

#
# Script to generate kdoc documentation for XORP.  
#

#
# Defaults
#
DEBUG=0

#
# Kdoc format
#
DEFAULT_KDOC_FORMAT="html"
KDOC_FORMAT="${DEFAULT_KDOC_FORMAT}"
KDOC_FORMATS="html check man texinfo docbook latex"

#
# Input and output directories
#
SRC_DIR="../.."
KDOC_DEST_DIR="${SRC_DIR}/docs/kdoc"
KDOC_LIB_DIR="${KDOC_DEST_DIR}/libdir"

#
# Variable for HTML top-level index generation
#
HTML_TEMPLATES="html_templates"
HTML_INDEX="html/index.html"
HTML_INDEX_DATA="html/index.dat"

#
# Misc. pre-defined variables
#
HTML_LOGO="http://www.xorp.org/xorp2.png"

#
# Print message to stderr if DEBUG is set to non-zero value
#
dbg_echo()
{
    [ ${DEBUG:-0} -ne 0 ] && echo "$*" >&2
}

#
# glob_files [<glob_pattern1> [<glob pattern2> ...]]
#
# Return glob expanded list of files
# Takes glob patterns as arguments, eg 
#  glob_files *.hh *.h
#
glob_files()
{
    FILES=""
    while [ $# -ne 0 ] ; do
	RFILES=`ls ${SRC_DIR}/$1 2>/dev/null`
	if [ -n "${RFILES}" ] ; then
	    FILES="$FILES $RFILES"
	fi
	shift
    done
    echo $FILES
}

#
# exclude files <exclude_glob_pattern> <file_list>
#
# Generates list of exclude files from <exclude_glob_pattern> and filters
# them from <file_list>
#  
exclude_files()
{
    if [ $# -eq 1 ] ; then
    	echo "exclude file usage" >&2
    fi
    excluded=`glob_files $1`
    echo "excluded=$excluded" >&2
    shift

    if [ -z "$excluded" ] ; then
	echo "$*"
	return
    fi

    included=""
    for i in $* ; do
	ex=0
	for x in $excluded ; do
	    if [ "$i" = "$x" ] ; then
		ex=1
	    fi
	done
	if [ $ex -eq 0 ] ; then
	    included="$included $i"
	fi
    done
    echo "${included}"
}

#
# Generate xref arguments
#
xref()
{
    XREF=""
    while [ $# -ne 0 ] ; do
	XREF="$XREF --xref $1"
	shift
    done
    echo ${XREF}
}

#
# Run kdoc
# Expects variables "lib", "files", "excludes", "xref" to be set.
#
kdocify() {
    echo "Processing lib $lib"
    echo "   Parameters files=\"$files\" excludes=\"$excludes\" xref=\"$xref\""

    excludes=${excludes:-NONE}
    FILES=`glob_files $files`
    FILES=`exclude_files "$excludes" "$FILES"`
    XREFS=`xref $xref`
    OUTDIR=${KDOC_DEST_DIR}/${KDOC_FORMAT}/${lib}

    echo "Making $OUTDIR and reading $FILES"
    mkdir -p $OUTDIR

    echo ${FILES} |							      \
    kdoc --outputdir ${OUTDIR}						      \
	 ${XREFS}         						      \
	 --format ${KDOC_FORMAT}					      \
	 --strip-h-path        	       					      \
	 --name $lib         						      \
	 --libdir ${KDOC_LIB_DIR}					      \
	 --html-logo ${HTML_LOGO}					      \
	 --stdin							      \
    | tee ${OUTDIR}/log
    if [ "${KDOC_FORMAT}" = "html" ] ; then
	html_index_add "${lib}" "${desc}" "${html_start_page}"
    fi
}

html_index_start()
{
    mkdir html
#
# All the html here is cut and paste from what kdoc generates, it will need
# updating if we move to style sheets or kdoc changes.
#
    cat /dev/null > ${HTML_INDEX_DATA}
}

html_index_end()
{
    local DIR_NAME DESC START_PAGE START_URL WHO WHERE WHEN WITH ROW_BG i

    OIFS="$IFS"
    IFS="@"

    echo "Generating ${HTML_INDEX}"

    cat ${HTML_TEMPLATES}/index.html.top |				      \
        sed -e "s<@HTML_LOGO@<${HTML_LOGO}<" > ${HTML_INDEX}

    i=0
    cat ${HTML_INDEX_DATA} | \
	while read DIR_NAME DESC START_PAGE ; do
	    # Frivalous coloring of cell background to match kdoc
	    if [ `expr $i % 2` -eq 1 ] ; then
		ROW_BG="bgcolor=\"#eeeeee\""
	    else
		ROW_BG=""
	    fi
	    START_URL=${DIR_NAME}/${START_PAGE}
	    cat ${HTML_TEMPLATES}/index.html.entry | sed 		      \
		-e "s<@START_URL@<${START_URL}<"			      \
		-e "s<@DIR_NAME@<${DIR_NAME}<"				      \
		-e "s<@DESC@<${DESC}<"					      \
		-e "s<@ROW_BG@<$ROW_BG<"				      \
		>> ${HTML_INDEX}
	    i=`expr $i + 1`
	done

    WHO=`whoami`
    WHEN=`date`
    WHERE=`hostname`
    WITH=`echo $0 | sed -e 's@.*/@@g'`
    cat ${HTML_TEMPLATES}/index.html.bottom | sed			      \
	-e "s<@WHO@<${WHO}<"						      \
	-e "s<@WHERE@<${WHERE}<"					      \
	-e "s<@WHEN@<${WHEN}<"						      \
	-e "s<@WITH@<${WITH}<"						      \
	>> ${HTML_INDEX}

    # Clean up and restore
    rm -f ${HTML_INDEX_DATA}
    IFS="$OIFS"
}

html_index_add()
{
    echo "$1@$2@$3" >> ${HTML_INDEX_DATA}
}

#
# Print usage string
#
usage()
{
    cat >&2 <<EOF 
usage: gen-kdoc.sh [-f <format>]

<format> must be one of "${KDOC_FORMATS}"
  default="${DEFAULT_KDOC_FORMAT}"

Note: option -f can be applied once.

EOF

    exit 1
}

validate_format()
{
    local FMT
    for FMT in ${KDOC_FORMATS} ; do
	if [ "X${FMT}" = "X${KDOC_FORMAT}" ] ; then
	    return
	fi
    done

    usage
}

#
# Main
#
while getopts "f:h?" ch
do
    case $ch in
        f) KDOC_FORMAT=$OPTARG ;;
        *) usage ;;
    esac
done

validate_format

if [ "${KDOC_FORMAT}" = "html" ] ; then
    html_index_start
fi

###############################################################################
#
# Most of the work of this script is done by the kdocify routine.  kdocify
# relies on the existence of a number of global variables to run correctly.
#
# The following MUST  be set before calling kdocify:
#
# lib		  := directory name as it appears under the top level 
#		     of the xorp tree.
#
# desc 		  := textual description for the top-level html documentation.
#
# html_start_page := html start page under the html/${lib}/ directory.  For
#		     pure C code, the kdoc index.html file is not useful
#		     since it normally lists classes and structures and not
#		     globally defined functions.
#
# files		  := files to parsed by kdoc under the top level of the xorp
#		     tree.
#
# excludes	  := files to excluded from from parsing.  Exists since
#		     it's easier to to wildcard include files, then exclude 
#		     a few.
#
# xref		  := list of already kdoc'ed directories to cross reference
#		     against.
#
###############################################################################

#
# libxorp
#
lib="libxorp"
desc="XORP core type and utility library"
html_start_page="index.html"
files="libxorp/*.h libxorp/*.hh" 
excludes="libxorp/callback.hh libxorp/old_trie.hh"
xref="" 
kdocify

#
# callback.hh in libxorp
#
lib="libxorp-callback"
desc="XORP callback routines"
html_start_page="index.html"
files="libxorp/callback.hh" 
excludes=""
xref="$xref libxorp"
kdocify

#
# libxipc
#
lib="libxipc"
desc="XORP interprocess communication library"
html_start_page="index.html"
files="libxipc/*.h libxipc/*.hh"
excludes=""
xref="$xref libxorp-callback"
kdocify

#
# libcomm
#
lib="libcomm"
desc="Socket library"
html_start_page="all-globals.html"
files="libcomm/*.h libcomm/*.hh"
excludes=""
xref="$xref libxipc"
kdocify

#
# libproto
#
lib="libproto"
desc="Protocol Node library used by XORP multicast processes"
html_start_page="index.html"
files="libproto/*.h libproto/*.hh"
excludes=""
xref="$xref libcomm"
kdocify

#
# mrt
#
lib="mrt"
desc="Multicast Routing Table library"
html_start_page="index.html"
files="mrt/*.h mrt/*.hh"
excludes=""
xref="$xref libproto"
kdocify

#
# cli
#
lib="cli"
desc="Command Line Interface library"
html_start_page="index.html"
files="cli/*.h cli/*.hh"
excludes=""
xref="$xref mrt"
kdocify

#
# mfea
#
lib="mfea"
desc="Multicast Forwarding Engine Abstraction daemon"
html_start_page="index.html"
files="mfea/*.h mfea/*.hh"
excludes=""
xref="$xref cli"
kdocify

#
# mld6igmp
#
lib="mld6igmp"
desc="Multicast Listener Discovery daemon"
html_start_page="index.html"
files="mld6igmp/*.h mld6igmp/*.hh"
excludes=""
xref="$xref mfea"
kdocify

#
# pim
#
lib="pim"
desc="Protocol Independent Multicast (PIM) daemon"
html_start_page="index.html"
files="pim/*.h pim/*.hh"
excludes=""
xref="$xref mld6igmp"
kdocify

#
# fea
#
lib="fea"
desc="Forwarding Engine Abstraction daemon"
html_start_page="index.html"
files="fea/*.hh"
excludes="fea/*click*hh"
xref="$xref"
kdocify

#
# bgp
#
lib="bgp"
desc="BGP4 daemon"
html_start_page="index.html"
files="bgp/*.hh"
excludes="bgp/*test*h"
xref=""
kdocify

#
# rib
#
lib="rib"
desc="Routing Information Base daemon"
html_start_page="index.html"
files="rib/*.hh"
excludes="rib/dummy_register_server.hh rib/parser_direct_cmds.hh rib/parser_xrl_cmds.hh rib/parser.hh"
xref=""
kdocify

#
# Build html index if appropriate
#
if [ "${KDOC_FORMAT}" = "html" ] ; then
    html_index_end
fi
