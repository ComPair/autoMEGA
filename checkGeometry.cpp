#include <iostream>
#include <fstream>
#include <iomanip>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <regex>
#include <thread>
#include <ctime>
#include <chrono>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>
#include <termios.h>
#include <sys/statvfs.h>
#include <glob.h>

using namespace std;
#include <TROOT.h>
#include <TApplication.h>
#include <TEnv.h>
#include <MString.h>
#include <TSystem.h>
#include "MInterfaceGeomega.h"


/**

 @brief Gets file of given file

 ## Get file size of given file. Credit to Matt on StackOverflow.

 ### Arguments
 - `std::string filename` - Filename to get size of

*/
long getFileSize(std::string filename){
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}


/**
 @brief Interface to geomega to run geometry checks

 ## Interface to geomega to run geometry checks

 ### Purpose
 Extends MInterfaceGeomega in order to allow autoMEGA to directly check the geometry for overlaps without a bash call or GUI.
*/
class aMInterfaceGeomega : public MInterfaceGeomega {
public:
    aMInterfaceGeomega() : MInterfaceGeomega() {}

    /**
 @brief Set geometry filename
    */
    bool SetGeometry(MString FileName, bool UpdateGui = true) {
        return m_Data->SetCurrentFileName(FileName);
    }

    /**
 @brief Check geometry for overlaps

 ## Check geometry for overlaps

 ### Arguments
 - `std::string outputFile` - Temp file to write cosima warnings to

 ### Notes
 Returns 1 if there is an overlap, returns 0 otherwise. If cosima cannot be found or files cannot be created for a test, then that test may be skipped.
    */
    bool TestIntersections(std::string outputFile){
        if(!ReadGeometry()) return 1;

        bool status = m_Geometry->CheckOverlaps();
        cerr << "Root status: " << status << endl;
        if(!status) return 1;

        if(!MFile::Exists(g_MEGAlibPath + "/bin/cosima")) return 0;

        MString FileName = gSystem->TempDirectory();
        FileName += "/DelMe.source";

        ofstream out;
        out.open(FileName);
        if (!out.is_open()) return 0;

        out<<"Version 1\nGeometry "<<m_Data->GetCurrentFileName()<<"\nCheckForOverlaps 10000 0.0001\nPhysicsListEM Standard\nRun Minimum\nMinimum.FileName DelMe\nMinimum.NEvents 1\nMinimum.Source MinimumS\nMinimumS.ParticleType 1\nMinimumS.Position 1 1 0 0 \nMinimumS.SpectralType 1\nMinimumS.Energy 10\nMinimumS.Intensity 1\n";
        out.close();

        MString WorkingDirectory = gSystem->WorkingDirectory();
        gSystem->ChangeDirectory(gSystem->TempDirectory());
        gSystem->Exec(MString("bash -c \"source ${MEGALIB}/bin/source-megalib.sh; cosima ") + FileName + MString(" 2>&1 | grep 'issued by : G4PVPlacement::CheckOverlaps()' &> "+outputFile+"\""));
        gSystem->Exec(MString("rm -f DelMe.*.sim ") + FileName);
        long int size = getFileSize(outputFile);
        gSystem->ChangeDirectory(WorkingDirectory);
        cerr << "Cosima size: " << size << endl;
        return size!=0;
    }

};

/**
@brief External cpp file to check geomega geometries without linking libraries or opening a GUI

## External cpp file to check geomega geometries without linking libraries or opening a GUI

### Arguments
All arguments are parsed as filenames to be checked

### Notes:
Returns the total number of invalid geometries

### To build:
```
git submodule update --init --recursive --remote
# Follow instructions to precompile pipeliningTools
g++ checkGeometry.cpp -std=c++11 -lX11 -lXtst -pthread -ldl -ldw -lyaml-cpp -g -lcurl -Ofast -Wall -o checkGeometry $(root-config --cflags --libs) -I$MEGALIB/include -L$MEGALIB/lib -lGeomegaGui -lGeomega -lCommonGui -lCommonMisc
```

*/
int main(int argc,char** argv){
    int overall = 0;

    // Attempt to mostly quiet the tests
    cout.setstate(ios_base::failbit);
    cerr.setstate(ios_base::failbit);
    gROOT->SetBatch(true);
    mout.setstate(std::ios_base::failbit);
    gErrorIgnoreLevel = kFatal;

    // Check each geometry, return number of failures
    for(int i=1;i<argc;i++){
        aMInterfaceGeomega geomega;
        geomega.SetGeometry(argv[i]);
        bool good = geomega.TestIntersections(string(argv[i])+".out");
        overall +=good;
    }


    return overall;
}
