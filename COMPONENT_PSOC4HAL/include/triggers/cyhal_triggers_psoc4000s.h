/***************************************************************************//**
* \file cyhal_triggers_psoc4000s.h
*
* \brief
* PSoC4000S family HAL triggers header
*
* \note
* Generator version: 1.6.7458.30860
*
********************************************************************************
* \copyright
* Copyright 2016-2020 Cypress Semiconductor Corporation
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

#ifndef _CYHAL_TRIGGERS_PSOC4000S_H_
#define _CYHAL_TRIGGERS_PSOC4000S_H_

/**
 * \addtogroup group_hal_impl_triggers_psoc4000s PSOC4000S
 * \ingroup group_hal_impl_triggers
 * \{
 * Trigger connections for psoc4000s
 */

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/** @brief Name of each output trigger. */
typedef enum
{
    TRIGGER_TCPWM_TR_IN12 = 0, //!< TCPWM Triggers - tcpwm.tr_in[12]
    TRIGGER_TCPWM_TR_IN13 = 1, //!< TCPWM Triggers - tcpwm.tr_in[13]
} cyhal_trigger_dest_psoc4000s_t;

/** Typedef from device family specific trigger dest to generic trigger dest */
typedef cyhal_trigger_dest_psoc4000s_t cyhal_dest_t;

/** \cond INTERNAL */
/** Maps each cyhal_destination_t to a mux index.
 * If bit 8 of the mux index is set, this denotes that the trigger is a
 * one to one trigger.
 */
extern const uint8_t cyhal_dest_to_mux[2];

/* Maps each cyhal_destination_t to a specific output in its mux */
extern const uint8_t cyhal_mux_dest_index[2];
/** \endcond */

#if defined(__cplusplus)
}
#endif /* __cplusplus */
/** \} group_hal_impl_triggers_psoc4000s */
#endif /* _CYHAL_TRIGGERS_PSOC4000S_H_ */


/* [] END OF FILE */
