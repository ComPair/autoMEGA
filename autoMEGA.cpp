/**
    autoMEGA
    Parameterizes and parallelizes running multiple similar MEGAlib simulations.

    @author Connor Bray
*/

#include "pipeliningTools/pipeline.h"
#include "yaml-cpp/yaml.h"

using namespace std;

// Default values for all arguments. Strings cannot be atomic, but they should only be read by threads, so there shouldnt be a problem.

/// Yaml comfig file for the simulation
string settings = "config.yaml";
/// Base geometry setup file for the simulations
string geoSetup = "empty";
/// Base cosima .source file for the simulations
string cosimaSource = "empty";
/// Geomega settings file (defaults to geomega default)
string geomegaSettings = "~/.geomega.cfg";
/// Revan settings file (defaults to revan default)
string revanSettings = "~/.revan.cfg";
/// Mimrec settings file (defaults to mimrec default)
string mimrecSettings = "~/.mimrec.cfg";
/// Slack hook (if empty, slack notifications are disabled)
string hook = "";
/// Email address for notifications (if empty, email notifications are disabled)
string address = "";
/// Maximum threads to use for simulations
int maxThreads = (std::thread::hardware_concurrency()==0)?4:std::thread::hardware_concurrency(); // If it cannot detect the number of threads, default to 4
/// Command used to clean the active directory (if called by the directory checker)
string cleanCMD="rm -rf *";
/// File to which simulation settings are logged
ofstream legend;
/// Mutex to make sure only one thing is writing to legend at a time
mutex legendLock;
/// Current thread count
atomic<int> currentThreadCount(0);
/// Int to indicate test level (0=real run, otherwise it disables some exiting or notification features)
atomic<int> test(0);

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
 @brief Parse iterative nodes in list or pattern mode

 ## Parse iterative nodes

 ### Arguments
 * `YAML::NODE config` - Node to parse

 * `vector<T> &values` - Vector in wich to put all parsed values

 ### Notes
 There are two distinct parsing modes. If there are exactly three elements in the list, then it assumes it is in the format [first value, last value, step size]. If there is exactly one element, it is assumed it is a list of all values to use.
*/
template<typename T>
void parseIterativeNode(YAML::Node config, vector<T> &values){
    if(test) cout << config << endl;
    if(config.size()==3){
        for(T i=config[0].as<T>();i<config[1].as<T>();i+=config[2].as<T>()) values.push_back(i);
    } else for(size_t i=0;i<config[0].size();i++) {
        values.push_back(config[0][i].as<T>());
    }
    if(test){
        for(T t : values) cout << t << ",";
        cout << endl;
    }
}

/**
 @brief Parse vectors of parameter's options to create vectors of options's parameters

 ## Parse vectors of parameter's options to create vectors of options's parameters

 The input is a double vector where the inner list is a list of options for the ith parameter and the outer list is the list of parameters. The function creates a double vector where the inner list is a list of parameters, and the outer list is of each option.

 ### Arguments
 * `const vector<vector<T>> &values` - Vector to parse

 * `vector<vector<T>> &results` - Vector in wich to put all parsed values

*/
template<typename T>
void parseOptionsFromDoubleVector(const vector<vector<T>> &values, vector<vector<T>> &results){
    vector<T> option(values.size(),0);
    vector<size_t> positions(values.size(),0); // Initialize vectors
    int i=positions.size()-1;
    for(size_t j=0;j<option.size();j++) option[j]=values[j][positions[j]];
    results.push_back(option); // Push first option
    while(i>=0){ // Treat like odometer, the LSB counts up, rolls over, then moves to the next one. When the whole thing rolls over, then its done.
        if(positions[i]<values[i].size()){
            option[i]=values[i][positions[i]];
            positions[i]++;
            results.push_back(option);
            i=positions.size()-1;
        } else {
            positions[i] = 0;
            option[i]=values[i][positions[i]];
            i--;
        }
    }
    sort( results.begin(), results.end() ); // Remove duplicates
    results.erase( unique( results.begin(), results.end() ), results.end() );
}

