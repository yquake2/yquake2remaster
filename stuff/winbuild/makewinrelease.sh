#!/bin/sh

# ----

error() {
	echo "$@" 1>&2
	exit 1
}

usage() {
	echo "Usage: ./$SCRIPTNAME -c ctf/ -q yq2/ -o outdir/ -r rogue/ -u curl.dll -x xatrix/" 1>&2
	echo " * -c: Path to CTF sources" 1>&2
	echo " * -q: Path to Yamagi Quake II sources" 1>&2
	echo " * -o: Output directory" 1>&2
	echo " * -r: Path to Ground Zero sources" 1>&2
	echo " * -u: Path to curl.dll to be included" 1>&2
	echo " * -x: Path to The Reckoning sources" 1>&2
	exit 2
}

# ----

# Our name, for error messages.
SCRIPTNAME=$0

# Process command line arguments.
ARGS=$(getopt c:q:o:r:u:x: $*)
if [ $? -ne 0 ] ; then
	usage
fi
set -- $ARGS

while : ; do
	case "$1" in
		-c)
			CTF=$2
			shift; shift
			;;
		-q)
			QUAKE2=$2
			shift; shift
			;;
		-o)
			OUTDIR=$2
			shift; shift
			;;
		-r)
			ROGUE=$2
			shift; shift
			;;
		-u)
			CURL=$2
			shift; shift
			;;
		-x)
			XATRIX=$2
			shift; shift
			;;
		--)
			shift
			break
			;;
	esac
done

if [ -z $CTF ] ; then
	usage
else
	if [ ! -d $CTF ] ; then
		error "Not a directory: $CTF"
	else
		CTF=$(realpath $CTF)
	fi
fi

if [ -z $QUAKE2 ] ; then
	usage
else
	if [ ! -d $QUAKE2 ] ; then
		error "Not a directory: $QUAKE2"
	else
		QUAKE2=$(realpath $QUAKE2)
	fi
fi

if [ -z $ROGUE ] ; then
	usage
else
	if [ ! -d $ROGUE ] ; then
		error "Not a directory: $ROGUE"
	else
		ROGUE=$(realpath $ROGUE)
	fi
fi

if [ -z $CURL ] ; then
	usage
else
	if [ ! -f $CURL ] ; then
		error "$CURL doesn't exists or isn't a regular file"
	else
		CURL=$(realpath $CURL)
	fi
fi

if [ -z $XATRIX ] ; then
	usage
else
	if [ ! -d $XATRIX ] ; then
		error "Not a directory: $XATRIX"
	else
		XATRIX=$(realpath $XATRIX)
	fi
fi

if [ -z $OUTDIR ] ; then
	usage
else
	if [ -e $OUTDIR ] ; then
		error "$OUTDIR already exists"
	else
		mkdir -p $OUTDIR
		if [ $? -ne 0 ] ; then
			error "Couldn't create $OUTDIR"
		fi
		OUTDIR=$(realpath $OUTDIR)
	fi
fi

# Directory hierarchy.
mkdir $OUTDIR/ctf
mkdir $OUTDIR/rogue
mkdir $OUTDIR/xatrix
mkdir $OUTDIR/misc
mkdir $OUTDIR/misc/docs
mkdir $OUTDIR/misc/docs/ctf
mkdir $OUTDIR/misc/docs/rogue
mkdir $OUTDIR/misc/docs/xatrix
mkdir $OUTDIR/misc/docs/yquake2
mkdir $OUTDIR/misc/mapfixes

# Quake II.
cd $QUAKE2
make cleanall
make -j4
cp -r release/* $OUTDIR
cp stuff/yq2.cfg $OUTDIR/misc/
cp -r stuff/mapfixes/baseq2/ $OUTDIR/misc/mapfixes/
cp -r stuff/mapfixes/juggernaut/ $OUTDIR/misc/mapfixes/
for FILE in $(ls -1 doc/) ; do
	cp doc/$FILE $OUTDIR/misc/docs/yquake2/${FILE%.md}.txt
done
cp CHANGELOG $OUTDIR/misc/docs/yquake2/CHANGELOG.txt
cp LICENSE $OUTDIR/misc/docs/yquake2/LICENSE.txt
cp README.md $OUTDIR/misc/docs/yquake2/README.txt

# CTF
cd $CTF
make clean
make -j4
cp release/game.dll $OUTDIR/ctf/
cp CHANGELOG $OUTDIR/misc/docs/ctf/CHANGELOG.txt
cp LICENSE $OUTDIR/misc/docs/ctf/LICENSE.txt
cp README.md $OUTDIR/misc/docs/ctf/README.txt

# Ground Zero
cd $ROGUE
make clean
make -j4
cp release/game.dll $OUTDIR/rogue/
cp -r stuff/mapfixes $OUTDIR/misc/mapfixes/rogue
cp CHANGELOG $OUTDIR/misc/docs/rogue/CHANGELOG.txt
cp LICENSE $OUTDIR/misc/docs/rogue/LICENSE.txt
cp README.md $OUTDIR/misc/docs/rogue/README.txt

# The Reckoning
cd $XATRIX
make clean
make -j4
cp release/game.dll $OUTDIR/xatrix/
cp -r stuff/mapfixes $OUTDIR/misc/mapfixes/xatrix
cp CHANGELOG $OUTDIR/misc/docs/xatrix/CHANGELOG.txt
cp LICENSE $OUTDIR/misc/docs/xatrix/LICENSE.txt
cp README.md $OUTDIR/misc/docs/xatrix/README.txt

# Copy the libraries.
cp $HOME/../dep/SDL3/$(uname -m)-w64-mingw32/bin/SDL3.dll $OUTDIR/
cp $CURL $OUTDIR/curl.dll
if [ "$(uname -m)" == "i686" ] ; then
	cp $HOME/../dep/openal-soft/bin/Win32/soft_oal.dll $OUTDIR/openal32.dll
else
	cp $HOME/../dep/openal-soft/bin/Win64/soft_oal.dll $OUTDIR/openal32.dll
fi
