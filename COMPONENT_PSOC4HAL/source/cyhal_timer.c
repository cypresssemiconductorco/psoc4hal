/***************************************************************************//**
* \file cyhal_timer.c
*
* \brief
* Provides a high level interface for interacting with the Cypress Timer/Counter.
* This interface abstracts out the chip specific details. If any chip specific
* functionality is necessary, or performance is critical the low level functions
* can be used directly.
*
********************************************************************************
* \copyright
* Copyright 2018-2020 Cypress Semiconductor Corporation
* SPDX-License-Identifier: Apache-2.0
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/
#include <string.h>
#include "cy_device_headers.h"
#include "cyhal_timer_impl.h"
#include "cyhal_hwmgr.h"
#include "cyhal_gpio.h"
#include "cyhal_syspm.h"
#include "cyhal_clock.h"

#if defined(CY_IP_MXTCPWM_INSTANCES) || defined(CY_IP_M0S8TCPWM_INSTANCES)

#if defined(__cplusplus)
extern "C" {
#endif

static const cy_stc_tcpwm_counter_config_t _cyhal_timer_default_config =
{
    .period = 32768,
    .clockPrescaler = CY_TCPWM_COUNTER_PRESCALER_DIVBY_1,
    .runMode = CY_TCPWM_COUNTER_CONTINUOUS,
    .countDirection = CY_TCPWM_COUNTER_COUNT_UP,
    .compareOrCapture = CY_TCPWM_COUNTER_MODE_CAPTURE,
    .compare0 = 16384,
    .compare1 = 16384,
    .enableCompareSwap = false,
    .interruptSources = CY_TCPWM_INT_NONE,
    .captureInputMode = 0x3U,
    .captureInput = CY_TCPWM_INPUT_0,
    .reloadInputMode = 0x3U,
    .reloadInput = CY_TCPWM_INPUT_0,
    .startInputMode = 0x3U,
    .startInput = CY_TCPWM_INPUT_0,
    .stopInputMode = 0x3U,
    .stopInput = CY_TCPWM_INPUT_0,
    .countInputMode = 0x3U,
    .countInput = CY_TCPWM_INPUT_1,
};

/** Convert timer direction from the HAL enum to the corresponding PDL constant
  *
  * @param[in] direction The direction, as a HAL enum value
  * @return The direction, as a PDL constant
  */
static inline uint32_t _cyhal_timer_convert_direction(cyhal_timer_direction_t direction)
{
    switch (direction)
    {
        case CYHAL_TIMER_DIR_UP:
            return CY_TCPWM_COUNTER_COUNT_UP;
        case CYHAL_TIMER_DIR_DOWN:
            return CY_TCPWM_COUNTER_COUNT_DOWN;
        case CYHAL_TIMER_DIR_UP_DOWN:
            return CY_TCPWM_COUNTER_COUNT_UP_DOWN_2;
    }
    return CY_TCPWM_COUNTER_COUNT_UP;
}

/*******************************************************************************
*       Timer HAL Functions
*******************************************************************************/

