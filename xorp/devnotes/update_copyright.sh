#!/bin/sh

#
# This is a script to update the copyright message for all files.
# It must be run in the top directory of a a fresh checked-out copy
# of the source code, otherwise it may overwrite something else.
#
# Note: You should give a heads up before commiting changes
# proposed by this script since it will modify almost all files in the
# source tree and this may interfere with other people's pending commits.
#

#set -x

#
# XXX: MODIFY THE FOLLOWING PARAMETERS AS APPROPRIATE!
#
# COPYRIGHT_HOLDER: The name of the copyright holder
#
OLD_COPYRIGHT_HOLDER="XORP, Inc."
COPYRIGHT_HOLDER="XORP, Inc and Others"

#
# Print usage and exit
#
usage()
{
    cat <<EOF
Usage: $0 [-h] <old_year> [<new_year>]
    <old_year>  The old year that was in the copyright messages
    <new_year>  The new year. If it is not specified, it will be set to
                the year after <old_year>.
    -h          Print usage
EOF
    exit 1
}

#
# Update a single file
#
# $1 = Old string
# $2 = New string
# $3 = File name
#
update_file()
{
    if [ $# -ne 3 ] ; then
	echo "$0: invalid number of arguments: expected 3, received $#"
	exit 1
    fi
    old_string="$1"
    new_string="$2"
    filename="$3"

    #
    # Internal variables
    #
    sed_cmd="s/${old_string}/${new_string}/"
    tmp_suffix="debog"

    grep -e "${old_string}" "${filename}" > /dev/null
    if [ $? -ne 0 ] ; then
	# OK: no match found
	return
    fi

    echo "sed cmd -:${sed_cmd}"

    cat "${filename}" | sed "${sed_cmd}" > "${filename}"."${tmp_suffix}"
    if [ $? -ne 0 ] ; then
	echo "Error updating ${filename}"
	exit 1
    fi
    cmp "${filename}" "${filename}"."${tmp_suffix}" >/dev/null
    if [ $? -ne 0 ] ; then
	mv "${filename}"."${tmp_suffix}" "${filename}"
	echo "Updating ${filename}"
    else
	rm "${filename}"."${tmp_suffix}"
    fi
}

#
# Update all files
#
# $1 = Old string
# $2 = New string
# *  file(s) to update
#
update_all_files()
{
    echo "$update_all_files: args: $#"
    if [ $# -lt 2 ] ; then
	echo "$0: invalid number of arguments: expected 2+, received $#"
	exit 1
    fi
    old_string="$1"
    new_string="$2"

    if [ $# -gt 2 ]
	then
	shift 2
	while [ "$1" ] ; do
	    echo  "update_file (reg) -:${old_string}:- -:${new_string}:- -:${1}:-"
	    update_file "${old_string}" "${new_string}" "${1}"
	    shift
	done
    else
        # Update all files
	find . -type f -print | 
	while read FILENAME ; do
	    update_file "${old_string}" "${new_string}" "${FILENAME}"
	done
    fi
}

#
# Update the template files
#
# $1 = Old string
# $2 = New string
#
update_template_files()
{
    if [ $# -lt 2 ] ; then
	echo "$0: invalid number of arguments: expected 2+, received $#"
	exit 1
    fi
    old_string="$1"
    new_string="$2"

    # Template files directory name pattern
    template_dir="devnotes"
    template_files="template.*"

    if [ $# -gt 2 ] ;
	then
	shift 2

        # Update only the template files
	while [ "$1" ] ; do
	    echo  "update_file (template) -:${old_string}:- -:${new_string}:- -:${1}:-"
	    #update_file "${old_string}" "${new_string}" "${1}"
	    shift
	done
    else
	if [ ! -d "${template_dir}" ] ; then
	# No sub-directory with the template files
	    return
	fi

        # Update only the template files
	find "${template_dir}" -name "${template_files}" -type f -print | 
	while read FILENAME ; do
	    update_file "${old_string}" "${new_string}" "${FILENAME}"
	done
    fi
}

#
# Extract the arguments
#
if [ $# -lt 2 ] ; then
    echo "$0: invalid number of arguments: received $#"
    usage
fi

if [ "$1" = "-h" ] ; then
    usage
fi

old_year="$1"
if [ $# -lt 2 ] ; then
    new_year=$(($old_year+1))
    shift
else
    new_year="$2"
    shift 2
fi


#
# Update the template files:
#     change "(c) OLD" copyright year to "(c) NEW" copyright year.
#
OLD_YEAR_ID="(c) ${old_year}"
NEW_YEAR_ID="(c) ${new_year}"
OLD_STRING="${OLD_YEAR_ID} ${OLD_COPYRIGHT_HOLDER}"
NEW_STRING="${NEW_YEAR_ID} ${COPYRIGHT_HOLDER}"
update_template_files "${OLD_STRING}" "${NEW_STRING}" $@

#
# Update all files:
#     change "-OLD" copyright years to "-NEW" copyright years.
#
OLD_YEAR_ID="-${old_year}"
NEW_YEAR_ID="-${new_year}"
OLD_STRING="${OLD_YEAR_ID} ${OLD_COPYRIGHT_HOLDER}"
NEW_STRING="${NEW_YEAR_ID} ${COPYRIGHT_HOLDER}"
update_all_files "${OLD_STRING}" "${NEW_STRING}" $@

#
# Update all files:
#     change "(c) OLD" copyright year to "(c) OLD-NEW" copyright years.
#
OLD_YEAR_ID="(c) ${old_year}"
NEW_YEAR_ID="(c) ${old_year}-${new_year}"
OLD_STRING="${OLD_YEAR_ID} ${OLD_COPYRIGHT_HOLDER}"
NEW_STRING="${NEW_YEAR_ID} ${COPYRIGHT_HOLDER}"
update_all_files "${OLD_STRING}" "${NEW_STRING}" $@
