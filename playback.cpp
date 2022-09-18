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

typedef struct __attribute__((packed)) lsmessage_s {
    uint8_t     ls_sync[2];
    uint16_t    ls_reserved;
    uint16_t    ls_anim;
    uint16_t    ls_speed;
    uint16_t    ls_option;
    uint32_t    ls_color;
    uint32_t    ls_strips;
} lsmessage_t;

void send_message(unsigned int strips, unsigned int anim,  unsigned int speed, unsigned int option, unsigned int palette)
{
    lsmessage_t msg;

    if (device <= 0) {
        return;
    }

    msg.ls_sync[0] = 0x02;
    msg.ls_sync[1] = 0xAA;
    msg.ls_strips = strips;
    msg.ls_anim = anim;
    msg.ls_speed = speed;
    msg.ls_option = option;
    msg.ls_color = palette;
    msg.ls_reserved = 0;

    if (write(device, &msg, sizeof(msg)) != sizeof(msg)) {
        perror("Write Error to Picolight");
        exit(1);
    }
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
            send_message(cmd->stripmask, anim, cmd->speed, cmd->option, cmd->palette);

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
        send_message(cmd->stripmask, anim, cmd->speed, cmd->option, cmd->palette);

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
    uint64_t mask = 0;

    if (curscript->lss_idlestrips) {
        try {
            mask = cursched->stripMask(NULL,curscript->lss_idlestrips);
        } catch (int e) {
            return;
        }
    }

    if (curscript->lss_idleanimation != "") {
        if (curscript->animTable->findSym(curscript->lss_idleanimation,v)) {
            send_message((uint32_t) mask, v, 500, 0, 0);
        } else {
            printf("Warning: idle animation '%s' is not valid\n",curscript->lss_idleanimation.c_str());
        }
    }
}


static void all_off(void)
{
    // Send "OFF" to everyone, then wait 200ms.
    if (offAnim) {
        send_message(0x7FFFFFFF, offAnim, 500, 0, 0);
        msleep(200);
    } 
}


void play_script(char *devname, int how)
{
    play_please_stop = false;

    if (devname != NULL) {
        device = open(devname,O_RDWR);

        if (device < 0) {
            fprintf(stderr,"Error: Could not open Picolight device %s: %s\n",devname, strerror(errno));
            return;
        }
    } else {
        device = -1;             // No device, just pretend.
    }

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


