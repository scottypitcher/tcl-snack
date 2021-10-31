#!/bin/bash
# 
# Script to run gdb with the tcl-snack library
# SVP 14OCT2021.
#
# I am getting segfaults inside the tcl-snack dlls.
# 

# Determine the build size.
case "`uname -s`" in
    MINGW32*)
	PLATFORM=win
	BUILD_SIZE=32
	BUILD_ARCH=intel
    ;;
    MINGW64*)
	PLATFORM=win
	BUILD_SIZE=64
	BUILD_ARCH=amd64
    ;;
    Linux)
	PLATFORM=unix
	BUILD_SIZE=$LINUX_BUILD_SIZE
	if [ "$BUILD_SIZE" == "64" ] ; then
	    BUILD_ARCH=x86_64
	else
	    BUILD_ARCH=amd
	fi
    ;;
esac

# /c/Tcl8.6.10-intel-Debug/bin/tclsh86g.exe
# /C/Tcl8.6.10-amd64-Debug/bin/tclsh86g.exe
gdb /c/Tcl8.6.10-$BUILD_ARCH-Debug/bin/tclsh86g.exe \
    -ex 'set  disassemble-next-line on' \
    -ex 'display /3i $pc' \
    -ex 'run tests/all.tcl' \
    -ex 'bt full no-filter'
    

#    -ex 'break Tcl_Exit'
# gdb /c/Tcl/bin/wish86g.exe \
#     -ex 'set  disassemble-next-line on' \
#     -ex 'display /3i $pc' \
#     -ex 'break main' \
#     -ex 'run c:/Threetronics/GAP-USB/TclTkAlarmPanelManager/ui/main.tcl --icanvastest'

#     -ex 'break GdiPhoto' \
#     -ex 'continue' \
#     -ex 'continue'

#     -ex 'continue' \
#     -ex 'disassemble /m $eip,+150' \
#     -ex 'info reg'
     
#     -ex 'break msvcrt!_invalid_parameter' \
#     -ex 'break msvcrt!_ftol2_sse_excpt' \
#     -ex 'break exit' \
#     -ex 'break Tcl_Exit' \
#     -ex 'break __gcc_deregister_frame' \
#     -ex 'continue' \
#     -ex 'disassemble /m $eip,+100'
 

 
 
 
 
 
 
 
 # 
#  The segault is at the call to Tk_FindPhoto().
# tkStubsPtr is empty!
#  
#  
#  (gdb) si
# 0x6b542e81      1106      if ( photoname == 0 ) {
# (gdb) nexti
# 1114      if ((photo_handle = Tk_FindPhoto (interp, photoname)) == 0) {
# (gdb) disassemble /m $eip,+150
# Dump of assembler code from 0x6b542eb8 to 0x6b542f4e:
# 1114      if ((photo_handle = Tk_FindPhoto (interp, photoname)) == 0) {
# => 0x6b542eb8 <GdiPhoto+836>:   mov    0x6b559520,%eax
#    0x6b542ebd <GdiPhoto+841>:   mov    0x108(%eax),%eax
#    0x6b542ec3 <GdiPhoto+847>:   mov    -0x1c(%ebp),%edx
#    0x6b542ec6 <GdiPhoto+850>:   mov    %edx,0x4(%esp)
#    0x6b542eca <GdiPhoto+854>:   mov    0xc(%ebp),%edx
#    0x6b542ecd <GdiPhoto+857>:   mov    %edx,(%esp)
#    0x6b542ed0 <GdiPhoto+860>:   call   *%eax
#    0x6b542ed2 <GdiPhoto+862>:   mov    %eax,-0x3c(%ebp)
#    0x6b542ed5 <GdiPhoto+865>:   cmpl   $0x0,-0x3c(%ebp)
#    0x6b542ed9 <GdiPhoto+869>:   jne    0x6b542f1f <GdiPhoto+939>

