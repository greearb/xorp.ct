/*
 * Nullsoft Installer Script for XORP/Win32
 *
 * $XORP: xorp/contrib/win32/installer/xorp.nsi,v 1.13 2007/03/21 19:16:15 pavlin Exp $
 */

!include LogicLib.nsh
!include StrFunc.nsh
!include WinMessages.nsh
!include Sections.nsh
!include Library.nsh

!include AddToPath.nsh

# XXX Uncomment to stop bundling all the .EXEs.
#!define DEBUG

!define DO_SPLASH

!define PRODUCT_NAME		"XORP"
!define PRODUCT_VERSION		"1.5-WIP"
!define PRODUCT_URL		"http://www.xorp.org/"
!define PRODUCT_HELP_URL	"mailto:feedback@xorp.org"
!define PRODUCT_PUBLISHER	"ICSI"
!define PRODUCT_UNINST_ROOT_KEY	"HKLM"
!define PRODUCT_UNINST_KEY	\
	"Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

!define LANG_ENGLISH	"1033"

# HKLM keys
!define SYSTEM_ENVIRONMENT_KEY \
	"SYSTEM\CurrentControlSet\Control\Session Manager\Environment"
!define TCPIP_KEY	\
	"SYSTEM\CurrentControlSet\Services\Tcpip\Parameters"

!define SPLASH_IMAGE	"xorp_splash.bmp"
#!define BRANDING_IMAGE	"xorp-logo-medium.bmp"
#!define ICON_IMAGE	"none_yet"

###################################################

# XXX: This needs to be a system-wide DLL directory.
# DLLs will be copied from this location.
!define DLLDIR		"C:\MinGW\bin"

# XXX: Set this to the Windows-style path of your source build directory.
!define SRCDIR		"U:\xorp"

# XXX
!define TMPDIR		"C:\windows\temp"

# The default location for the installed stuff.
!define DEFAULT_INSTALL_DIR "C:\XORP"

###################################################

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
Caption "${PRODUCT_NAME} ${PRODUCT_VERSION} Installer"
InstProgressFlags smooth
XPStyle on

OutFile "xorp-${PRODUCT_VERSION}-setup.exe"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "${PRODUCT_NAME}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${PRODUCT_VERSION}"
VIProductVersion "1.2.0.0"
VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" "Find more information about ${PRODUCT_NAME} at ${PRODUCT_URL}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalTrademarks" ""
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "(c) 2001-2008 International Computer Science Institute"


InstallDir ${DEFAULT_INSTALL_DIR}


SilentInstall normal
ShowInstDetails show
ShowUninstDetails show

SetDateSave on
SetDataBlockOptimize on
CRCCheck on

AutoCloseWindow true
AllowRootDirInstall true
AllowSkipFiles on
SetOverwrite off

PageEx license
 LicenseData "${SRCDIR}\LICENSE"
 LicenseForceSelection radiobuttons
PageExEnd
Page components
Page directory
Page custom EnterInstallOptions LeaveInstallOptions
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles


#
# Required DLLs; hidden, and installed in app-private dir.
#

Section "-RequiredDLLs"
  SetOutPath $INSTDIR\dlls
  CreateDirectory $INSTDIR\dlls

  !insertmacro InstallLib DLL NOTSHARED NOREBOOT_NOTPROTECTED "${DLLDIR}\libeay32.dll" "$INSTDIR\dlls\libeay32.dll" "${TMPDIR}"
  !insertmacro InstallLib DLL NOTSHARED NOREBOOT_NOTPROTECTED "${DLLDIR}\libssl32.dll" "$INSTDIR\dlls\libssl32.dll" "${TMPDIR}"
  !insertmacro InstallLib DLL NOTSHARED NOREBOOT_NOTPROTECTED "${DLLDIR}\pcre3.dll" "$INSTDIR\dlls\pcre3.dll" "${TMPDIR}"
  !insertmacro InstallLib DLL NOTSHARED NOREBOOT_NOTPROTECTED "${DLLDIR}\pcreposix3.dll" "$INSTDIR\dlls\pcreposix3.dll" "${TMPDIR}"

  #
  # Add XORP_ROOT to environment
  #
  Push "XORP_ROOT"
  Push "$INSTDIR"
  Call AddToEnvVar

  #
  # Add XORP_ROOT\dlls to PATH
  #
  Push "$INSTDIR\dlls"
  Call AddToPath

