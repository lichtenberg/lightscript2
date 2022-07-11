
extern "C" {
    int playMusicFile(const char *filename, int (*callback)(double time), double start_cue);
};
