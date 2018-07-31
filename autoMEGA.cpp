/**
    autoMEGA
    Parameterizes and parallelizes running multiple similar MEGAlib simulations.

    @author Connor Bray
*/

#include "pipeliningTools/pipeline.h"
#include "yaml-cpp/yaml.h"
#include <regex>
#include <libgen.h>
#include <termios.h>
#include <unistd.h>

using namespace std;

// Default values for all arguments. Strings cannot be atomic, but they should only be read by threads, so there shouldnt be a problem.

/// Yaml config file for the simulation
string settings = "config.yaml";
/// Revan settings file (defaults to revan default)
string revanSettings = "~/.revan.cfg";
/// Slack hook (if empty, slack notifications are disabled)
string token = "";
/// Slack channel
string channel = "";
/// Email address for notifications (if empty, email notifications are disabled)
string address = "";
/// Maximum threads to use for simulations (defaults to system thread count)
int maxThreads = (std::thread::hardware_concurrency()==0)?4:std::thread::hardware_concurrency(); // If it cannot detect the number of threads, default to 4
/// Legend file
ofstream legend;
/// Mutex to make sure only one thing is writing to legend at a time
mutex legendLock;
/// Current thread count
atomic<int> currentThreadCount(0);
/// Int to indicate test level (0=real run, otherwise it disables some exiting or notification features)
atomic<int> test(0);
/// Bool to indicate what files to keep (false = keep no intermediary files, true = keep all)
atomic<bool> keepAll(false);
/// Int to indicate slack verbosity level. Level 3 prints all messages, level 2 prints fewer messages, level one prints only error messages, and level zero only prints final messages. Defaults to zero
atomic<int> slackVerbosity(0);
/// Int to indicate cosima verbosity level. Defaults to zero
atomic<int> cosimaVerbosity(0);
/// Array to store current state for status bar
atomic<int> statusBar[9];
/// Running average of simulation length
chrono::seconds averageTime(0);
/// semaphore for average time
mutex timeLock;

/**
 @brief Print simulation status bar

 ## Print simulation status bar

 ### Notes
 Constructs status and adds a spinner to indicate that the simulation is still alive. Prints the same message to slack.

 Also prints it to slack, and updates it
*/
void handleStatus(){
    char spinner[4] = {'-','\\','|','/'};
    unsigned int i=0;
    string ts = "";
    if(!token.empty() && !channel.empty()) ts=slackBotPost(token,channel,">J.A.R.V.I.S., are you up?\nFor you sir, always.");
    while(!exitFlag){
        stringstream currentStatus;
        if(statusBar[0]) currentStatus << std::setprecision(3) << "Geomega: " << ((double) statusBar[1]*100)/statusBar[2] << "% ["+to_string(statusBar[1])+"/"+to_string(statusBar[2])+"] | ";
        if(statusBar[3]) currentStatus << std::setprecision(3) << "Cosima: " << ((double) statusBar[4]*100)/statusBar[5] << "% ["+to_string(statusBar[4])+"/"+to_string(statusBar[5])+"] | ";
        if(statusBar[6]) currentStatus << std::setprecision(3) << "Revan: " << ((double) statusBar[7]*100)/statusBar[8] << "% ["+to_string(statusBar[7])+"/"+to_string(statusBar[8])+"] | ";
        if(averageTime.count()!=0) currentStatus << "Running average time: " + beautify_duration(averageTime) + " | ";
        cout << "\r" << currentStatus.str() << spinner[i++%4]<< flush;
        if(i%3==0 && !token.empty() && !channel.empty()) slackBotUpdate(token,channel,ts,currentStatus.str());
        usleep(400000);
    }
}

/**
 @brief Quick alias for slack notifications
*/
void quickSlack(string message){
    if(!token.empty() && !channel.empty()) slackBotPost(token,channel,message);
}