SectionEnd

#
# XORP core install. Non-optional.
#
Section "-Base"

  SetOutPath $INSTDIR

  File /oname=BUGS.txt ${SRCDIR}\BUGS
  File /oname=BUILD_NOTES.txt ${SRCDIR}\BUILD_NOTES
  File /oname=ERRATA.txt ${SRCDIR}\ERRATA
  File /oname=LICENSE.txt ${SRCDIR}\LICENSE
  File /oname=README.txt ${SRCDIR}\README
  File /oname=RELEASE_NOTES.txt ${SRCDIR}\RELEASE_NOTES
  File /oname=TODO.txt ${SRCDIR}\TODO
  File /oname=VERSION.txt ${SRCDIR}\VERSION

!ifndef DEBUG

  # XORP core installation

  CreateDirectory $INSTDIR\cli\tools
  SetOutPath $INSTDIR\cli\tools
  File ${SRCDIR}\cli\tools\send_cli_processor_xrl.exe

  CreateDirectory $INSTDIR\fea
  SetOutPath $INSTDIR\fea
  File ${SRCDIR}\fea\xorp_fea.exe
  File ${SRCDIR}\fea\xorp_fea_dummy.exe

  CreateDirectory $INSTDIR\fea\tools
  SetOutPath $INSTDIR\fea\tools
  File ${SRCDIR}\fea\tools\show_interfaces.exe

  CreateDirectory $INSTDIR\fib2mrib
  SetOutPath $INSTDIR\fib2mrib
  File ${SRCDIR}\fib2mrib\xorp_fib2mrib.exe

  CreateDirectory $INSTDIR\libxipc
  SetOutPath $INSTDIR\libxipc
  File ${SRCDIR}\libxipc\call_xrl.exe
  File ${SRCDIR}\libxipc\xorp_finder.exe

  CreateDirectory $INSTDIR\policy
  SetOutPath $INSTDIR\policy
  File ${SRCDIR}\policy\xorp_policy.exe

  #CreateDirectory $INSTDIR\policy\test
  #SetOutPath $INSTDIR\policy\test
  #File ${SRCDIR}\policy\test\compilepolicy.exe
  #File ${SRCDIR}\policy\test\execpolicy.exe

  CreateDirectory $INSTDIR\rib
  SetOutPath $INSTDIR\rib
  File ${SRCDIR}\rib\add_route.exe
  File ${SRCDIR}\rib\main_routemap.exe
  File ${SRCDIR}\rib\xorp_rib.exe

  CreateDirectory $INSTDIR\rib\tools
  SetOutPath $INSTDIR\rib\tools
  File ${SRCDIR}\rib\tools\show_routes.exe

  CreateDirectory $INSTDIR\rtrmgr
  SetOutPath $INSTDIR\rtrmgr
  File ${SRCDIR}\rtrmgr\xorpsh.exe
  File ${SRCDIR}\rtrmgr\xorp_profiler.exe
  File ${SRCDIR}\rtrmgr\xorp_rtrmgr.exe

  CreateDirectory $INSTDIR\static_routes
  SetOutPath $INSTDIR\static_routes
  File ${SRCDIR}\static_routes\xorp_static_routes.exe

!endif

  # Template and command files for xorpsh / rtrmgr
  CreateDirectory $INSTDIR\etc\templates
  SetOutPath $INSTDIR\etc\templates
  File ${SRCDIR}\etc\templates\*.cmds
  File ${SRCDIR}\etc\templates\*.tp

  # XRL targets
  CreateDirectory $INSTDIR\xrl\targets
  SetOutPath $INSTDIR\xrl\targets
  File ${SRCDIR}\xrl\targets\*.tgt
  File ${SRCDIR}\xrl\targets\*.xrls

SectionEnd

Section "BGP" 1
 SectionSetText 1 "BGP: Border Gateway Protocol"
