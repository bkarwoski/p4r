#define _GNU_SOURCE
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <float.h>
#include <pthread.h>
#include <termios.h>
#include "bmp.h"
#include "graphics.h"
#include "image_server.h"
#include "collision.h"
#define M_PI 3.14159265358979323846
#define WIDTH 640
#define HEIGHT 480
#define MAP "XXXXXXXXXXXXXXXX" \
            "X              X" \
            "X  XXXX   XXX  X" \
            "X   XX      X  X" \
            "X       XXX    X" \
            "XXXXXX         X" \
            "X         XXXXXX" \
            "X    XXX       X" \
            "X  XX     XX   X" \
            "X   X    XX    X" \
            "X      XXX     X" \
            "XXXXXXXXXXXXXXXX"
#define BLOCK_SIZE 40
#define MAP_W (1.0 * WIDTH / BLOCK_SIZE)
#define MAP_H (1.0 * HEIGHT / BLOCK_SIZE)
#define MAX_DEPTH 4
#define ROB_HEIGHT 20
#define ROB_LENGTH (ROB_HEIGHT * 4 / 3.0)
#define HL_ON "\e[7m"
#define HL_OFF "\e[0m"
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
struct termios original_termios;

void reset_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
    //reactivate blinking cursor
    printf("\e[?25h\n");
}

typedef struct agent {
    bool is_runner;
    double x;
    double y;
    double theta;
    double vel;
    double ang_vel;
} agent_t;

typedef struct state {
    int user_action;
    int time_step;
    bitmap_t bmp;
    size_t image_size;
    uint8_t *image_data;
    bool runner_caught;
    agent_t runner;
    agent_t chaser;
    int init_runner_idx;
    int delay_every;
    int select_idx;
    double to_goal_magnitude;
    int to_goal_power;
    double avoid_obs_magnitude;
    int avoid_obs_power;
    int max_velocity;
} state_t;

void drawMap(bitmap_t *bmp) {
    color_bgr_t black = {0, 0, 0};
    color_bgr_t white = {255, 255, 255};
    for (int i = 0; i < bmp->width * bmp->height; i++) {
        bmp->data[i] = white;
    }
    for (int i = 0; i < MAP_W * MAP_H; i++) {
        if (MAP[i] == 'X') {
            vector_xy_t nextBlock = gx_rect(BLOCK_SIZE, BLOCK_SIZE);
            double xPos = BLOCK_SIZE / 2.0 + (i % (int)MAP_W) * BLOCK_SIZE;
            double yPos = BLOCK_SIZE / 2.0 + ((i - i % (int)MAP_W) * 1.0 / MAP_W) * BLOCK_SIZE;
            gx_trans(xPos, yPos, &nextBlock);
            gx_fill_poly(bmp, black, &nextBlock);
            vector_delete(&nextBlock);
        }
    }
}

void updateGraphics(state_t *state) {
    color_bgr_t red = {0, 0, 255};
    color_bgr_t green = {0, 255, 0};
    drawMap(&state->bmp);
    vector_xy_t runPoints = gx_rob();
    gx_rot(state->runner.theta, &runPoints);
    gx_trans(state->runner.x, state->runner.y, &runPoints);
    gx_fill_poly(&state->bmp, green, &runPoints);
    vector_delete(&runPoints);
    vector_xy_t chasePoints = gx_rob();
    gx_rot(state->chaser.theta, &chasePoints);
    gx_trans(state->chaser.x, state->chaser.y, &chasePoints);
    gx_fill_poly(&state->bmp, red, &chasePoints);
    vector_delete(&chasePoints);
}

int runnerAction(void) {
    int action = rand() % 20;
    if (action == 1 || action == 2) {
        return action;
    }
    return 0;
}

void applyAction(agent_t *bot, int action) {
    if (action == 1) {
        bot->vel += 2;
        if (bot->vel > 12) {
            bot->vel = 12;
        }
    }
    if (action == 2) {
        bot->ang_vel += M_PI / 16.0;
    }
    if (action == 3) {
        bot->ang_vel -= M_PI / 16.0;
    }
}

