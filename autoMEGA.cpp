/**
    autoMEGA
    Parameterizes and parallelizes running multiple similar MEGAlib simulations.

    @author Connor Bray
*/

#include "pipeliningTools/pipeline.h"
#include "yaml-cpp/yaml.h"

using namespace std;

string hook,address;

/**
 @brief Clean the current directory

 ## Clean the current directory

 ### Arguments:
 * `bool autoClean` - Bool of whether or not to automatically clean the directory. It defualts to zero, which will prompt the user first, but if set to one the program cleans the directory without outside input.

 * `string cleanCMD` - Sh (bourne shell) command used to clean directory. Defaults to `rm -f *.root`.

 ### Notes:
 Ensures there are no files matching g4out*.root in the current directory. If there are, it offers to automatically remove them, or to exit the program to allow the user to save and remove them manually.
 Also note that this assumes that all output files end in `.root`, and no other files end in `.root`. If you want to preserve these files, you should move them to a differnt directory or change their filename (as an example, `g4out.root`->`g4out.root.keep`).
*/
bool clean(bool autoClean, string cleanCMD="rm -rf *"){
    if(autoClean){
        bash(cleanCMD.c_str()); // Clean directory
        return 1;
    }
    int i, ret = system(("echo \"There may be conflicting files in your current directory. If there are none, then press s then enter to skip cleaning and continue execution. If you wish to exit the program, press enter to exit. Otherwise, press c then enter to clean up.\" \n read cleanup \n if [ \"$cleanup\" == \"c\" ]; then \n "+cleanCMD+" && exit 1 \n fi \n if [ \"$cleanup\" == \"s\" ]; then \n exit 1 \n fi \nexit 0 \n").c_str()); // Ask user if they want to run the clean command, then run it if necessary
    i=WEXITSTATUS(ret); // Get return value: 1 if clean, 0 if not
    return i;
}


