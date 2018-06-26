# autoMEGA

In-development library for automating MEGAlib simulations. The eventual goal is to run iterative simulations based on a set of parameters. These will likely inlcude
running simulations of slightly modified geometries or sources and plotting the changes in output values such as A_eff or the PSF.

## Notes

The library is currently incomplete and completely untested.

You will need to recursively initiate git modules to compile.

Depends on yaml-cpp.

The recommended compilation mehtod: `g++ autoMEGA.cpp -std=c++11 -lX11 -lXtst -pthread -ldl -ldw -lyaml-cpp -g -lcurl -Ofast -Wall -o autoMEGA`

Go to [Gitlab pages](https://cbray.gitlab.io/autoMEGA/autoMEGA_8cpp.html) for full documentation.

![coverage](https://gitlab.com/cbray/autoMEGA/badges/master/build.svg)
