#ifndef BDK_TIMER_H
#define BDK_TIMER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief General-purpose / advanced timer instance (TIMx).
 */
typedef enum {
    BDK_TIM_1 = 1,
    BDK_TIM_2,
    BDK_TIM_3,
    BDK_TIM_4,
    BDK_TIM_5,
    BDK_TIM_6,
    BDK_TIM_7,
    BDK_TIM_8,
    BDK_TIM_9,
    BDK_TIM_10,
    BDK_TIM_11,
    BDK_TIM_12,
    BDK_TIM_13,
    BDK_TIM_14
} bdk_tim_id_t;

/**
 * @brief Timer channel 1..4.
 */
typedef enum {
    BDK_TIM_CH1 = 1,
    BDK_TIM_CH2,
    BDK_TIM_CH3,
    BDK_TIM_CH4
} bdk_tim_ch_t;

/**
 * @brief Time-base configuration (PSC/ARR).
 */
typedef struct {
    bdk_tim_id_t id;
    uint16_t     psc; /**< Prescaler (PSC register value). */
    uint32_t     arr; /**< Auto-reload (ARR); 16- or 32-bit depending on TIMx. */
} bdk_tim_config_t;

/**
 * @brief Configure the counter time base and leave the timer disabled.
 * @param config Time-base configuration; must not be NULL.
 */
void bdk_tim_init(const bdk_tim_config_t *config);

/**
 * @brief Start the counter (CEN).
 * @param id Timer instance.
 */
void bdk_tim_start(bdk_tim_id_t id);

/**
 * @brief Stop the counter.
 * @param id Timer instance.
 */
void bdk_tim_stop(bdk_tim_id_t id);

/**
 * @brief Configure a channel as PWM output.
 * @param id     Timer instance.
 * @param ch     Channel.
 * @param pulse  Duty compare value (CCRx).
 */
void bdk_tim_pwm_init(bdk_tim_id_t id, bdk_tim_ch_t ch, uint32_t pulse);

/**
 * @brief Update PWM duty (CCRx).
 * @param id    Timer instance.
 * @param ch    Channel.
 * @param pulse New compare value.
 */
void bdk_tim_pwm_set_pulse(bdk_tim_id_t id, bdk_tim_ch_t ch, uint32_t pulse);

/**
 * @brief Configure a channel as input capture.
 * @param id Timer instance.
 * @param ch Channel.
 */
void bdk_tim_ic_init(bdk_tim_id_t id, bdk_tim_ch_t ch);

/**
 * @brief Read the last captured value (CCRx).
 * @param id Timer instance.
 * @param ch Channel.
 * @return Captured counter value.
 */
uint32_t bdk_tim_ic_read(bdk_tim_id_t id, bdk_tim_ch_t ch);

#ifdef __cplusplus
}
#endif

#endif /* BDK_TIMER_H */
