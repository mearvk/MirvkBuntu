# SPDX-License-Identifier: GPL-2.0
#
# Reference copy of the TAC3 build rules. The authoritative build-wired Makefile
# lives in the kernel tree at kernels/linux-5.15.204/linux-5.15.204/fs/tac3/.

# --- Kernel module (kbuild) ------------------------------------------------
# Builds the mountable TAC3 filesystem from tac3.c under kbuild.
obj-$(CONFIG_TAC3)	+= tac3.o

# --- Userspace C++ port (portable engine) ---------------------------------
# tac3.cpp/tac3.hpp are a standalone C++ equivalent of the wear/pressure/health
# engine (Tables 1/2/3), for simulation/testing/tooling. Build with a host
# compiler:  make tac3-cpp
ifndef KERNELRELEASE
CXX      ?= c++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic

.PHONY: tac3-cpp cpp-clean
tac3-cpp: tac3_cpp.o

tac3_cpp.o: tac3.cpp tac3.hpp
	$(CXX) $(CXXFLAGS) -c tac3.cpp -o tac3_cpp.o

cpp-clean:
	rm -f tac3_cpp.o
endif