void resolveWallCollisions(agent_t *bot) {
    bool collided = false;
    bool any_collision = true;
    double bRadius = sqrt(2 * BLOCK_SIZE * BLOCK_SIZE) / 2.0;
    double rRadius = sqrt(pow(ROB_LENGTH / 2.0, 2) + pow(ROB_HEIGHT / 2.0, 2));
    double collision_dist_sq = pow((bRadius + rRadius), 2);
    while (any_collision) {
        any_collision = false;
        for (int x = 0; x < MAP_W; x++) {
            for (int y = 0; y < MAP_H; y++) {
                int i = x + (int)MAP_W * y;
                if (MAP[i] == 'X') {
                    double xPos = BLOCK_SIZE * (x + 0.5);
                    double yPos = BLOCK_SIZE * (y + 0.5);
                    double dist_sq = pow(xPos - bot->x, 2) + pow(yPos - bot->y, 2);
                    if (dist_sq <= collision_dist_sq) {
                        vector_xy_t nextBlock = gx_rect(BLOCK_SIZE, BLOCK_SIZE);
                        gx_trans(xPos, yPos, &nextBlock);
                        vector_xy_t rob = gx_rob();
                        gx_rot(bot->theta, &rob);
                        gx_trans(bot->x, bot->y, &rob);
                        bool isCollided = collision(&nextBlock, &rob);
                        if (isCollided) {
                            any_collision = true;
                            //printf("Collision w/ tile %d, %d, ", x, y);
                            double dx = bot->x - xPos;
                            double dy = bot->y - yPos;
                            double dist = sqrt(dx * dx + dy * dy);
                            //printf("movement from %.2f, %.2f ", bot->x, bot->y);
                            bot->x += dx / dist * 0.5;
                            bot->y += dy / dist * 0.5;
                            collided = true;
                            //printf("to %.2f, %.2f ", bot->x, bot->y);
                            //printf("with robot xs: %.2f, %.2f, %.2f ",
                            //    rob.xData[0], rob.xData[1], rob.xData[2]);
                            //printf("ys: %.2f, %.2f, %.2f\n",
                            //    rob.yData[0], rob.yData[1], rob.yData[2]);
                            //isCollided = collision(&nextBlock, &rob);
                        }
                        vector_delete(&nextBlock);
                        vector_delete(&rob);
                    }
                }
            }
        }
    }
    if (collided) {
        bot->vel *= 0.25;
    }
}

bool robCollision(agent_t runner, agent_t chaser) {
    vector_xy_t runnerPoints = gx_rob();
    gx_rot(runner.theta, &runnerPoints);
    gx_trans(runner.x, runner.y, &runnerPoints);
    vector_xy_t chaserPoints = gx_rob();
    gx_rot(chaser.theta, &chaserPoints);
    gx_trans(chaser.x, chaser.y, &chaserPoints);
    bool doesCollide = collision(&runnerPoints, &chaserPoints);
    vector_delete(&chaserPoints);
    vector_delete(&runnerPoints);
    return doesCollide;
}

void moveBot(agent_t *bot, int action) {
    applyAction(bot, action);
    bot->theta += bot->ang_vel;
    bot->ang_vel *= 0.8;
    bot->x += bot->vel * cos(bot->theta);
    bot->y += bot->vel * -sin(bot->theta);
    // if (resolveWallCollisions(bot)) {
    //     bot->vel *= 0.25;
    // }
}

