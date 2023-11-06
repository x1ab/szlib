##############################################################################
#
# Makefile for BusyBox's built-ing pdpmake, using extensions that are NOT
# compatible with GNUmake!
#
#! NOTE: The MAKE process must be run in the project (root) dir,
#!       because everything is assumed to be relative to that!
#!	 (BB's pdpmake doesn't even support abs. paths for targets on Windows! :-o )
#!

#--- BUILD MODE DEFAULTS -----------------------------------------------------
DEBUG = 0
CRT = static

#--- PRJ DEFINITIONS ---------------------------------------------------------
SZ_APP_NAME ?= bb-build-test
SZ_PRJ_DIR  ?= .
SZ_SRC_SUBDIR ?= src
SZ_OUT_SUBDIR ?= .tmp/build
#!! Can't be empty, for error-prone $(...DIR)/... concats (yet)!
SZ_RUN_SUBDIR ?= .

exe = $(exedir)/$(SZ_APP_NAME)$(cflags_crt_linkmode_with_debug)$(buildmode_sfml_linkmode_tag).exe

outdir = $(SZ_OUT_SUBDIR)
#!!objdir = $(vroot)/obj
#!!Alas...:
objdir = $(vroot)/$(SZ_SRC_SUBDIR)
#!!ifcdir = $(vroot)/ifc
#!!Alas...:
ifcdir = $(vroot)/$(SZ_SRC_SUBDIR)
exedir = $(SZ_RUN_SUBDIR)

#!! Kludge until I find out the correct way to support C++ modules...
#!!Use != ls -b or something *.ifc!
#!!CPP_MODULE_IFCS = $(ifcdir)/*.ifc
#CPP_MODULE_IFCS = $(ifcdir)/Storage.ifc
#!! Also, any module change will trigger full rebuild, coz no sane way
#!! to auto-track actual deps yet (even less than headers)! :-/
#!! (E.g. https://stackoverflow.com/questions/69190433/auto-generate-dependencies-for-modules,
#!! https://github.com/premake/premake-core/issues/1735)
#!! So, well, sigh... (or add a manaully updated list...):
objs_using_modules = $(objs)

EXT_LIBS =

#----------------------------
# Static/DLL CRT link mode
#------
# Keep these -... not /... because they're used for (lib) file tagging, too! :)

_cflags_crt_linkmode__static := -MT
_cflags_crt_linkmode__dll    := -MD
cflags_crt_linkmode := $(_cflags_crt_linkmode__$(CRT))

_cflags_crt_linkmode_with_debug__0 = $(cflags_crt_linkmode)
_cflags_crt_linkmode_with_debug__1 = $(cflags_crt_linkmode)d
cflags_crt_linkmode_with_debug = $(_cflags_crt_linkmode_with_debug__$(DEBUG))

#----------------------
# DEBUG/RELEASE mode
#------
_cflags_debug__0 = -DNDEBUG /O2
_cflags_debug__1 = /DDEBUG /ZI /Od /Oy- /Ob0 /RTCsu /Fd$(objdir)/
#	The -O...s above are borrowed from Dr. Memory's README/Quick start.
#	-ZI enables edit-and-continue (but it only exists for Intel CPUs!).
cflags_debug = $(_cflags_debug__$(DEBUG))

_linkflags_debug__0 =
_linkflags_debug__1 = -debug -incremental -editandcontinue -ignore:4099
linkflags_debug = $(_linkflags_debug__$(DEBUG))

#------------------------------
# SFML link mode (static/dll)
#------
_cflags_sfml_linkmode__static = -DSFML_STATIC
cflags_sfml_linkmode = $(_cflags_sfml_linkmode__$(SFML))


# Output dir/file tags for build alternatives
# NOTE: Must differ from their "no ..." counterparts to avoid code (linking) mismatches!
# Also: CRT=dll is always the case with the pre-built SFML libs, so no use changing it...
##!!
##!! Instead of the horror of having 3 sets of these macros (a generic tag,
##!! a dir tag, a file tag -- where each must separately dispatch on their
##!! parameter!), use a uniform format suitable for each purpose (as a suffix):
##!!
_buildmode_debug_tag__0 =
_buildmode_debug_tag__1 = .d
buildmode_debug_tag     = $(_buildmode_debug_tag__$(DEBUG))
_buildmode_crtdll_tag__static =
_buildmode_crtdll_tag__dll    = .crtdll
buildmode_crtdll_tag          = $(_buildmode_crtdll_tag__$(CRT))

# All combined:
buildmode_dir_tag  = $(buildmode_crtdll_tag)$(buildmode_debug_tag)
buildmode_file_tag = $(buildmode_crtdll_tag)$(buildmode_debug_tag)
#debug:; @echo buildmode_dir_tag: $(buildmode_dir_tag)
#debug:; @echo buildmode_file_tag: $(buildmode_file_tag)


#--- CUSTOMIZATIONS ----------------------------------------------------------
CL   := cl /nologo
CC   := $(CL) -c
CXX  := $(CL) -c /EHsc
LINK := link.exe /nologo
SH   := busybox sh


