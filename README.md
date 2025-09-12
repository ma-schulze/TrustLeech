# TrustLeech: Privileged System Analysis using Nested Virtualization

This repository contains the code and instructions for the ACSAC 2025 Artifact Evaluation.

## Build
The repository contains a toolchain to build all artifacts of the paper.   
All code is provided in the `./artifact/` folder.   
The `README.md` inside of this folder provides an overview of the respective artifacts.

The code is built using a buildroot toolchain, which outputs images to be used with QEMU to then run the resulting system.   
To ease development, a docker environment is also provided, which provides all dependencies for buildroot. 

The `./install.sh` script uses this by executing the following steps:
1. Build the Docker image 
2. Connect to the image and open a shell
3. Build the toolchain inside the container 

After this, the shell is kept open in case parts need to be rebuilt.
This should, however, not be neccessary, so the container may be quit now.

## Running 
In general, the resulting images are all run using the provided QEMU version.
More information is given in the respective `claims`.

For many testcases, you need multiple terminal sessions.
Therefore, if the artifact is accessed via CloudLab, we advice you opening a `tmux` session or similar.

Also, you can quit a QEMU instance using `ctr+a,x`. 


## Misc
The buildroot environment uses `ccache`. The cache is put in the top directory under `src`.
However, the actual source code is in placed in `./artifact`.
