#   File:       snack.make
#   Target:     snack
#   Created:    Tuesday, March 21, 2000 01:27:20 PM


MAKEFILE        = snack.make
�MondoBuild�    = {MAKEFILE}  # Make blank to avoid rebuilds when makefile is modified

TclLibDir    = Macintosh HD:Systemmapp:Till�gg:Tool Command Language

SrcDir          = ::generic:
ObjDir          = :
Includes        = -i "{SrcDir}" -d HAS_STDARG -d MAC -d MAC_TCL -i "Macintosh HD:Desktop Folder:K�re:inc:"

Sym-PPC         = 

PPCCOptions     = {Includes} {Sym-PPC} 


### Source Files ###

SrcFiles        =  �
				  ":jkAudIO_mac.c" �
				  "{SrcDir}ffa.c" �
				  "{SrcDir}g711.c" �
				  "{SrcDir}jkCanvSect.c" �
				  "{SrcDir}jkCanvSpeg.c" �
				  "{SrcDir}jkCanvWave.c" �
				  "{SrcDir}jkPitchCmd.c" �
				  "{SrcDir}jkSound.c" �
				  "{SrcDir}jkSoundEngine.c" �
				  "{SrcDir}jkSoundEdit.c" �
				  "{SrcDir}jkSoundFile.c" �
				  "{SrcDir}jkSoundProc.c" �
				  "{SrcDir}snack.c" �
				  "{SrcDir}jkFormatMP3.c" �
				  "{SrcDir}jkMixer.c" �
				  "{SrcDir}jkAudio.c" �
				  "{SrcDir}jkFilter.c" �
				  "{SrcDir}jkFilterIIR.c" �
				  "{SrcDir}jkSynthesis.c" �
				  "{SrcDir}shape.c" �
				  "{SrcDir}snackStubInit.c"
				  
#

### Object Files ###

ObjFiles-PPC    =  �
                  "{ObjDir}jkAudIO_mac.c.x" �
				  "{ObjDir}ffa.c.x" �
				  "{ObjDir}g711.c.x" �
				  "{ObjDir}jkCanvSect.c.x" �
				  "{ObjDir}jkCanvSpeg.c.x" �
				  "{ObjDir}jkCanvWave.c.x" �
				  "{ObjDir}jkPitchCmd.c.x" �
				  "{ObjDir}jkSound.c.x" �
				  "{ObjDir}jkSoundEngine.c.x" �
				  "{ObjDir}jkSoundEdit.c.x" �
				  "{ObjDir}jkSoundFile.c.x" �
				  "{ObjDir}jkSoundProc.c.x" �
				  "{ObjDir}snack.c.x" �
				  "{ObjDir}jkFormatMP3.c.x" �
				  "{ObjDir}jkMixer.c.x" �
				  "{ObjDir}jkAudio.c.x" �
				  "{ObjDir}jkFilter.c.x" �
				  "{ObjDir}jkFilterIIR.c.x" �
				  "{ObjDir}jkSynthesis.c.x" �
				  "{ObjDir}shape.c.x" �
				  "{ObjDir}snackStubInit.c.x"

### Libraries ###

LibFiles-PPC    =  �
				  "{SharedLibraries}InterfaceLib" �
				  "{SharedLibraries}StdCLib" �
				  "{SharedLibraries}MathLib" �
				  "{PPCLibraries}StdCRuntime.o" �
				  "{PPCLibraries}PPCCRuntime.o" �
				  "{PPCLibraries}PPCToolLibs.o" �
				   "{TclLibDir}:Tcl8.3.shlb" �
				   "{TclLibDir}:Tk8.3.shlb"


### Default Rules ###

.c.x  �  .c  {�MondoBuild�}
	{PPCC} {depDir}{default}.c -o {targDir}{default}.c.x {PPCCOptions}


### Build Rules ###

snack.shlb  ��  {ObjFiles-PPC} {LibFiles-PPC} {�MondoBuild�}
	PPCLink �
		-o {Targ} �
		{ObjFiles-PPC} �
		{LibFiles-PPC} �
		{Sym-PPC} �
		-mf -d �
		-t 'shlb' �
		-c '????' �
		-xm s �
		-export Snack_Init
	Rez snack.r -o snack.shlb

snack  ��  snack.shlb


### Required Dependencies ###

"{ObjDir}ffa.c.x"  �  "{SrcDir}ffa.c"
"{ObjDir}g711.c.x"  �  "{SrcDir}g711.c"
"{ObjDir}jkAudIO_mac.c.x"  �  ":jkAudIO_mac.c"
"{ObjDir}jkCanvSect.c.x"  �  "{SrcDir}jkCanvSect.c"
"{ObjDir}jkCanvSpeg.c.x"  �  "{SrcDir}jkCanvSpeg.c"
"{ObjDir}jkCanvWave.c.x"  �  "{SrcDir}jkCanvWave.c"
"{ObjDir}jkPitchCmd.c.x"  �  "{SrcDir}jkPitchCmd.c"
"{ObjDir}jkSound.c.x"  �  "{SrcDir}jkSound.c"
"{ObjDir}jkSoundEngine.c.x"  �  "{SrcDir}jkSoundEngine.c"
"{ObjDir}jkSoundEdit.c.x"  �  "{SrcDir}jkSoundEdit.c"
"{ObjDir}jkSoundFile.c.x"  �  "{SrcDir}jkSoundFile.c"
"{ObjDir}jkSoundProc.c.x"  �  "{SrcDir}jkSoundProc.c"
"{ObjDir}snack.c.x"  �  "{SrcDir}snack.c"
"{ObjDir}jkFormatMP3.c.x"  �  "{SrcDir}jkFormatMP3.c"
"{ObjDir}jkAudio.c.x"  �  "{SrcDir}jkAudio.c"
"{ObjDir}jkFilter.c.x"  �  "{SrcDir}jkFilter.c"
"{ObjDir}jkFilterIIR.c.x"  �  "{SrcDir}jkFilterIIR.c"
"{ObjDir}jkSynthesis.c.x"  �  "{SrcDir}jkSynthesis.c"
"{ObjDir}jkMixer.c.x"  �  "{SrcDir}jkMixer.c"
"{ObjDir}shape.c.x"  �  "{SrcDir}shape.c"
"{ObjDir}snackStubInit.c.x"  �  "{SrcDir}snackStubInit.c"


### Optional Dependencies ###
### Build this target to generate "include file" dependencies. ###

Dependencies  �  $OutOfDate
	MakeDepend �
		-append {MAKEFILE} �
		-ignore "{CIncludes}" �
		-objdir "{ObjDir}" �
		-objext .x �
		{Includes} �
		{SrcFiles}


