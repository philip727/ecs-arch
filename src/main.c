#include "ecs/entity.h"
#include "ecs/query.h"
#include "ecs/resource.h"
#include "ecs/systems.h"
#include "ecs/world.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define HEALTH_COMPONENT_TYPE "component::health"

typedef struct {
    uint32_t health;
    uint32_t maxHealth;
} CHealth;

void create_character_sys(World *world) {
    // Create a local player
    size_t id = world_create_empty_entity(world);

    ComponentData nameComponent = {};
    CName *name = (CName *)malloc(sizeof(CName));
    name->data = "LocalActor";
    nameComponent.data = (ComponentPtr)name;
    nameComponent.type = NAME_COMPONENT_TYPE;

    ComponentData healthComponent = {};
    CHealth *health = (CHealth *)malloc(sizeof(CHealth));
    health->health = 100;
    health->maxHealth = 100;
    healthComponent.data = (ComponentPtr)health;
    healthComponent.type = HEALTH_COMPONENT_TYPE;

    world_insert_component(world, id, nameComponent);
    world_insert_component(world, id, healthComponent);
}

void damage_health_sys(World *world) {
    char *types[] = {HEALTH_COMPONENT_TYPE};
    Query query = {
        .amountOfComponents = 1,
        .componentTypes = types,
    };
    QueryResult result = world_query(world, query);

    if (query_result_is_empty(result)) {
        query_result_cleanup(&result);
        return;
    }

    for (size_t i = 0; i < result.amountOfResults; i++) {
        QueryResultEntry entry = result.entries[i];
        CHealth *health = (CHealth *)entry.components[0]->data;
        uint32_t damage = 10;

        printf("%d\n", health->health);
        if (damage > health->health) {
            health->health = 0;
            continue;
        }

        health->health -= 10;
    }

    query_result_cleanup(&result);
}

void delete_zero_health_sys(World *world) {
    char *types[] = {HEALTH_COMPONENT_TYPE};
    Query query = {
        .amountOfComponents = 1,
        .componentTypes = types,
    };
    QueryResult result = world_query(world, query);

    if (query_result_is_empty(result)) {
        query_result_cleanup(&result);
        return;
    }

    for (size_t i = 0; i < result.amountOfResults; i++) {
        QueryResultEntry entry = result.entries[i];
        CHealth *health = (CHealth *)entry.components[0]->data;

        if (health->health == 0) {
            world_remove_entity(world, entry.entityId);
            printf("Killed enttiy\n");
        }
    }

    query_result_cleanup(&result);
}

void update_frame_resource(World *world) {
    ResourceData *frameCount =
        world_get_resource(world, "resource::frame_count");
    if (frameCount == NULL) {
        return;
    }

    uint32_t *frame = (uint32_t *)frameCount->data;
    *frame += 1;
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    World world = {};
    world_alloc(&world);
    SystemRunner sysRunner = {};
    system_runner_alloc(&sysRunner);

    uint32_t *frame = malloc(sizeof(uint32_t));
    *frame = 0;

    ResourceData frameCount = {
        .data = (void *)frame,
        .type = "resource::frame_count",
    };

    world_insert_resource(&world, frameCount);

    // Startup systems
    system_runner_add_system(&sysRunner, &world, SYSTEM_SET_STARTUP,
                             create_character_sys);

    system_runner_add_system(&sysRunner, &world, SYSTEM_SET_UPDATE,
                             damage_health_sys);
    system_runner_add_system(&sysRunner, &world, SYSTEM_SET_UPDATE,
                             delete_zero_health_sys);

    system_runner_add_system(&sysRunner, &world, SYSTEM_SET_UPDATE,
                             update_frame_resource);

    // Startup systems
    system_runner_run_startup_systems(&sysRunner, &world);

    // Update systems
    for (size_t i = 0; i < 100; i++) {
        system_runner_run_update_systems(&sysRunner, &world);
    }

    system_runner_cleanup(&sysRunner);
    world_cleanup(&world);
    printf("Gracefully cleaned up ecs\n");

    return 0;
}