cy_rslt_t cyhal_timer_init(cyhal_timer_t *obj, cyhal_gpio_t pin, const cyhal_clock_t *clk)
{
    CY_ASSERT(NULL != obj);

    // No support currently for pin connections on this device
    if (CYHAL_NC_PIN_VALUE != pin)
        return CYHAL_TIMER_RSLT_ERR_BAD_ARGUMENT;

    memset(obj, 0, sizeof(cyhal_timer_t));
    cy_rslt_t result = cyhal_hwmgr_allocate(CYHAL_RSC_TCPWM, &obj->resource);
    if (CY_RSLT_SUCCESS == result)
    {
        cyhal_resource_inst_t *timer = &obj->resource;
        obj->base = _CYHAL_TCPWM_DATA[timer->block_num].base;
        en_clk_dst_t pclk = (en_clk_dst_t)(_CYHAL_TCPWM_DATA[timer->block_num].clock_dst + timer->channel_num);

        if (NULL != clk)
        {
            obj->clock = *clk;
            _cyhal_utils_update_clock_format(&obj->clock);
            obj->clock_hz = cyhal_clock_get_frequency(&obj->clock);
            if (CY_SYSCLK_SUCCESS != Cy_SysClk_PeriphAssignDivider(pclk, (cy_en_divider_types_t)obj->clock.block, obj->clock.channel))
            {
                result = CYHAL_TIMER_RSLT_ERR_CLOCK_INIT;
            }
        }
        else if (CY_RSLT_SUCCESS == (result = cyhal_clock_allocate(&(obj->clock), CYHAL_CLOCK_BLOCK_PERIPHERAL_16BIT)))
        {
            obj->dedicated_clock = true;
            result = cyhal_timer_set_frequency(obj, CYHAL_TIMER_DEFAULT_FREQ);
            if(CY_RSLT_SUCCESS == result)
            {
                if(CY_RSLT_SUCCESS != Cy_SysClk_PeriphAssignDivider(pclk, (cy_en_divider_types_t)obj->clock.block, obj->clock.channel))
                {
                    result = CYHAL_TIMER_RSLT_ERR_CLOCK_INIT;
                }
            }
        }

        if (CY_RSLT_SUCCESS == result)
        {
            result = Cy_TCPWM_Counter_Init(obj->base, _CYHAL_TCPWM_CNT_NUMBER(obj->resource), &_cyhal_timer_default_config);
        }

        if (result == CY_RSLT_SUCCESS)
        {
            _cyhal_tcpwm_init_data(obj);
            Cy_TCPWM_SetInterruptMask(obj->base, _CYHAL_TCPWM_CNT_NUMBER(obj->resource), CY_TCPWM_INT_NONE);
            Cy_TCPWM_Counter_Enable(obj->base, _CYHAL_TCPWM_CNT_NUMBER(obj->resource));
        }
        else
        {
            cyhal_timer_free(obj);
        }
    }

    return result;
}

cy_rslt_t cyhal_timer_configure(cyhal_timer_t *obj, const cyhal_timer_cfg_t *cfg)
{
    cy_rslt_t rslt;
    obj->default_value = cfg->value;
    cy_stc_tcpwm_counter_config_t config = _cyhal_timer_default_config;
    config.period = cfg->period;
    config.compare0 = cfg->compare_value;
    config.runMode = cfg->is_continuous ? CY_TCPWM_COUNTER_CONTINUOUS : CY_TCPWM_COUNTER_ONESHOT;
    config.compareOrCapture = cfg->is_compare ? CY_TCPWM_COUNTER_MODE_COMPARE : CY_TCPWM_COUNTER_MODE_CAPTURE;
    config.countDirection = _cyhal_timer_convert_direction(cfg->direction);
    // DeInit will clear the interrupt mask; save it now and restore after we re-nit
    uint32_t old_mask = Cy_TCPWM_GetInterruptMask(obj->base, _CYHAL_TCPWM_CNT_NUMBER(obj->resource));
    Cy_TCPWM_Counter_DeInit(obj->base, _CYHAL_TCPWM_CNT_NUMBER(obj->resource), &config);
    rslt = (cy_rslt_t)Cy_TCPWM_Counter_Init(obj->base, _CYHAL_TCPWM_CNT_NUMBER(obj->resource), &config);
    Cy_TCPWM_SetInterruptMask(obj->base, _CYHAL_TCPWM_CNT_NUMBER(obj->resource), old_mask);

    // This must be called after Cy_TCPWM_Counter_Init
    cyhal_timer_reset(obj);

    return rslt;
}

