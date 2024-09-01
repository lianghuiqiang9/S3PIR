Artifact: Simple and Practical Sublinear Private Information Retrieval Using Dummy Subsets

This artifact provides an implementation of the S3PIR protocol. Running the artifact will reproduce S3PIR's performance results given in table 2 and table 3.

# Overview
* Installing Dependencies (5 human-minutes)
* Build (1~2 human-minutes)
* Run Experiments (5 compute-minutes ~ 4 compute-hours)

There are two ways of running the artifact: building from source or using the provided dockerfile. 

# Install Dependencies (5 human-minutes)

## From Source (Linux/Ubuntu)
  1. Install g++ and make.
  2. Install cryptopp. Installation instructions can be found at https://www.cryptopp.com/wiki/Linux. We suggest using apt-get/your distro's package manager to install the package. 

## From Dockerfile
  1. Install docker. Instructions can be found at https://docs.docker.com/engine/install/

# Build (1~2 human-minutes)

## From Source
From the project root directory, run 
```
mkdir -p build
make
```

This will create two binaries, `s3pir` and `s3pir_simlargeserver` in the build folder. 

## From Dockerfile
From the project root directory, run 
```
mkdir -p results
docker build -t s3pir .
```
(5 minutes to install) 

This builds the docker image for s3pir and makes a folder to store the benchmark results. 

# Run Experiments (5 compute-minutes ~ 4 compute-hours)
There are two versions of our artifact. The default version allocates about ~64gb of memory for the largest database size in our benchmark. If your machine does not have sufficient memory, we recommend using the simulated large server version which simulates the database and allocates less memory (less than 0.5gb for the largest database). 

We also provide three sets of benchmarks which reproduce the performance results of tables 2 and 3 for S3PIR in the paper. 
The small set reproduces the first 2 rows and takes around ~3 minutes to run. 
The large set reproduces the first 4 rows and takes around ~1.25 hours to run.
The full set reproduces all the rows and takes around ~2.5 hours to run. These times are estimates, and the actual time taken may vary depending on your machine.

Running the scripts will save the benchmark results in a csv file as well as print the results to standard out. These results are directly comparable with the data in the tables and can be used to validate the performance for S3PIR.

## From Source
You may have to change the permissions of `benchmark.sh` to run it (`chmod +x benchmark.sh`). The following scripts save the benchmark results in a csv file at `build/output.csv` (and will overwrite such a file if it exists). 

* Run `./benchmark.sh -b SMALL` to run the small benchmark.
* Run `./benchmark.sh -b LARGE` to run the large benchmark.
* Run `./benchmark.sh -b FULL` to run the full benchmark.

To run the simulated large server version:
* Run `./benchmark.sh -s -b SMALL` to run the small benchmark.
* Run `./benchmark.sh -s -b LARGE` to run the large benchmark.
* Run `./benchmark.sh -s -b FULL` to run the full benchmark.

## From Dockerfile
The following scripts save the benchmark results in a csv file at `result/output.csv` (and will overwrite such a file if it exists).

* Run `docker run --mount type=bind,source=./results,target=/results s3pir -b SMALL` to run the small benchmark.
* Run `docker run --mount type=bind,source=./results,target=/results s3pir -b LARGE` to run the large benchmark.
* Run `docker run --mount type=bind,source=./results,target=/results s3pir -b FULL` to run the full benchmark.

To run the simulated large server version:
* Run `docker run --mount type=bind,source=./results,target=/results s3pir -s -b SMALL` to run the small benchmark.
* Run `docker run --mount type=bind,source=./results,target=/results s3pir -s -b LARGE` to run the large benchmark.
* Run `docker run --mount type=bind,source=./results,target=/results s3pir -s -b FULL` to run the full benchmark.

# Additional Usage
The binaries can also be used to profile the implementation with different database parameters.

* Run `./build/s3pir --one-server <Log2 DB Size> <Entry Size> <Output File>` or 
`./build/s3pir --two-server <Log2 DB Size> <Entry Size> <Output File>` to run the protocol on a database with `<Log2 DB Size>` number of entries and entries of `<Entry Size>` bytes with the one server or two server variant respectively. 
* Run the `s3pir_simlargeserver` binary with the same arguments to use the simulated large server version.

To interact with these binaries using the Dockerfile, run `docker run -it s3pir -interactive` which opens an interactive shell. The binaries will be found in `./build`.

