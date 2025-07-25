#!/bin/bash

user=$2
if [ "$user" == "" ];then
    user=$USER
fi

echo "Clearing shared memory segments for user: $user"

SEMID=`ipcs -m | grep $user | awk '{ if ($6==0) print $2 }'`

if [ -z "$SEMID" ]; then
    echo "No shared memory segments found for user: $user"
else
    echo "Found shared memory segments to clear:"
    ipcs -m | grep $user | awk '{ if ($6==0) printf "  ID: %s, Size: %s\n", $2, $5 }'
    
    for ID in $SEMID
    do
        ipcrm -m $ID && echo "shm cleared: $ID"
    done
fi

echo "Shared memory cleanup completed for user: $user"

#ipcs -m | awk '{if($6==0)print $2}' | xargs ipcrm shm