void field_control(state_t *s) {
    double height = ROB_HEIGHT;
    double width = height * 4 / 3.0;
    double robot_r = sqrt((height / 2) * (height / 2) + (width / 2 * width / 2));
    double wall_r = BLOCK_SIZE / sqrt(2);
    double fx = 0;
    double fy = 0;
    double dx = s->runner.x - s->chaser.x;
    double dy = s->runner.y - s->chaser.y;
    double to_goal_dist = sqrt(dx * dx + dy * dy);
    double dx_norm = dx / to_goal_dist;
    double dy_norm = dy / to_goal_dist;
    fx += dx_norm * s->to_goal_magnitude * pow(to_goal_dist, s->to_goal_power);
    fy += dy_norm * s->to_goal_magnitude * pow(to_goal_dist, s->to_goal_power);

    for (int x = 0; x < (int)MAP_W; x++) {
        for (int y = 0; y < (int)MAP_H; y++) {
            int i = x + (int)MAP_W * y;
            if (MAP[i] == 'X') {
                double bx = BLOCK_SIZE * (x + 0.5);
                double by = BLOCK_SIZE * (y + 0.5);
                double obs_center_dist = sqrt(pow(s->chaser.x - bx, 2) + pow(s->chaser.y - by, 2));
                double to_obs_dist = obs_center_dist - robot_r - wall_r;
                to_obs_dist = fmax(0.1, to_obs_dist);
                double to_chaserx = (s->chaser.x - bx) / obs_center_dist;
                double to_chasery = (s->chaser.y - by) / obs_center_dist;
                fx += to_chaserx * s->avoid_obs_magnitude * pow(to_obs_dist, s->avoid_obs_power);
                fy += to_chasery * s->avoid_obs_magnitude * pow(to_obs_dist, s->avoid_obs_power);
            }
        }
    }
    double target_theta = atan2(-fy, fx);
    double theta_error = target_theta - s->chaser.theta;
    if (theta_error > M_PI) {
        theta_error -= 2 * M_PI;
    }
    if (theta_error < -M_PI) {
        theta_error += 2 * M_PI;
    }
    s->chaser.ang_vel = 0.4 * theta_error;
    if (s->chaser.ang_vel > M_PI / 16) {
        s->chaser.ang_vel = M_PI / 16;
    }
    if (s->chaser.ang_vel < -M_PI / 16) {
        s->chaser.ang_vel = -M_PI / 16;
    }
    s->chaser.vel = fmin(s->max_velocity, s->chaser.vel + 2.0);
}

void reset_sim(state_t *s) {
    srand(0);
    s->runner.x = BLOCK_SIZE / 2.0 + (s->init_runner_idx % (int)MAP_W) * BLOCK_SIZE;
    s->runner.y = BLOCK_SIZE / 2.0 + ((s->init_runner_idx - s->init_runner_idx % (int)MAP_W) /
                                      MAP_W) *
                                         BLOCK_SIZE;
    s->runner.theta = 0;
    s->runner.ang_vel = 0;
    s->runner.vel = 0;
    s->chaser.x = WIDTH / 2.0;
    s->chaser.y = HEIGHT / 2.0;
    s->chaser.theta = 0;
    s->chaser.ang_vel = 0;
    s->chaser.vel = 0;
    s->time_step = 0;
}

void setup_sim(state_t *s) {
    s->init_runner_idx = 17;
    s->delay_every = 1;
    s->to_goal_magnitude = 50.0;
    s->to_goal_power = 0;
    s->avoid_obs_magnitude = 1;
    s->avoid_obs_power = -2;
    s->max_velocity = 12;
    s->bmp.width = WIDTH;
    s->bmp.height = HEIGHT;
    s->bmp.data = calloc(s->bmp.width * s->bmp.height, sizeof(color_bgr_t));
    s->image_size = bmp_calculate_size(&s->bmp);
    s->image_data = malloc(s->image_size);
    s->select_idx = 0;
    reset_sim(s);
}

void handle_up(state_t *s) {
    if (s->select_idx == 0) {
        s->init_runner_idx++;
        while (MAP[s->init_runner_idx] == 'X') {
            s->init_runner_idx++;
        }
    }
    if (s->select_idx == 1) {
        s->delay_every++;
    }
    if (s->select_idx == 2) {
        s->to_goal_magnitude *= 2;
    }
    if (s->select_idx == 3) {
        if (s->to_goal_power <= 2) {
            s->to_goal_power++;
        }
    }
    if (s->select_idx == 4) {
        s->avoid_obs_magnitude *= 2;
    }
    if (s->select_idx == 5) {
        if (s->avoid_obs_power <= 2) {
            s->avoid_obs_power++;
        }
    }
    if (s->select_idx == 6) {
        if (s->max_velocity <= 11) {
            s->max_velocity++;
        }
    }
}

