# ptest-runner2

The ptest-runner2 is C written program for run ptests of Yocto/Openembedded, is fully compatible with previous 
version of ptest-runner [1] that was a shell script that runs ptests sequencially.

## What's new in ptest-runner2?

The mainly reason for the new ptest-runner in C is because is designed for run in tiny systems also the old shell 
script version have problems when certain ptest hang it becomes blocked indefinitely.

Now the ptest-runner2 support the next features:

- Specify the directory for search ptests.
- List available ptests.
- Specify the timeout for avoid blocking indefinetly.
- Only run certain ptests.
- XML-ouput

Proposed features:

- Adds support for per ptest output file.
- Adds support for run ptests in parallel (review possible colisions in ptests).

## How to compile?

```
$ git clone git://git.yoctoproject.org/ptest-runner2
$ cd ptest-runner2
$ make
```

## How to run testsuite?

For run the test suite you need to install check unittest framework [2],

```
$ make check
```

Also if you want to run memory leaks tests you need mcheck available in glibc,

```
$ MEMCHECK=1 make
$ export MALLOC_TRACE=/tmp/t
$ ./ptest-runner -d tests/data -t 1
$ mtrace ./ptest-runner $(MALLOC_TRACE)
```

## Contributions

For contribute please send a patch with subject prefix "[ptest-runner]" to 
yocto@yoctoproject.org and cc the current maintainer that is Aníbal Limón 
<anibal.limon@linux.intel.com>.

## Links

[1] http://git.yoctoproject.org/cgit/cgit.cgi/poky/tree/meta/recipes-support/ptest-runner/files/ptest-runner

[2] http://libcheck.github.io/check/