/**
 @brief Parse iterative nodes in list or pattern mode

 ## Parse iterative nodes

 ### Arguments
 * `YAML::NODE contents` - Node to parse

 ### Notes
 There are two distinct parsing modes. If there are exactly three elements in the list, then it assumes it is in the format [first value, last value, step size]. If there is exactly one element, it is assumed it is a list of all values to use.

 Values are assumed as doubles if they are in three element format, otherwise they are assumed as strings.
*/
vector<string> parseIterativeNode(YAML::Node contents, std::string prepend=""){
    vector<string> options; options.push_back(prepend);
    vector<string> newOptions;
    for(size_t i=0;i<contents.size();i++){
        // Parse options into vector of strings
        vector<string> parameters;
        if(contents[i].size()==3){
            double final = contents[i][1].as<double>();
            double step = contents[i][2].as<double>();
            for(double initial = contents[i][0].as<double>();initial<final;initial+=step) parameters.push_back(to_string(initial));
        } else for(size_t j=0;j<contents[i][0].size();j++) parameters.push_back(contents[i][0][j].as<string>());
        for(size_t j=0;j<options.size();j++){
            for(size_t k=0;k<parameters.size();k++)
                newOptions.push_back(options[j]+" "+parameters[k]);
        }
        options.clear();
        options = std::move(newOptions);
    }
    return options;
}


/**
 @brief Outputs (to file) input file with all included files fully evaluated

 ## Output (to to file) input file with all included files fully evaluated

 ### Arguments
 - `string inputFile` - Input filename
 - `ofstream& out` - Ofstream output object
*/
int geoMerge(string inputFile, ofstream& out, int recursionDepth=0){
    if(recursionDepth>1024){
        cerr << "Exceeded max recursion depth of 1024. This is likely due to a circular dependency. If not, then your geometry is way to complex. Exiting." << endl;
        if(slackVerbosity>=1) quickSlack("GEOMERGE: Exceeded max recursion depth of 1024. This is likely due to a circular dependency. If not, then your geometry is way to complex. Exiting.");
        return -1;
    }
    if(recursionDepth==0) out << "///Include " << inputFile << "\n"; // Note initial file

    // Open file
    ifstream input(inputFile);
    if(!input.is_open() || !input.good()){
        cerr << "Could not open included file \"" << inputFile << "\"." << endl;
        if(slackVerbosity>=1) quickSlack("GEOMERGE: Could not open included file \"" + inputFile + "\".");
        return 1;
    }

    // Loop over file
    for(string line;getline(input,line);){
        stringstream ss(line);
        string command; ss >> command;

        //  Include other files
        if(command=="Include"){
            out << "///" << line << "\n";
            string includedFile; ss >> includedFile;
            string baseFile = includedFile;
            if(includedFile[0]!='/') includedFile=inputFile.substr(0,inputFile.find_last_of('/'))+"/"+includedFile; // Workaround for relative file references
            if(geoMerge(includedFile,out,recursionDepth+1)) return 1;
            out << "///End " << baseFile << "\n";
        }else{
            out << line << "\n";
        }
    }

    if(recursionDepth==0) out << "///End " << inputFile << "\n"; // Note final file
    return 0;
}

/**

 @brief Check geometry file using checkGeometry

 ## Check geometry file using checkGeometry

 ### Arguments
 - `string& filename` - Geometry file to test
 - `string path` - Path to folder containing checkGeometry

 ### Notes
 filename will be empty after the test if it is invalid
*/
void testGeometry(string& filename, string path){
    int status, ret=system((path+"/checkGeometry "+filename+" > /dev/null 2> /dev/null").c_str());
    status=WEXITSTATUS(ret); // Get return value
    if(status){
        cerr << "GEOMEGA: Geometry error in geometry \""+filename+"\". Removing geometry from list." << endl;
        if(slackVerbosity>=1) quickSlack("GEOMEGA: Geometry error in geometry \""+filename+"\". Removing geometry from list.");
        filename="";
        statusBar[2]--;
    } else statusBar[1]++;
    currentThreadCount--;
}


