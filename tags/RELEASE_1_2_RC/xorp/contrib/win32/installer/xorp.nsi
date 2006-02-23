/*
 * Nullsoft Installer Script for XORP/Win32
 *
 * $XORP$
 */

!include LogicLib.nsh
!include StrFunc.nsh
!include WinMessages.nsh
!include Sections.nsh

#!define DEBUG
!define DO_SPLASH
!define DO_ADD_DLLS_TO_PATH

!define PRODUCT_NAME		"XORP/Win32"
!define PRODUCT_VERSION		"1.2"
!define PRODUCT_URL		"http://www.xorp.org/"
!define PRODUCT_HELP_URL	"mailto:feedback@xorp.org"
!define PRODUCT_PUBLISHER	"ICIR"
!define PRODUCT_UNINST_ROOT_KEY	"HKLM"
!define PRODUCT_UNINST_KEY	\
	"Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"

!define LANG_ENGLISH	"1033"

# HKLM keys
!define SYSTEM_ENVIRONMENT_KEY \
	"SYSTEM\CurrentControlSet\Control\Session Manager\Environment"
!define TCPIP_KEY	\
	"SYSTEM\CurrentControlSet\Services\Tcpip\Parameters"

!define SRCDIR		"E:\p4\xorp_win"
!define DLLDIR		"D:\gnuwin32\bin"
!define DEFAULT_INSTALL_DIR "C:\XORP"

!define SPLASH_IMAGE	"xorp_splash.bmp"
#!define BRANDING_IMAGE	"xorp-logo-medium.bmp"
#!define ICON_IMAGE	"none_yet"




Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
Caption "${PRODUCT_NAME} ${PRODUCT_VERSION} Installer"
#Icon "${ICON_IMAGE}
# Old fashioned desktop background
#BGGradient 008080 008080 ffffff
InstProgressFlags smooth
XPStyle on

OutFile "xorpinst.exe"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "${PRODUCT_NAME}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${PRODUCT_VERSION}"
VIProductVersion "1.2.0.0"
VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" "Find more information about ${PRODUCT_NAME} at ${PRODUCT_URL}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalTrademarks" ""
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "(c) 2001-2005 International Computer Science Institute"


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
!ifdef DO_ADD_DLLS_TO_PATH
${StrStr}
VAR ind
VAR reg
!endif

Section "-RequiredDLLs"
  SetOutPath $INSTDIR\dlls
  CreateDirectory $INSTDIR\dlls

  File ${DLLDIR}\libssl32.dll
  File ${DLLDIR}\libeay32.dll
  File ${DLLDIR}\pcreposix.dll

!ifdef DO_ADD_DLLS_TO_PATH
  #ReadRegStr $reg HKCU "Environment" "path"
  #WriteRegStr HKCU "Environment" "XORP_ROOT" "$INSTDIR"
  ReadRegStr $reg HKLM "${SYSTEM_ENVIRONMENT_KEY}" "path"
  WriteRegStr HKLM "${SYSTEM_ENVIRONMENT_KEY}" "XORP_ROOT" "$INSTDIR"
  ${StrStr} $ind $reg "%XORP_ROOT%\dlls"
  StrCmp $ind "" notfound found
  SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
  SetRebootFlag true

notfound:
  #WriteRegStr HKCU "Environment" "path" "$reg;%XORP_ROOT%\dlls;"
  WriteRegStr HKLM "${SYSTEM_ENVIRONMENT_KEY}" "path" "$reg;%XORP_ROOT%\dlls;"
  SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
  SetRebootFlag true
  
found:

!endif
SectionEnd

#
# XORP core install.
#
Section "!Base"

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

  CreateDirectory $INSTDIR\libxipc
  SetOutPath $INSTDIR\libxipc
  File ${SRCDIR}\libxipc\call_xrl.exe
  File ${SRCDIR}\libxipc\xorp_finder.exe

  CreateDirectory $INSTDIR\policy
  SetOutPath $INSTDIR\policy
  File ${SRCDIR}\policy\xorp_policy.exe

  CreateDirectory $INSTDIR\rib
  SetOutPath $INSTDIR\rib
  File ${SRCDIR}\rib\add_route.exe
  File ${SRCDIR}\rib\main_routemap.exe
  File ${SRCDIR}\rib\xorp_rib.exe
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

# TODO: add documentation section
# TODO: add source section

Section "BGP"
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

Section "RIP"
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

Section Uninstall
  Delete "$INSTDIR\uninst.exe"
  # XXX: Much more to do here
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  SetAutoClose true
SectionEnd

Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd
