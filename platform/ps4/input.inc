/* Included by events.cpp after its event/poll queues. Native ScePad -> engine input. */
#include <orbis/Pad.h>
#include <orbis/UserService.h>
static int ps4PadHandle=-1;
static bool ps4PadOwned=false;
static bool ps4Keys[K_LAST_KEY]={};
static void PS4_PollPad() {
    if(ps4PadHandle<0) {
        sceUserServiceInitialize(NULL);
        int user=-1;
        if(sceUserServiceGetInitialUser(&user)<0 || scePadInit()<0) return;
        ps4PadHandle=scePadOpen(user,ORBIS_PAD_PORT_TYPE_STANDARD,0,NULL);
        if((uint32_t)ps4PadHandle==ORBIS_PAD_ERROR_ALREADY_OPENED)
            ps4PadHandle=scePadGetHandle(user,ORBIS_PAD_PORT_TYPE_STANDARD,0);
        else ps4PadOwned=ps4PadHandle>0;
        if(ps4PadHandle<=0) {ps4PadHandle=-1;return;}
        Sys_Printf("PS4 PAD ready handle=%d\n",ps4PadHandle);
    }
    OrbisPadData pad={};
    bool active=scePadReadState(ps4PadHandle,&pad)>=0 && pad.connected;
    bool next[K_LAST_KEY]={};
    const bool menu=sessLocal.GetActiveMenu()!=NULL;
    if(active) {
        next[menu?K_UPARROW:'f']=pad.buttons&ORBIS_PAD_BUTTON_UP;
        next[menu?K_DOWNARROW:'c']=pad.buttons&ORBIS_PAD_BUTTON_DOWN;
        next[menu?K_LEFTARROW:'[']=pad.buttons&ORBIS_PAD_BUTTON_LEFT;
        next[menu?K_RIGHTARROW:']']=pad.buttons&ORBIS_PAD_BUTTON_RIGHT;
        next[K_ESCAPE]=pad.buttons&(ORBIS_PAD_BUTTON_OPTIONS|ORBIS_PAD_BUTTON_CIRCLE);
        next[menu?K_ENTER:K_SPACE]=pad.buttons&ORBIS_PAD_BUTTON_CROSS;
        next['r']=pad.buttons&ORBIS_PAD_BUTTON_SQUARE;
        next[K_TAB]=pad.buttons&ORBIS_PAD_BUTTON_TRIANGLE;
        next[K_SHIFT]=pad.buttons&ORBIS_PAD_BUTTON_L1;
        next['c']=next['c'] || (pad.buttons&ORBIS_PAD_BUTTON_R1);
        next[K_MOUSE1]=(pad.buttons&ORBIS_PAD_BUTTON_R2)||pad.analogButtons.r2>=32;
        next['z']=(pad.buttons&ORBIS_PAD_BUTTON_L2)||pad.analogButtons.l2>=32;
        next['w']=pad.leftStick.y<96; next['s']=pad.leftStick.y>160;
        next['a']=pad.leftStick.x<96; next['d']=pad.leftStick.x>160;
        int dx=int(pad.rightStick.x)-128,dy=int(pad.rightStick.y)-128;
        if(abs(dx)<24) dx=0; if(abs(dy)<24) dy=0;
        static int lastTime=Sys_Milliseconds();
        int now=Sys_Milliseconds(), elapsed=now-lastTime;lastTime=now;
        if(elapsed<1)elapsed=1;if(elapsed>100)elapsed=100;
        dx=dx*elapsed/128;dy=dy*elapsed/128;
        if(dx||dy) {
            mouse_polls.Append(mouse_poll_t(M_DELTAX,dx));
            mouse_polls.Append(mouse_poll_t(M_DELTAY,dy));
            sysEvent_t ev={SE_MOUSE,dx,dy,0,NULL};event_overflow.Append(ev);
        }
    }
    for(int k=0;k<K_LAST_KEY;++k) if(next[k]!=ps4Keys[k]) {
        ps4Keys[k]=next[k];
        sysEvent_t ev={SE_KEY,k,int(next[k]),0,NULL};event_overflow.Append(ev);
        if(k==K_MOUSE1||k==K_MOUSE2)
            mouse_polls.Append(mouse_poll_t(k==K_MOUSE1?M_ACTION1:M_ACTION2,next[k]));
        else kbd_polls.Append(kbd_poll_t(k,next[k]));
        Sys_Printf("PS4 PAD key=%d down=%d\n",k,int(next[k]));
    }
}
static void PS4_ClosePad() {
    if(ps4PadOwned && ps4PadHandle>0) scePadClose(ps4PadHandle);
    ps4PadHandle=-1;ps4PadOwned=false;
}