/**
 @brief Parse geomega settings and setup .geo.setup files

 ## Parse geomega settings and setup .geo.setup files

 ### Arguments
 - `YAML::NODE geomega` - Geomega node to aprse settings from
 - `vector<string> &geometries` - Vector of filenames of generated files (return by reference)

 ### Return value
 Returns the success value: 0 for success, return code otherwise

 ### Notes
 Merges all dependencies into a single file, my default g.geo.setup, then creates additional files from there. In my experience this has worked fine, but let me know if there is a problem with your geometry.
*/
int geomegaSetup(YAML::Node geomega, vector<string> &geometries){
    // Update status
    statusBar[0]=1;

    // Merge all files together
    ofstream baseGeometry("g.geo.setup");
    if(!baseGeometry.is_open()){ cerr << "Could not create new base geometry file. Exiting" << endl; if(slackVerbosity>=1) quickSlack("GEOMEGA SETUP: Could not create new base geometry file. Exiting."); return 3;}
    if(geoMerge(geomega["filename"].as<string>(),baseGeometry)) return 1;
    baseGeometry.close();

    // Generate all options
    vector<string> files;
    vector<int> lines;
    vector<vector<string>> options;
    if(geomega["parameters"].size()!=0){
        for(YAML::const_iterator it=geomega["parameters"].begin();it != geomega["parameters"].end();++it){
            files.push_back(it->second["filename"].as<string>());
            lines.push_back(it->second["lineNumber"].as<int>());
            options.push_back(parseIterativeNode(it->second["contents"]));
        }

        legendLock.lock();
        legend.open("geo.legend");

        // Create new files
        vector<size_t> odometer(lines.size(),0);
        int position=odometer.size()-1;
        while(position>=0){
            if(odometer[position]==options[position].size()){
                odometer[position]=0;
                if(--position<0) break;
                odometer[position]++;
            } else {
                statusBar[2]++;
                // Create legend
                legend << "Geometry";
                for(auto& o:odometer) legend << "." << o;
                legend << "\n";
                for(size_t i=0;i<lines.size();i++) legend << "File:" << files[i] << "\nLine: " << lines[i] << "\nOption: " << options[i][odometer[i]] << "\n";
                legend << "\n";

                // Read base geometry
                ifstream baseGeometryIn("g.geo.setup");
                stringstream alteredGeometry;
                copy(istreambuf_iterator<char>(baseGeometryIn),istreambuf_iterator<char>(),ostreambuf_iterator<char>(alteredGeometry));

                // Alter geometry
                for(size_t i=0;i<odometer.size();i++){
                    stringstream newGeometry;
                    string line;

                    bool foundFile = 0;
                    // Seek to "///Include "+files[i]
                    while(getline(alteredGeometry,line)){
                        newGeometry << line << "\n";
                        if(line=="///Include "+files[i]){ foundFile=1; break;}
                    }

                    // Make sure we found the file
                    if(!foundFile){
                        cerr << "Attempted to alter line number past end of file. File: \""+files[i]+"\". Exiting." << endl;
                        if(slackVerbosity>=1) quickSlack("GEOMEGA SETUP: Attempted to alter line number past end of file. File: "+files[i]);
                        return 5;
                    }

                    // Seek lines[i] lines ahead
                    for(int j=0;j<lines[i]-1;j++){
                        getline(alteredGeometry,line);
                        newGeometry << line << "\n";

                        stringstream newLine(line);
                        string command,file; newLine >> command >> file;
                        // Skip over other includes
                        if(command=="///Include"){
                            while(getline(alteredGeometry,line)){
                                newGeometry << line << "\n";
                                // Check we havent passed "///End "+files[i]
                                if(line=="///End "+files[i]){
                                    cerr << "Attempted to alter line number past end of file. File: \""+files[i]+"\". Exiting." << endl;
                                    if(slackVerbosity>=1) quickSlack("GEOMEGA SETUP: Attempted to alter line number past end of file. File: "+files[i]);
                                    return 4;
                                }
                                if(line=="///End "+file) break;
                            }
                        }
                        // Check we havent passed "///End "+files[i]
                        if(line=="///End "+files[i]){
                            cerr << "Attempted to alter line number past end of file. File: \""+files[i]+"\". Exiting." << endl;
                            if(slackVerbosity>=1) quickSlack("GEOMEGA SETUP: Attempted to alter line number past end of file. File: "+files[i]);
                            return 4;
                        }
                    }

                    // Replace that line with options[i][odometer[i]]
                    getline(alteredGeometry,line);
                    line=options[i][odometer[i]];
                    newGeometry<<line<<"\n";

                    // Copy rest of stream and swap streams
                    while(getline(alteredGeometry,line)) newGeometry << line << "\n";
                    alteredGeometry.swap(newGeometry);
                }

                // Create new file
                string fileName = "g";
                for(auto& o:odometer) fileName+="."+to_string(o);
                fileName+=".geo.setup";
                ofstream newGeometry(fileName);
                geometries.push_back(fileName);

                // Write to file and close it
                newGeometry << alteredGeometry.rdbuf();
                newGeometry.close();

                // Manage odometer
                position=odometer.size()-1;
                odometer[position]++;
            }
        }
        legend.close();
        legendLock.unlock();
    } else geometries.push_back("g.geo.setup");

    char result[ 1024 ];
    ssize_t count = readlink("/proc/self/exe", result, 1024);
    string path = (count != -1)?dirname(result):".";

    vector<thread> threadpool;
    // Verify all geometries
    if(!test) for(size_t i=0;i<geometries.size();i++){
        while(currentThreadCount>=maxThreads)sleep(0.1);
        threadpool.push_back(thread(testGeometry,std::ref(geometries[i]),path));
        currentThreadCount++;
    } else for(size_t i=0;i<geometries.size();i++) cout << (path+"/checkGeometry "+geometries[i]) << endl;

    // Join simulation threads
    for(size_t i=0;i<threadpool.size();i++) threadpool[i].join();
    // Properly order vector and remove empty strings (failed geometries)
    std::sort(geometries.begin(), geometries.end());
    geometries.erase(std::remove(geometries.begin(), geometries.end(), ""), geometries.end());


    return 0;
}

