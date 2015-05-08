#include "led.h"
#include "notify.h"
#include "profile.h"
#include "usb.h"

// Compare two light structures, ignore keys
static int rgbcmp(const lighting* lhs, const lighting* rhs){
    return memcmp(lhs->r + LED_MOUSE, rhs->r + LED_MOUSE, N_MOUSE_ZONES) + memcmp(lhs->g + LED_MOUSE, rhs->g + LED_MOUSE, N_MOUSE_ZONES) + memcmp(lhs->b + LED_MOUSE, rhs->b + LED_MOUSE, N_MOUSE_ZONES);
}

// Return true if all mouse zones are black
static int isblack(const lighting* light){
    uchar black[N_MOUSE_ZONES] = { 0 };
    return !memcmp(light->r + LED_MOUSE, black, sizeof(black)) && !memcmp(light->g + LED_MOUSE, black, sizeof(black)) && !memcmp(light->b + LED_MOUSE, black, sizeof(black));
}

int updatergb_mouse(usbdevice* kb, int force){
    if(!kb->active)
        return 0;
    lighting* lastlight = &kb->profile->lastlight;
    lighting* newlight = &kb->profile->currentmode->light;
    // Don't do anything if the lighting hasn't changed
    if(!force && !rgbcmp(lastlight, newlight))
        return 0;

    // Send the RGB values for each zone to the mouse
    uchar data_pkt[2][MSG_SIZE] = {
        { 0x07, 0x22, 0x04, 0x01, 0 },      // RGB colors
        { 0x07, 0x05, 0x02, 0 }             // Lighting on/off
    };
    uchar* rgb_data = &data_pkt[0][4];
    for(int i = 0; i < N_MOUSE_ZONES; i++){
        *rgb_data++ = i + 1;
        *rgb_data++ = newlight->r[LED_MOUSE + i];
        *rgb_data++ = newlight->g[LED_MOUSE + i];
        *rgb_data++ = newlight->b[LED_MOUSE + i];
    }
    int was_black = isblack(lastlight), is_black = isblack(newlight);
    // Send RGB data
    if(!usbsend(kb, data_pkt[0], 1))
        return -1;
    if(is_black){
        // If the lighting is black, send the deactivation packet
        if(!usbsend(kb, data_pkt[1], 1))
            return -1;
    } else if(was_black || force){
        // If the lighting WAS black, send the activation packet
        data_pkt[1][4] = 1;
        if(!usbsend(kb, data_pkt[1], 1))
            return -1;
    }

    memcpy(lastlight, newlight, sizeof(lighting));
    return 0;
}
