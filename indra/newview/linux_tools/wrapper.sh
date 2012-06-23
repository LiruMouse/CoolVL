#!/bin/bash

## Here are some configuration options for Linux Client Testers.
## These options are for self-assisted troubleshooting during this beta
## testing phase; you should not usually need to touch them.

## - Avoids using any OpenAL audio driver.
export LL_BAD_OPENAL_DRIVER=x
## - Avoids using any FMOD audio driver.
#export LL_BAD_FMOD_DRIVER=x

## - Avoids using the FMOD ESD audio driver.
#export LL_BAD_FMOD_ESD=x
## - Avoids using the FMOD OSS audio driver.
#export LL_BAD_FMOD_OSS=x
## - Avoids using the FMOD ALSA audio driver.
#export LL_BAD_FMOD_ALSA=x

## - Avoids the optional OpenGL extensions which have proven most problematic
##   on some hardware.  Disabling this option may cause BETTER PERFORMANCE but
##   may also cause CRASHES and hangs on some unstable combinations of drivers
##   and hardware.
## NOTE: This is now disabled by default.
#export LL_GL_BASICEXT=x

## - Avoids *all* optional OpenGL extensions.  This is the safest and least-
##   exciting option.  Enable this if you experience stability issues, and
##   report whether it helps in the Linux Client Testers forum.
#export LL_GL_NOEXT=x

## - For advanced troubleshooters, this lets you disable specific GL
##   extensions, each of which is represented by a letter a-o.  If you can
##   narrow down a stability problem on your system to just one or two
##   extensions then please post details of your hardware (and drivers) to
##   the Linux Client Testers forum along with the minimal
##   LL_GL_BLACKLIST which solves your problems.
#export LL_GL_BLACKLIST=abcdefghijklmno

## - Some ATI/Radeon users report random X server crashes when the mouse
##   cursor changes shape.  If you suspect that you are a victim of this
##   driver bug, try enabling this option and report whether it helps:
#export LL_ATI_MOUSE_CURSOR_BUG=x

## - If you experience crashes with streaming video and music, you can
##   disable these by enabling this option:
#export LL_DISABLE_GSTREAMER=x

## Everything below this line is just for advanced troubleshooters.
##-------------------------------------------------------------------

## - For advanced debugging cases, you can run the viewer under the
##   control of another program, such as strace, gdb, or valgrind.  If
##   you're building your own viewer, bear in mind that the executable
##   in the bin directory will be stripped: you should replace it with
##   an unstripped binary before you run.
#export LL_WRAPPER='valgrind --smc-check=all --error-limit=no --log-file=secondlife.vg --leak-check=full --suppressions=/usr/lib/valgrind/glibc-2.5.supp --suppressions=secondlife-i686.supp'

## - Avoids an often-buggy X feature that doesn't really benefit us anyway.
export SDL_VIDEO_X11_DGAMOUSE=0

## - Works around a problem with misconfigured 64-bit systems not finding GL
export LIBGL_DRIVERS_PATH="${LIBGL_DRIVERS_PATH}":/usr/lib64/dri:/usr/lib32/dri:/usr/lib/dri

## - The 'scim' GTK IM module widely crashes the viewer.  Avoid it.
if [ "$GTK_IM_MODULE" = "scim" ]; then
    export GTK_IM_MODULE=xim
fi

## Work around for a crash bug when restarting OpenGL after a change in the
## graphic settings (anti-aliasing, VBO, FSAA, full screen mode, UI scale).
## When you enable this work around, you can change the settings without
## crashing, but you will have to restart the viewer after changing them
## because the display still gets corrupted.
export LL_OPENGL_RESTART_CRASH_BUG=x

## - Work around the ATI mouse cursor crash bug with fglrx and Mobility Radeon:
##   If you don't have lspci, or if you think this can solve problems with a
##	 non-mobility Radeon card, you may simply uncomment the following line to
##   force the work around:
#export LL_ATI_MOUSE_CURSOR_BUG=x
if [ -x /usr/bin/lspci ] ; then
	if lspci | grep "Mobility Radeon" &>/dev/null ; then
		if lsmod | grep fglrx &>/dev/null ; then
			export LL_ATI_MOUSE_CURSOR_BUG=x
		fi
	fi
