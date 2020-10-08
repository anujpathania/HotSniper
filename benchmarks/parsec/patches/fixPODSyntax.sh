#! /bin/bash
cd parsec-2.1
for i in 0 1 2 3 4 5 6 7 8 9 
do
    echo "Replacing '=item $i' to '=item C<$i>'"
    grep -rl "=item $i" * | xargs sed -i "s/=item $i/=item C<$i>/g"
done
exit 0