/**
 @brief Parse cosima settings and setup source files

 ## Parse cosima settings and setup source files

 ### Arguments
 - `YAML::Node cosima` - Cosima node to parse settings from
 - `vector<string> &sources` - Vector of strings of output filenames (return by reference)
 - `vector<string> &geometries` - Vector of strings of geometry filenames

 ### Return value
 Returns the success value: 0 for success, return code otherwise.

 ### Notes
 Only replaces line in a source file, it does not add them as that would be undefined behavior. Make sure that all of your operations replace lines, otherwise they will not be parsed correctly. This may not always throw an error, so manually check that your iterations are properly parsing.
*/
int cosimaSetup(YAML::Node cosima, vector<string> &sources, vector<string> &geometries){
    // Update status
    statusBar[3]=statusBar[6]=1;

    string baseFileName = cosima["filename"].as<string>();
    // Make sure config file exists
    if(!fileExists(baseFileName)){
        cerr << "File \"" << baseFileName << "\" does not exist, but was requested. Exiting."<< endl;
        if(slackVerbosity>=1) quickSlack("COSIMA SETUP: File \"" + baseFileName + "\" does not exist, but was requested. Exiting.");
        return 1;
    }

    // Parse iterative nodes, but need to specially format them with the correct source and name.
    map<string,vector<string>> options;
    for(YAML::const_iterator it=cosima["parameters"].begin();it != cosima["parameters"].end();++it){
        if(it->second["beam"]) options[it->second["source"].as<string>()+".Beam"] = parseIterativeNode(it->second["beam"],it->second["source"].as<string>()+".Beam");
        if(it->second["spectrum"]) options[it->second["source"].as<string>()+".Spectrum"] = parseIterativeNode(it->second["spectrum"],it->second["source"].as<string>()+".Spectrum");
        if(it->second["flux"]) options[it->second["source"].as<string>()+".Flux"] = parseIterativeNode(it->second["flux"],it->second["source"].as<string>()+".Flux");
        if(it->second["polarization"]) options[it->second["source"].as<string>()+".Polarization"] = parseIterativeNode(it->second["polarization"],it->second["source"].as<string>()+".Polarization");
    }
    if(geometries.size()!=0){
        for(auto& g : geometries) g = "Geometry "+g;
        options["Geometry"] = geometries;
    }

    // Read base geometry
    ifstream baseSource(cosima["filename"].as<string>());
    stringstream baseSourceStream;
    copy(istreambuf_iterator<char>(baseSource),istreambuf_iterator<char>(),ostreambuf_iterator<char>(baseSourceStream));
    vector<string> alteredSources;
    alteredSources.push_back(baseSourceStream.str());

    // Parse cosima parameters to create a bunch of base run?.source files
    for(auto &elem:options){
        vector<string> newSources;
        for(auto &option:elem.second){
            for(auto &s : alteredSources){
                stringstream alteredSource(s);
                stringstream newSource;
                for(string line; getline(alteredSource,line);){
                    stringstream ss(line);
                    string command; ss >> command;
                    if(command==elem.first){
                        newSource << option << "\n";
                    } else newSource << line << "\n";
                }
                newSources.push_back(newSource.str());
            }
        }
        alteredSources.swap(newSources);
    }

    for(size_t i=0;i<alteredSources.size();i++){
        string filename = "run"+to_string(i)+".source";
        sources.push_back(filename);
        ofstream out(filename);
        // Fix output filename
        regex e(".FileName .*\n");
        out << regex_replace(alteredSources[i],e,".FileName run"+to_string(i)+"\n");;
        out.close();
    }

    // Update status
    statusBar[5]=statusBar[8]=alteredSources.size();

    return 0;
}

