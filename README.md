# autoMEGA - Release Mirror

> This is the latest release of autoMEGA. To submit feature requests, bug reports, or to view the development tree, go to [gitlab.com/cbray/autoMEGA](https://gitlab.com/cbray/autoMEGA)

Tool for automating MEGAlib simulations. The goal is to create a user-friendly method for running iterative simulations.

Features:
- Automatically generate modified Geomega (`*.geo.setup`) and Cosima (`*.source`) files
  - Flexible parameters
  - Unlimited configurations
- Capable of running dozens of simultaneous simulations
  - Ideal for optimizing instrument dimensions, generating plots of continuum sensitivity, or calculating Aeff
- User-Friendly
  - Convenient YAML configuration
  - Capable of notifying the user of simulation completion (or errors)
    - Email
    - Slack
- ~~Causes robot uprising~~

## Notes

The library is currently in a early release state. Bug reports and feature requests welcome!

### Dependencies:
- MEGAlib (Tested on v2.34)
- yaml-cpp (0.5 or newer)
- g++ with C++11 (Tested on 5.4.1, 7.3.0, and 8.1.1)
   - clang++ may replace g++, but may require modifications to the Makefile (tested on clang++ 6.0.1)
- sendmail (optional, required only for email functionality)
- curl (optional, required only for slack functionality)
- backward-cpp and libdw-dev (optional, required only for debug functionality. backward-cpp is automatically fetched during `make debug`)

### To compile:

```
make
```

Or, manually:
```
g++ checkGeometry.cpp -o checkGeometry -std=c++11 -pthread -lyaml-cpp -O2 -Wall $(root-config --cflags --glibs) -I$MEGALIB/include -L$MEGALIB/lib -lGeomegaGui -lGeomega -lCommonGui -lCommonMisc
g++ autoMEGA.cpp -o autoMEGA -std=c++11 -pthread -lyaml-cpp -O2 -Wall
```

Go to [Gitlab pages](https://cbray.gitlab.io/autoMEGA/autoMEGA_8cpp.html) for full documentation.

[![pipeline status](https://gitlab.com/cbray/autoMEGA/badges/master/pipeline.svg)](https://gitlab.com/cbray/autoMEGA/pipelines)
