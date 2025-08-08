

int play_opendevice(char *devname);
void play_closedevice(void);
void play_initdevice(LSScript_t *script);
void play_script(int how);
void play_init(LSScript_t *script, LSSchedule *sched);
void play_interrupt(void);

int env_setenv(char *name, char *val);
int env_getenv(char *name, char *val, int vallen);
int env_listenv(char *val, int vallen);
int env_eraseall(void);

void check_version(void);
int reset_to_dfu(void);
