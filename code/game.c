#include "game.h"

static i32
round_fi32(f32 value){
    i32 result = (i32)(value + 0.5f);
    return(result);
}

static f32
round_ff(f32 value){
    f32 result = (f32)(i32)(value + 0.5f);
    return(result);
}

static void
swapf(f32 *a, f32 *b){
    f32 t = *a;
    *a = *b;
    *b = t;
}

static void
swapv2(v2 *a, v2 *b){
    v2 t = *a;
    *a = *b;
    *b = t;
}

// QUESTION: i dont think this should be floats, should be ints, ask about it
static void
set_pixel(RenderBuffer *buffer, f32 x, f32 y, Color c){
    x = round_ff(x);
    y = round_ff(y);

    ui32 color = (round_fi32(c.r * 255) << 16 | round_fi32(c.g * 255) << 8 | round_fi32(c.b * 255));
    ui8 *row = (ui8 *)buffer->memory + ((buffer->height - 1 - (i32)y) * buffer->pitch) + ((i32)x * buffer->bytes_per_pixel);
    ui32 *pixel = (ui32 *)row;
    *pixel = color;
}

static void 
line(RenderBuffer *buffer, v2 p1, v2 p2, Color c){
    p1.x = round_ff(p1.x);
    p1.y = round_ff(p1.y);
    p2.x = round_ff(p2.x);
    p2.y = round_ff(p2.y);

    f32 distance_x =  ABS(p2.x - p1.x);
    f32 distance_y = -ABS(p2.y - p1.y);
    f32 step_x = p1.x < p2.x ? 1.0f : -1.0f;
    f32 step_y = p1.y < p2.y ? 1.0f : -1.0f; 

    f32 error = distance_x + distance_y;

    for(;;){
        set_pixel(buffer, p1.x, p1.y, c); // NOTE: before break, so you can draw a single point line (a pixel)
        if(p1.x == p2.x && p1.y == p2.y) break;

        f32 error2 = 2 * error;
        if (error2 >= distance_y){
            error += distance_y; 
            p1.x += step_x; 
        }
        if (error2 <= distance_x){ 
            error += distance_x; 
            p1.y += step_y; 
        }
    }
}

static void
fill_flattop_triangle(RenderBuffer *buffer, v2 p1, v2 p2, v2 p3, Color c){
    f32 slope_1 = (p1.x - p3.x) / (p1.y - p3.y);
    f32 slope_2 = (p2.x - p3.x) / (p2.y - p3.y);
    
    f32 x1_inc = p3.x;
    f32 x2_inc = p3.x;

    for(f32 y=p3.y; y < p1.y; ++y){
        v2 new_p1 = {x1_inc, y};
        v2 new_p2 = {x2_inc, y};
        line(buffer, new_p1, new_p2, c);
        x1_inc += slope_1;
        x2_inc += slope_2;
    }
}

static void
fill_flatbottom_triangle(RenderBuffer *buffer, v2 p1, v2 p2, v2 p3, Color c){
    f32 slope_1 = -(p2.x - p1.x) / (p2.y - p1.y);
    f32 slope_2 = -(p3.x - p1.x) / (p3.y - p1.y);
    
    f32 x1_inc = p1.x;
    f32 x2_inc = p1.x;

    for(f32 y=p1.y; y >= p2.y; --y){
        v2 new_p1 = {x1_inc, y};
        v2 new_p2 = {x2_inc, y};
        line(buffer, new_p1, new_p2, c);
        x1_inc += slope_1;
        x2_inc += slope_2;
    }
}

