# Tight-Sketch

We present Tight-Sketch, a highly efficient sketch designed to achieve both high detection accuracy and fast update speed. It employs a probabilistic decay strategy to replace tracked items in buckets based on multidimensional features, including item information and arrival strength. Our implementation of Tight-Sketch is based on the open-source code of MV-Sketch, and we would like to express our gratitude to the original authors for their valuable contributions.


## Compile and Run the examples
Tight-Sketch is implemented in C++. Below, we provide instructions for compiling the examples on Ubuntu using g++ and make.

### Requirements
Before proceeding, please ensure that the following requirements are met:

- g++ and make are installed on your system. Our experimental platform uses Ubuntu 20.04 and g++ 9.4.0.

- The libpcap library is installed on your system. Most Linux distributions include libpcap and can be installed using package managers such as apt-get in Ubuntu.

### Dataset

- You can download the pcap file and specify the path of each pcap file in the iptraces.txt file.

### Compile
- To compile the examples, use the following commands with make:

```
    $ make main_hitter
    $ make main_changer
```
  

### Run
- To run the examples and attain statistics on the detection accuracy and update speed, execute the following commands:

```
$ ./main_hitter
$ ./main_changer
```
