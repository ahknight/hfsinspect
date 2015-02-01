#!/bin/bash

if [ -e "$1" ]; then
    # Append latest
    TAG=$( git describe --tags --abbrev=0 HEAD )
    DATE=$( git log --date=short --format="%cd" --abbrev-commit -n 1 $TAG)
    echo $2 / $DATE  >> $1
    echo ==================  >> $1
    echo >> $1
    git log --reverse --format=" * %s" ${TAG}.. >> $1
    
else
    # Write whole file
    for TAG in $( git tag ); do
        DATE=$( git log --date=short --format="%cd" --abbrev-commit -n 1 $TAG)
        echo $TAG / $DATE  >> $1
        echo ==================  >> $1
        echo >> $1
        if [ -z "$LAST" ]; then
            git log --reverse --format=" * %s" $TAG >> $1
        else
            git log --reverse --format=" * %s" ${LAST}..${TAG} >> $1
        fi
        echo >> $1
        LAST="$TAG"
    done
fi
