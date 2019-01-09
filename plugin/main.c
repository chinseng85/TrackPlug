#include <vitasdk.h>
#include <taihen.h>

#define SECOND        1000000
#define SAVE_PERIOD   10

int snprintf(char *s, size_t n, const char *format, ...);

// ScePspemu
static SceUID         sub_810053F8_hookid  = -1;
static tai_hook_ref_t sub_810053F8_hookref = {0};

static char adrenaline_titleid[12];

static uint8_t is_pspemu_loaded = 0;
static uint8_t is_pspemu_custom_bbl = 0;
static uint8_t is_playtime_loaded = 0;

static char playtime_bin_path[128];
static uint64_t playtime_start = 0;
static uint64_t tick_start = 0;

static void load_playtime(const char *titleid) {

    snprintf(playtime_bin_path, 128, "ux0:/data/TrackPlug/%s.bin", titleid);
    tick_start = sceKernelGetProcessTimeWide();

    SceUID fd = sceIoOpen(playtime_bin_path, SCE_O_RDONLY, 0777);
    if (fd < 0) {
        playtime_start = 0;
        is_playtime_loaded = 1;
        return;
    }

    sceIoRead(fd, &playtime_start, sizeof(uint64_t));
    sceIoClose(fd);

    is_playtime_loaded = 1;
}

static void write_playtime(uint64_t playtime) {

    SceUID fd = sceIoOpen(playtime_bin_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd < 0) {
        return;
    }

    sceIoWrite(fd, &playtime, sizeof(uint64_t));
    sceIoClose(fd);
}

void write_title(const char *titleid, const char *title) {

    char path[128];
    snprintf(path, 128, "ux0:/data/TrackPlugArchive/%s.txt", titleid);

    // Check if already exists
    SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0777);
    if (fd >= 0) {
        sceIoClose(fd);
        return;
    }

    fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT, 0777);
    if (fd < 0) {
        return;
    }

    sceIoWrite(fd, title, strlen(title));
    sceIoClose(fd);
}

static int check_adrenaline() {
    void *SceAdrenaline = (void *)0x73CDE000;

    char *title = SceAdrenaline + 24;
    char *titleid = SceAdrenaline + 152;

    if (title[0] == 0 || !strncmp(title, "XMB", 3))
        return 0;

    // Game changed?
    if (strncmp(adrenaline_titleid, titleid, 9)) {
        is_playtime_loaded = 0;
        strcpy(adrenaline_titleid, titleid);

        load_playtime(titleid);
        write_title(titleid, title);
        return 0;
    }

    return 1;
}

int sub_810053F8_patched(int a1, int a2) {

    char *pspemu_titleid = (char *)(a2 + 68);

    // If using custom bubble
    if (strncmp(pspemu_titleid, "PSPEMUCFW", 9)) {
        is_pspemu_custom_bbl = 1;
        load_playtime(pspemu_titleid);
    }

    is_pspemu_loaded = 1;

    return TAI_CONTINUE(int, sub_810053F8_hookref, a1, a2);
}

static int tracker_thread(SceSize args, void *argp) {

    while(1) {
        uint64_t tick_now = sceKernelGetProcessTimeWide();

        if (is_pspemu_loaded) {
            // Check if XMB/game has changed
            if (!is_pspemu_custom_bbl && !check_adrenaline()) {
                goto CONT;
            }
        }

        if (!is_playtime_loaded) {
            goto CONT;
        }

        write_playtime(playtime_start + (tick_now - tick_start)/SECOND);

CONT:
        sceKernelDelayThread(SAVE_PERIOD * SECOND);
    }

    return sceKernelExitDeleteThread(0);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {

    char titleid[12];
    sceAppMgrAppParamGetString(0, 12, titleid, 12);

    if (!strncmp(titleid, "NPXS10028", 9)) {

        tai_module_info_t tai_info;
        tai_info.size = sizeof(tai_module_info_t);
        taiGetModuleInfo("ScePspemu", &tai_info);

        sub_810053F8_hookid = taiHookFunctionOffset(
                &sub_810053F8_hookref,
                tai_info.modid,
                0, 0x53F8, 1,
                sub_810053F8_patched);

    } else {

        load_playtime(titleid);
    }

    SceUID tracker_thread_id = sceKernelCreateThread("TrackPlugX", tracker_thread, 0x10000100, 0x10000, 0, 0, NULL);
    if (tracker_thread_id >= 0)
        sceKernelStartThread(tracker_thread_id, 0, NULL);

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

    if (sub_810053F8_hookid >= 0) {
        taiHookRelease(sub_810053F8_hookid, sub_810053F8_hookref);
    }

    return SCE_KERNEL_STOP_SUCCESS;
}