fi

## unset proxy vars
unset http_proxy https_proxy no_proxy HTTP_PROXY HTTPS_PROXY

## Nothing worth editing below this line.
##-------------------------------------------------------------------

SCRIPTSRC=`readlink -f "$0" || echo "$0"`
RUN_PATH=`dirname "${SCRIPTSRC}" || echo .`
echo "Running from ${RUN_PATH}"
cd "${RUN_PATH}"

# Run under gdb when --debug was passed as the first option in the command
# line. If you have a symbols file in bin/ (that you can obtained via:
# eu-strip --strip-debug --remove-comment -o /dev/null -f secondlife-bin.debug secondlife-bin
# ), then it will be auto-loaded as well.
GDB_INIT="/tmp/gdb.init.$$"
if [ "$1" == "--debug" ] ; then
	shift
	symbols=$RUN_PATH/bin/secondlife-bin.debug
	if [ -f $symbols ] ; then
		# Alas, quoting as "-ex \"symbol-file $symbols\"" in the gdb command
		# line won't work (symbol-file and $symbols would be counted as two
		# separate options instead of a quoted entity)... bash is stupid !
		# We therefore use a temporary init file instead...
		echo "set verbose on" >$GDB_INIT
		echo "symbol-file $symbols" >>$GDB_INIT
		echo "set verbose off" >>$GDB_INIT
		export LL_WRAPPER="gdb -x $GDB_INIT --args"
	else
		export LL_WRAPPER='gdb --args'
	fi
fi

# If fmod support is missing, activate OpenAL support
if [ ! -f "./lib/libfmod-3.75.so" ] ; then
	unset LL_BAD_OPENAL_DRIVER
	export LL_BAD_FMOD_DRIVER=x
fi

# Re-register the secondlife:// protocol handler every launch, for now.
./register_secondlifeprotocol.sh

export VIEWER_BINARY='cool_vl_viewer-bin'
export SL_ENV='LD_LIBRARY_PATH="`pwd`""/lib:${LD_LIBRARY_PATH}"'
export SL_CMD='$LL_WRAPPER bin/$VIEWER_BINARY'
export SL_OPT="`cat gridargs.dat` $@"

# Run the program
eval ${SL_ENV} ${SL_CMD} ${SL_OPT} || LL_RUN_ERR=runerr

# Handle any resulting errors
if [ -n "$LL_RUN_ERR" ]; then
	LL_RUN_ERR_MSG=""
	if [ "$LL_RUN_ERR" = "runerr" ]; then
		# generic error running the binary
		echo '*** Bad shutdown. ***'
		BINARY_TYPE=$(expr match "$(file -b ${RUN_PATH}/bin/SLPlugin)" '\(.*executable\)')
		BINARY_SYSTEM=$(expr match "$(file -b /bin/uname)" '\(.*executable\)')
		if [ "${BINARY_SYSTEM}" == "ELF 64-bit LSB executable" -a "${BINARY_TYPE}" == "ELF 32-bit LSB executable" ]; then
			echo
			cat << EOFMARKER
You are running the Second Life Viewer on a x86_64 platform.  The
most common problems when launching the Viewer (particularly
'bin/$VIEWER_BINARY: not found' and 'error while
loading shared libraries') may be solved by installing your Linux
distribution 32-bit compatibility packages.
For example, on Ubuntu and other Debian-based Linuxes you might run:
$ sudo apt-get install ia32-libs ia32-libs-gtk ia32-libs-kde ia32-libs-sdl
EOFMARKER
		fi
	fi
fi

# Remove the gdb init file if it was created
if [ -f $GDB_INIT ] ; then
	rm -f $GDB_INIT
fi

echo
echo '*******************************************************'
echo 'This is a BETA release of the Second Life linux client.'
echo 'Thank you for testing!'
echo 'Please see README-linux.txt before reporting problems.'
echo
