/*
 * HAPRegularAdvertisementBuilder.h
 *
 *  Created on: Dec 3, 2019
 *      Author: gdbeckstein
 *
 *  Developed using HAP Specification Non-Commercial Release R2
 */

#ifndef FEATURE_BLE_HOMEKIT_HAPREGULARADVERTISEMENTBUILDER_H_
#define FEATURE_BLE_HOMEKIT_HAPREGULARADVERTISEMENTBUILDER_H_

#include "HAPTypes.h"
#include "AdvertisingDataSimpleBuilder.h"

#include "platform/Span.h"

#include "ble/BLETypes.h"

namespace HAP
{
	/**
	 * Apple HomeKit Accessory Protocol (HAP) Regular Advertisement Builder
	 *
	 * Builds an advertisement payload conforming to HAP Specification R2 Section 7.4.2.1
	 */
	class HAPRegularAdvertisementBuilder
	{

	public:

		typedef mbed::Span<const uint8_t, 4> setup_hash_t;

		typedef enum {
			HAP_PAIRING_STATUS_PAIRED	= 0x0, /** The accessory has been paired with a controllers. */
			HAP_PAIRING_STATUS_UNPAIRED	= 0x1  /** The accessory has not been paired with any controllers. */
		} hap_pairing_flag_t;


	public:

		HAPRegularAdvertisementBuilder(hap_pairing_flag_t status_flags, device_id_t device_id, uint16_t category_id,
				uint16_t global_state_number, uint8_t config_number, setup_hash_t setup_hash) : builder() {

			/**
			 * Set the flags according to Section 7.4.2.1.1
			 */
			builder.setFlags(ble::adv_data_flags_t::BREDR_NOT_SUPPORTED |
					ble::adv_data_flags_t::LE_GENERAL_DISCOVERABLE);

			/** Company ID (0x004C, Apple, Inc) */
			manufacturer_data[0] = 0x4C;
			manufacturer_data[1] = 0x00;

			/** Type */
			manufacturer_data[2] = 0x06;

			/** Subtype/Length */
			manufacturer_data[3] = 0x31;

			/** Status Flags */
			manufacturer_data[4] = status_flags;

			/** Device ID */
			memcpy(&manufacturer_data[5], device_id.data(), device_id.size());

			/** Accessory Category ID */
			memcpy(&manufacturer_data[11], &category_id, sizeof(category_id));

			/** Global State Number */
			memcpy(&manufacturer_data[13], &global_state_number, sizeof(global_state_number));

			/** Configuration Number */
			manufacturer_data[15] = config_number;

			/** Compatible Version */
			manufacturer_data[16] = 0x02;

			/** Setup Hash */
			memcpy(&manufacturer_data[17], setup_hash.data(), setup_hash.size());

			/** Set up the HAP manufacturer data payload */
			builder.setManufacturerSpecificData(mbed::make_Span<const uint8_t, 21>(manufacturer_data));

		}

		/**
		 * Gets the advertising data for this HAPRegularAdvertisementDataBuilder
		 *
		 * @retval advertising_data Payload, may be passed directly to `gap::setAdvertisingPayload`
		 */
		mbed::Span<const uint8_t> get_advertising_data(void) {
			return builder.getAdvertisingData();
		}

	public:

		/** HAP Regular Advertisement Payload Length is 21-bytes (+2 type/length bytes) */
		static const unsigned int payload_length = 21;

	protected:

		ble::AdvertisingDataSimpleBuilder<ble::LEGACY_ADVERTISING_MAX_SIZE> builder;
		uint8_t manufacturer_data[payload_length];

	};

}


#endif /* FEATURE_BLE_HOMEKIT_HAPREGULARADVERTISEMENTBUILDER_H_ */
