# autoMEGA

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
- YAML-cpp (0.5 or newer)
- g++ (Tested on 5.4.1, 7.3.0, and 8.1.1)
- sendmail (optional, required only for email functionality)
- curl (optional, required only for slack functionality)
- backward-cpp and libdw-dev (optional, required only for debug functionality)

### To compile:

```
make
```

Or, manually:
```
git submodule update --init --recursive --remote
# Follow instructions to precompile pipeliningTools
g++ checkGeometry.cpp -std=c++11 -lX11 -lXtst -pthread -ldl -ldw -lyaml-cpp -g -lcurl -Ofast -Wall -o checkGeometry $(root-config --cflags --glibs) -I$MEGALIB/include -L$MEGALIB/lib -lGeomegaGui -lGeomega -lCommonGui -lCommonMisc
g++ autoMEGA.cpp -std=c++11 -lX11 -lXtst -pthread -ldl -ldw -lyaml-cpp -g -lcurl -Ofast -Wall -o autoMEGA
```

Go to [Gitlab pages](https://cbray.gitlab.io/autoMEGA/autoMEGA_8cpp.html) for full documentation.

[![pipeline status](https://gitlab.com/cbray/autoMEGA/badges/master/pipeline.svg)](https://gitlab.com/cbray/autoMEGA/pipelines)