/**
 @brief Runs the Cosima simulation and Revan data reduction for one set of parameters

 ##  Runs the Cosima simulation and Revan data reduction for one set of parameters

 ### Arguments
 - `const string source` - *.source file for cosima
 - `const int threadNumber` - Thread number to avoid file name collisions

 ### Notes
 Often runs out of storage if you are not careful

*/
void runSimulation(const string source, const int threadNumber){
    auto start = chrono::steady_clock::now();

    // Get seed
    uint32_t seed = random_seed<uint32_t>(1);

    // Create legend
    legendLock.lock();
    legend << "Run number " << threadNumber << ":";
    legend << "\nSource: " << source;
    legend << "\nSeed:" << to_string(seed) << "\n" << endl;
    legendLock.unlock();

    // Get geometry file
    ifstream sourceFile(source);
    string geoSetup;
    while(!sourceFile.eof() && geoSetup!="Geometry") sourceFile>>geoSetup;
    if(geoSetup!="Geometry"){cerr << "Cannot locate geometry file. Exiting." << endl; if(slackVerbosity>=1) quickSlack("RUN SIMULATION"+to_string(threadNumber)+": Cannot locate geometry file."); return;}
    sourceFile>>geoSetup;
    sourceFile.close();

    // Actually run simulation and analysis
    // Remove intermediary files when they are no longer necessary (unless keepAll is set)
    if(!test){
        int status, ret=system(("bash -c \"source ${MEGALIB}/bin/source-megalib.sh; cosima -v "+to_string(cosimaVerbosity)+" -z -s "+to_string(seed)+" run"+to_string(threadNumber)+".source |& xz -3 > cosima.run"+to_string(threadNumber)+".log.xz; exit $?\"").c_str());
        status=WEXITSTATUS(ret); // Get return value
        if(status){
            quickSlack("Run "+to_string(threadNumber)+" failed.");
            return;
        }
        statusBar[4]++;
        ret=system(("bash -c \"source ${MEGALIB}/bin/source-megalib.sh; revan -c "+revanSettings+" -n -a -f run"+to_string(threadNumber)+".*.sim.gz -g "+geoSetup+" |& xz -3 > revan.run"+to_string(threadNumber)+".log.xz; exit $?\"").c_str());
        status=WEXITSTATUS(ret); // Get return value
        if(status){
            quickSlack("Run "+to_string(threadNumber)+" failed.");
            return;
        }
        statusBar[7]++;
        if(!keepAll) removeWildcard("run"+to_string(threadNumber)+".*.sim.gz");
    }else{
        cout << "bash -c \"source ${MEGALIB}/bin/source-megalib.sh; cosima -v "+to_string(cosimaVerbosity)+" -z -s "+to_string(seed)+" run"+to_string(threadNumber)+".source |& xz -3 > cosima.run"+to_string(threadNumber)+".log.xz; exit $?\"\n";
        cout << "bash -c \"source ${MEGALIB}/bin/source-megalib.sh; revan -c "+revanSettings+" -n -a -f run"+to_string(threadNumber)+".*.sim.gz -g "+geoSetup+" |& xz -3 > revan.run"+to_string(threadNumber)+".log.xz; exit $?\"\n";
        if(!keepAll) cout << "rm run"+to_string(threadNumber)+".*.sim.gz\n";
    }

    // End timer, calculate new average time
    auto end = chrono::steady_clock::now();
    chrono::seconds thisTime = chrono::duration_cast<chrono::seconds>(end-start);
    timeLock.lock();
    averageTime = (averageTime.count()!=0)?(averageTime*10+thisTime)/(11):thisTime;
    timeLock.unlock();
    // Cleanup and exit
    currentThreadCount--;
    return;
}

