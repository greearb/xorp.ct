#
# $XORP: CVSROOT/cfg_local.pm,v 1.1 2004/08/02 07:06:53 pavlin Exp $
# $FreeBSD: CVSROOT-src/cfg_local.pm,v 1.27 2004/06/05 10:47:00 des Exp $
#

####################################################################
####################################################################
# This file contains local configuration for the CVSROOT perl
# scripts.  It is loaded by cfg.pm and overrides the default
# configuration in that file.
#
# It is advised that you test it with
#     'env CVSROOT=/path/to/cvsroot perl -cw cfg.pm'
# before you commit any changes.  The check is to cfg.pm which
# loads this file.
####################################################################
####################################################################

#$DEBUG = 1;
$TZ = 'Etc/UTC';
$CHECK_HEADERS = 0;
$IDHEADER = 'XORP';
$UNEXPAND_RCSID = 1;

%TEMPLATE_HEADERS = (
	"Bugzilla URL"		=> '.*',
	"Submitted by"		=> '.*',
	"Requested by"		=> '.*',
	"Reviewed by"		=> '.*',
	"Approved by"		=> '.*',
	"Obtained from"		=> '.*',
#	"PR"			=> '.*',
#	"MFC after"		=> '\d+(\s+(days?|weeks?|months?))?'
);

#$MAILCMD = "/usr/local/bin/mailsend -H";
$MAILCMD = "/usr/sbin/sendmail";
$MAIL_SUBJECT_PREAMBLE = "XORP cvs commit:";
$MAIL_BRANCH_HDR  = "X-XORP-CVS-Branch";
$ADD_TO_LINE = 1;
$MAILBANNER = "XORP CVS repository";
if (defined $ENV{'CVS_COMMIT_ATTRIB'}) {
  my $attrib = $ENV{'CVS_COMMIT_ATTRIB'};
  $MAILBANNER .= " ($attrib committer)";
}

# Sanity check to make sure we belong to group 'xorp'.
#
$COMMITCHECK_EXTRA = sub {
	my $GRPS=`/usr/bin/id -Gn`;
	chomp $GRPS;
	my @grps_list = split(' ', $GRPS);
	foreach my $grp (@grps_list) {
		if ( $grp =~ /^xorp$/ ) {
			return 1;
		}
	}
	print "You do not belong to group xorp (commitcheck)!\n";
	return 1;
	exit 1;	# We could return false here.  But there's
		# nothing to stop us taking action here instead.
};

# Sanity check to make sure we've been run through the wrapper and are
# now primary group 'xorp'.
#
# $COMMITCHECK_EXTRA = sub {
# 	my $GRP=`/usr/bin/id -gn`;
# 	chomp $GRP;
# 	unless ( $GRP =~ /^xorp$/ ) {
# 		print "You do not have group xorp (commitcheck)!\n";
# 		exit 1;	# We could return false here.  But there's
# 			# nothing to stop us taking action here instead.
# 	}
# 	return 1;
# };

# Wrap this in a hostname check to prevent mail to the XORP
# list if someone borrows this file and forgets to change it.
my $hostname = `/bin/hostname`;
die "Can't determine hostname!\n" if $? >> 8;
chomp $hostname;
if ($hostname =~ /^xorpc\.icir\.org$/i) {
	$MAILADDRS='xorp-cvs@icir.org';
	$MAILADDRS = 'cvs-test@icir.org' if $DEBUG;

	@COMMIT_HOSTS = qw(xorpc.icir.org);
}


@LOG_FILE_MAP = (
	'CVSROOT'	=> '^CVSROOT/',
	'data'		=> '^data/',
	'other'		=> '^other/',
	'www'		=> '^www/',
	'xorp'		=> '^xorp/',
	'misc'		=> '.*'
);

1; # Perl requires all modules to return true.  Don't delete!!!!
#end