void handle_down(state_t *s) {
    if (s->select_idx == 0) {
        s->init_runner_idx--;
        while (MAP[s->init_runner_idx] == 'X') {
            s->init_runner_idx--;
        }
    }
    if (s->select_idx == 1) {
        if (s->delay_every > 1) {
            s->delay_every--;
        }
    }
    if (s->select_idx == 2) {
        s->to_goal_magnitude /= 2;
    }
    if (s->select_idx == 3) {
        if (s->to_goal_power >= -2) {
            s->to_goal_power--;
        }
    }
    if (s->select_idx == 4) {
        s->avoid_obs_magnitude /= 2;
    }
    if (s->select_idx == 5) {
        if (s->avoid_obs_power >= -2) {
            s->avoid_obs_power--;
        }
    }
    if (s->select_idx == 6) {
        if (s->max_velocity > 1) {
            s->max_velocity--;
        }
    }
}

void disp_interface(state_t *s) {
    //printf("\r %stesting%s index = %d", HL_ON, HL_OFF, s->select_idx);
    printf("\ridx=");
    printf("%s%d%s", s->select_idx == 0 ? HL_ON : "", s->init_runner_idx, HL_OFF);
    //printf("%s%8.2f%s", goal_mag_selected ? HIGHLIGHT : "", to_goal_magnitude, CLEAR_HIGHLIGHT);
    printf(" delay_every=");
    printf("%s%d%s", s->select_idx == 1 ? HL_ON : "", s->delay_every, HL_OFF);

    printf(" to_goal_mag=");
    printf("%s%4.2f%s", s->select_idx == 2 ? HL_ON : "", s->to_goal_magnitude, HL_OFF);

    printf(" to_goal_pow=");
    printf("%s%d%s", s->select_idx == 3 ? HL_ON : "", s->to_goal_power, HL_OFF);

    printf(" avoid_obs_mag=");
    printf("%s%4.2f%s", s->select_idx == 4 ? HL_ON : "", s->avoid_obs_magnitude, HL_OFF);

    printf(" avoid_obs_pow=");
    printf("%s%d%s", s->select_idx == 5 ? HL_ON : "", s->avoid_obs_power, HL_OFF);

    printf(" max_vel=");
    printf("%s%d%s", s->select_idx == 6 ? HL_ON : "", s->max_velocity, HL_OFF);
    fflush(stdout);
}

void *io_thread(void *user) {
    //deactivate blinking cursor
    printf("\e[?25l\n");
    state_t *state = user;
    tcgetattr(0, &original_termios);
    atexit(reset_terminal);
    struct termios new_termios;
    memcpy(&new_termios, &original_termios, sizeof(new_termios));
    new_termios.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    while (true) {
        int c = getc(stdin);
        if (c == 'q') {
            exit(0);
        }
        if (c == 'r') {
            reset_sim(state);
        }
        if (c == 27) {
            c = getc(stdin);
            if (c == 91) {
                c = getc(stdin);
                if (c == 'A') {
                    handle_up(state);
                } else if (c == 'B') {
                    handle_down(state);
                } else if (c == 'C') {
                    //right
                    state->select_idx++; //update to wraparound
                } else if (c == 'D') {
                    //left
                    state->select_idx--;
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int seconds = 0;
    long nanoseconds = 40 * 1000 * 1000;
    struct timespec interval = {seconds, nanoseconds};
    state_t state = {0};
    setup_sim(&state);
    pthread_t userInput;
    pthread_create(&userInput, NULL, io_thread, &state);
    if (argc < 2) {
        image_server_start("8000");
    }
    while (true) {
        if (argc < 2) {
            updateGraphics(&state);
            bmp_serialize(&state.bmp, state.image_data);
            image_server_set_data(state.image_size, state.image_data);
        }
        if (state.time_step % state.delay_every == 0) {
            nanosleep(&interval, NULL);
        }
        // if (state.time_step == 4) {
        //     //for gdb
        //     printf("time_step = %d\n", state.time_step);
        // }
        field_control(&state);
        moveBot(&state.chaser, state.user_action);
        int runAct = runnerAction();
        moveBot(&state.runner, runAct);
        resolveWallCollisions(&state.chaser);
        resolveWallCollisions(&state.runner);
        //printf("%.2f %.2f\n", state.chaser.vel, state.chaser.ang_vel);
        disp_interface(&state);
        if (robCollision(state.runner, state.chaser)) {
            printf("\e[2K\rRunner caught on step %d\n", state.time_step);
            reset_sim(&state);
        } else {
            state.time_step++;
        }
    }
    free(state.image_data);
    free(state.bmp.data);
    return 0;
}
