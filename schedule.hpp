

#include "parser.hpp"


typedef struct schedcmd_s {
    double time;
    int line;
    uint64_t stripmask;
    int animation;
    int speed;
    int brightness;
    int palette;
    int direction;
    int option;
} schedcmd_t;



typedef std::vector<schedcmd_t *> schedule_t;

class LSSchedule {
public:
    LSSchedule(LSScript_t *s);
    ~LSSchedule();
public:
    uint64_t stripMask(LSCommand_t *c, idlist_t *list);

private:
    int nestLevel;

private:
    void insert(double baseTime, LSCommand_t *c);
    void insert_do(double baseTime, LSCommand_t *c);
    void insert_cascade(double baseTime, LSCommand_t *c);
    void insert_macro(double baseTime, LSCommand_t *c);
    schedcmd_t *newSchedCmd(double baseTime, LSCommand_t *cmd);
    void setAnimation(LSCommand_t *cmd, schedcmd_t *scmd);
    void setColor(LSCommand_t *cmd, schedcmd_t *scmd);
    void stripVec1(LSCommand_t *c, std::vector<int> *vec, idlist_t *list);
    std::vector<int> *stripVec(LSCommand_t *c, idlist_t *list);

    void addSched(schedcmd_t *scmd);

    LSScript_t *script;

    schedule_t schedule;

    bool generate1(void);

public:
    bool generate(void);
    void printSched(void);
    void printSchedEntry(schedcmd_t *scmd);
    int size(void);
    schedcmd_t *getAt(int i);

};
