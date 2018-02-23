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
#include <stdio.h>
#include <errno.h>

#include <config.h>

// Run execl() in a new forked process and wait for it to complete.
#define fexecl(...) \
    { \
        int pid, status; \
        pid = fork(); \
        ehandle(pid == -1, E_FORK_FAILED); \
        if (pid == 0) \
            execl(__VA_ARGS__, (char*)NULL); \
        else \
            waitpid(pid, &status, 0); \
    }

// Handle an error.
#define ehandle(ACTION, EID) \
    if (ACTION) { \
        fprintf(stderr, "Error on %s:%d: %s\n", __FILE__, __LINE__, strerror(errno)); \
        exit(EID); \
    }

// Errors to be returned by program - ensures unique error numbers. 
enum errors {
    E_OK,
    E_NO_ACTION,
    E_BAD_ARG,
    E_CPUFREQ_WRITE,
    E_PWR_STATE_WRITE,
    E_PWR_STATE_READ,
    E_FORK_FAILED
};

// Regular User ID
static int ruid;

// All cmdline flags after parsing.
struct s_flags {
    const char* program_name;  // argv[0]
    int (*action)();           // Action for program to perform.
    int no_restart;            // Flag: disables restarting of display manager.
};

// The actual parsed go into this struct instance.
static struct s_flags flags;

// Returns true if an executable file exists at the given path.
static int binary_exists (const char* path);

static const char* wlan_name (); // Get wifi interface name.

static void restart_display_manager ();       // Uses systemctl to restart display-manager.
static void prime_select (const char* card);  // Uses prime-select to switch GPUs.
static void wifi_power (const char* state);   // Set wifi power-saving state.
static void cpu_governor (const char* rule);  // Set performance governor.

static const char* get_pwr_state ();           // Get the power state info.
static void set_pwr_state (const char* state); // Save the power state info.

static int glob_error (const char* path, int error); // Handle glob errors.

// Actions that may be performed by the program, depending on flags given.
static int action_none ();       // No action specified, print error and exit.
static int action_perform ();    // Enable performance mode.
static int action_powersave ();  // Enable power-saving mode.
static int action_version ();    // Print version information.
static int action_help ();       // Print help information.

static int parse_args (int argc, char** argv); // Parse cmdline args and set appropriate flags.


int main (int argc, char** argv) {
    ruid = geteuid();

    int result = parse_args(argc, argv);
    if (result != E_OK) return result;

    return flags.action();
}


static int binary_exists (const char* path) {
    struct stat status;
    if (stat(path, &status) < 0) return 0;
    return status.st_mode & S_IEXEC != 0;
}

