stringContain() { case $2 in *$1* ) return 0;; *) return 1;; esac ;}

for f in $(find ./testcases -name '*.s');
do
	if stringContain "charmv5plus" "$f"; then
		python3 verify.py $f -ec
	else
		python3 verify.py $f
	fi
done
