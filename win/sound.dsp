# Microsoft Developer Studio Project File - Name="sound" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=sound - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sound.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sound.mak" CFG="sound - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sound - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sound - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sound - Win32 Tcl80" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sound - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOUND_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "C:\Program Files\Tcl\include" /I "../generic" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOUND_EXPORTS" /D "WIN" /D "USE_TCL_STUBS" /D "TCL_81_API" /D "BUILD_snack" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib tclstub83.lib /nologo /dll /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"C:\Program Files\Tcl\lib\snack2.0\libsound.dll" /libpath:"C:\Program Files\Tcl\lib"

!ELSEIF  "$(CFG)" == "sound - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOUND_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "C:\Program Files\Tcl\include" /I "../generic" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOUND_EXPORTS" /D "WIN" /D "USE_TCL_STUBS" /D "TCL_81_API" /D "BUILD_snack" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"Debug/libsound.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "sound - Win32 Tcl80"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "sound___Win32_Tcl80"
# PROP BASE Intermediate_Dir "sound___Win32_Tcl80"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "sound___Win32_Tcl80"
# PROP Intermediate_Dir "sound___Win32_Tcl80"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "C:\Program Files\Tcl\include" /I "../generic" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOUND_EXPORTS" /D "WIN" /D "USE_TCL_STUBS" /D "TCL_81_API" /D "BUILD_snack" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "C:\Program Files\Tcl\include80" /I "../generic" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SOUND_EXPORTS" /D "WIN" /D "BUILD_snack" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib tclstub83.lib /nologo /dll /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"C:\Program Files\Tcl\lib\snack2.0\libsound.dll" /libpath:"C:\Program Files\Tcl\lib"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib tcl80.lib /nologo /dll /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"C:\Program Files\Tcl\lib\snack2.0\libsound.dll" /libpath:"C:\Program Files\Tcl\lib"

!ENDIF 

# Begin Target

# Name "sound - Win32 Release"
# Name "sound - Win32 Debug"
# Name "sound - Win32 Tcl80"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\generic\ffa.c
# End Source File
# Begin Source File

SOURCE=..\generic\g711.c
# End Source File
# Begin Source File

SOURCE=..\generic\jkAudio.c
# End Source File
# Begin Source File

SOURCE=.\jkAudIO_win.c
# End Source File
# Begin Source File

SOURCE=..\generic\jkFilter.c
# End Source File
# Begin Source File

SOURCE=..\generic\jkSynthesis.c
# End Source File
# Begin Source File

SOURCE=..\generic\jkFormatMP3.c
# End Source File
# Begin Source File

SOURCE=..\generic\jkMixer.c
# End Source File
# Begin Source File

SOURCE=..\generic\jkPitchCmd.c
# End Source File
# Begin Source File

SOURCE=..\generic\jkSound.c
# End Source File
# Begin Source File

SOURCE=..\generic\jkSoundEdit.c
# End Source File
# Begin Source File

SOURCE=..\generic\jkSoundEngine.c
# End Source File
# Begin Source File

SOURCE=..\generic\jkSoundFile.c
# End Source File
# Begin Source File

SOURCE=..\generic\jkSoundProc.c
# End Source File
# Begin Source File

SOURCE=..\generic\shape.c
# End Source File
# Begin Source File

SOURCE=..\generic\snackStubInit.c

!IF  "$(CFG)" == "sound - Win32 Release"

!ELSEIF  "$(CFG)" == "sound - Win32 Debug"

!ELSEIF  "$(CFG)" == "sound - Win32 Tcl80"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\generic\sound.c
# End Source File
# Begin Source File

SOURCE=.\sound.def
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\generic\jkAudIO.h
# End Source File
# Begin Source File

SOURCE=..\generic\jkCanvItems.h
# End Source File
# Begin Source File

SOURCE=..\generic\jkFormatMP3.h
# End Source File
# Begin Source File

SOURCE=..\generic\jkSound.h
# End Source File
# Begin Source File

SOURCE=..\generic\snack.h
# End Source File
# Begin Source File

SOURCE=..\generic\snackDecls.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project