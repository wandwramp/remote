#
# Install shell script for minicom and friends.
#
NAME="`whoami`" 2>/dev/null
if [ "$NAME" = "" ]
then
	echo "No whoami? Hmmm.. must be Coherent.. Trying \"who am i\""
	NAME="`who am i | cut -d' ' -f1`"
fi

if test "$NAME" != root
then
	echo "You must be root to install remote."
	exit 1
fi

if test $# != 3
then
	echo "Usage: install.sh libdir bindir mandir"
	exit 1
fi

if test ! -d $1
then
	mkdir $1
	if [ $? != 0 ]
	then
		echo "$1: No such directory"
		exit 1
	fi
fi

if test ! -d $2
then
	echo "$2: No such directory"
	exit 1
fi

if test ! -d $3
then
	echo "$3: No such directory"
	exit 1
fi

if test -f remote
then
	echo "Installing remote in $2"
	cp remote $2/remote
	chmod 755 $2/remote
	chown remote $2/remote
#	chgrp remote $2/remote
fi

if test -f down
then
	echo "Installing down in $2"
	cp down $2/down
	chmod 755 $2/down
	chown remote $2/down
#	chgrp remote $2/down
fi

for i in runscript xremote
do
  if test -f $i
  then
	echo "Installing $i in $2"
	cp $i $2/$i
	chmod 755 $2/$i
	chown remote $2/$i
#	chgrp remote $2/$i
  fi
done

if test -f keyserv
then
	echo "Installing keyserv in $1"
	cp keyserv $1/keyserv
	chmod 755 $1/keyserv
	chown remote $1/keyserv
#	chgrp remote $1/keyserv
fi

echo "Installing manpages in $3"
for i in remote.1 runscript.1
do
	cp ../man/$i $3
	chmod 644 $3/$i
	chown remote $3/$i
#	chgrp remote $3/$i
done

if [ ! -f $1/remote.users ]
then
	echo "Installing sample config file remote.users in $1"
	cp remote.users $1
	chown remote $1/remote.users
#	chgrp remote $1/remote.users
	chmod 644 $1/remote.users
fi

if [ ! -f $1/remoterc.dfl ]
then
	echo "Installing sample config file remoterc.dfl in $1"
	cp remoterc.dfl $1
	chown remote $1/remoterc.dfl
#	chgrp remote $1/remoterc.dfl
	chmod 644 $1/remoterc.dfl
fi

	
echo "Making remote setuid to username remote"
chmod 4755 $2/remote

exit 0
