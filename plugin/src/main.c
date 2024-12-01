#include <vitasdkkern.h>
#include <taihen.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <psp2kern/kernel/debug.h> 
#include <psp2kern/kernel/modulemgr.h>

#include "main.h"

int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);
int ksceSblACMgrIsPspEmu(SceUID pid);

int patch_netrecv(void) {
	SceUID module_id;
	void *patch_point;
	char inst[0x20];
	module_id = ksceKernelSearchModuleByName("SceNetPs");
	module_get_offset(0x10005, module_id, 0, 0x6986, (uintptr_t *)&patch_point);
	memcpy(inst, patch_point, 0x1E);
	memcpy(&(inst[0x0]), (const char[4]){0xDD, 0xF8, 0x30, 0xC0}, 4);
	memcpy(&(inst[0xC]), (const char[4]){0xC4, 0xE9, 0x07, 0xC5}, 4);
	memcpy(&(inst[0x16]), (const char[4]){0xC4, 0xE9, 0x04, 0x55}, 4);
	taiInjectDataForKernel(0x10005, module_id, 0, 0x6986, inst, 0x1E);
	return 0;
}

static SceUID g_thread_uid = -1;
static bool g_thread_run = true;

static uint32_t *SceAppMgr_mutex_uid;
static uint32_t *SceAppMgr_app_list;

static char g_sfo_cache_titleid[TITLEID_LEN] = "";
static char g_sfo_cache_title[TITLE_LEN] = "";

static int extract_sfo_title(char *out_title, const char *bubbleid) {
    char sfo_path[128];
    snprintf(sfo_path, 128, "ux0:app/%s/sce_sys/param.sfo", bubbleid);
    snprintf(out_title, TITLE_LEN, "%s", bubbleid);

    int ret = 0;
    int fd = ksceIoOpen(sfo_path, SCE_O_RDONLY, 0777);
    if (fd < 0)
        return 0;

    char key[6];

    sfo_header_t hdr;
    ret = ksceIoRead(fd, &hdr, sizeof(sfo_header_t));
    if (ret != sizeof(sfo_header_t) || hdr.magic != 0x46535000) {
        goto ERR_CLOSE;
    }

    sfo_entry_t entry;
    for (uint32_t i = 0; i < hdr.indexTableEntries; i++) {
        ret = ksceIoPread(fd, &entry, sizeof(sfo_entry_t), sizeof(sfo_header_t) + sizeof(sfo_entry_t) * i);
        if (ret != sizeof(sfo_entry_t))
            goto ERR_CLOSE;

        ret = ksceIoPread(fd, key, 5, hdr.keyTableOffset + entry.keyOffset);
        if (ret != 5)
            goto ERR_CLOSE;

        if (strncmp(key, "TITLE", 5))
            continue;

        ret = ksceIoPread(fd, out_title, TITLE_LEN, hdr.dataTableOffset + entry.dataOffset);
        if (ret != TITLE_LEN)
            goto ERR_CLOSE;

        ksceIoClose(fd);
        return 1;
    }

ERR_CLOSE:
    ksceIoClose(fd);
    return 0;
}

static int extract_sfo_contentid(char *out_title, const char *bubbleid) {
    char sfo_path[128];
    snprintf(sfo_path, 128, "ux0:app/%s/sce_sys/param.sfo", bubbleid);
    snprintf(out_title, TITLE_LEN, "%s", bubbleid);

    int ret = 0;
    int fd = ksceIoOpen(sfo_path, SCE_O_RDONLY, 0777);
    if (fd < 0)
        return 0;

    char key[11];

    sfo_header_t hdr;
    ret = ksceIoRead(fd, &hdr, sizeof(sfo_header_t));
    if (ret != sizeof(sfo_header_t) || hdr.magic != 0x46535000) {
        goto ERR_CLOSE;
    }

    sfo_entry_t entry;
    for (uint32_t i = 0; i < hdr.indexTableEntries; i++) {
        ret = ksceIoPread(fd, &entry, sizeof(sfo_entry_t), sizeof(sfo_header_t) + sizeof(sfo_entry_t) * i);
        if (ret != sizeof(sfo_entry_t))
            goto ERR_CLOSE;

        ret = ksceIoPread(fd, key, 10, hdr.keyTableOffset + entry.keyOffset);
        if (ret != 10)
            goto ERR_CLOSE;

        if (strncmp(key, "CONTENT_ID", 10))
            continue;

        ret = ksceIoPread(fd, out_title, CONTENTID_LEN, hdr.dataTableOffset + entry.dataOffset);
        if (ret != CONTENTID_LEN)
            goto ERR_CLOSE;

        ksceIoClose(fd);
        return 1;
    }

ERR_CLOSE:
    ksceIoClose(fd);
    return 0;
}

