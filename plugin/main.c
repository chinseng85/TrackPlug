#include <vitasdk.h>
#include <taihen.h>
#include <kuio.h>
#include <libk/string.h>
#include <libk/stdio.h>

static SceUID hook;
static tai_hook_ref_t ref;

static char real_titleid[16];
static char titleid[128];
static char full_title[128];

static char fname[256];
static char fname_title[256];

static uint64_t playtime = 0;
static uint64_t tick = 0;

static char playtime_loaded = 0;

static int (* ScePspemuConvertAddress)(uint32_t addr, int mode, uint32_t cache_size);
#define ADRENALINE_ADDRESS 0xABCDE000
#define ADRENALINE_SIZE 0x2000

void load_playtime() {
	// Getting current playtime
	SceUID fd;
	sprintf(fname, "ux0:/data/TrackPlug/%s.bin", titleid);
	kuIoOpen(fname, SCE_O_RDONLY, &fd);
	if (fd >= 0){
		kuIoRead(fd, &playtime, sizeof(uint64_t));
		kuIoClose(fd);
	} else {
		playtime = 0;
	}
	playtime_loaded = 1;
}

int get_pspemu_title() {
	void *adrenaline = (void *)ScePspemuConvertAddress(ADRENALINE_ADDRESS, 0x1/*KERMIT_INPUT_MODE*/, ADRENALINE_SIZE);
	if (adrenaline <= 0)
		return 0;

	char *title = adrenaline + 0x18;

	if (title[0] == 0 || strncmp(title, "XMB", 3) == 0)
		return 0;

	// Game changed?
	if (strcmp(full_title, title) != 0) {
		playtime_loaded = 0;
	}

	strcpy(full_title, title);

	titleid[0] = 'P';
	titleid[1] = 'S';
	titleid[2] = 'P';
	titleid[3] = '_';
	int j = 4, i = 0;

	while (j < sizeof(titleid) - 1 && title[i] != 0) {
		if ((title[i] >= 'A' && title[i] <= 'Z') ||
			(title[i] >= 'a' && title[i] <= 'z') ||
			(title[i] >= '0' && title[i] <= '9')) {
			titleid[j++] = title[i];
		}
		i++;
	}

	// Pad to min 9 chars, prevent app crashes
	while (j < 9) {
		titleid[j++] = 'x';
	}

	titleid[j] = 0;

	return 1;
}

void write_pspemu_title() {
	char buffer[128];
	snprintf(buffer, sizeof(buffer), "[PSP] ");
	int i = 0, j = 6;
	while (j < sizeof(buffer) && full_title[i] != 0) {
		// Strip invalid chars
		if (full_title[i] >= 32 && full_title[i] <= 126) {
			buffer[j++] = full_title[i];
		}
		i++;
	}
	buffer[j++] = 0;

	SceUID fd;
	sprintf(fname_title, "ux0:/data/TrackPlugArchive/%s.txt", titleid);
	kuIoOpen(fname_title, SCE_O_RDONLY, &fd);
	if (fd < 0){
		kuIoOpen(fname_title, SCE_O_WRONLY|SCE_O_CREAT, &fd);
		if (fd >= 0) {
			kuIoWrite(fd, buffer, strlen(buffer));
			kuIoClose(fd);
		}
	}
}

int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync) {

	uint64_t t_tick = sceKernelGetProcessTimeWide();

	// Saving playtime every 10 seconds
	if ((t_tick - tick) > 10000000){
		// If in PspEmu
		if (strcmp(real_titleid, "NPXS10028") == 0) {
			if (get_pspemu_title()) {
				// New game?
				if (!playtime_loaded) {
					load_playtime();
					write_pspemu_title();
				}
			} else {
				// No game running
				playtime_loaded = 0;
			}
		}

		if (!playtime_loaded) {
			goto exit;
		}

		tick = t_tick;
		playtime += 10;
		SceUID fd;
		kuIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, &fd);
		kuIoWrite(fd, &playtime, sizeof(uint64_t));
		kuIoClose(fd);
	}

exit:
	return TAI_CONTINUE(int, ref, pParam, sync);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {

	// Getting game Title ID
	sceAppMgrAppParamGetString(0, 12, real_titleid , 256);

	// PspEmu game?
	if (strcmp(real_titleid, "NPXS10028") == 0) {
		// Get ScePspemu tai module info
		tai_module_info_t ScePspemu_tai_info;
		ScePspemu_tai_info.size = sizeof(tai_module_info_t);
		taiGetModuleInfo("ScePspemu", &ScePspemu_tai_info);

		// Module info
		SceKernelModuleInfo ScePspemu_mod_info;
		ScePspemu_mod_info.size = sizeof(SceKernelModuleInfo);
		sceKernelGetModuleInfo(ScePspemu_tai_info.modid, &ScePspemu_mod_info);

		// Addresses
		uint32_t text_addr = (uint32_t)ScePspemu_mod_info.segments[0].vaddr;
		ScePspemuConvertAddress = (void *)(text_addr + 0x6364 + 0x1);
	} else {
		strcpy(titleid, real_titleid);
		load_playtime();
	}

	// Getting starting tick
	tick = sceKernelGetProcessTimeWide();

	hook = taiHookFunctionImport(&ref,
				     TAI_MAIN_MODULE,
				     TAI_ANY_LIBRARY,
				     0x7A410B64,
				     sceDisplaySetFrameBuf_patched);

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
	return SCE_KERNEL_STOP_SUCCESS;
}
