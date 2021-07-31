g++ load.cpp -o load -O0
chmod 0744 load

echo -1 | load > basic.out

for i in {0..7}
do
    echo $i | load > $i.out
done