# The optional `...FLAGS_` macros can be passed via the cmdline for overriding!
# Not using += because we're prepending, to support overriding (e.g. from the CMDLINE)!
#! Kludge: avoid recursive assignment, while still allowing deferred expansion!
CFLAGS   = $(cflags_crt_linkmode_with_debug) $(cflags_debug)
#!! Incomplatible with /RTCs if DEBUG, so moved back to the debug dispatching:
#!!CFLAGS  += /O2
# Add a /I also for including the commit hash file:
CFLAGS  += /I$(objdir) /I$(outdir)
CFLAGS  += /W4
CFLAGS  += $(CFLAGS_)
CPPFLAGS   = $(CFLAGS) /std:c++latest /ifcOutput $(ifcdir)/ /ifcSearchDir $(ifcdir)/
CPPFLAGS  += /GL
CPPFLAGS  += $(CPPFLAGS_)
LINKFLAGS   = $(linkflags_debug)
LINKFLAGS  += /LTCG
LINKFLAGS  += $(linkflags_debug) $(LINKFLAGS_)


#-----------------------------------------------------------------------------
# BUILD "ENGINE"
#-----------------------------------------------------------------------------

# Preparations...

# This is the build-mode-specific (and accordingly tagged) "VPATH" root:
vroot = $(SZ_OUT_SUBDIR)/v$(buildmode_dir_tag)
#!!But... for a later hack to grep this out form /showInlcude outputs, and
#!!for (surprisingly) pdpmake not being able to $(...:x=y) mid-word, we still
#!!need one with \ not /...:
vroot_bs = $(SZ_OUT_SUBDIR)\v$(buildmode_dir_tag)
#dump:;@echo $(vroot)

src_subdir=$(SZ_SRC_SUBDIR)
SOURCES != busybox sh -c "find $(src_subdir) -name '*.c' -o -name '*.cpp' -o -name '*.ixx'"
#!!?? Couldn't get the BB `find` syntax right without the `sh -c` kludge! :-/
SOURCES := $(SOURCES:$(src_subdir)/%=%)
#test:: ; @echo SOURCES: $(SOURCES)

# Generated per-obj header auto-dep. includes are collected here:
hdep_makinc_file = $(vroot)/hdeps.mak

# No need for such pattern shenanigans, as the current dir (./) has
# been appended by `find`
#!!_src_prep := FPFX_PLACEHOLDER_$(SOURCES:%.c=FPFX_PLACEHOLDER_%.c)
objs := $(SOURCES:%=$(objdir)/%)
#dump:; @echo objs: $(objs)
objs := $(objs:%.c=%.obj)
objs := $(objs:%.cpp=%.obj)
objs := $(objs:%.ixx=%.obj)
#dump:; @echo objs: $(objs)
#!! No idea how this would translate to 2-file module sources though:
#!! CPP_MODULE_IFCS already has $(ifcdir), which is currently identical to $(objdir), so...
objs_of_modules = $(CPP_MODULE_IFCS:%.ifc=%.obj)
#dump:; @echo objs_of_modules: $(objs_of_modules)

#-----------------------------------------------------------------------------
# List the phonies so `make -t` won't create empty files for them:
.PHONY: build info mk_outdirs collect_sources cpp_modules

build: info .WAIT mk_outdirs .WAIT collect_sources .WAIT cpp_modules .WAIT $(exe);
	@echo "Main target ready: $(exe)"
info:
	@echo "Build mode: DEBUG=$(DEBUG), CRT=$(CRT), SFML=$(SFML)"

_mkdir = test -d $@ || mkdir $@
mk_outdirs: $(outdir) $(vroot) $(objdir) $(ifcdir) $(exedir); @#
# Double colons in order to not fail on possibly identical dirs:
$(outdir)::; @$(_mkdir)
$(vroot)::;  @$(_mkdir)
$(objdir)::; @$(_mkdir)
$(ifcdir)::; @$(_mkdir)
$(exedir)::; @$(_mkdir)

collect_sources:
	@echo Syncing the build cache...
#	This (cp -u) only copies newer files (confirmed with -uia):
	@cp -ufa '$(src_subdir)' '$(vroot)'
#	As cp can only add and overwrite but not delete existing files,
#	the cached tree must be deleted from time to time to remove cruft!
	@touch "$(vroot)/.cached_src_age"
	@_cached_src_age=$$((`cat "$(vroot)/.cached_src_age"`)) &&\
		if [ "$$_cached_src_age" != "10" ]; then\
			echo "$$((_cached_src_age + 1))"> "$(vroot)/.cached_src_age";\
		else\
			echo "[!!NOT IMPLEMENTED YET!!] Src cache too old, deleting...";\
			echo 0> "$(vroot)/.cached_src_age";\
		fi
