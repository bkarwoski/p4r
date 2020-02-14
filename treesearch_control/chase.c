#define _GNU_SOURCE
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <float.h>
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

typedef struct agent {
    bool is_runner;
    double x;
    double y;
    double theta;
    double vel;
    double ang_vel;
} agent_t;

typedef struct search_node {
    int depth;
    agent_t runner;
    agent_t chaser;
} search_node_t;

typedef struct state {
    int time_step;
    bitmap_t bmp;
    size_t image_size;
    uint8_t *image_data;
    bool runner_caught;
    agent_t runner;
    agent_t chaser;
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
        printf("%d ", action);
        return action;
    }
    action = 0;
    printf("%d ", action);
    return action;
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

bool resolveWallCollisions(agent_t *bot) {
    bool collided = false;
    bool any_collision = true;
    double bRadius = sqrt(2 * BLOCK_SIZE * BLOCK_SIZE) / 2.0;
    double rRadius = sqrt(pow((4.0 / 3 * 20), 2) + 10 * 10) / 2.0;
    double collision_dist_sq = pow((bRadius + rRadius), 2);
    while (any_collision) {
        any_collision = false;
        for (int x = 0; x < MAP_W; x++) {
            for (int y = 0; y < MAP_H; y++) {
                int i = x + MAP_W * y;
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
                            double dx = bot->x - xPos;
                            double dy = bot->y - yPos;
                            double dist = sqrt(dx * dx + dy * dy);
                            bot->x += dx / dist * 0.5;
                            bot->y += dy / dist * 0.5;
                            collided = true;
                            isCollided = collision(&nextBlock, &rob);
                        }
                        vector_delete(&nextBlock);
                        vector_delete(&rob);
                    }
                }
            }
        }
    }
    return collided;
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
    if (resolveWallCollisions(bot)) {
        bot->vel *= 0.25;
    }
}

double search_actions(search_node_t node, int *best_action) {
    double bestScore = DBL_MAX;
    if (robCollision(node.runner, node.chaser)) {
        return 0;
    }
    if (node.depth >= MAX_DEPTH) {
        double dx = node.chaser.x - node.runner.x;
        double dy = node.chaser.y - node.runner.y;
        return sqrt(dx * dx + dy * dy);
    }
    for (int action = 0; action <= 3; action++) {
        search_node_t next_node = node;
        next_node.depth++;
        moveBot(&next_node.chaser, action);
        int count = 0;
        while (count < 3 && !robCollision(next_node.runner, next_node.chaser)) {
            moveBot(&next_node.chaser, 0);
            count++;
        }
        int chosenAction = action;
        double score = search_actions(next_node, &chosenAction);
        score += 200 / fmin(2, next_node.chaser.vel);
        if (score < bestScore) {
            bestScore = score;
            *best_action = action;
        }
    }
    return bestScore;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s <time steps> <fast=0|1|2> <initial runner index>\n", argv[0]);
        return 1;
    }
    int seconds = 0;
    long nanoseconds = 40 * 1000 * 1000;
    struct timespec interval = {seconds, nanoseconds};
    int fast = atoi(argv[2]);
    int runnerIndex = atoi(argv[3]);
    int chaserIndex = 97;
    state_t state = {0};
    state.time_step = atoi(argv[1]);
    state.bmp.width = WIDTH;
    state.bmp.height = HEIGHT;
    state.bmp.data = calloc(state.bmp.width * state.bmp.height, sizeof(color_bgr_t));
    state.image_size = bmp_calculate_size(&state.bmp);
    state.image_data = malloc(state.image_size);
    state.runner.x = BLOCK_SIZE / 2.0 + (runnerIndex % (int)MAP_W) * BLOCK_SIZE;
    state.runner.y = BLOCK_SIZE / 2.0 + ((runnerIndex - runnerIndex % (int)MAP_W) / MAP_W) * BLOCK_SIZE;
    state.runner.theta = 0;
    state.runner.ang_vel = 0;
    state.chaser.x = WIDTH / 2.0;
    state.chaser.y = HEIGHT / 2.0;
    state.chaser.theta = 0;
    state.chaser.ang_vel = 0;
    if (fast == 0) {
        image_server_start("8000");
    }
    for (int i = 0; i < state.time_step; i++) {
        if (fast == 0) {
            updateGraphics(&state);
            bmp_serialize(&state.bmp, state.image_data);
            image_server_set_data(state.image_size, state.image_data);
            nanosleep(&interval, NULL);
        }
        search_node_t search_node = {0};
        search_node.chaser = state.chaser;
        search_node.runner = state.runner;
        search_node.depth = 0;
        int chosen_action = 0;
        search_actions(search_node, &chosen_action);
        moveBot(&state.runner, runnerAction());
        moveBot(&state.chaser, chosen_action);
        printf("%d\n", chosen_action);
        if (robCollision(state.runner, state.chaser)) {
            exit(0);
        }
    }
    free(state.image_data);
    free(state.bmp.data);
    return 0;
}