/**
## autoMEGA

### Arguments:

 - `--settings` - Settings file - defaults to "config.yaml"
 - `--test` - Enter test mode. Largely undefined behavior, but it will generally perform a dry run and limit slack notifications. Use at your own risk.

### Configuration:
Most settings are only configurable from the yaml configuration file. The format is:

autoMEGA settings:
 - `address` - Email to send an email to when done (relies on sendmail). If not present, email notifications are disabled. Note: depends on a system call to sendmail, so it may not work on all systems.
 - `token` - Slack OAuth2 token to send notification when done. If not present, slack notifications are disabled.
 - `channel` - Slack channel to send notification when done. (See slack API for how to obtain slack channel code, format `C12345678`). If not present, slack notifications are disabled.
 - `maxThreads` - Maximum threads to use (defaults to system threads if not given)
 - `keepAll` - Flag to keep intermediary files (defaults to off = 0)
General settings files:
 - `revanSettings` - Defaults to system default (`~/revan.cfg`)
 - `slackVerbosity` - Slack verbosity. Level 3 prints all messages, level 2 prints fewer messages, level one prints only error messages, and level zero only prints final messages. Defaults to zero
 - `cosimaVerbosity` - Cosima verbosity. Defaults to zero.

Standard parameter format:

If an array is given, it is assumed to be in one of two formats.

If there are three values, then the parameter starts at the first value and increments at the third value until it gets to the second value.

If the array is a double array of values, those are taken as the literal values of the parameter.

Cosima settings:
 - `filename` - Base cosima .source file
 - `parameters` - Array of parameters, formatted as such:
    - `source` - Name of the source to modify
    - `beam` - Beam settings: Array of values in the standard format, to be separated by spaces in the file. (Optional, if not present, then it is not modified from the base file).
    - `spectrum` - Spectrum settings: Array of values in the standard format, to be separated by spaces in the file. (Optional, if not present, then it is not modified from the base file).
    - `flux` - Array of values in the standard format, to be separated by spaces in the file. (Optional, if not present, then it is not modified from the base file).
    - `polarization` - Polarization settings: Array of values in the standard format, to be separated by spaces in the file. (Optional, if not present, then it is not modified from the base file).

Geomega settings:
 - `filename` - Base geomega .geo.setup file
 - `parameters` - Array of parameters, formatted as such:
    - `filename` - Filename of the file to modify
    - `line number` - line number of the file to modify
    - `contents` - Contents of the line. Array of values(including strings) in the standard format, to be separated by spaces in the file.

### Dependencies:
- MEGAlib
- YAML-cpp
- pipeliningTools

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

*/
int main(int argc,char** argv){
    auto start = chrono::steady_clock::now();
    for(int i=0;i<9;i++)statusBar[i]=0;

    // Parse command line arguments
    for(int i=0;i<argc;i++){
        if(i<argc-1) if(string(argv[i])=="--settings") settings = argv[++i];
        if(string(argv[i])=="--test") test = 1;
    }

    // Make sure config file exists
    if(!fileExists(settings)){
        cerr << "File \"" << settings << "\" does not exist, but was requested. Exiting."<< endl;
        quickSlack("MAIN: File \"" + settings + "\" does not exist, but was requested. Exiting.");
        return 1;
    }

    // Check directory
    if(directoryEmpty(".")) return 3;

    // Disable echo
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag &= ~ECHO;
    (void) tcsetattr(STDIN_FILENO, TCSANOW, &tty);

    // Parse config file
    YAML::Node config = YAML::LoadFile(settings);
    if(config["address"]) address = config["address"].as<string>();
    if(config["token"]) token = config["token"].as<string>();
    if(config["channel"]) channel = config["channel"].as<string>();
    if(config["keepAll"]) keepAll = config["keepAll"].as<bool>();
    if(config["slackVerbosity"]) slackVerbosity = config["slackVerbosity"].as<int>();
    if(config["cosimaVerbosity"]) cosimaVerbosity = config["cosimaVerbosity"].as<int>();
    if(config["revanSettings"]) revanSettings = config["revanSettings"].as<string>();
    if(config["maxThreads"]) maxThreads = config["maxThreads"].as<int>();

    // Start status thread
    thread statusThread(handleStatus);

    if(slackVerbosity>=3) quickSlack("Starting Geomega stage.");
    vector<string> geometries;
    if(config["geomega"]) if(geomegaSetup(config["geomega"],geometries)!=0) return 2;

    if(slackVerbosity>=3) quickSlack("Starting Cosima parsing stage");
    vector<string> sources;
    if(config["cosima"]) if(cosimaSetup(config["cosima"],sources,geometries)!=0) return 3;


    // Create threadpool
    vector<thread> threadpool;
    cout << "\rUsing " << maxThreads << " threads.                                      " << endl;
    legend.open("run.legend");

    // Start watchdog thread(s)
    thread watchdog0(storageWatchdog,2000);

    // Calculate total number of simulations
    if(slackVerbosity>=3) quickSlack("Starting all simulations");

    // Start all simulation threads.
    for(size_t i=0;i<sources.size();i++){
        while(currentThreadCount>=maxThreads)sleep(0.1);
        threadpool.push_back(thread(runSimulation,sources[i],threadpool.size()));
        currentThreadCount++;
    }
    // Join simulation threads
    for(size_t i=0;i<threadpool.size();i++) threadpool[i].join();
    legend.close();

    // Enable echo
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag |= ECHO;
    (void) tcsetattr(STDIN_FILENO, TCSANOW, &tty);

    // End timer, print command duration
    exitFlag=1;
    watchdog0.join();
    statusThread.join();
    auto end = chrono::steady_clock::now();
    cout << endl << "Total simulation and analysis elapsed time: " << beautify_duration(chrono::duration_cast<chrono::seconds>(end-start)) << endl;
    quickSlack("Simulation complete. Elapsed time: "+beautify_duration(chrono::duration_cast<chrono::seconds>(end-start)));
    if(!address.empty()) email(address,"Simulation Complete. Elapsed time: "+beautify_duration(chrono::duration_cast<chrono::seconds>(end-start)));
    return 0;
}
