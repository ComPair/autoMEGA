# autoMEGA

In-development library for automating MEGAlib simulations. The eventual goal is to run iterative simulations based on a set of parameters. These will likely inlcude running simulations of slightly modified geometries or sources and plotting the changes in output values such as A_eff or the PSF.

## Notes

The library is currently incomplete and completely untested.

### Dependencies:
- YAML-cpp
- pipeliningTools
- C++17 (filesystem) - which means you have to use a g++ version from this year

### To compile:

```
git submodule update --init --recursive --remote
# Follow instructions to precompile pipeliningTools
g++ autoMEGA.cpp -std=c++17 -lX11 -lXtst -pthread -ldl -ldw -lyaml-cpp -g -lcurl -Ofast -Wall -lstdc++fs -o autoMEGA
```

Go to [Gitlab pages](https://cbray.gitlab.io/autoMEGA/autoMEGA_8cpp.html) for full documentation.

[![pipeline status](https://gitlab.com/cbray/autoMEGA/badges/master/pipeline.svg)](https://gitlab.com/cbray/autoMEGA/pipelines)