static void
triangle(RenderBuffer *buffer, v2 p1, v2 p2, v2 p3, Color c){
    if(p1.y < p2.y){ swapv2(&p1, &p2); }
    if(p1.y < p3.y){ swapv2(&p1, &p3); }
    if(p2.y < p3.y){ swapv2(&p2, &p3); }

    if(p2.y == p3.y){
        fill_flatbottom_triangle(buffer, p1, p2, p3, c);
    }
    if(p1.y == p2.y){
        fill_flattop_triangle(buffer, p1, p2, p3, c);
    }
    else{
        f32 x = p1.x + ((p2.y - p1.y) / (p3.y - p1.y)) * (p3.x - p1.x);
        v2 p4 = {x, p2.y};
        fill_flatbottom_triangle(buffer, p1, p2, p4, c);
        fill_flattop_triangle(buffer, p2, p4, p3, c);
    }
    line(buffer, p1, p2, c);
    line(buffer, p2, p3, c);
    line(buffer, p3, p1, c);
}

static void
rect(RenderBuffer *buffer, v2 pos, v2 dim, Color c){
    i32 rounded_x = round_fi32(pos.x);
    i32 rounded_y = round_fi32(pos.y);
    i32 rounded_w = round_fi32(dim.x);
    i32 rounded_h = round_fi32(dim.y);

    if(rounded_x < 0) { rounded_x = 0; }
    if(rounded_y < 0) { rounded_y = 0; }
    if(rounded_x + rounded_w > buffer->width){ rounded_x = buffer->width - rounded_w; }
    if(rounded_y + rounded_h > buffer->height){ rounded_y = buffer->height - rounded_h; }

    for(int y = rounded_y; y < rounded_y + rounded_h; ++y){
        for(int x = rounded_x; x < rounded_x + rounded_w; ++x){
            set_pixel(buffer, (f32)x, (f32)y, c);
        }
    }
}

MAIN_GAME_LOOP(main_game_loop){
    //GameState *game_state = (GameState *)memory->permanent_storage;
    
    if(!memory->initialized){
        memory->initialized = true;
    }

    for(ui32 i=0; i < events->index; ++i){
        Event *event = &events->event[i];
        if(event->type == EVENT_KEYDOWN){
        }
        if(event->type == EVENT_KEYUP){
            if(event->key == KEY_ESCAPE){
                memory->running = false;
            }
        }
    }

    Color red = {1.0f, 0.0f, 0.0f};
    Color blue = {0.0f, 0.0f, 1.0f};
    Color green = {0.0f, 1.0f, 0.0f};
    Color white = {1.0f, 1.0f, 1.0f};

    v2 pos = {0, 0};
    v2 dim = {(f32)render_buffer->width, (f32)render_buffer->height};
    //rect(render_buffer, pos, dim, white);

    v2 t0[3] = {{10.0f, 70.0f}, {50.0f, 160.0f}, {70.0f, 80.0f}};
    v2 t1[3] = {{180.0f, 50.0f}, {150.0f, 1.0f}, {70.0f, 180.0f}};
    v2 t2[3] = {{180.0f, 150.0f}, {120.0f, 160.0f}, {130.0f, 180.0f}};
    triangle(render_buffer, t0[0], t0[1], t0[2], red);
    triangle(render_buffer, t1[0], t1[1], t1[2], blue);
    triangle(render_buffer, t2[0], t2[1], t2[2], green);

    v2 p0 = {10, 300};
    v2 p1 = {100, 300};
    line(render_buffer, p0, p1, red);

    v2 p2 = {100, 300};
    v2 p3 = {100, 200};
    line(render_buffer, p2, p3, red);

    // flat bottom
    v2 t3[3] = {{200.0f, 70.0f}, {250.0f, 160.0f}, {300.0f, 70.0f}};
    triangle(render_buffer, t3[2], t3[1], t3[0], red);
    // flat top
    v2 t4[3] = {{400.0f, 160.0f}, {450.0f, 70.0f}, {500.0f, 160.0f}};
    triangle(render_buffer, t4[0], t4[1], t4[2], green);

    v2 t5[3] = {{650.0f, 160.0f}, {670.0f, 80.0f}, {614.0f, 80.0f}};
    triangle(render_buffer, t5[2], t5[1], t5[0], red);
}
