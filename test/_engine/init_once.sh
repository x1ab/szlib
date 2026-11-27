#! Don't forget to export everything (non-local), as the per-case runs will be
#! separate processes! (WHY, BTW??)

#export _TEST_ENGINE_DIR=${_TEST_ENGINE_DIR:-`dirname $0`}
	#!BEWARE: this $0 would ONLY work if we were the root script! :-o
	#!Which wouldn't make much sense, so... Better just to require it!
if [ -z "$_TEST_ENGINE_DIR" ]; then
	echo "- ERROR: _TEST_ENGINE_DIR must be set before calling init_once.sh!"
	exit 101
fi
#-----------------------------------------------------------------------------
export SPACE_TEST_ENGINE_VERSION=$(cat ${_TEST_ENGINE_DIR}/VERSION | xargs)
	# xargs is just to strip any surrounding *inline* whitespace.
	# $() have removed any surrounding newlines already.
#-----------------------------------------------------------------------------
# ...but it's not required to have already been normalized, so:
_TEST_ENGINE_DIR=`realpath ${_TEST_ENGINE_DIR}`
export SPACE_TEST_ENGINE_DIR=$_TEST_ENGINE_DIR
. $_TEST_ENGINE_DIR/functions.sh
#-----------------------------------------------------------------------------
# Banner...
#!! No config yet, so can't control it (also: DEBUG is still a noop) here!
echo "Space Test $SPACE_TEST_ENGINE_VERSION"
#-----------------------------------------------------------------------------

# This should also come preset, but we can fallback to CWD:
export TEST_DIR=${TEST_DIR:-`pwd`}
#DEBUG "Test dir is: \"$TEST_DIR\""
if [ ! -e "$TEST_DIR" ]; then FATAL "TEST_DIR \"$TEST_DIR\" doesn't seem to exist!" 102; fi

export TMP_DIR="$TEST_DIR/.tmp"
#!! Fall back to $TMP, $TEMP, /tmp etc...
if [ ! -d "$TMP_DIR" ]; then
	mkdir -p  "$TMP_DIR"
	if [ ! -d "$TMP_DIR" ]; then
		FATAL "Couldn't create tmp dir '$TMP_DIR'!:" 103
	fi
	WARNING "$TMP_DIR did not exist, created."
fi


# Add (prepend! -> #36) the engine dir to the path
#-----------------------------------------------------------------------------
p=`abspath_fixup $_TEST_ENGINE_DIR`
#DEBUG "_TEST_ENGINE_DIR=$_TEST_ENGINE_DIR"
#DEBUG "p=$p"
PATH="$p`get_path_sep`$PATH"
# ^^^ Yeah, nice, except this doesn't help with running scripts directly with
#     `sh script.sh`, as PATH lookup is not required (even discouraged) for that!
#     Fortunately, sourcing works, i.e. `sh -c ". build.sh"` *does* force PATH lookup! ;-o

# CFG + DEFAULTS
#-----------------------------------------------------------------------------

# Load the (optional) config:
test -e "$TEST_DIR/ .cfg" && . "$TEST_DIR/ .cfg"

# Set some "hard" defaults:
export TEST_NAME=${TEST_NAME:-test}
export EXPECT_FILE_NAME=${EXPECT_FILE_NAME:-EXPECT}
export TEST_CASE_SCRIPT_NAME=${TEST_CASE_SCRIPT_NAME:-SCRIPT}
export TEST_CASE_FILE_EXT=${TEST_CASE_FILE_EXT:-.case}
# Public globals should be prefixed with `SPACE_` (most legacy names don't yet):
export SPACE_TC_SETUP_SCRIPT_NAME="setup.sh"
export SPACE_TC_CLEANUP_SCRIPT_NAME="cleanup.sh"


# "Soft" defaults:
export TOOLSET=`get_toolset`

# ROOT-LEVEL BUILD
#-----------------------------------------------------------------------------
export CASE="$TEST_NAME"
export TEST_CASE_DIR="$TEST_DIR"
build_dir "$TEST_CASE_DIR" || FATAL "Failed to build the test case!" $?