static const char* wlan_name () {
    struct ifaddrs* iface;
    getifaddrs(&iface);
    struct ifaddrs* first = iface;

    int found = 0;

    while (iface != NULL) {
        if (iface->ifa_name[0] == 'w' && iface->ifa_name[1] == 'l') {
            found = 1;
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


static void restart_display_manager () {
    if (!flags.no_restart && binary_exists("/bin/systemctl"))
        fexecl("/bin/systemctl", "systemctl", "restart", "display-manager");
}

static void prime_select (const char* card) {
    if (binary_exists("/usr/bin/prime-select"))
        fexecl("/usr/bin/prime-select", "prime-select", card);
}

static void wifi_power (const char* state) {
    const char* iface = wlan_name();

    if (binary_exists("/sbin/iwconfig") && iface != NULL)
        fexecl("/sbin/iwconfig", "iwconfig", iface, "power", state);
    
    free((void*)iface);
}

static void cpu_governor (const char* rule) {
    glob_t results = { 0 };

    glob("/sys/devices/system/cpu/cpu*/cpufreq/scaling_governor", 0, glob_error, &results);

    for (int i = 0; i < results.gl_pathc; i++) {
        FILE* f = fopen(results.gl_pathv[i], "w");
        ehandle(f == NULL, E_CPUFREQ_WRITE);
        fprintf(f, "%s\n", rule);
        fclose(f);
    }

    globfree(&results);
}


static const char* get_pwr_state () {
    FILE* pstate = fopen("/var/lib/pwr_state", "r");
    ehandle(pstate == NULL, E_PWR_STATE_READ);

    // Default if we can't read the file.
    if (pstate == NULL) return "perform";
    char* result = NULL;
    size_t len = 0;
    getline(&result, &len, pstate);
    fclose(pstate);

    // Remove trailing newline, if any.
    result[strcspn(result, "\n")] = 0;

    return result;
}

static void set_pwr_state (const char* state) {
    FILE* pstate = fopen("/var/lib/pwr_state", "w");
    ehandle(pstate == NULL, E_PWR_STATE_WRITE);
    fprintf(pstate, "%s\n", state);
    fclose(pstate);
}


// Kindly provided by u/lordvadr on reddit.
static int glob_error (const char* path, int error) {
    if (path)
        fprintf(stderr, "glob: %s: %s\n", path, strerror(error));
    else
        fprintf(stderr, "glob: %s\n", strerror(error));

    return 0;
}


static int action_none () {
    fprintf(stderr, "No action specified\n");
    fprintf(stderr, "Run `%s` --help` for help.\n", flags.program_name);
    return E_NO_ACTION;
}

static int action_perform () {
    seteuid(0);

    cpu_governor("performance");
    prime_select("nvidia");
    wifi_power("off");
    restart_display_manager();

    set_pwr_state("perform");

    seteuid(ruid);
    return E_OK;
}

static int action_powersave () {
    seteuid(0);

    cpu_governor("powersave");
    prime_select("intel");
    wifi_power("on");
    restart_display_manager();

    set_pwr_state("powersave");

    seteuid(ruid);
    return E_OK;
}

static int action_query () {
    puts(get_pwr_state());
    return E_OK;
}

static int action_toggle () {
    const char* state = get_pwr_state();
    if (!strcmp(state, "powersave")) return action_perform();
    else return action_powersave();
    free((void*)state);
}

static int action_version () {
    puts("pwr v" S_Pwr_VERSION "\n");
    puts("Copyright 2018 Ethan McTague.");
    puts("Licensed under the MIT License.");
    puts("https://github.com/emctague/pwr");
    return E_OK;
}

static int action_help () {
    puts("pwr - Switches between performance and power-saving modes.");
    printf("Usage: %s [action] [flags]\n", flags.program_name);
    puts("Actions:");
    puts(" perform (pe)      Go into performance mode.");
    puts(" powersave (ps)    Go into power-saving mode.");
    puts(" toggle (to)       Toggles the current state.");
    puts(" query (qu)        Query the current state, prints 'perform' or 'powersave'.");
    puts(" --help            Prints this help information.");
    puts(" --version         Prints version, contact, and copyright information.\n");
    puts("Flags:");
    puts(" --norestart (-n)  Do not restart display manager after changing modes.");
    return E_OK;
}


static int parse_args (int argc, char** argv) {
    flags.action = action_none;
    flags.program_name = argv[0];
    flags.no_restart = 0;

    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];

        if (!strcmp(arg, "perform") || !strcmp(arg, "pe"))
            flags.action = action_perform;
        
        else if (!strcmp(arg, "powersave") || !strcmp(arg, "ps"))
            flags.action = action_powersave;

        else if (!strcmp(arg, "query") || !strcmp(arg, "qu"))
            flags.action = action_query;
        
        else if (!strcmp(arg, "toggle") || !strcmp(arg, "to"))
            flags.action = action_toggle;
        
        else if (!strcmp(arg, "--help"))
            flags.action = action_help;

        else if (!strcmp(arg, "--version"))
            flags.action = action_version;
        
        else if (!strcmp(arg, "--norestart") || !strcmp(arg, "-n"))
            flags.no_restart = 1;
        
        else {
            fprintf(stderr, "Bad argument encountered: %s\n", arg);
            return E_BAD_ARG;
        }
    }

    return E_OK;
}
