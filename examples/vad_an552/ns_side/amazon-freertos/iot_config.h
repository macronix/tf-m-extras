/*
 * FreeRTOS V202012.00
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/* This file contains configuration settings for the demos. */

#ifndef IOT_CONFIG_H_
#define IOT_CONFIG_H_

#include "FreeRTOS.h"
#include "platform/iot_platform_types_freertos.h"

#define IotThreads_Free                         vPortFree

/* Platform thread stack size and priority. */
#define IOT_THREAD_DEFAULT_PRIORITY             (tskIDLE_PRIORITY)
#define IOT_THREAD_DEFAULT_STACK_SIZE           (configMINIMAL_STACK_SIZE * 2)
#define IOT_NETWORK_RECEIVE_TASK_PRIORITY       (tskIDLE_PRIORITY + 1)
#define IOT_NETWORK_RECEIVE_TASK_STACK_SIZE     (configMINIMAL_STACK_SIZE * 2)

#endif /* ifndef IOT_CONFIG_H_ */
