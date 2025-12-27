#!/bin/bash
a=0
while [ $a -le 1000 ]
do
	echo Iter $a
	a=$((a + 1))
	make run
	sleep 0.3
done

