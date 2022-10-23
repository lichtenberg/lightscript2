

int play_opendevice(char *devname);
void play_closedevice(void);
void play_initdevice(LSScript_t *script);
void play_script(int how);
void play_init(LSScript_t *script, LSSchedule *sched);
void play_interrupt(void);