!ifndef DEBUG
  CreateDirectory $INSTDIR\bgp
  SetOutPath $INSTDIR\bgp
  File ${SRCDIR}\bgp\xorp_bgp.exe

  CreateDirectory $INSTDIR\bgp\tools
  SetOutPath $INSTDIR\bgp\tools
  File ${SRCDIR}\bgp\tools\print_peers.exe
  File ${SRCDIR}\bgp\tools\print_routes.exe
  File ${SRCDIR}\bgp\tools\xorpsh_print_peers.exe
  File ${SRCDIR}\bgp\tools\xorpsh_print_routes.exe
!endif
SectionEnd

Section "RIP" 2
 SectionSetText 2 "RIP: Routing Information Protocol"
!ifndef DEBUG
  CreateDirectory $INSTDIR\rip
  SetOutPath $INSTDIR\rip
  File ${SRCDIR}\rip\xorp_rip.exe
  File ${SRCDIR}\rip\xorp_ripng.exe

  CreateDirectory $INSTDIR\rip\tools
  SetOutPath $INSTDIR\rip\tools
  File ${SRCDIR}\rip\tools\ripng_announcer.exe
  File ${SRCDIR}\rip\tools\rip_announcer.exe
  File ${SRCDIR}\rip\tools\show_peer_stats.exe
  File ${SRCDIR}\rip\tools\show_stats.exe
!endif
SectionEnd

Section /o "OSPF" 3
 SectionSetText 3 "OSPF: Open Shortest Path First"
!ifndef DEBUG
  CreateDirectory $INSTDIR\ospf
  SetOutPath $INSTDIR\ospf
  File ${SRCDIR}\ospf\xorp_ospfv2.exe
  File ${SRCDIR}\ospf\xorp_ospfv3.exe

  CreateDirectory $INSTDIR\ospf\tools
  SetOutPath $INSTDIR\ospf\tools
  File ${SRCDIR}\ospf\tools\print_lsas.exe
  File ${SRCDIR}\ospf\tools\print_neighbours.exe
!endif
SectionEnd

Section /o "PIM" 4
 SectionSetText 4 "PIM: Protocol Independent Multicast"
!ifndef DEBUG
  CreateDirectory $INSTDIR\pim
  SetOutPath $INSTDIR\pim
  File ${SRCDIR}\pim\xorp_pimsm4.exe
  File ${SRCDIR}\pim\xorp_pimsm6.exe
!endif
SectionEnd

Section /o "MLD6IGMP" 5
 SectionSetText 5 "MLD: Multicast Listener Discovery and IGMP"
!ifndef DEBUG
  CreateDirectory $INSTDIR\mld6igmp
  SetOutPath $INSTDIR\mld6igmp
  File ${SRCDIR}\mld6igmp\xorp_igmp.exe
  File ${SRCDIR}\mld6igmp\xorp_mld.exe
!endif
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_URL}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "HelpLink" "${PRODUCT_HELP_URL}"
  WriteRegDword ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoModify" 1
  WriteRegDword ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoRepair" 1
SectionEnd


####### Other functions for main installer

Function .onInit

  # Initialize custom properties page plugin
  InitPluginsDir
  File /oname=$PLUGINSDIR\options.ini options.ini

!ifdef DO_SPLASH
  SetOutPath "$TEMP"
  File /oname=spltmp.bmp ${SPLASH_IMAGE}
  splash::show 1800 $TEMP\spltmp
  Pop $0
  Delete $TEMP\spltmp.bmp
!endif

FunctionEnd

Function EnterInstallOptions
  Push $R0
  MessageBox MB_OK "A reboot may be required to enable options for the TCPIP.SYS driver."
  InstallOptions::dialog $PLUGINSDIR\options.ini
  Pop $R0
FunctionEnd

Function LeaveInstallOptions
  ReadINIStr $0 "$PLUGINSDIR\options.ini" "Field 2" "State"
  StrCmp $0 1 0 +3
  WriteRegDword HKLM "${TCPIP_KEY}" "IPEnableRouter" 1
  SetRebootFlag true

  ReadINIStr $0 "$PLUGINSDIR\options.ini" "Field 3" "State"
  StrCmp $0 1 0 +3
  WriteRegDword HKLM "${TCPIP_KEY}" "EnableICMPRedirect" 0
  SetRebootFlag true

  ReadINIStr $0 "$PLUGINSDIR\options.ini" "Field 4" "State"
  StrCmp $0 1 0 +3
  WriteRegDword HKLM "${TCPIP_KEY}" "ForwardBroadcasts" 0
  SetRebootFlag true

  ReadINIStr $0 "$PLUGINSDIR\options.ini" "Field 5" "State"
  StrCmp $0 1 0 +3
  WriteRegDword HKLM "${TCPIP_KEY}" "MaxNormLookupMemory" 0xFFFFFFFF
  SetRebootFlag true
