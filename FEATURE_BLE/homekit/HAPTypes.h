/*
 * HAPTypes.h
 *
 *  Created on: Dec 3, 2019
 *      Author: gdbeckstein
 */

#ifndef FEATURE_BLE_HOMEKIT_HAPTYPES_H_
#define FEATURE_BLE_HOMEKIT_HAPTYPES_H_

#include "platform/Span.h"

#include <stdint.h>

namespace HAP
{

	typedef mbed::Span<const uint8_t, 6> device_id_t;
}


#endif /* FEATURE_BLE_HOMEKIT_HAPTYPES_H_ */