// If there's a better way of obtaining foreground app info, please do let me know
static int get_fg_app(char *out_titleid, char *out_title, char *out_contentid) {
    out_titleid[0] = '\0';
    out_title[0] = '\0';
    out_contentid[0] = '\0';

    int ret = ksceKernelLockMutex(*SceAppMgr_mutex_uid, 1, 0);
    if (ret < 0)
        return 0;

    uint32_t pcurrent;
    for (int i = 0; i < 20; i++) {
        pcurrent = (uint32_t)SceAppMgr_app_list + (i * 0x2410);

        SceUID pid = APP_LIST_GET_PID(pcurrent);
        if (pid <= 0)
            continue;

        uint32_t state = APP_LIST_GET_STATE(pcurrent);
        if (state != APP_RUNNING)
            continue;

        bool is_pspemu = ksceSblACMgrIsPspEmu(pid);
        const char *titleid = (const char *)(APP_LIST_GET_TITLEID(pcurrent));
        const char *bubbleid = (const char *)(APP_LIST_GET_BUBBLEID(pcurrent));

        // Filter out bg stuff
        if (!strncmp(titleid, "NPXS", 4) &&
                (!strncmp(&titleid[4], "10079", 5) ||     // Daily Checker BG
                 !strncmp(&titleid[4], "10063", 5))) { // MsgMW
            continue;
        }

        // PspEmu launched through Adrenaline
        if (is_pspemu && !strncmp(bubbleid, "PSPEMUCFW", 9)) {
            SceAdrenaline adrenaline;
            ksceKernelMemcpyUserToKernelForPid(pid, &adrenaline, (uintptr_t)0x73CDE000, sizeof(SceAdrenaline));

            if (adrenaline.titleid[0] == '\0' || !strncmp(adrenaline.titleid, "XMB", 3)) {
                snprintf(out_titleid, TITLEID_LEN, "XMB");
                snprintf(out_title, TITLE_LEN, "Adrenaline XMB Menu");
            } else {
                memcpy(out_titleid, adrenaline.titleid, TITLEID_LEN); out_titleid[TITLEID_LEN - 1] = '\0';
                memcpy(out_title, adrenaline.title, TITLE_LEN); out_title[TITLE_LEN - 1] = '\0';
            }
        }
        // PspEmu launched through custom bubble
        else if (is_pspemu) {
            // Check if title is cached
            if (!strncmp(g_sfo_cache_titleid, bubbleid, 9)) {
                snprintf(out_titleid, TITLEID_LEN, "%s", g_sfo_cache_titleid);
                snprintf(out_title, TITLE_LEN, "%s", g_sfo_cache_title);
            }
            // If not, extract title from SFO
            else {
                extract_sfo_title(out_title, bubbleid);
                snprintf(out_titleid, TITLEID_LEN, "%s", bubbleid);

                // Update cache
                memcpy(g_sfo_cache_titleid, out_titleid, TITLEID_LEN);
                memcpy(g_sfo_cache_title, out_title, TITLE_LEN);
            }
        }
        // PSVita game/app
        else {
            snprintf(out_titleid, TITLEID_LEN, "%s", titleid);
            extract_sfo_title(out_title, bubbleid);
            extract_sfo_contentid(out_contentid, bubbleid);
        }

        ksceKernelUnlockMutex(*SceAppMgr_mutex_uid, 1);
        return i + 1;
    }

    ksceKernelUnlockMutex(*SceAppMgr_mutex_uid, 1);
    return 0;
}