cy_rslt_t cyhal_timer_set_frequency(cyhal_timer_t *obj, uint32_t hz)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    if(!obj->dedicated_clock)
    {
        result = CYHAL_TIMER_RSLT_ERR_SHARED_CLOCK;
    }

    if(CY_RSLT_SUCCESS == result)
    {
        if((CY_RSLT_SUCCESS == cyhal_clock_set_enabled(&obj->clock, false, false)) &&
           (CY_RSLT_SUCCESS == cyhal_clock_set_frequency(&obj->clock, hz, NULL)) &&
           (CY_RSLT_SUCCESS == cyhal_clock_set_enabled(&obj->clock, true, false)))
        {
            obj->clock_hz = cyhal_clock_get_frequency(&obj->clock);
        }
        else
        {
            result = CYHAL_TIMER_RSLT_ERR_CLOCK_INIT;
        }
    }

    return result;
}

cy_rslt_t cyhal_timer_start(cyhal_timer_t *obj)
{
    CY_ASSERT(NULL != obj);
    if (_cyhal_tcpwm_pm_transition_pending())
    {
        return CYHAL_SYSPM_RSLT_ERR_PM_PENDING;
    }
    #if defined(CY_IP_MXTCPWM) && (CY_IP_MXTCPWM_VERSION == 2)
    Cy_TCPWM_TriggerStart_Single(obj->base, _CYHAL_TCPWM_CNT_NUMBER(obj->resource));
    #else
    Cy_TCPWM_TriggerStart(obj->base, 1 << _CYHAL_TCPWM_CNT_NUMBER(obj->resource));
    #endif
    return CY_RSLT_SUCCESS;
}

cy_rslt_t cyhal_timer_stop(cyhal_timer_t *obj)
{
    CY_ASSERT(NULL != obj);
    #if defined(CY_IP_MXTCPWM) && (CY_IP_MXTCPWM_VERSION == 2)
    Cy_TCPWM_TriggerStopOrKill_Single(obj->base, _CYHAL_TCPWM_CNT_NUMBER(obj->resource));
    #else
    Cy_TCPWM_TriggerStopOrKill(obj->base, 1 << _CYHAL_TCPWM_CNT_NUMBER(obj->resource));
    #endif
    return CY_RSLT_SUCCESS;
}

cy_rslt_t cyhal_timer_reset(cyhal_timer_t *obj)
{
    CY_ASSERT(NULL != obj);
    bool is_running = CY_TCPWM_PWM_STATUS_COUNTER_RUNNING & Cy_TCPWM_PWM_GetStatus(obj->base, _CYHAL_TCPWM_CNT_NUMBER(obj->resource));
    #if defined(CY_IP_MXTCPWM) && (CY_IP_MXTCPWM_VERSION == 2)
    if (is_running)
        Cy_TCPWM_TriggerStopOrKill_Single(obj->base, _CYHAL_TCPWM_CNT_NUMBER(obj->resource));
    Cy_TCPWM_Counter_SetCounter(obj->base, _CYHAL_TCPWM_CNT_NUMBER(obj->resource), obj->default_value);
    if (is_running)
        Cy_TCPWM_TriggerStart_Single(obj->base, _CYHAL_TCPWM_CNT_NUMBER(obj->resource));
    #else
     if (is_running)
        Cy_TCPWM_TriggerStopOrKill(obj->base, 1 << _CYHAL_TCPWM_CNT_NUMBER(obj->resource));
    Cy_TCPWM_Counter_SetCounter(obj->base, _CYHAL_TCPWM_CNT_NUMBER(obj->resource), obj->default_value);
    if (is_running)
        Cy_TCPWM_TriggerStart(obj->base, 1 << _CYHAL_TCPWM_CNT_NUMBER(obj->resource));
    #endif

    return CY_RSLT_SUCCESS;
}

uint32_t cyhal_timer_read(const cyhal_timer_t *obj)
{
    CY_ASSERT(NULL != obj);
    return Cy_TCPWM_Counter_GetCounter(obj->base, _CYHAL_TCPWM_CNT_NUMBER(obj->resource));
}

#if defined(__cplusplus)
}
#endif

#endif /* defined(CY_IP_MXTCPWM_INSTANCES) */