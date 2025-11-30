#nslookup google.de | awk '/^Address: / { print $2 }'|head -1 > /tmp/ip.txt


gdb --args ./out/build/Debugging/io_uring_interface --no-tune  --server

