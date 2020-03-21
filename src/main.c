#include <stdio.h>
#include <taihen.h>
#include <psp2/io/fcntl.h> 
#include <psp2kern/kernel/sysmem.h> 
#include <string.h>
#include <stdarg.h>
#include <vitasdkkern.h>
#include <stdlib.h>
#include <psp2/appmgr.h> 

#define printf ksceDebugPrintf
#define SECOND        1000000

static tai_hook_ref_t event_handler_ref;
static SceUID hooks[1];

//I don't know if its related but without this 2 line, adrenaline won't launch, as far as I'm aware.
//Taken from Electry's PSVShell code.
SceUID (*_ksceKernelGetProcessMainModule)(SceUID pid);
int (*_ksceKernelGetModuleInfo)(SceUID pid, SceUID modid, SceKernelModuleInfo *info);

// Boolean values
static uint8_t playtime_loaded = 0;
static uint8_t playtime_written = 0;
static uint8_t pspemu = 0;
static uint8_t isxmb = 0;
// String values
static char adrenaline_bin_path[128];
static char playtime_bin_path[128];
static char id[32];
static char adrenaline_id[12] = "pleasePLEASE";
// Timer values
static uint64_t adrenaline_tick_start = 0;
static uint64_t adrenaline_playtime_start = 0;
static uint64_t playtime_start = 0;
static uint64_t tick_start = 0;
// PID for adrenaline
static SceUID pid_of_adrenaline;

// Taken from Adrenaline
typedef struct {
	int savestate_mode;
	int num;
	unsigned int sp;
	unsigned int ra;

	int pops_mode;
	int draw_psp_screen_in_pops;
	char title[128];
	char titleid[12];
	char filename[256];

	int psp_cmd;
	int vita_cmd;
	int psp_response;
	int vita_response;
} SceAdrenaline;

static void load_playtime(char *local_titleid) {
    // To prevent loading twice.
    if (playtime_loaded == 1) return;
    playtime_loaded = 1;
    playtime_written = 0;

    snprintf(playtime_bin_path, 128, "ux0:/data/TrackPlug/Records/%s.bin", local_titleid);
    tick_start = ksceKernelGetSystemTimeWide();
    SceUID fd = ksceIoOpen(playtime_bin_path, SCE_O_RDONLY, 0777);
    if (fd < 0) {
        playtime_start = 0;
        return;
    }
    ksceIoRead(fd, &playtime_start, sizeof(uint64_t));
    ksceIoClose(fd);
}

static void write_playtime(char *local_titleid) {
    // To prevent writing twice.
    if (playtime_written == 1) return;
    playtime_loaded = 0;
    playtime_written = 1;

    uint32_t playtime = playtime_start +
                (ksceKernelGetSystemTimeWide() - tick_start) / SECOND;

    SceUID fd = ksceIoOpen(playtime_bin_path,
            SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd < 0) return;

    ksceIoWrite(fd, &playtime, sizeof(uint64_t));
    ksceIoClose(fd);
}

static void adrenaline_titlewriter(char *dummy_id,char *dummy_title){
    //We don't want XMB's title to be written.
    if (dummy_title[0] == 0 || !strncmp(dummy_title, "XMB", 3)) return;
    char path[128];
    snprintf(path, 128, "ux0:/data/TrackPlug/Names/%s.txt", dummy_id);
    SceUID fd = ksceIoOpen(path,
        SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd < 0) return;

    ksceIoWrite(fd, dummy_title, strlen(dummy_title));
    ksceIoClose(fd);
}

static void adrenaline_loader(char *dummy_id) {
    snprintf(adrenaline_bin_path, 128, "ux0:/data/TrackPlug/Records/%s.bin", dummy_id);
    adrenaline_tick_start = ksceKernelGetSystemTimeWide();
    SceUID fd = ksceIoOpen(adrenaline_bin_path, SCE_O_RDONLY, 0777);
    if (fd < 0) {
        adrenaline_playtime_start = 0;
        return;
    }
    ksceIoRead(fd, &adrenaline_playtime_start, sizeof(uint64_t));
    ksceIoClose(fd);
}