/**
## autoMEGA

### Arguments (also configurable from config yaml file):

* `--settings` - Settings file - defaults to "config.yaml"

* `--geoSetup` - Base geometry file to use. Requried.

* `--source` - Base cosima source to use. Required.

* `--geomega-settings` - Geomega config file. Defaults to the system default ("~/.geomega.cfg").

* `--revan-settings` - Revan config file. Defaults to the system default ("~/.revan.cfg").

* `--mimrec-settings` - Mimrec config file. Defaults to the system default ("~/.mimrec.cfg").

* `--max-threads` - Max threads. Defaults to system maximum (or 4 if the system max is undetectable).

### Notes:

To redirect stdout and stderr to a file and still view on command line, use `[autoMEGA command & arguments] 2>&1 | tee file.txt`, where `[autoMEGA command & arguments]` is the command and arguments, and `file.txt` is the desired output file.

To compile, use `g++ -std=c++11 -lX11 -lXtst -pthread -ldl -ldw -g -lcurl -lyaml-cpp -Ofast -Wall autoMEGA.cpp -o autoMEGA`

TODO:
Implement cosima iterations
Implement goemetry iterations
Implement memory watchdog should that become an issue
Setup automated build testing and documentation building

*/
int main(int argc,char** argv){
    auto start = chrono::steady_clock::now();
    string settings = "config.yaml";// Default values for all arguments
    string geoSetup = "empty";
    string cosimaSource = "empty";
    string geomegaSettings = "~/.geomega.cfg";
    string revanSettings = "~/.revan.cfg";
    string mimrecSettings = "~/.mimrec.cfg";
    int maxThreads = (std::thread::hardware_concurrency()==0)?4:std::thread::hardware_concurrency(); // If it cannot detect the number of threads, default to 4
    cout << "Using " << maxThreads << " threads." << endl;
    string cleanCMD="rm -rf *";
    int test = 0;

    for(int i=0;i<argc;i++){ // Assign arguments to values
        if(i<argc-1){
            if(string(argv[i])=="--settings") settings = argv[++i];
            if(string(argv[i])=="--geoSetup") geoSetup = argv[++i];
            if(string(argv[i])=="--source") cosimaSource = argv[++i];
            if(string(argv[i])=="--geomega-settings") geomegaSettings = argv[++i];
            if(string(argv[i])=="--revan-settings") revanSettings = argv[++i];
            if(string(argv[i])=="--mimrec-settings") mimrecSettings = argv[++i];
            if(string(argv[i])=="--max-threads") maxThreads = atoi(argv[++i]);
            if(string(argv[i])=="--test") test = 1;
        }
    }

    struct stat buffer;
    if(!(stat (settings.c_str(), &buffer) == 0)){
        cerr << "File \"" << settings << "\" does not exist, but was requested. Exiting."<< endl;
        return 1;
    }

    YAML::Node config = YAML::LoadFile(settings);
    if(config["geomegaSettings"]) geomegaSettings = config["geomegaSettings"].as<string>();
    if(config["revanSettings"]) revanSettings = config["revanSettings"].as<string>();
    if(config["mimrecSettings"]) mimrecSettings = config["mimrecSettings"].as<string>();
    if(config["geoSetup"]) geoSetup = config["geoSetup"].as<string>();
    if(config["cosimaSource"]) cosimaSource = config["cosimaSource"].as<string>();
    if(config["threads"]) maxThreads = config["threads"].as<int>();
    if(config["address"]) address = config["address"].as<string>();
    else{
        address = "example@example.com";
        cerr << "Could not find address, assuming dummy address." << endl;
    }
    if(config["hook"]) hook = config["hook"].as<string>();
    else{
        hook = "https://example.com";
        cerr << "Could not find hook, assuming dummy address." << endl;
    }

    if(geoSetup=="empty") {cout << "Geometry setup required. Exiting." << endl;return 1;}
    if(cosimaSource=="empty") {cout << "Cosima source required. Exiting." << endl;return 1;}

    vector<string> files = {settings,geoSetup,cosimaSource,geomegaSettings,revanSettings,mimrecSettings};
    for(auto& s : files){
        struct stat buffer;
        if(!(stat (s.c_str(), &buffer) == 0)){
            cerr << "File \"" << s << "\" does not exist, but was requested. "<< endl;
            if(!test){
                cerr << "Exiting." << endl;
                return 1;
            }
        }
    }

    // Check directory
    if(!test && directoryEmpty(".")) return 3; // Make sure its empty

    // Create threadpool
    vector<thread> threadpool;
    int currentThreadCount = 0;

    // TODO: Parse geomega section of config, check each geometry, and multithread

    // Geomega section
    bash("geomega -f "+geoSetup+" --check-geometry | tee geomega.run0.out");
    ifstream overlapCheck("geomega.run0.out");
    bool check0=0,check1=0;
    if(overlapCheck.is_open()) for(string line;getline(overlapCheck,line);){
        if(line=="No extrusions and overlaps detected with ROOT (ROOT claims to be able to detect 95% of them)"){
            check0=1;
        }
        if(line=="-------- Cosima output start --------"){
            getline(overlapCheck,line);
            if(line=="-------- Cosima output stop ---------"){
                check1=1;
            }
        }
    }
    if(!(check0&&check1)){
        cerr << "Geometry error." << endl;
        if(!test){
            cerr << "Exiting." << endl;
            return 2;
        }
    }
    // End geomega section

    // Generate list of simulations
    // vector<string>
    // if(config["cosimaBeam"]){

    // }

    // (need to figure out what order I will do stuff in, but multithread this with maxthreads)
    // For each geometry, run all of the cosima things. (requires temp cosima sources, generating seeds, renaming outputs)
        // Generate seed
        // Modify filename
        // Parse and modify beam
    // For each of the resulting .sim files, run revan
    // For each .tra file, run mimrec things on them according to config
    // Gather data from each simulation output and use it to make a plot of something

    // End timer, print command duration
    auto end = chrono::steady_clock::now();
    cout << endl << "Total simulation and analysis elapsed time: " << beautify_duration(chrono::duration_cast<chrono::seconds>(end-start)) << endl;
    if(!test){
        slack("Simulation complete",hook);
        email(address,"Simulation Complete");
    }
    return 0;
}
