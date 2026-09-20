#include "bdk_timer.h"

/* Bodies are intentionally empty. Implement from RM0090 TIM (chapters 17–20). */

void bdk_tim_init(const bdk_tim_config_t *config)
{
    (void)config;
}

void bdk_tim_start(bdk_tim_id_t id)
{
    (void)id;
}

void bdk_tim_stop(bdk_tim_id_t id)
{
    (void)id;
}

void bdk_tim_pwm_init(bdk_tim_id_t id, bdk_tim_ch_t ch, uint32_t pulse)
{
    (void)id;
    (void)ch;
    (void)pulse;
}

void bdk_tim_pwm_set_pulse(bdk_tim_id_t id, bdk_tim_ch_t ch, uint32_t pulse)
{
    (void)id;
    (void)ch;
    (void)pulse;
}

void bdk_tim_ic_init(bdk_tim_id_t id, bdk_tim_ch_t ch)
{
    (void)id;
    (void)ch;
}

uint32_t bdk_tim_ic_read(bdk_tim_id_t id, bdk_tim_ch_t ch)
{
    (void)id;
    (void)ch;
    return 0;
}
