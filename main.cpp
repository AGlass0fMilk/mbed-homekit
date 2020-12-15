/*
 * main.cpp
 *
 *  Created on: Dec 3, 2019
 *      Author: gdbeckstein
 */

#include <stdint.h>
#include <stdio.h>

#include "events/EventQueue.h"
#include "platform/Callback.h"
#include "platform/NonCopyable.h"

#include "ble/BLE.h"
#include "ble/Gap.h"
#include "ble/GapAdvertisingParams.h"
#include "ble/GapAdvertisingData.h"
#include "ble/FunctionPointerWithContext.h"

#include "homekit/HAPRegularAdvertisementBuilder.h"

static const char device_name[] = "mbed-homekit"; // Mbed HomeKit
static const uint8_t device_id[6] = {0xCA, 0xFE, 0xBA, 0xBE, 0x13, 0x37}; // Random
static const uint8_t setup_hash[4] = {0x12, 0x53, 0x3A, 0x4F}; // Random

/**
 * Handle initialization adn shutdown of the BLE Instance.
 *
 * Setup advertising payload and manage advertising state.
 * Delegate to GattClientProcess once the connection is established.
 */
class BLEProcess : private mbed::NonCopyable<BLEProcess>, public ble::Gap::EventHandler {
public:
    /**
     * Construct a BLEProcess from an event queue and a ble interface.
     *
     * Call start() to initiate ble processing.
     */
    BLEProcess(events::EventQueue &event_queue, BLE &ble_interface) :
        _event_queue(event_queue),
        _ble_interface(ble_interface),
        _post_init_cb() {
    }

    ~BLEProcess()
    {
        stop();
    }

   /**
     * Subscription to the ble interface initialization event.
     *
     * @param[in] cb The callback object that will be called when the ble
     * interface is initialized.
     */
    void on_init(mbed::Callback<void(BLE&, events::EventQueue&)> cb)
    {
        _post_init_cb = cb;
    }

    /**
     * Initialize the ble interface, configure it and start advertising.
     */
    bool start()
    {
        printf("ble: process started.\r\n");

        if (_ble_interface.hasInitialized()) {
            printf("ble: error, the ble instance has already been initialized.\r\n");
            return false;
        }

        _ble_interface.onEventsToProcess(
            makeFunctionPointer(this, &BLEProcess::schedule_ble_events)
        );

        ble_error_t error = _ble_interface.init(
            this, &BLEProcess::when_init_complete
        );

        if (error) {
            printf("ble: error %u returned by BLE::init.\r\n", error);
            return false;
        }

        return true;
    }

    /**
     * Close existing connections and stop the process.
     */
    void stop()
    {
        if (_ble_interface.hasInitialized()) {
            _ble_interface.shutdown();
            printf("ble: process stopped.");
        }
    }

private:

    /**
     * Schedule processing of events from the BLE middleware in the event queue.
     */
    void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *event)
    {
        _event_queue.call(mbed::callback(&event->ble, &BLE::processEvents));
    }

    /**
     * Sets up adverting payload and start advertising.
     *
     * This function is invoked when the ble interface is initialized.
     */
    void when_init_complete(BLE::InitializationCompleteCallbackContext *event)
    {
        if (event->error) {
            printf("ble: error %u during the initialization\r\n", event->error);
            return;
        }
        printf("ble: instance initialized\r\n");

        Gap &gap = _ble_interface.gap();
        gap.onConnection(this, &BLEProcess::when_connection);
        gap.onDisconnection(this, &BLEProcess::when_disconnection);

        if (!set_advertising_parameters()) {
            return;
        }

        if (!set_advertising_data()) {
            return;
        }

        if (!start_advertising()) {
            return;
        }

        if (_post_init_cb) {
            _post_init_cb(_ble_interface, _event_queue);
        }
    }

    void when_connection(const Gap::ConnectionCallbackParams_t *connection_event)
    {
        printf("ble: connected\r\n");
    }

    void when_disconnection(const Gap::DisconnectionCallbackParams_t *event)
    {
        printf("ble: disconnected\r\n");
        start_advertising();
    }
public:
    bool start_advertising(void)
    {
        Gap &gap = _ble_interface.gap();

        /* Start advertising the set */
        ble_error_t error = gap.startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (error) {
            printf("Error %u during gap.startAdvertising.\r\n", error);
            return false;
        } else {
            printf("ble: advertising started.\r\n");
            return true;
        }
    }

private:
    bool set_advertising_parameters()
    {
        Gap &gap = _ble_interface.gap();

        ble_error_t error = gap.setAdvertisingParameters(
            ble::LEGACY_ADVERTISING_HANDLE,
            ble::AdvertisingParameters()
        );

        if (error) {
            printf("ble: Gap::setAdvertisingParameters() failed with error %d", error);
            return false;
        }

        return true;
    }

    bool set_advertising_data()
    {
        Gap &gap = _ble_interface.gap();

        ble_error_t error = gap.setAdvertisingPayload(
            ble::LEGACY_ADVERTISING_HANDLE,
			HAP::HAPRegularAdvertisementBuilder(HAP::HAPRegularAdvertisementBuilder::HAP_PAIRING_STATUS_UNPAIRED,
			mbed::make_Span<const uint8_t, 6>(device_id),
			5, 0, 1, mbed::make_Span<const uint8_t, 4>(setup_hash))
        		.get_advertising_data()
		);

        if (error) {
            printf("ble: Gap::setAdvertisingPayload() failed with error %d", error);
            return false;
        }

        gap.setAdvertisingScanResponse(ble::LEGACY_ADVERTISING_HANDLE,
        		ble::AdvertisingDataSimpleBuilder<ble::LEGACY_ADVERTISING_MAX_SIZE>()
				.setName(device_name)
				.getAdvertisingData());

        if (error) {
            printf("ble: Gap::setAdvertisingScanResponse() failed with error %d", error);
            return false;
        }

        gap.setDeviceName((const unsigned char* )device_name);

        return true;
    }

    events::EventQueue &_event_queue;
    BLE &_ble_interface;
    mbed::Callback<void(BLE&, events::EventQueue&)> _post_init_cb;


};

events::EventQueue main_queue;
BLEProcess* ble_process_ptr;

int main(void) {

	BLE& ble = BLE::Instance();
	BLEProcess ble_process(main_queue, ble);
	ble_process.start();

	main_queue.dispatch_forever();

	return 0;
}

