/* pwr: Power-saving-mode controller for linux laptops.
 * Copyright 2018 Ethan McTague.
 * Licensed under the MIT license. See LICENSE for full license text.
 * https://github.com/emctague/pwr
 */ 

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <glob.h>
#include <ifaddrs.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>

#include <config.h>

using namespace std;

// Run execl() in a new thread and wait for it to complete.
#define fexecl(...) \
    {\
        int pid, status;\
        if ((pid = fork()) == 0)\
            execl(__VA_ARGS__, NULL);\
        else waitpid(pid, &status, 0);\
    }

// Errors to be returned by program - ensures unique error numbers. 
enum errors {
    E_OK,
    E_NO_ACTION,
    E_BAD_ARG
};

// Regular User ID
int ruid;

// All cmdline flags after parsing.
struct s_flags {
    string program_name;        // argv[0]
    int (*action)();            // Action for program to perform.
    bool no_restart;            // Flag: disables restarting of display manager.
};

// The actual parsed go into this struct instance.
struct s_flags flags;

// Returns true if an executable file exists at the given path.
bool binary_exists (const char* path);

const char* wlan_name (); // Get wifi interface name.

void restart_display_manager ();       // Uses systemctl to restart display-manager.
void prime_select (const char* card);  // Uses prime-select to switch GPUs.
void wifi_power (const char* state);   // Set wifi power-saving state.
void cpu_governor (const char* rule);  // Set performance governor.

// Actions that may be performed by the program, depending on flags given.
int action_none ();       // No action specified, print error and exit.
int action_perform ();    // Enable performance mode.
int action_powersave ();  // Enable power-saving mode.
int action_version ();    // Print version information.
int action_help ();       // Print help information.

int parse_args (int argc, char** argv); // Parse cmdline args and set appropriate flags.

int main (int argc, char** argv) {
    ruid = geteuid();

    int result = parse_args(argc, argv);
    if (result != E_OK) return result;

    return flags.action();
}

bool binary_exists (const char* path) {
    struct stat status;
    if (stat(path, &status) < 0) return false;
    return status.st_mode & S_IEXEC != 0;
}

const char* wlan_name () {
    struct ifaddrs* iface;
    getifaddrs(&iface);
    struct ifaddrs* first = iface;

    bool found = false;

    while (iface != NULL) {
        if (iface->ifa_name[0] == 'w' && iface->ifa_name[1] == 'l') {
            found = true;
            break;
        }

        iface = iface->ifa_next;
    }

    char* ifname = (char*)malloc(strlen(iface->ifa_name) + 1);
    strcpy(ifname, iface->ifa_name);

    freeifaddrs(first);
    if (found) return ifname;
    else return NULL;

}

void restart_display_manager () {
    if (!flags.no_restart && binary_exists("/bin/systemctl"))
        fexecl("/bin/systemctl", "systemctl", "restart", "display-manager");
}

void prime_select (const char* card) {
    if (binary_exists("/usr/bin/prime-select"))
        fexecl("/usr/bin/prime-select", "prime-select", card, NULL);
}

void wifi_power (const char* state) {
    const char* iface = wlan_name();

    if (binary_exists("/sbin/iwconfig") && iface != NULL)
        fexecl("/sbin/iwconfig", "iwconfig", iface, "power", state, NULL);
    
    free((void*)iface);
}

void cpu_governor (const char* rule) {
    glob_t results = { 0 };

    glob("/sys/devices/system/cpu/cpu*/cpufreq/scaling_governor", GLOB_DOOFFS, NULL, &results);

    for (int i = 0; i != results.gl_pathc; i++) {
        ofstream gov(results.gl_pathv[i]);
        gov << rule << endl;
        gov.close();
    }

    globfree(&results);
}

int action_none () {
    cerr << "No action specified" << endl;
    cerr << "Run `" << flags.program_name << " --help` for help." << endl;
    return E_NO_ACTION;
}

int action_perform () {
    seteuid(0);

    cpu_governor("performance");
    prime_select("nvidia");
    wifi_power("off");
    restart_display_manager();

    seteuid(ruid);
    return E_OK;
}

int action_powersave () {
    seteuid(0);

    cpu_governor("powersave");
    prime_select("intel");
    wifi_power("on");
    restart_display_manager();

    seteuid(ruid);
    return E_OK;
}

int action_version () {
    cout << "pwr v" << Pwr_VERSION_MAJOR << "." << Pwr_VERSION_MINOR << endl;
    cout << "Copyright 2018 Ethan McTague." << endl;
    cout << "Licensed under the MIT License." << endl;
    cout << "https://github.com/emctague/pwr" << endl;
    return E_OK;
}

int action_help () {
    cout << "pwr - Switches between performance and power-saving modes." << endl;
    cout << "Usage:" << flags.program_name << " [action] [flags]" << endl;
    cout << "Actions:" << endl;
    cout << " perform (pe)      Go into performance mode." << endl;
    cout << " powersave (ps)    Go into power-saving mode." << endl;
    cout << " --help            Prints this help information." << endl;
    cout << " --version         Prints version, contact, and copyright information." << endl << endl;
    cout << "Flags:" << endl;
    cout << " --norestart (-n)  Do not restart display manager after changing modes." << endl;
    return E_OK;
}

int parse_args (int argc, char** argv) {
    flags.action = action_none;
    flags.program_name = string(argv[0]);
    flags.no_restart = false;

    for (int i = 1; i < argc; i++) {
        string arg = string(argv[i]);

        if (arg == "perform" || arg == "pe")
            flags.action = action_perform;
        
        else if (arg == "powersave" || arg == "ps")
            flags.action = action_powersave;
        
        else if (arg == "--help")
            flags.action = action_help;

        else if (arg == "--version")
            flags.action = action_version;
        
        else if (arg == "--norestart" || arg == "-n")
            flags.no_restart = true;
        
        else {
            cerr << "Bad argument encountered: " << arg << endl;
            return E_BAD_ARG;
        }
    }

    return E_OK;
}