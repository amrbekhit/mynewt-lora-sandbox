#include "os/os_task.h"
#include "os/os_mbuf.h"
#include "node/lora.h"

#define DEBUG_LORA

#ifdef DEBUG_LORA
#include "console/console.h"
#define dprintf(...)    console_printf(__VA_ARGS__)
#else
#define dprintf(...)
#endif

#define TASK_PRI            (100)
#define TASK_STACK_SIZE     (256)
static struct os_task lora_task;
static os_stack_t lora_task_stack[TASK_STACK_SIZE];

uint8_t DevEUI[] = {1, 2, 3, 4, 5, 6, 7, 8};
uint8_t AppEUI[] = {0,0,0,0,0,0,0,0};
uint8_t AppKey[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static enum {
    LoraJoin,
    LoraWait,
    LoraJoined,
    LoraFailed,
} lora_state = LoraJoin;

LoRaMacEventInfoStatus_t lora_status;

static void lora_join_callback(LoRaMacEventInfoStatus_t status, uint8_t attempts)
{
    if (status == LORAMAC_EVENT_INFO_STATUS_OK) {
        lora_state = LoraJoined;
    } else {
        lora_status = status;
        lora_state = LoraFailed;
    }
}

static void lora_task_func(void *arg)
{
    lora_app_set_join_cb(lora_join_callback);

    while(1) {
        switch (lora_state) {
            case LoraJoin:
                // Trigger a join request
                lora_app_join(DevEUI, AppEUI, AppKey, 1);
                lora_state = LoraWait;
                break;
            case LoraWait:
                // Idle 
                os_time_delay(1000);
                break;
            case LoraFailed:
                dprintf("Lora join failed: %i\n", lora_status);
                lora_state = LoraWait;
                break;
            case LoraJoined:
                dprintf("Lora joined\n");
                lora_state = LoraWait;
                break;
        }
    }
}

void lora_init(void)
{
    int rc;

    rc = os_task_init(&lora_task, "lora_app", lora_task_func, NULL, 
                    TASK_PRI, OS_WAIT_FOREVER, lora_task_stack, TASK_STACK_SIZE);

    assert(rc == 0);
}