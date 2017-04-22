#!/bin/sh
SIZE=$(( 1024 * 1024 * 2 ))

PARTITIONS="APM MBR GPT"

for PART in $PARTITIONS
do
    DEV=$(hdiutil attach -nomount ram://$SIZE)
    diskutil partitionDisk $DEV $PART \
        "HFS+" "HFS $PART" 25% \
        "HFSX" "HFSX $PART" 25% \
        "JHFS+" "JHFS $PART" 25% \
        "JHFSX" "JHFSX $PART" 25%
    hdiutil pmap -complete $DEV
done

# diskutil list
