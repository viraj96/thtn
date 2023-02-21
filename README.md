# Timeline-Based Multi-Robot Planner

This repository contains the implementation of T-HTN a Timeline-based multi-robot planner. Please refer to the [paper](https://icaps22.icaps-conference.org/workshops/HPlan/papers/paper-09.pdf) for more details about the problem formulation, new innovations and planner details.

## Prerequisites

* A compiler with C++17 support
* CMake 3.16 or higher
* Bison 3.8.2 or higher
* Flex 2.6.4 or higher
* Boost 1.71 or higher
* Plog 1.1.9 or higher

## Installation

To build the project follow the commands as shown below which will install the planner binary in the build directory,

```sh
git clone https://github.com/viraj96/thtn.git
cd thtn
mkdir build; cd build
cmake ..
make
```

## Usage

Invoke the planner using the following template,

```sh
./planner <path-to-domain-file> <path-to-problem-file>
```

Note that you need to use the update HDDL syntax when defining your own domain and problem files for this planner. The details of the updated syntax can also be found in the [paper](https://icaps22.icaps-conference.org/workshops/HPlan/papers/paper-09.pdf). 

* Tested on Ubuntu 20.04 and MacOS Ventura 13.1 with Apple M1 Chip

## Reference

If you find this work useful, then please cite the following paper!

```
@InProceedings{HPlan2022paper-09,
  author           = {Viraj Parimi and Zachary B. Rubinstein and Stephen F. Smith},
  title            = {T-HTN: Timeline Based HTN Planning for Multiple Robots},
  booktitle        = {Proceedings of the 5th ICAPS Workshop on Hierarchical Planning (HPlan 2022)},
  year             = {2022},
  pages            = {59--67},
  url_paper        = {https://icaps22.icaps-conference.org/workshops/HPlan/papers/paper-09.pdf},
  url_presentation = {https://youtu.be/eGNyj5lOrXY}
}
```
