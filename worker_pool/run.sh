#!/bin/bash
a=0
while [ $a -le 20 ]
do
	echo ----------------- Iter $a -------------------------
	a=$((a + 1))
	make run
	sleep 0.3
	echo ------------------ END ----------------------------
done