#!! mv can't merge into non-empty dirs with same name, so the 2nd runs failed:
#!! Resorting to keeping the entire src subtree under objdir...
#	mv $(objdir)/$(srcdir)/* -t $(objdir)
#	rmdir $(objdir)/$(srcdir)
#!! This (just like -t) would NOT preserve the src subdirs!
#	cp -u $(SOURCES) $(objdir)/
#	cp -u $(HEADERS) $(objdir)/
	@echo -n "" > $(hdep_makinc_file)
	@for o in $(objs:%.obj=%.obj.dep); do echo "-include $$o" >> "$(hdep_makinc_file)"; done

cpp_modules:: $(CPP_MODULE_IFCS) $(objs_of_modules)
	@echo "C++ modules up-to-date."

# (Sigh... See the issue at the def. of objs_using_modules!)
#!!?? Is adding $(objs_of_modules) actually necessary?
#!!??$(objs_using_modules) $(objs_of_modules): $(CPP_MODULE_IFCS)
$(objs_using_modules): $(CPP_MODULE_IFCS)

# Include autodeps if present
-include $(hdep_makinc_file)

# Strip the ugly paths from each object
#!! Alas, can't be done without my path-inference hack to pdpmake:
#!!$(exe): $(objs:$(objdir)/%=%)
#!!	@echo LINK $?
#!!	$(LINK) $(LINKFLAGS) /libpath:$(objdir) $(objs) $(EXT_LIBS) /out:$@
$(exe): $(objs)
	$(LINK) $(LINKFLAGS) $(objs) $(EXT_LIBS) /out:$@

#-----------------------------------------------------------------------------
# Inference rules...
#-----------------------------------------------------------------------------
.SUFFIXES: .c .cpp .ixx

_cc_info = @echo Compiling $(<:$(vroot)/%=%)...
_cc_out  = $(outdir)/.tool-output.tmp
_cc_exitcode = $(outdir)/.tool-exitcode.tmp

# Special fucked-up hack to work around the /showIncludes output of CL
# incorrectly being sent to stdout -- AS WELL AS ITS ERRORS!!! :-O OMFG... :-/
# Also make sure to fail on behalf of CL if there were errors...
_CL_dephack = /showIncludes > $(_cc_out); echo $$? > $(_cc_exitcode)
_CL_dephack_sh = $(outdir)/.cl-stderr-fuckup.sh
_CL_hdeps = $(outdir)/.hdeps.tmp
_CL_dephack_errhandler =@\
	echo "grep -v \"Note: including file:\" $(_cc_out)"                    >  $(_CL_dephack_sh) &&\
	echo "grep -q \" error\" $(_cc_out) && exit `cat \"$(_cc_exitcode)\"`" >> $(_CL_dephack_sh) &&\
	echo "grep    \"Note: including file:\" $(_cc_out) > $(_CL_hdeps)"     >> $(_CL_dephack_sh) &&\
	echo "exit 0"                                                          >> $(_CL_dephack_sh) &&\
	. $(_CL_dephack_sh)

#! Note: using $(outdir) instead of $(vroot), to not miss build-mode-
#!       independent (e.g. generated) includes; see:
#!		if (index($$0, "$(--> NOT: vroot <--)") != 0) print $$0 "\\" }\
#!! IT WILL HAVE TO BE ADJUSTED AGAIN, WHEN ELIMINATING THE SRC TREE DUPLICATION!...
_CL_dephack_procdep = @busybox awk -e 'BEGIN{print"$@:\\"}\
	{ sub($$1 FS $$2 FS $$3 FS, ""); gsub(/\\/, "/", $$0);\
		if (index($$0, "$(outdir)") != 0) print $$0 "\\" }\
	END{print""}' "$(_CL_hdeps)" > $@.dep
#!!?? WTF: this worked directly from the cmd (FAR) commandline, but not here:
#!!??_teehack2 = busybox grep -iF $(vroot_bs)\$(src_subdir) $@.dep.tmp

.c.obj:
	$(_cc_info)
	-$(CC) $(CFLAGS) /Fo$(objdir)/ $< $(_CL_dephack)
		$(_CL_dephack_errhandler)
	$(_CL_dephack_procdep)
	@mv $(objdir)/`basename $@` -t `dirname $@`

.cpp.obj:
	$(_cc_info)
	-$(CXX) $(CPPFLAGS) /Fo$(objdir)/ $< $(_CL_dephack)
		$(_CL_dephack_errhandler)
	$(_CL_dephack_procdep)
	@mv $(objdir)/`basename $@` -t `dirname $@`

.ixx.ifc:
	$(_cc_info)
	-$(CXX) $(CPPFLAGS) /Fo$(objdir)/ $< $(_CL_dephack)
		$(_CL_dephack_errhandler)
	$(_CL_dephack_procdep)

.ixx.obj:
	$(_cc_info)
	-$(CXX) $(CPPFLAGS) /Fo$(objdir)/ $< $(_CL_dephack)
		$(_CL_dephack_errhandler)
	$(_CL_dephack_procdep)
	@mv $(objdir)/`basename $@` -t `dirname $@`

#-----------------------------------------------------------------------------
# Custom rules...
#-----------------------------------------------------------------------------
