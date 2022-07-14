#include <vector>
#include <string>
#include "schedule.hpp"
#include "symtab.hpp"



LSSchedule::LSSchedule(LSScript_t *s)
{
    script = s;
}


LSSchedule::~LSSchedule()
{

}

#if 0
static const char *lsctypes[] = {
    [LSC_UNKNOWN] = "UNKNOWN",
    [LSC_CASCADE] = "CASCADE",
    [LSC_DO] = "DO",
    [LSC_MACRO] = "MACRO",
    [LSC_COMMENT] = "COMMENT",
};
#endif

schedcmd_t *LSSchedule::newSchedCmd(double baseTime, LSCommand_t *cmd)
{
    // Create a new empty schedule record.
    schedcmd_t *scmd = new schedcmd_t;

    memset(scmd,0,sizeof(schedcmd_t));

    // Fill in what we know.

    if (cmd) {
        scmd->time = baseTime + cmd->lsc_from;
        scmd->line = cmd->lsc_line;
        scmd->speed = cmd->opt_speed;
        scmd->brightness = cmd->opt_brightness;
        scmd->direction = cmd->opt_reverse ? 1 : 0;
        scmd->option = cmd->opt_option;
    }
    
    //
    return scmd;
}

void LSSchedule::stripVec1(LSCommand_t *c, stripvec_t *vec, idlist_t *list)
{
    int v;
    idlist_t::iterator i;
    idlist_t *sublist;

    for (i = list->begin();  i < list->end(); i++) {

        if (script->stripListTable->findStripList(*i, sublist)) {
            if (nestLevel > 8) {
                printf("[Line %d]: Strip lists nested too deep, are you putting a list in itself?\n", c->lsc_line);
            } else {
                nestLevel++;
                stripVec1(c, vec,sublist);
                nestLevel--;
            }
        } else if (script->stripTable->findSym(*i, v)) {
            vec->push_back(v);
        } else {
            printf("[Line %d]: Could not find strip name: '%s'\n",c->lsc_line, i->c_str());
            throw -1;
        }
    }

}


stripvec_t *LSSchedule::stripVec(LSCommand_t *c, idlist_t *list)
{
    stripvec_t *vec = new stripvec_t;
    nestLevel = 0;
    stripVec1(c, vec, list);
    return vec;
}



uint64_t LSSchedule::stripMask(LSCommand_t *c, idlist_t *list)
{
    uint64_t mask = 0LL;
    int v;
    idlist_t::iterator i;
    idlist_t *sublist;

    nestLevel++;

    for (i = list->begin();  i < list->end(); i++) {

        if (script->stripListTable->findStripList(*i, sublist)) {
            if (nestLevel > 8) {
                printf("[Line %d]: Strip lists nested too deep, are you putting a list in itself?\n", c ? c->lsc_line : 0);
            } else {
                mask |= stripMask(c, sublist);
            }
        } else if (script->stripTable->findSym(*i, v)) {
            mask |= 1LL << ((uint64_t) v);
        } else {
            printf("[Line %d]: Could not find strip name: '%s'\n",c ? c->lsc_line : 0, i->c_str());
            throw -1;
        }
    }

    nestLevel--;

    return mask;
}

void LSSchedule::setAnimation(LSCommand_t *cmd, schedcmd_t *scmd)
{
    int v;

    // Fill in the animation.
    if (script->animTable->findSym(cmd->lsc_animation, v)) {
        scmd->animation = v;
    }
    else {
        printf("[Line %d]: Could not find animation '%s', is it defined in your config file?\n",
               cmd->lsc_line,
               cmd->lsc_animation.c_str());
        // Throw exception.
    }
}

void LSSchedule::setColor(LSCommand_t *cmd, schedcmd_t *scmd)
{
    if (cmd->opt_colorIdent != "") {
        int v;
        if (script->colorTable->findSym(cmd->opt_colorIdent, v)) {
            scmd->palette = v;
        } else {
            printf("[Line %d]: Color not found: '%s'\n",cmd->lsc_line,cmd->opt_colorIdent.c_str());
            throw -1;
        }
    } else {
        scmd->palette = cmd->opt_color;
   }
}



void LSSchedule::insert_do(double baseTime, LSCommand_t *c)
{
    int i;
    double t;
    double deltaTime;
    
    assert(c->lsc_count != 0);

    // Compute total time that this command spans
    deltaTime = c->lsc_to - c->lsc_from;

    // Generate commands in the span.  If it's only one command, then the time is zero,
    // otherwise it is spaced evenly across the span, including the end stops.
    for (i = 0; i < c->lsc_count; i++) {
        if (c->lsc_count == 1) {
            t = 0.0;
        } else {
            t = deltaTime * ((double) i / (double) (c->lsc_count-1));
        }

        // Create a template schedule command
        schedcmd_t *scmd = newSchedCmd(baseTime, c);

        // Fill in the strip mask, since this is a 'do' it works on all listed strips.
        if (c->lsc_strips) {
            nestLevel = 0;
            scmd->stripmask = stripMask(c,c->lsc_strips);
        }

        // Set the animation
        setAnimation(c, scmd);
        setColor(c, scmd);

        // Place in the final schedule.
        addSched(scmd);
    }
    
}

