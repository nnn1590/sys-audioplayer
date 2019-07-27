#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>

#include <switch.h>

#include "mp3.h"
#include "util.h"

bool onboot = false;

#define ERPT_SAVE_ID 0x80000000000000D1
#define TITLE_ID 0x4200000000000000
#define HEAP_SIZE 0x000320000
extern "C"
{
    // we aren't an applet
u32 __nx_applet_type = AppletType_None;

// setup a fake heap (we don't need the heap anyway)
char fake_heap[HEAP_SIZE];


// we override libnx internals to do a minimal init
void __libnx_initheap(void)
{
    extern char *fake_heap_start;
    extern char *fake_heap_end;

    // setup newlib fake heap
    fake_heap_start = fake_heap;
    fake_heap_end = fake_heap + HEAP_SIZE;
}

void __attribute__((weak)) __appInit(void)
{
    Result rc;
    svcSleepThread(10000000000L);

    rc = smInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));
        
    /*
    rc = timeInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_Time));

    __libnx_init_time();
    */

    rc = hidInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_HID));

    rc = fsInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));

    // init vsync events?
    rc = viInitialize(ViServiceType_System);
    if (R_FAILED(rc))
        fatalSimple(rc);

    // need this to get pId
    rc = pmdmntInitialize();
    if (R_FAILED(rc)) {
        fatalSimple(rc);
    }

    // need this to get applicationid
    rc = pminfoInitialize();
    if (R_FAILED(rc))
        fatalSimple(rc);

    // setting hos version because apparently it changes some functions
    rc = setsysInitialize();
    if (R_SUCCEEDED(rc)) {
        SetSysFirmwareVersion fw;
        rc = setsysGetFirmwareVersion(&fw);
        if (R_SUCCEEDED(rc))
            hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
        setsysExit();
    }
    rc = nsInitialize();
    if (R_FAILED(rc))
        fatalSimple(rc);
    
    fsdevMountSdmc();
}

void __attribute__((weak)) __appExit(void)
{
    fsdevUnmountAll();
    fsExit();
    smExit();
    hidExit();
    audoutExit();
    //timeExit();
    pmdmntExit();
    nsExit();
}
}

u64 GetActiveApplicationProcessID()
{
    u64 pid;
    pmdmntGetApplicationPid(&pid);
    return pid;
}

u64 GetActiveTitleID()
{
    u64 title;
    u64 pid;
    pid = GetActiveApplicationProcessID();
    pminfoGetTitleId(&title, pid);
    return title;
}

void writelog(char *input)
{
    FILE *f = fopen("config/sys-audioplayer/last_played.log", "w");
    fprintf(f, input);
    fclose(f);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    //FILE *f = fopen("/log", "w");
    //stdout = f;
    //stderr = f;
    FILE *filetitle;
    FILE *fileplayonboot;
    int i = 0, k;
    char NameArr[20][256];

        
    DIR *dir;
    struct dirent *ent;
    char* flag_path = "/config/sys-audioplayer/titles"; 
    char titleid_flag_path[50];
    sprintf(titleid_flag_path, "%s/%016lx", flag_path, GetActiveTitleID());
    //WriteOut(titleid_flag_path);
    
	char* path_titleid_flag = "config/sys-audioplayer/flags/titleid.flag.on";
    char* play_on_boot_flag = "config/sys-audioplayer/flags/playonboot.flag.on";
    dir = opendir(titleid_flag_path);

    if ((filetitle = fopen(path_titleid_flag, "r")))
    {
        dir = opendir(titleid_flag_path);
        fclose(filetitle);
    }
    else
    {
        dir = opendir("/music");
    }

    while ((ent = readdir(dir)))
    {
        printf(ent->d_name);
        char filename[500];
        char name[500];
        
        strcpy(NameArr[i], ent->d_name);
        i++;
        
        

        if ((filetitle = fopen(path_titleid_flag, "r")))
        {
            snprintf(filename, 1000, "%s/%s", titleid_flag_path, ent->d_name);
            writelog(filename);
            fclose(filetitle);
        }
        else
        {
            snprintf(filename, 263, "/music/%s", ent->d_name);
        }

        if ((fileplayonboot = fopen(play_on_boot_flag, "r")))
        {
            onboot = true;
            fclose(fileplayonboot);
        }
        
        if (onboot)
        {
            playMp3(filename);
        }
        else
        {
            /* code */
        }
        
        
        
    writelog(filename);
    }
    closedir(dir);

    //fclose(path_titleid_flag);
    //fclose(path_titleid);
    //fclose(f);

    return 0;
}
