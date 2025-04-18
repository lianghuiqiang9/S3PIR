# 机器名
  Model name:             AMD Ryzen 7 8845HS w/ Radeon 780M Graphics 
  Mem: 8GB

# 基本吻合
这里提一个设想，真的希望每一个论文都有一个sh存终端命令，一键运行。

# 终端命令
g++ -o s3pir -I src/include src/client.cpp src/server.cpp src/main.cpp src/utils.cpp src/include/client.h src/include/server.h src/include/utils.h -Ofast -std=c++11 -lcryptopp 

./s3pir --one-server 20 32 output.csv

g++ -DSimLargeServer -o s3pir_simlargeserver -I src/include src/client.cpp src/server.cpp src/main.cpp src/utils.cpp src/include/client.h src/include/server.h src/include/utils.h -Ofast -std=c++11 -lcryptopp 

./s3pir_simlargeserver --one-server 20 32 output.csv

# 下一步计划，
1. 更新如何进行的
2. 数据库怎么后面是0了呢？


#  ./benchmark.sh -b SMALL
Running small benchmark..
make: Nothing to be done for 'all'.
== One server variant ==
LogDBSize: 20
EntrySize: 32 bytes
Running offline phase..
Offline: cutoffs done, invalid hints: 7
Offline: extra indices done.
Offline: 3.687 s
Running 1024 queries
Completed: 0%
Completed: 20%
Completed: 40%
Completed: 60%
Completed: 80%
Completed: 100%
Ran 1024 queries
Online: 212 ms
Cost Per Query: 0.207031 ms
Amortized compute time per query: 0.297046 ms

== One server variant ==
LogDBSize: 24
EntrySize: 32 bytes
Running offline phase..
Offline: cutoffs done, invalid hints: 0
Offline: extra indices done.
Offline: 59.806 s
Running 4096 queries
Completed: 0%
Completed: 20%
Completed: 40%
Completed: 60%
Completed: 80%
Completed: 100%
Ran 4096 queries
Online: 2734 ms
Cost Per Query: 0.66748 ms
Amortized compute time per query: 1.03251 ms

== Two server variant ==
LogDBSize: 20
EntrySize: 32 bytes
Running offline phase..
Invalid hints: 3
Offline: 2.488 s
Running 1024 queries
Completed: 0%
Completed: 20%
Completed: 40%
Completed: 60%
Completed: 80%
Completed: 100%
Ran 1024 queries
Online: 139 ms
Cost Per Query: 0.135742 ms

== Two server variant ==
LogDBSize: 24
EntrySize: 32 bytes
Running offline phase..
Invalid hints: 0
Offline: 43.066 s
Running 4096 queries
Completed: 0%
Completed: 20%
Completed: 40%
Completed: 60%
Completed: 80%
Completed: 100%
Ran 4096 queries
Online: 2570 ms
Cost Per Query: 0.627441 ms

# ./benchmark.sh -b LARGE
Running large benchmark..
make: Nothing to be done for 'all'.
== One server variant ==
LogDBSize: 20
EntrySize: 32 bytes
Running offline phase..
Offline: cutoffs done, invalid hints: 7
Offline: extra indices done.
Offline: 3.911 s
Running 1024 queries
Completed: 0%
Completed: 20%
Completed: 40%
Completed: 60%
Completed: 80%
Completed: 100%
Ran 1024 queries
Online: 151 ms
Cost Per Query: 0.147461 ms
Amortized compute time per query: 0.242944 ms

== One server variant ==
LogDBSize: 24
EntrySize: 32 bytes
Running offline phase..
Offline: cutoffs done, invalid hints: 0
Offline: extra indices done.
Offline: 61.685 s
Running 4096 queries
Completed: 0%
Completed: 20%
Completed: 40%
Completed: 60%
Completed: 80%
Completed: 100%
Ran 4096 queries
Online: 2522 ms
Cost Per Query: 0.615723 ms
Amortized compute time per query: 0.992218 ms

== One server variant ==
LogDBSize: 28
EntrySize: 8 bytes
Running offline phase..
Offline: cutoffs done, invalid hints: 6
Offline: extra indices done.
Offline: 889.357 s
Running 16384 queries
Completed: 0%
Completed: 20%
Completed: 40%
Completed: 60%
Completed: 80%
Completed: 100%
Ran 16384 queries
Online: 34433 ms
Cost Per Query: 2.10162 ms
Amortized compute time per query: 3.45867 ms

== One server variant ==
LogDBSize: 28
EntrySize: 32 bytes
Running offline phase..
Offline: cutoffs done, invalid hints: 6
Offline: extra indices done.
Offline: 8956.44 s
Running 16384 queries
Completed: 0%
Completed: 20%
Completed: 40%
Completed: 60%
Completed: 80%
Completed: 100%
Ran 16384 queries
Online: 44586 ms
Cost Per Query: 2.72131 ms
Amortized compute time per query: 16.3878 ms

== Two server variant ==
LogDBSize: 20
EntrySize: 32 bytes
Running offline phase..
Invalid hints: 3
Offline: 2.456 s
Running 1024 queries
Completed: 0%
Completed: 20%
Completed: 40%
Completed: 60%
Completed: 80%
Completed: 100%
Ran 1024 queries
Online: 135 ms
Cost Per Query: 0.131836 ms

== Two server variant ==
LogDBSize: 24
EntrySize: 32 bytes
Running offline phase..
Invalid hints: 0
Offline: 44.423 s
Running 4096 queries
Completed: 0%
Completed: 20%
Completed: 40%
Completed: 60%
Completed: 80%
Completed: 100%
Ran 4096 queries
Online: 2182 ms
Cost Per Query: 0.532715 ms

== Two server variant ==
LogDBSize: 28
EntrySize: 8 bytes
Running offline phase..
Invalid hints: 2
Offline: 680.48 s
Running 16384 queries
Completed: 0%
Completed: 20%
Completed: 40%
Completed: 60%
Completed: 80%
Completed: 100%
Ran 16384 queries
Online: 29634 ms
Cost Per Query: 1.80872 ms

== Two server variant ==
LogDBSize: 28
EntrySize: 32 bytes
Running offline phase..
Invalid hints: 2
Offline: 847.514 s
Running 16384 queries
Completed: 0%
Completed: 20%
Completed: 40%
Completed: 60%
Completed: 80%
Completed: 100%
Ran 16384 queries
Online: 40827 ms
Cost Per Query: 2.49188 ms