FunctionEnd

Function .onInstSuccess
 IfRebootFlag 0 noreboot
  MessageBox MB_YESNO "A reboot is required to finish the installation. Do you wish to reboot now?" IDNO noreboot
    Reboot
noreboot:
FunctionEnd

########################
## Uninstaller
########################

# XXX: TODO: Tidy this up.

Section Uninstall

  # Try removing BGP.
  Delete "$INSTDIR\bgp\tools\*.exe"
  RMDir "$INSTDIR\bgp\tools"
  Delete "$INSTDIR\bgp\*.exe"
  RMDir "$INSTDIR\bgp"

  # Try removing OSPF.
  Delete "$INSTDIR\ospf\tools\*.exe"
  RMDir "$INSTDIR\ospf\tools"
  Delete "$INSTDIR\ospf\*.exe"
  RMDir "$INSTDIR\ospf"

  # Try removing RIP.
  Delete "$INSTDIR\rip\tools\*.exe"
  RMDir "$INSTDIR\rip\tools"
  Delete "$INSTDIR\rip\*.exe"
  RMDir "$INSTDIR\rip"

  # Try removing everything else.
  Delete "$INSTDIR\pim\*.exe"
  RMDir "$INSTDIR\pim"
  Delete "$INSTDIR\mld6igmp\*.exe"
  RMDir "$INSTDIR\mld6igmp"

!ifndef DEBUG
  Delete "$INSTDIR\cli\tools\*.exe"
  RMDir "$INSTDIR\cli\tools"
  RMDir "$INSTDIR\cli"
  Delete "$INSTDIR\fea\tools\*.exe"
  RMDir "$INSTDIR\fea\tools"
  Delete "$INSTDIR\fea\*.exe"
  RMDir "$INSTDIR\fea"
  Delete "$INSTDIR\fib2mrib\*.exe"
  RMDir "$INSTDIR\fib2mrib"
  Delete "$INSTDIR\libxipc\*.exe"
  RMDir "$INSTDIR\libxipc"
  #Delete "$INSTDIR\policy\test\*.exe"
  #RMDir "$INSTDIR\policy\test"
  Delete "$INSTDIR\policy\*.exe"
  RMDir "$INSTDIR\policy"
  Delete "$INSTDIR\rib\tools\*.exe"
  RMDir "$INSTDIR\rib\tools"
  Delete "$INSTDIR\rib\*.exe"
  RMDir "$INSTDIR\rib"
  Delete "$INSTDIR\rtrmgr\*.exe"
  RMDir "$INSTDIR\rtrmgr"
  Delete "$INSTDIR\static_routes\*.exe"
  RMDir "$INSTDIR\static_routes"
!endif
  Delete "$INSTDIR\xrl\targets\*.tgt"
  Delete "$INSTDIR\xrl\targets\*.xrls"
  RMDir "$INSTDIR\xrl\targets"
  RMDir "$INSTDIR\xrl"

  Delete "$INSTDIR\etc\templates\*.cmds"
  Delete "$INSTDIR\etc\templates\*.tp"
  RMDir "$INSTDIR\etc\templates"
  RMDir "$INSTDIR\etc"

  Delete "$INSTDIR\dlls\*.dll"
  RMDir "$INSTDIR\dlls"

  Delete "$INSTDIR\*.txt"

  Delete "$INSTDIR\uninst.exe"
  RMDir "$INSTDIR"

  # and finally the registry cleanup

  Push "$INSTDIR\dlls"
  Call un.RemoveFromPath

  Push "XORP_ROOT"
  Push "$INSTDIR"
  Call un.RemoveFromEnvVar
 
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"

  #SetAutoClose true
SectionEnd

Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd
