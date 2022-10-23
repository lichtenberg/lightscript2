/*  *********************************************************************
    *  LightScript - A script processor for LED animations
    *  
    *  Picolight Playback
    *  
    *  Author:  Mitch Lichtenberg
    ********************************************************************* */


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

#include <vector>
#include <string>
#include "schedule.hpp"
#include "symtab.hpp"

#include "lightscript.h"
#include "musicplayer.h"

#define MSGSIZE 16

static int device = -1;
static time_t epoch;
static int offAnim = 0;
double start_offset = 0;
static bool play_please_stop = false;

static void msleep(int ms)
{
    struct timespec ts;

    ts.tv_sec = 0;
    ts.tv_nsec = ms * 1000000;
    nanosleep(&ts,NULL);
}

static double current_time(LSScript_t *script)
{
    struct timeval tv;

    gettimeofday(&tv,NULL);

    return ((double) (tv.tv_sec - epoch)) + ((double)(tv.tv_usec)/1000000.0);
}


static int send_command(lsmessage_t *msg)
{
    uint8_t sync[2];
    int txlen;

    if (device <= 0) {
        return -1;
    }

    sync[0] = 0x02;
    sync[1] = 0xAA;
    if (write(device, sync, sizeof(sync)) != sizeof(sync)) {
        perror("Write Error to Picolight [sync]");
        exit(1);
    }

    txlen = LSMSG_HDRSIZE + msg->ls_length;

    if (write(device, msg, txlen) != txlen) {
        perror("Write Error to Picolight [cmd]");
        exit(1);
    }

    return 0;
}

static int readdata(int device, uint8_t *buf, int len)
{
    int res;
    int ttl = 0;

    while (len > 0) {
        res = read(device, buf, len);
        if (res <= 0) {
            printf("Read error from PicoLight: %d\n",res);
            exit(1);
        }
        buf += res;
        len -= res;
        ttl += res;
    }

    return ttl;
}


#define STATE_SYNC1 0
#define STATE_SYNC2 1
static int recv_response(lsmessage_t *msg)
{
    uint8_t b;
    int res;
    int reading = 1;
    int state = STATE_SYNC1;

    if (device <= 0) {
        return -1;
    }

    #if 0
    for (;;) {
        if (readdata(device,&b,1) == 1) { printf("%02X ",b); fflush(stdout);}
        else {
            printf("read error\n");
            exit(1);
        }
    }
    #endif

    while (reading) {
        if (readdata(device,&b,1) < 1) {
            printf("Read error from Picolight [sync]\n");
            exit(1);
        }
        switch (state) {
            case STATE_SYNC1:
                if (b == 0x02) {
                    state = STATE_SYNC2;
                }
                break;
            case STATE_SYNC2:
                if (b == 0xAA) {
                    reading = 0;
                } else {
                    state = STATE_SYNC1;
                }
        }
    }

    memset(msg,0,sizeof(lsmessage_t));

    if (readdata(device,(uint8_t *) msg,LSMSG_HDRSIZE) != LSMSG_HDRSIZE) {
        printf("Read error from Picolight [hdr]\n");
            exit(1);
    }

    int rxlen = msg->ls_length;

    if (rxlen != 0) {
        if ((res = readdata(device,(uint8_t *) &(msg->info), rxlen)) != rxlen) {
            printf("Read error from Picolight [payload] %d\n",res);
            exit(1);
        }
    }

    return 0;
            
}

static void check_version(void)
{
    lsmessage_t msg;

    msg.ls_command = LSCMD_VERSION;
    msg.ls_length = 0;

    send_command(&msg);
    recv_response(&msg);

    printf("Protocol version: %u     Firmware Version %u.%u.%u\n",
           msg.info.ls_version.lv_protocol,
           msg.info.ls_version.lv_major,
           msg.info.ls_version.lv_minor,
           msg.info.ls_version.lv_eco);
}

static void send_animate(uint32_t *strips, uint16_t anim,  uint16_t speed, uint16_t option, uint32_t color)
{
    lsmessage_t msg;

    memset(&msg,0,sizeof(msg));

    for (int i = 0; i < MAXVSTRIPS/32; i++) {
        msg.info.ls_animate.la_strips[i] = strips[i];
    }

    msg.info.ls_animate.la_anim = anim;
    msg.info.ls_animate.la_speed = speed;
    msg.info.ls_animate.la_option = option;
    msg.info.ls_animate.la_color = color;
    msg.ls_length = sizeof(lsanimate_t);
    msg.ls_command = LSCMD_ANIMATE;

    send_command(&msg);
}

static void upload_config(LSScript_t *script)
{
    lsmessage_t txMessage;
    lsmessage_t rxMessage;
    int i;

    printf("Retting panel\n");
    // Send a RESET command
    memset(&txMessage,0,sizeof(txMessage));
    txMessage.ls_command = LSCMD_RESET;
    txMessage.ls_length = 0;
    send_command(&txMessage);
    recv_response(&rxMessage);

    printf("Sending physical strips\n");
    // Send over the physical strips
    for (i = 0; i < MAXPSTRIPS; i++) {
        uint32_t info = script->physicalStrips[i].info;
        if (PSTRIP_COUNT(info) > 0) {
            memset(&txMessage,0,sizeof(txMessage));
            txMessage.ls_command = LSCMD_SETPSTRIP;
            txMessage.ls_length = sizeof(lspstrip_t);
            txMessage.info.ls_pstrip.lp_pstrip = info;
            send_command(&txMessage);
            recv_response(&rxMessage);
        }
    }
            

    printf("Sending virtual strips\n");
    // Send over the logical strips
    for (i = 0; i < script->virtualStripCount; i++) {
        VStrip_t *vstrip = &script->virtualStrips[i];
        memset(&txMessage,0,sizeof(txMessage));
        txMessage.ls_command = LSCMD_SETVSTRIP;
        txMessage.ls_length = sizeof(lsvstrip_t);
        txMessage.info.ls_vstrip.lv_idx = i;
        txMessage.info.ls_vstrip.lv_count = vstrip->substripCount;
        memcpy(txMessage.info.ls_vstrip.lv_substrips,
               vstrip->substrips,
               vstrip->substripCount * sizeof(uint32_t));
        send_command(&txMessage);
        recv_response(&rxMessage);
    }
            
    printf("Initializing panel with new config\n");
    // Send the INIT command
    memset(&txMessage,0,sizeof(txMessage));
    txMessage.ls_command = LSCMD_INIT;
    txMessage.ls_length = 0;
    send_command(&txMessage);
    recv_response(&rxMessage);
}


static void play_events(LSScript_t *script,LSSchedule *sched)
{
    int idx = 0;
    int events = sched->size();
    double start_time;

    start_time = current_time(script) + start_offset;

    if (script->lss_startcue != 0.0) {
        while (idx < events) {
            schedcmd_t *cmd = sched->getAt(idx);
            if (cmd->time < script->lss_startcue) {
                idx++;
            } else {
                break;
            }
        }
    }

    while (!play_please_stop && (idx < events)) {
        double now;

        schedcmd_t *cmd = sched->getAt(idx);

        // Figure out the difference between the time stamp
        // at the start and now.
        now = current_time(script) - start_time + script->lss_startcue;

        // If the current time is past the script command's time,
        // do the command.

        if (now >= cmd->time) {
            unsigned int anim;

            anim = cmd->animation;
            if (cmd->direction) anim |= 0x8000;
            
            sched->printSchedEntry(cmd);
            if (cmd->comment == NULL) {
                send_animate(cmd->stripmask, anim, cmd->speed, cmd->option, cmd->palette);
            }

            idx++;
        }

        if ((script->lss_endcue != 0) && (now > (script->lss_endcue))) {
            break;
        }
    }

}
#if 1

static int musicpos;
static int musicend;
static LSScript_t *curscript;
static LSSchedule *cursched;

int player_callback(double now)
{
    // If the current time is past the script command's time,
    // do the command.

    if (musicpos == musicend) {
        // End of script, stop playing
        return 0;
    }

    if (play_please_stop) {
        return 0;
    }

    schedcmd_t *cmd = (schedcmd_t *) cursched->getAt(musicpos);

    if ((curscript->lss_endcue != 0) && (now >= curscript->lss_endcue)) {
        // Past end cue, stop
        return 0;
    }

    if (now >= cmd->time) {
        unsigned int anim;

        anim = cmd->animation;
        if (cmd->direction) anim |= 0x8000;
            
        cursched->printSchedEntry(cmd);
        if (cmd->comment == NULL) {
            send_animate(cmd->stripmask, anim, cmd->speed, cmd->option, cmd->palette);
        }

        musicpos++;
    }

    // Keep going
    return 1;

}

extern int playMusicFile(const char *name, int (*callback)(double),double start_cue);

static void play_music(LSScript_t *script,LSSchedule *sched)
{
    curscript = script;
    cursched = sched;
    
    musicpos = 0;
    musicend = sched->size();

    // Seek in script to cue point

    if (script->lss_startcue != 0.0) {
        while (musicpos != musicend) {
            schedcmd_t *cmd = sched->getAt(musicpos);
            if (cmd->time > script->lss_startcue) break;
            musicpos++;
        }

        // We started past the end of the script, bail.
        if (musicpos == musicend) {
            return;
        }
    }

    char *path = realpath(script->lss_music.c_str(), NULL);
    printf("Playing: %s\n",path);
    playMusicFile(path, player_callback, script->lss_startcue);
}
#endif

static void play_idle(void)
{
    int v;
    uint32_t mask[MAXVSTRIPS/32];

    if (curscript->lss_idlestrips) {
        try {
            cursched->stripMask(NULL,curscript->lss_idlestrips,mask);
        } catch (int e) {
            return;
        }
    }

    if (curscript->lss_idleanimation != "") {
        if (curscript->animTable->findSym(curscript->lss_idleanimation,v)) {
            send_animate(mask, v, 500, 0, 0);
        } else {
            printf("Warning: idle animation '%s' is not valid\n",curscript->lss_idleanimation.c_str());
        }
    }
}


static void all_off(void)
{
    uint32_t mask[MAXVSTRIPS/32];

    memset(mask,0,sizeof(mask));
    mask[0] = 0x7FFFFFFF;
    
    // Send "OFF" to everyone, then wait 200ms.
    if (offAnim) {
        send_animate(mask, offAnim, 500, 0, 0);
        msleep(200);
    } 
}

int play_opendevice(char *devname)
{
    if (devname != NULL) {
        device = open(devname,O_RDWR);

        if (device < 0) {
            fprintf(stderr,"Error: Could not open Picolight device %s: %s\n",devname, strerror(errno));
            return -1;
        }
    } else {
        device = -1;             // No device, just pretend.
    }

    return 0;
}

void play_closedevice(void)
{
    if (device > 0) {
        close(device);
        device = -1;
    }
}

void play_initdevice(LSScript_t *script)
{
    check_version();
    upload_config(script);
//    exit(1);
}



void play_script(int how)
{
    play_please_stop = false;

    all_off();
    play_idle();

    printf("\n\n");
    printf("Press ENTER to start playback\n"); getchar();

    time(&epoch);
    
    if (how == 0) {
        play_events(curscript, cursched);
    } else {
        play_music(curscript, cursched);
    }

    msleep(500);

    all_off();
    play_idle();

    msleep(200);
    
    if (device != -1) {
        close(device);
    }
}

void play_init(LSScript_t *script, LSSchedule *sched)
{
    std::string offStr = "OFF";
    int v;

    curscript = script;
    cursched = sched;

    // Send "OFF" to everyone, then wait 200ms.
    if (script->animTable->findSym(offStr,v)) {
        offAnim = v;
    }

    play_please_stop = false;
}


void play_interrupt(void)
{
    printf("[stopping]\n");
    play_please_stop = true;
}


