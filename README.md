# aesd-assignments
This repo contains public starter source code, scripts, and documentation for Advanced Embedded Software Development (ECEN-5713) and Advanced Embedded Linux Development assignments University of Colorado, Boulder.

## Assignment 5 Part 1:
	1. creating necessary directories
	2. Makefile (Ref: )
	3. asedsocket.c
		a. signal handler using sigaction (Ctrl+C failed using signal())
		b. can we have a function inside signal handler for the success check or is it something not allowed as in interrupt handler?
			ref: https://github.com/cu-ecen-aeld/aesd-lectures/blob/master/lecture9/signal_handler.c
		c. Socket communication
		d. realloc()	: realloc(old_pointer, new_size)
						: if enough space exists, extends the same block pointed by the old_pointer
						: else allocate new bigger block, copy old data, free old block, return new pointer
		e. malloc(filesize) can't be used as we can't assume that filesize fits in RAM
		d. Error:==12356== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
			deleting marker file /tmp/tmp.BMmZlC2tZc
			~/AESD/assignment-1-Jaju8756/assignment-autotest/test/assignment5 ~/AESD/assignment-1-Jaju8756/assignment-autotest/test/assignment5 ~/AESD/assignment-1-Jaju8756/server ~/AESD/assignment-1-Jaju8756/assignment-autotest/test/assignment5 ~/AESD/assignment-1-Jaju8756
			Testing target localhost on port 9000
			sending string abcdefg to localhost on port 9000
			Differences found after sending abcdefg to localhost on port 9000
			Expected contents to match:
			abcdefg
			But found contents:
			With differences
			--- /tmp/tmp.tqUpkoGGI5	2026-02-14 15:24:00.200547552 -0700
			+++ /tmp/tmp.ACcUXLk11F	2026-02-14 15:24:00.195548547 -0700
			@@ -1 +0,0 @@
			-abcdefg
			Test complete with failure
			Validation Error:sockettest.sh returned 1 attempting to run against native compiled aesdsocket instance running under valgrind
		

## Assignment 5 Part 2:
	a. add: aesdsocket-start-stop
	b. Modify aesd-assignments.mk
		full commit hash: git rev-parse HEAD
	c. Forward host port 9000 in runqemu.sh

## Assignment 6 Part 1:
	a. add pthread, struct thread_data, mutex
	b. modififed handle_client 
	c. created handle_event threads inside allocate
	d. mutex implementation 
	e. timer and timestamp prints	
		
	
## Setting Up Git

Use the instructions at [Setup Git](https://help.github.com/en/articles/set-up-git) to perform initial git setup steps. For AESD you will want to perform these steps inside your Linux host virtual or physical machine, since this is where you will be doing your development work.

## Setting up SSH keys

See instructions in [Setting-up-SSH-Access-To-your-Repo](https://github.com/cu-ecen-aeld/aesd-assignments/wiki/Setting-up-SSH-Access-To-your-Repo) for details.

## Specific Assignment Instructions

Some assignments require further setup to pull in example code or make other changes to your repository before starting.  In this case, see the github classroom assignment start instructions linked from the assignment document for details about how to use this repository.

## Testing

The basis of the automated test implementation for this repository comes from [https://github.com/cu-ecen-aeld/assignment-autotest/](https://github.com/cu-ecen-aeld/assignment-autotest/)

The assignment-autotest directory contains scripts useful for automated testing  Use
```
git submodule update --init --recursive
```
to synchronize after cloning and before starting each assignment, as discussed in the assignment instructions.

As a part of the assignment instructions, you will setup your assignment repo to perform automated testing using github actions.  See [this page](https://github.com/cu-ecen-aeld/aesd-assignments/wiki/Setting-up-Github-Actions) for details.

Note that the unit tests will fail on this repository, since assignments are not yet implemented.  That's your job :) 