static void adrenaline_writer(char *dummy_id) {
    if (!strncmp(dummy_id,"pleasePLEASE",12)) return;
    uint32_t playtime = adrenaline_playtime_start +
        (ksceKernelGetSystemTimeWide() - adrenaline_tick_start) / SECOND;
    
    SceUID fd = ksceIoOpen(adrenaline_bin_path,
        SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd < 0) return;

    ksceIoWrite(fd, &playtime, sizeof(uint64_t));
    ksceIoClose(fd);
}

// Load-Write handler for Adrenaline.
static void check_adrenaline() {
    // Memleketime can kurban.
    SceAdrenaline Bartin;
    // This memory location was taken from Electry's TrackPlugX code.
    ksceKernelMemcpyUserToKernelForPid(pid_of_adrenaline, &Bartin, (void *)0x73CDE000, sizeof(Bartin));
    char *local_title = (void*)&Bartin + 24;
    char *local_id = (void*)&Bartin + 152;

    //If in the XMB, say its in the xmb and return.
    if (local_title[0] == 0 || !strncmp(local_title, "XMB", 3)){
        isxmb=1;
    }
    // If the game is changed, create its titlename, load the games time data and change the global variable to current game.
    else if (strncmp(adrenaline_id, local_id, 9)){
        isxmb=0;
        adrenaline_titlewriter(local_id,local_title);
        adrenaline_writer(adrenaline_id);
        adrenaline_loader(local_id);
        snprintf(adrenaline_id,strlen(local_id),local_id);
    }
    return;
}

// This thread is to detect if adrenaline is running once in every second.
static int adrenaline_checker(SceSize args, void *argp) {
    while (1) {
        if(pspemu){
            check_adrenaline(); 
            ksceKernelDelayThread(SECOND);
        } else ksceKernelDelayThread(SECOND);
    }
    return ksceKernelExitDeleteThread(0);
}

// This handles the events of every app.
int event_handler(int pid, int ev, int a3, int a4, int *a5, int a6) {
    ksceKernelGetProcessTitleId(pid, id, sizeof(id));
    //If Adrenaline is launched, start the adrenaline checker.
    if (!strncmp(id, "PSPEMUCFW", 9)){
        pid_of_adrenaline = pid;
        pspemu = 1;
    }
    // If its a system app, ignore and go on your own way.
    else if (!strncmp(id, "main", 4)||!strncmp(id, "NPXS", 4)) {
        //pspemu = 0;
        return TAI_CONTINUE(int, event_handler_ref, pid, ev, a3, a4, a5, a6);
    }
    switch(ev){
        case 1: // Startup
        //We dont need to load game time at adrenaline launch.
            if (!pspemu)
                load_playtime(id);
        break;

        case 5: // Resume
            if (pspemu)
                adrenaline_loader(adrenaline_id);
            else
                load_playtime(id);
        break;
        case 3: // Exit
            if (pspemu){
                //Write games playtime and reset the variable. If Adrenaline was in XMB, no need
                //to write it again as it will be handled by the thread.
                if (!isxmb) write_playtime(adrenaline_id);
                snprintf(adrenaline_id,12,"pleasePLEASE");
                pspemu = 0;
            } else
                write_playtime(id);
        break;

        case 4: // Suspend
            if (pspemu){
                // Same story with the Exit.
                if (!isxmb) write_playtime(adrenaline_id);
                pspemu = 0;
            } else
                write_playtime(id);
        break;
    }
    return TAI_CONTINUE(int, event_handler_ref, pid, ev, a3, a4, a5, a6);
}

void _start() __attribute__((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
    hooks[0] = taiHookFunctionImportForKernel(KERNEL_PID,
            &event_handler_ref,
            "SceProcessmgr",
            TAI_ANY_LIBRARY,
            0x414CC813, // ksceKernelInvokeProcEventHandler
            event_handler);

    SceUID adrenaline_checker_id = ksceKernelCreateThread(
        "AdrenalineChecker",
        adrenaline_checker,
        0x10000100,
        0x1000, // 4 KiB allocated, as said by cuevavirus.
        0, 0, NULL);
    
    if (adrenaline_checker_id >= 0)
        ksceKernelStartThread(adrenaline_checker_id, 0, NULL);

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
    taiHookReleaseForKernel(hooks[0], event_handler_ref);
    return SCE_KERNEL_STOP_SUCCESS;
}