static int vitapresence_thread(SceSize args, void *argp) {
    SceUID server_sockfd = -1;
    SceUID client_sockfd = -1;

    SceNetSockaddrIn clientaddr;
    SceNetSockaddrIn serveraddr;
    serveraddr.sin_family = SCE_NET_AF_INET;
    serveraddr.sin_addr.s_addr = SCE_NET_INADDR_ANY;
    serveraddr.sin_port = ksceNetHtons(VITAPRESENCE_PORT);

    unsigned int addrlen = sizeof(SceNetSockaddrIn);
    int ret = 0;

    static vitapresence_data_t presence_data;

    while (g_thread_run) {
        server_sockfd = ksceNetSocket("vitapresence_socket", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);
        if (server_sockfd < 0)
            return 0;

        do {
            ksceKernelDelayThread(2 * 1000 * 1000);

            ret = ksceNetBind(server_sockfd, (SceNetSockaddr *)&serveraddr, sizeof(SceNetSockaddrIn));
            if (ret < 0)
                continue;

            ret = ksceNetListen(server_sockfd, 128);
            if (ret < 0)
                continue;
        } while (ret < 0);

        while (g_thread_run && ret >= 0) {
            client_sockfd = ksceNetAccept(server_sockfd, (SceNetSockaddr *)&clientaddr, &addrlen);
            if (client_sockfd < 0)
                break;
            
            char buf[BUFFER_SIZE];
            int32_t n = ksceNetRecv(client_sockfd, buf, BUFFER_SIZE - 1, 0);
            if (n > 0)
            {
                buf[n] = '\0';
                ksceDebugPrintf("Client msg:\n%s\n", buf);
            }
            else
                ksceDebugPrintf("error retrieving data: 0x%08X\n", n);

            get_fg_app(presence_data.titleid, presence_data.title, presence_data.contentid);
            if (strncmp(buf, "GET /icon0.png", 14) == 0)
            {
                const char * response_image = 
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Connection: close\r\n\r\n"
                    "<!DOCTYPE html>"
                    "<html>"
                    "<head><title>Image</title></head>"
                    "<body>"
                    "<p>work in progress lmao</p>"
                    "</body>"
                    "</html>";
                ret = ksceNetSend(client_sockfd, response_image, sizeof(response_image), 0);
                ksceNetSocketClose(client_sockfd);
            }
            else
            {
                const char * response_template = 
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Connection: close\r\n\r\n"
                    "<!DOCTYPE html>"
                    "<html>"
                    "<head><title>Presence</title></head>"
                    "<body>"
                    "<p>%s</p>"
                    "<P>%s</p>"
                    "<img src=\"icon0.png\" width=\"128\" height=\"128\">"
                    "</body>"
                    "</html>";
                SceSize html_size = strlen(response_template) + (strlen(presence_data.titleid) * 2) + strlen(presence_data.title);
                SceUID response_uid = ksceKernelAllocMemBlock("presence", SCE_KERNEL_MEMBLOCK_TYPE_KERNEL_RW, 0x0020000, NULL);
                char * response_header;
                if (response_uid < 0)
                {
                    ksceDebugPrintf("Failed to allocate memblock with code 0x%08X\n", response_uid);
                    break;
                }
                ksceKernelGetMemBlockBase(response_uid, (void **)&response_header);
                snprintf(response_header, html_size, response_template, presence_data.titleid, presence_data.title, presence_data.titleid);
                ret = ksceNetSend(client_sockfd, response_header, html_size, 0);
                ksceNetSocketClose(client_sockfd);
                ksceKernelFreeMemBlock(response_uid);
            }
        }

        ksceNetSocketClose(server_sockfd);
    }

    return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
    patch_netrecv();
    tai_module_info_t tai_info;
    tai_info.size = sizeof(tai_module_info_t);
    if (taiGetModuleInfoForKernel(KERNEL_PID, "SceAppMgr", &tai_info) < 0)
        return SCE_KERNEL_START_NO_RESIDENT;

    module_get_offset(KERNEL_PID, tai_info.modid, 1, 0x4A4, (uintptr_t *)&SceAppMgr_mutex_uid);
    module_get_offset(KERNEL_PID, tai_info.modid, 1, 0x500, (uintptr_t *)&SceAppMgr_app_list);

    g_thread_uid = ksceKernelCreateThread("vitapresence_thread", vitapresence_thread, 0x3C, 0x1000, 0, 0x10000, 0);
    if (g_thread_uid < 0)
        return SCE_KERNEL_START_NO_RESIDENT;

    ksceKernelStartThread(g_thread_uid, 0, NULL);
    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
    if (g_thread_uid >= 0) {
        g_thread_run = false;
        ksceKernelWaitThreadEnd(g_thread_uid, NULL, NULL);
        ksceKernelDeleteThread(g_thread_uid);
    }

    return SCE_KERNEL_STOP_SUCCESS;
}