/**
 @brief Run one simulation and analysis (cosima, revan, mimrec) (incomplete)

 ## Run one simulation and analysis (cosima, revan, mimrec) (incomplete)

 ### Arguments
 Incomplete

 ### Notes
 Incomplete

*/
void runSimulation(const int threadNumber, const string beamType, const vector<double> &beamOptions, const string &spectrumType, const vector<double> &spectrumOptions){
    // Create legend
    legendLock.lock();
    legend << "Run number " << threadNumber << ":" << endl;
    legend << "Beam: " << beamType << " ";
    for(size_t i=0;i<beamOptions.size();i++) legend << beamOptions[i] << " ";
    legend << endl << "Spectrum: " << spectrumType << " ";
    for(size_t i=0;i<spectrumOptions.size();i++) legend << spectrumOptions[i] << " ";
    legend << endl << endl;
    legendLock.unlock();

    // Create new cosima .source file (with run number)
        // Modify save filename
        // Parse run object and source object name
        // Replace beam and spectrum lines
    // Run cosima, log (with run number)
    // Run revan, log (with run number)
    // Run mimrec, log (with run number)

    currentThreadCount--;
    // slack("Run "+to_string(threadNumber)+" complete.", hook);
    return;
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

To compile, use `g++ autoMEGA.cpp -std=c++11 -lX11 -lXtst -pthread -ldl -ldw -g -lcurl -lyaml-cpp -Ofast -Wall -o autoMEGA`

TODO:
Implement cosima iterations
Implement goemetry iterations
Implement memory watchdog should that become an issue
Setup automated build testing and documentation building

*/
int main(int argc,char** argv){
    auto start = chrono::steady_clock::now();

    for(int i=0;i<argc;i++){ // Assign arguments to values
        if(i<argc-1){
            if(string(argv[i])=="--settings") settings = argv[++i];
            else if(string(argv[i])=="--geoSetup") geoSetup = argv[++i];
            else if(string(argv[i])=="--source") cosimaSource = argv[++i];
            else if(string(argv[i])=="--geomega-settings") geomegaSettings = argv[++i];
            else if(string(argv[i])=="--revan-settings") revanSettings = argv[++i];
            else if(string(argv[i])=="--mimrec-settings") mimrecSettings = argv[++i];
            else if(string(argv[i])=="--max-threads") maxThreads = atoi(argv[++i]);
        }
        if(string(argv[i])=="--test") test = 1;
    }

    cout << "Using " << maxThreads << " threads." << endl;

    struct stat buffer;
    if(!(stat (settings.c_str(), &buffer) == 0)){
        cerr << "File \"" << settings << "\" does not exist, but was requested. Exiting."<< endl;
        return 1;
    }

    cout << settings << endl;
    YAML::Node config = YAML::LoadFile(settings);
    if(config["geomegaSettings"]) geomegaSettings = config["geomegaSettings"].as<string>();
    if(config["revanSettings"]) revanSettings = config["revanSettings"].as<string>();
    if(config["mimrecSettings"]) mimrecSettings = config["mimrecSettings"].as<string>();
    if(config["geoSetup"]) geoSetup = config["geoSetup"].as<string>();
    if(config["cosima"]["source"]) cosimaSource = config["cosima"]["source"].as<string>();
    if(config["threads"]) maxThreads = config["threads"].as<int>();
    if(config["address"]) address = config["address"].as<string>();
    if(config["hook"]) hook = config["hook"].as<string>();

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
    // Innermost vectors are for each possibility, any outer vectors are for having multiple iterations

    vector<string> beamType;
    vector<vector<double>> beam;
    if(config["cosima"]["beam"]){
        // First parameter is a string for the type. Use another list to iterate over.
        for(size_t i=0;i<config["cosima"]["beam"][0].size();i++) beamType.push_back(config["cosima"]["beam"][0][i].as<string>());
        // The rest of the parameters are for options for the beam type. Make sure that they are valid with the cosima manual. If there are three arguments, I will assume they are in the format [initial value, final value, delta value], if there is only one, I will assume it is a nested list of values to iterate over
        for(size_t i=1;i<config["cosima"]["beam"].size();i++){
            vector<double> tempValues;
            parseIterativeNode<double>(config["cosima"]["beam"][i],tempValues);
            beam.push_back(tempValues);
        }
    }
    vector<string> spectrumType;
    vector<vector<double>> spectrum;
    if(config["cosima"]["spectrum"]){
        // First parameter is a string for the type. Use another list to iterate over.
        for(size_t i=0;i<config["cosima"]["spectrum"][0].size();i++) spectrumType.push_back(config["cosima"]["spectrum"][0][i].as<string>());
        // The rest of the parameters are for options for the spectrum
        for(size_t i=1;i<config["cosima"]["spectrum"].size();i++){
            vector<double> tempValues;
            parseIterativeNode<double>(config["cosima"]["spectrum"][i],tempValues);
            spectrum.push_back(tempValues);
        }
    }

    int totalSims = beamType.size()*spectrumType.size();
    for(size_t i=0;i<beam.size();i++) totalSims*=beam[i].size();
    for(size_t i=0;i<spectrum.size();i++) totalSims*=spectrum[i].size();
    cout << totalSims << endl;
    // TODO: Iterate over more things in cosima.

    legend.open("run.legend");
    for(size_t i=0;i<beamType.size();i++){
        for(size_t j=0;j<spectrumType.size();j++){
            vector<vector<double>> beamOptions;
            parseOptionsFromDoubleVector(beam,beamOptions);
            for(size_t k=0;k<beamOptions.size();k++){
                vector<vector<double>> spectrumOptions;
                parseOptionsFromDoubleVector(spectrum,spectrumOptions);
                for(size_t l=0;l<spectrumOptions.size();l++){
                    while(currentThreadCount>maxThreads)sleep(0.1);
                    threadpool.push_back(thread(runSimulation,threadpool.size(),beamType[i], beamOptions[k], spectrumType[j], spectrumOptions[l]));
                    currentThreadCount++;
                }
            }
        }
    }
    for(size_t i=0;i<threadpool.size();i++)threadpool[i].join();


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
        if(!hook.empty()) slack("Simulation complete",hook);
        if(!address.empty() email(address,"Simulation Complete");
    }
    return 0;
}
