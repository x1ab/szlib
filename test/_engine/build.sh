#!/bin/bash
#!!
#!! NOTE: Git sh insists on "helpfully" screwing around with paths here... :-/
#!! E.g. "adjusting" "C:/sz/prj/Space_Test/test/tmp"
#!!               to "C:\sz\prj\Space_Test\test\C:\Program Files\Git\sz\prj\Space_Test\test\tmp\". :-o
#!! Fascinating... :-/
#!! Fortunately, prefixing commands with MSYS_NO_PATHCONV=1 helps.
#!!?? Or perhaps even https://www.msys2.org/wiki/Porting/#user-content-filesystem-namespaces:
#!!?? "Setting MSYS2_ARG_CONV_EXCL=* prevents any path transformation."?
export MSYS2_ARG_CONV_EXCL=*

if [ -z $SPACE_TEST_ENGINE_DIR ]; then
	echo "- [Space Test] INTERNAL ERROR: SPACE_TEST_ENGINE_DIR not set before calling build.sh!"
	exit 1
fi
. $SPACE_TEST_ENGINE_DIR/functions.sh

# "$1" - source to compile; default: $TEST_NAME
#! NOTE: Any individual $n args are expected to have been _quoted_ in the makefiles before passed to this script!

#echo ARGS: $@
#echo TOOLSET: $TOOLSET

# The makefile passes the case name and also the exe name!
case="$1"
exe="$2"
	#echo "ALL ARGS: [$*]"
	#echo "case: $case"
	#echo "exe: $exe"
	#echo "TEST_DIR: ${TEST_DIR}"

main_src="${case}.cpp"
	#echo "src: $main_src"

outdir="${TMP_DIR:-tmp}"
mkdir -p "$outdir"

case $TOOLSET in
msvc)
	export INCLUDE="$_TEST_DIR;$_TEST_DIR/..;$INCLUDE"
	#https://stackoverflow.com/a/73220812/1479945
	cmd="cl -nologo $_CFLAGS_MSVC $CFLAGS_MSVC \"-Fo$outdir/\" \"$main_src\" $CFLAGS_MSVC_"
	#!OLD:	MSYS_NO_PATHCONV=1 cl ...
	#!OLD:	# ^^^ Redundant now, with `export MSYS2_ARG_CONV_EXCL=*`, but keeping as a memento!... ;)
	;;
gcc*)
	# $CPATH is used implicitly"
	#!!?? Not quite sure why GCC on e.g. w64devkit survives the botched path autoconv.:
	#!!?? IOW: why no MSYS_NO_PATHCONV=1 here (only at MSVC)? Is it one-way, from Win to POSIX? (-> #100)
	cmd="g++ $_CFLAGS_GCC $CFLAGS_GCC \"-I${TEST_DIR}\" \"-I${TEST_DIR}/..\" -o \"$exe\" \"$main_src\" $CFLAGS_GCC_"
	;;
esac

DEBUG "$cmd"
eval "$cmd"
