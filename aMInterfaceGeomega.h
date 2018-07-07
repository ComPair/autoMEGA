#include "pipeliningTools/pipeline.h"

using namespace std;
#include <TROOT.h>
#include <TApplication.h>
#include <TEnv.h>
#include <MString.h>
#include <TSystem.h>
#include "MInterfaceGeomega.h"

class aMInterfaceGeomega : public MInterfaceGeomega {
public:
    bool SetGeometry(MString FileName, bool UpdateGui = true) {
        return m_Data->SetCurrentFileName(FileName);
    }
    bool TestIntersections(std::string outputFile){
        if(!ReadGeometry()) return 1;

        bool status = m_Geometry->CheckOverlaps();
        cerr << "Root status: " << status << endl;
        //if(!status) return 1;

        if(!MFile::Exists(g_MEGAlibPath + "/bin/cosima")) return 0;

        MString FileName = gSystem->TempDirectory();
        FileName += "/DelMe.source";

        ofstream out;
        out.open(FileName);
        if (!out.is_open()) return 0;

        out<<"Version 1\nGeometry "<<m_Data->GetCurrentFileName()<<"\nCheckForOverlaps 1000 0.0001\nPhysicsListEM Standard\nRun Minimum\nMinimum.FileName DelMe\nMinimum.NEvents 1\nMinimum.Source MinimumS\nMinimumS.ParticleType 1\nMinimumS.Position 1 1 0 0 \nMinimumS.SpectralType 1\nMinimumS.Energy 10\nMinimumS.Intensity 1\n";
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
