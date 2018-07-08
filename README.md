# autoMEGA

Tool for automating MEGAlib simulations. The eventual goal is to run iterative simulations based on a set of parameters. These will likely inlcude running simulations of slightly modified geometries or sources and plotting the changes in output values such as A_eff or the PSF.

## Notes

The library is currently in a QA state before an initial release.

### Dependencies:
- MEGAlib
- Sh shell links to bash (ubuntu default is dash, to fix use: `sudo ln -sf bash /bin/sh`)
- YAML-cpp
- pipeliningTools

### To compile:

```
git submodule update --init --recursive --remote
# Follow instructions to precompile pipeliningTools
g++ autoMEGA.cpp -std=c++11 -lX11 -lXtst -pthread -ldl -ldw -lyaml-cpp -g -lcurl -Ofast -Wall -o autoMEGA $(root-config --cflags --glibs) -I$MEGALIB/include -L$MEGALIB/lib -lGeomegaGui -lGeomega -lCommonGui -lCommonMisc -D_GLIBCXX_USE_CXX11_ABI=1
```

Go to [Gitlab pages](https://cbray.gitlab.io/autoMEGA/autoMEGA_8cpp.html) for full documentation.

[![pipeline status](https://gitlab.com/cbray/autoMEGA/badges/master/pipeline.svg)](https://gitlab.com/cbray/autoMEGA/pipelines)