# 1115        Tcl_AppendResult(interp, "gdi photo: Photo name ", photoname, " can't be located\n", usage_message, 0);
#    0x6b542edb <GdiPhoto+871>:   mov    0x6b55950c,%eax
#    0x6b542ee0 <GdiPhoto+876>:   mov    0x120(%eax),%eax
#    0x6b542ee6 <GdiPhoto+882>:   movl   $0x0,0x14(%esp)
#    0x6b542eee <GdiPhoto+890>:   movl   $0x6b551b00,0x10(%esp)
#    0x6b542ef6 <GdiPhoto+898>:   movl   $0x6b5533d1,0xc(%esp)
#    0x6b542efe <GdiPhoto+906>:   mov    -0x1c(%ebp),%edx
#    0x6b542f01 <GdiPhoto+909>:   mov    %edx,0x8(%esp)
#    0x6b542f05 <GdiPhoto+913>:   movl   $0x6b5533e4,0x4(%esp)
#    0x6b542f0d <GdiPhoto+921>:   mov    0xc(%ebp),%edx
#    0x6b542f10 <GdiPhoto+924>:   mov    %edx,(%esp)
#    0x6b542f13 <GdiPhoto+927>:   call   *%eax

# 1116        return TCL_ERROR;
#    0x6b542f15 <GdiPhoto+929>:   mov    $0x1,%eax
#    0x6b542f1a <GdiPhoto+934>:   jmp    0x6b543301 <GdiPhoto+1933>

# 1117      }
# 1118      Tk_PhotoGetImage (photo_handle, &img_block);
#    0x6b542f1f <GdiPhoto+939>:   mov    0x6b559520,%eax
#    0x6b542f24 <GdiPhoto+944>:   mov    0x250(%eax),%eax
#    0x6b542f2a <GdiPhoto+950>:   lea    -0x74(%ebp),%edx
#    0x6b542f2d <GdiPhoto+953>:   mov    %edx,0x4(%esp)
#    0x6b542f31 <GdiPhoto+957>:   mov    -0x3c(%ebp),%edx
#    0x6b542f34 <GdiPhoto+960>:   mov    %edx,(%esp)
#    0x6b542f37 <GdiPhoto+963>:   call   *%eax

# 1119
# 1120      nx  = img_block.width;
#    0x6b542f39 <GdiPhoto+965>:   mov    -0x70(%ebp),%eax
#    0x6b542f3c <GdiPhoto+968>:   mov    %eax,-0x40(%ebp)

# 1121      ny  = img_block.height;
#    0x6b542f3f <GdiPhoto+971>:   mov    -0x6c(%ebp),%eax
#    0x6b542f42 <GdiPhoto+974>:   mov    %eax,-0x44(%ebp)

# 1122      sll = ((3*nx + 3) / 4)*4; /* must be multiple of 4 */
#    0x6b542f45 <GdiPhoto+977>:   mov    -0x40(%ebp),%eax
#    0x6b542f48 <GdiPhoto+980>:   lea    0x1(%eax),%edx
#    0x6b542f4b <GdiPhoto+983>:   mov    %edx,%eax
#    0x6b542f4d <GdiPhoto+985>:   add    %eax,%eax
#    0x6b542f4f <GdiPhoto+987>:   add    %edx,%eax
#    0x6b542f51 <GdiPhoto+989>:   lea    0x3(%eax),%edx
#    0x6b542f54 <GdiPhoto+992>:   test   %eax,%eax
#    0x6b542f56 <GdiPhoto+994>:   cmovs  %edx,%eax
#    0x6b542f59 <GdiPhoto+997>:   sar    $0x2,%eax
#    0x6b542f5c <GdiPhoto+1000>:  shl    $0x2,%eax
#    0x6b542f5f <GdiPhoto+1003>:  mov    %eax,-0x48(%ebp)

# End of assembler dump.
# (gdb)
