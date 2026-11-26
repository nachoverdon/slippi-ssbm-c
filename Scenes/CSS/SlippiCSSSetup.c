#ifndef SLIPPI_CSS_SETUP_C
#define SLIPPI_CSS_SETUP_C

#include "Main.c"

//Runs every frame during CSS
void minor_think(void *minor_data) {
    void (*CSS_think)(void *) = (void *) 0x802669F4;
    CSS_think(minor_data);
    UpdateOnlineCSS();
}

//Runs when CSS is loaded
void minor_load(void *minor_data) {
    void (*CSS_load)(void *) = (void *) 0x8026688C;
    CSS_load(minor_data);
    InitOnlineCSS();
    // OSReport("CSS_load\n");
}

//Runs when leaving CSS
void minor_exit(void *minor_data) {
    void (*CSS_exit)(void *) = (void *) 0x80266D70;
    CSS_exit(minor_data);
    // OSReport("CSS_exit\n");
}

#endif // SLIPPI_CSS_SETUP_C