# lsd v1.2 - low-level data copying utility 

### Installation
```
$ git clone https://github.com/Ad4ndi/lsd
$ cd lsd
$ clang main.c -o lsd
```

### Usage: lsd [options]
```
in=FILE          input file
out=FILE         output file
size=SIZE        block size
count=COUNT      number of blocks to copy
skip=SKIP        skip blocks at start
seek=SEEK        skip blocks at start of output
cbs=SIZE         conversion block size
conv=CONV        conversion types (lcase, ucase, swab, sync, block, unblock)
```