void LSSchedule::insert_cascade(double baseTime, LSCommand_t *c)
{
    stripvec_t *vec;
    stripvec_t::iterator s;
    int i = 0;

    vec = stripVec(c,c->lsc_strips);

    for (s = vec->begin(); s < vec->end(); s++,i++) {
        schedcmd_t *scmd = newSchedCmd(baseTime, c);
        setAnimation(c,scmd);
        setColor(c, scmd);
        scmd->stripmask = 1LL << ((uint64_t) *s);
        scmd->time += c->opt_delay * (double) i;

        // Place in the final schedule.
        addSched(scmd);
    }
}

void LSSchedule::insert_comment(double baseTime, LSCommand_t *c)
{
    schedcmd_t *scmd = newSchedCmd(baseTime, c);
    scmd->comment = c->lsc_comment.c_str();

    // Place in the final schedule.
    addSched(scmd);
}

void LSSchedule::insert_macro(double baseTime, LSCommand_t *c)
{
    cmdlist_t *commands;
    idlist_t *args;

    if (script->macroTable->findMacro(c->lsc_macro, args, commands)) {
        int i;
        for (i = 0; i < commands->size(); i++) {
            LSCommand_t *mc = commands->at(i);
            insert(c->lsc_from, mc);
        }
    } else {
        printf("[Line %d]: Macro not defined: '%s'\n",c->lsc_line,c->lsc_macro.c_str());
        throw -1;
    }
}

void LSSchedule::insert(double baseTime, LSCommand_t *c)
{

    switch (c->lsc_type) {
        case LSC_CASCADE:
            insert_cascade(baseTime, c);
            break;
        case LSC_DO:
            insert_do(baseTime, c);
            break;
        case LSC_MACRO:
            insert_macro(baseTime, c);
            break;
        case LSC_COMMENT:
            insert_comment(baseTime, c);
            break;
        default:
            break;
    }
}

bool LSSchedule::generate1(void)
{
    int i;

    for (i = 0; i < script->lss_commands.size(); i++) {
        LSCommand_t *cmd = script->lss_commands[i];
        insert(0.0, cmd);
    }
    
    return true;
}

bool LSSchedule::generate(void)
{
    bool result = true;

    try {
        generate1();
    } catch (int e) {
        result = false;
    }

    return result;
}

void LSSchedule::addSched(schedcmd_t *scmd)
{
    schedule_t::iterator inshere;

    // Perform an insertion sort into our schedule.
    for (inshere = schedule.begin(); inshere < schedule.end(); inshere++) {
        if (scmd->time < (*inshere)->time) {
            break;
        }
    }
    schedule.insert(inshere, scmd);
    
}

static void fmttime(char *dest, double t)
{
    unsigned int minutes = (int) (t / 60.0);
    double seconds = (t - ((double) minutes)*60.0);
    sprintf(dest,"%2u:%05.02f",
            minutes,seconds);
}
#define MAXSTRIPS 31
static char *maskstr(char *str,uint32_t m)
{
    int i;

    for (i = 0; i<MAXSTRIPS; i++) {
        str[(MAXSTRIPS-1)-i] = ((1 << i) & m) ? "123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i] : '.';
    }
    str[MAXSTRIPS] = 0;
    return str;
}

void LSSchedule::printSchedEntry(schedcmd_t *scmd)
{
    char tmpstr[MAXSTRIPS+1];
    char animstr[32];
    char colorstr[32];
    char timestr[16];
    std::string name;

    fmttime(timestr,scmd->time);

    if (scmd->comment) {
        printf("\n");
        printf("Time %8s | Line %3d | %s\n",timestr,scmd->line,scmd->comment);
        printf("\n");
    } else {
        if (script->animTable->findVal(scmd->animation, name)) {
            snprintf(animstr,sizeof(animstr),"%s",name.c_str());
        } else {
            snprintf(animstr,sizeof(animstr),"%u",scmd->animation);
        }

        if (script->colorTable->findVal(scmd->palette, name)) {
            snprintf(colorstr,sizeof(colorstr),"%s",name.c_str());
        } else {
            if (scmd->palette & COLORFLG) {
                snprintf(colorstr,sizeof(colorstr),"color 0x%06X", scmd->palette & 0x00FFFFFF);
            } else {
                snprintf(colorstr,sizeof(colorstr),"palette %2u",scmd->palette);
            }
        }

        printf("Time %8s | Line %3d | %-15.15s %c | speed %5u | option %5u | %-14.14s %c | strips %s\n",timestr,scmd->line, animstr,
               scmd->direction ? 'R' : 'F',
               scmd->speed, scmd->option,
               colorstr, (scmd->palette & COLORFLG ? ' ' : 'P'),
               maskstr(tmpstr,scmd->stripmask));
    }
}


void LSSchedule::printSched(void)
{
    schedule_t::iterator inshere;

    for (inshere = schedule.begin(); inshere < schedule.end(); inshere++) {
        schedcmd_t *scmd = *inshere;

        printSchedEntry(scmd);
    }

}

int LSSchedule::size(void)
{
    return schedule.size();
}

schedcmd_t *LSSchedule::getAt(int idx)
{
    return schedule[idx];
}
