/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    HealthHeartRateServiceContent.cpp

Abstract:

    Contains Health HeartRate Service Implementation.


--*/

#include "stdafx.h"

#include "HealthHeartRateServiceContent.tmh"

const PropertyAttributeInfo g_SupportedServiceProperties[] =
{
    //
    // Common service properties
    //
    {&WPD_OBJECT_ID,                                VT_LPWSTR,          UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_OBJECT_PERSISTENT_UNIQUE_ID,              VT_LPWSTR,          UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_OBJECT_PARENT_ID,                         VT_LPWSTR,          UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_OBJECT_NAME,                              VT_LPWSTR,          UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_OBJECT_FORMAT,                            VT_CLSID,           UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_OBJECT_CONTENT_TYPE,                      VT_CLSID,           UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_OBJECT_CAN_DELETE,                        VT_BOOL,            UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID,    VT_LPWSTR,          UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_FUNCTIONAL_OBJECT_CATEGORY,               VT_CLSID,           UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&WPD_SERVICE_VERSION,                          VT_LPWSTR,          UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NULL},
    {&PKEY_Services_ServiceDisplayName,             VT_LPWSTR,          UnspecifiedForm_CanRead_CannotWrite_CannotDelete_Fast, NAME_Services_ServiceDisplayName},

    //
    // Characteristics of the HealthHeartRate Service
    //
};

HealthHeartRateServiceContent::~HealthHeartRateServiceContent()
{
    //
    // Remove the value change event
    //
    if (NULL != m_ValueChangeEventReg)
    {
        BluetoothGATTUnregisterEvent(m_ValueChangeEventReg, BLUETOOTH_GATT_FLAG_NONE);
        m_ValueChangeEventReg = NULL;
    }
}


HRESULT HealthHeartRateServiceContent::InitializeContent(
    _In_ IWDFDevice* pDevice,
    _Inout_ DWORD *pdwLastObjectID,
    _In_ HANDLE hDeviceHandle,
    _In_ BthLEDevice * pBthLEDevice)
{
    HRESULT hr = S_OK;

    //
    // Service
    //
    PBTH_LE_GATT_SERVICE services = NULL;
    USHORT numServices = 0;
    USHORT numServicesActual = 0;
    USHORT serviceIndex = 0;
    BOOLEAN isFoundService = FALSE;

    //
    // Characteristic
    //
    PBTH_LE_GATT_CHARACTERISTIC characteristics = NULL;
    USHORT numChar = 0;
    USHORT numCharsActual = 0;
    BOOLEAN isCharacteristicFound = FALSE;
    USHORT foundCharacteristicIndex = 0;

    if (pDevice == NULL ||
        pdwLastObjectID == NULL ||
        hDeviceHandle == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
    }

    //
    // Save the WDF device for async events
    //
    if (SUCCEEDED(hr))
    {
        m_pDevice = pDevice;
        m_pBthLEDevice = pBthLEDevice;        
    }

    //
    // Get the WpdSerializer to use to Object Change events
    //
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_WpdSerializer,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IWpdSerializer,
                              (VOID**)&m_pWpdSerializer);
        CHECK_HR(hr, "Failed to CoCreate CLSID_WpdSerializer");
    }


    //
    // Get the number of services
    //
    if (SUCCEEDED(hr))
    {
        hr = BluetoothGATTGetServices(
                            hDeviceHandle,
                            0,
                            NULL,
                            &numServices,
                            BLUETOOTH_GATT_FLAG_NONE);

        if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) == hr)
        {
            hr = S_OK;
        }
        else
        {
            CHECK_HR(hr, "Failed to get the service count - changing to E_FAIL");
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        services = new BTH_LE_GATT_SERVICE[numServices];
        if (NULL == services)
        {
            hr = E_OUTOFMEMORY;
        }
        CHECK_HR(hr, "Failed to allocate the service buffer");
    }

    if (SUCCEEDED(hr))
    {
        //
        // Get the services
        //
        hr = BluetoothGATTGetServices(
                            hDeviceHandle,
                            numServices,
                            services,
                            &numServicesActual,
                            BLUETOOTH_GATT_FLAG_NONE);
        CHECK_HR(hr, "Failed to get the services");
    }

    if (SUCCEEDED(hr))
    {
        //
        // Double check to confirm that the number of services that are returned
        // are what we have expected, just in case if the cache has changed due
        // to things like service change events.
        //
        if (numServices != numServicesActual)
        {
            // The returned number of services do not match what we have
            // originally.
            hr = E_FAIL;
        }
        CHECK_HR(hr, "The service count doesn't match the returned services");
    }

    if (SUCCEEDED(hr))
    {
        //
        // Find the service in the list
        //
        for(USHORT i = 0; i < numServices; i++)
        {
            if (IsHealthHeartRateServiceUuid(services[i].ServiceUuid)) {
                isFoundService = TRUE;
                serviceIndex = i;
                break;
            }            
        }

        if (FALSE == isFoundService)
        {
            hr = E_FAIL;
        }
        CHECK_HR(hr, "The HealthHeartRate Service was not found");

    }

    if (SUCCEEDED(hr))
    {
        //
        // Find the key press state characteristic
        //
        hr = BluetoothGATTGetCharacteristics(
                hDeviceHandle,
                &services[serviceIndex],
                0,
                NULL,
                &numChar,
                BLUETOOTH_GATT_FLAG_NONE);

        if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) == hr)
        {
            hr = S_OK;
        }
        else
        {
            CHECK_HR(hr, "Failed to get the Characteristic count - changing to E_FAIL");
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        characteristics= new BTH_LE_GATT_CHARACTERISTIC[numChar];
        if (NULL == characteristics)
        {
            hr = E_OUTOFMEMORY;
        }
        CHECK_HR(hr, "Failed to allocate the Characteristic buffer");
    }

    if (SUCCEEDED(hr))
    {
        hr = BluetoothGATTGetCharacteristics(
                hDeviceHandle,
                &services[serviceIndex],
                numChar,
                characteristics,
                &numCharsActual,
                BLUETOOTH_GATT_FLAG_NONE);
        CHECK_HR(hr, "Failed to get the Characteristics");
    }

    if (SUCCEEDED(hr))
    {
        //
        // Double check to confirm that the number of characteristics that are
        // returned are what we have expected,.
        //
        if (numChar != numCharsActual)
        {
            // The returned number of characteristics do not match what we have
            // originally.
            hr = E_FAIL;
        }
        CHECK_HR(hr, "Number of Characteristics do not match");
    }

    if (SUCCEEDED(hr))
    {
        for (USHORT i = 0; i < numChar; i++)
        {
            if (IsHeartRateMeasurementCharacteristic(characteristics[i].CharacteristicUuid))
            {
                isCharacteristicFound = TRUE;
                foundCharacteristicIndex = i;
                break;
                }
        }

        if (FALSE == isCharacteristicFound)
        {
            hr = E_FAIL;
        }
        CHECK_HR(hr, "HeartRateMeasurement characteristic not found");
    }

    if (SUCCEEDED(hr))
    {
        //
        // Cache the characteristic to differ reading the value when the client requests it (client polling)
        //
        CopyMemory(&m_HeartRateMeasurementCharacteristic, &characteristics[foundCharacteristicIndex], sizeof(m_HeartRateMeasurementCharacteristic));
            
        //
        // Setting the member DeviceHandle for use by SetCCCD
        //
        m_hDeviceHandle = hDeviceHandle;

        hr = SetCCCD();
        CHECK_HR(hr, "Setting the CCCD on Device Content initialization failed");

        if (SUCCEEDED(hr)) 
        {
            //
            // Setting a member flag to ensure we don't try to set the CCCD more than once
            // This is a performance optimization that avoids a cache query to get the current CCCD value
            // and also avoids resetting the CCCD (over the air - with the biggest performance hit)
            //

            CComCritSecLock<CComAutoCriticalSection> Lock(m_CCCDSetCS);
            m_bCCCDSet = TRUE;
        }

        //
        // This only needs to be set once, so ignoring errors in the event the device is not connectable (ie: reboot)
        //
        hr = S_OK;
    }

    //
    // Cleanup
    //
    if (NULL != services)
    {
        delete [] services;
    }

    if (NULL != characteristics)
    {
        delete [] characteristics;
    }

    return hr;
}

HRESULT HealthHeartRateServiceContent::CreatePropertiesOnlyObject(
    _In_    IPortableDeviceValues* pObjectProperties,
    _Inout_ DWORD*                 pdwLastObjectID,
    _Inout_ AbstractDeviceContent**          ppNewObject)
{
    UNREFERENCED_PARAMETER(pObjectProperties);
    UNREFERENCED_PARAMETER(pdwLastObjectID);
    UNREFERENCED_PARAMETER(ppNewObject);

    return S_OK;
}

HRESULT HealthHeartRateServiceContent::GetSupportedProperties(
    _Inout_ IPortableDeviceKeyCollection* pKeys)
{
    HRESULT hr = S_OK;

    if (pKeys == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    for (DWORD dwIndex = 0; (dwIndex < ARRAYSIZE(g_SupportedServiceProperties)) && (hr == S_OK); dwIndex++)
    {
        // Common WPD service properties
        hr = pKeys->Add(*g_SupportedServiceProperties[dwIndex].pKey);
        CHECK_HR(hr, "Failed to add common service property");
    }

    return hr;
}

HRESULT HealthHeartRateServiceContent::GetPropertyAttributes(
            REFPROPERTYKEY         Key,
    _Inout_ IPortableDeviceValues* pAttributes)
{
    HRESULT hr = S_OK;

    if(pAttributes == NULL)
    {
        hr = E_POINTER;
        CHECK_HR(hr, "Cannot have NULL parameter");
        return hr;
    }

    hr = SetPropertyAttributes(Key, &g_SupportedServiceProperties[0], ARRAYSIZE(g_SupportedServiceProperties), pAttributes);

    return hr;
}

HRESULT HealthHeartRateServiceContent::GetValue(
            REFPROPERTYKEY         Key,
    _Inout_ IPortableDeviceValues* pStore)
{
    HRESULT hr = S_OK;

    PropVariantWrapper pvValue;

    //
    // Common Service properties
    //
    if (IsEqualPropertyKey(Key, WPD_OBJECT_ID))
    {
        // Add WPD_OBJECT_ID
        pvValue = ObjectID;
        hr = pStore->SetValue(WPD_OBJECT_ID, &pvValue);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_ID"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_NAME))
    {
        // Add WPD_OBJECT_NAME
        pvValue = Name;
        hr = pStore->SetValue(WPD_OBJECT_NAME, &pvValue);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_NAME"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_PERSISTENT_UNIQUE_ID))
    {
        // Add WPD_OBJECT_PERSISTENT_UNIQUE_ID
        pvValue = PersistentUniqueID;
        hr = pStore->SetValue(WPD_OBJECT_PERSISTENT_UNIQUE_ID, &pvValue);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_PERSISTENT_UNIQUE_ID"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_PARENT_ID))
    {
        // Add WPD_OBJECT_PARENT_ID
        pvValue = ParentID;
        hr = pStore->SetValue(WPD_OBJECT_PARENT_ID, &pvValue);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_PARENT_ID"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_FORMAT))
    {
        // Add WPD_OBJECT_FORMAT
        hr = pStore->SetGuidValue(WPD_OBJECT_FORMAT, Format);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_FORMAT"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_CONTENT_TYPE))
    {
        // Add WPD_OBJECT_CONTENT_TYPE
        hr = pStore->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, ContentType);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_CONTENT_TYPE"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_CAN_DELETE))
    {
        // Add WPD_OBJECT_CAN_DELETE
        hr = pStore->SetBoolValue(WPD_OBJECT_CAN_DELETE, CanDelete);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_CAN_DELETE"));
    }
    else if (IsEqualPropertyKey(Key, WPD_FUNCTIONAL_OBJECT_CATEGORY))
    {
        // Add WPD_FUNCTIONAL_OBJECT_CATEGORY
        hr = pStore->SetGuidValue(WPD_FUNCTIONAL_OBJECT_CATEGORY, FunctionalCategory);
        CHECK_HR(hr, ("Failed to set WPD_FUNCTIONAL_OBJECT_CATEGORY"));
    }
    else if (IsEqualPropertyKey(Key, WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID))
    {
        // Add WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID
        hr = pStore->SetStringValue(WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID, ContainerFunctionalObjectID);
        CHECK_HR(hr, ("Failed to set WPD_OBJECT_CONTAINER_FUNCTIONAL_OBJECT_ID"));
    }
    else if (IsEqualPropertyKey(Key, WPD_SERVICE_VERSION))
    {
        // Add WPD_SERVICE_VERSION
        hr = pStore->SetStringValue(WPD_SERVICE_VERSION, Version);
        CHECK_HR(hr, ("Failed to set WPD_SERVICE_VERSION"));
    }
    else if (IsEqualPropertyKey(Key, PKEY_Services_ServiceDisplayName))
    {
        // Add PKEY_Services_ServiceDisplayName
        pvValue = HumanReadableName;
        hr = pStore->SetValue(PKEY_Services_ServiceDisplayName, &pvValue);
        CHECK_HR(hr, ("Failed to set PKEY_Services_ServiceDisplayName"));
    }

    //
    // Begin Gatt Service properties
    //
    
    //
    // End Gatt Service properties
    //
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        CHECK_HR(hr, "Property {%ws}.%d is not supported", CComBSTR(Key.fmtid), Key.pid);
    }

    return hr;
}

HRESULT HealthHeartRateServiceContent::DispatchClientArrival(
    )
{
    HRESULT hr = S_OK;
    
    //
    // A service which doesn't need to retain data while there are no clients listening
    // should not register an event callback.  This event notifies the Service content
    // that a new client as arrived and that the service content should register it's callback if
    // it hasn't already done so.
    //
    CComCritSecLock<CComAutoCriticalSection> Lock(m_ClientCS);

    if (0 == m_ClientCount)
    {
        //
        // Must register the client callback
        //
        BLUETOOTH_GATT_VALUE_CHANGED_EVENT_REGISTRATION registration = {0};

        //
        // Register a value change callback (asynchronous value change events)
        //
        registration.NumCharacteristics = 1;
        registration.Characteristics[0] = m_HeartRateMeasurementCharacteristic;

        hr = BluetoothGATTRegisterEvent(
                m_hDeviceHandle,
                CharacteristicValueChangedEvent,
                (PVOID)&registration,
                s_HeartRateMeasurementEvent,
                (PVOID)this,
                &m_ValueChangeEventReg,
                BLUETOOTH_GATT_FLAG_NONE);
        CHECK_HR(hr, "Failed to register for Value change events");
    }

    m_ClientCount++;
    
    return hr;
}

HRESULT HealthHeartRateServiceContent::DispatchClientDeparture(
    )
{
    HRESULT hr = S_OK;
    
    //
    // A service which doesn't need to retain data while there are no clients listening
    // not register an event callback.  This event notifies the Service content
    // that a client as departed and that the service content should unregister it's callback.
    //
    CComCritSecLock<CComAutoCriticalSection> Lock(m_ClientCS);    
    if (0 == m_ClientCount)
    {
        hr = E_FAIL;
        CHECK_HR(hr, "Too many client departure");
        return hr;
    }

    if (1 == m_ClientCount)
    {
        //
        // Must unregister the event callback
        //
        if (NULL != m_ValueChangeEventReg)
        {
            BluetoothGATTUnregisterEvent(m_ValueChangeEventReg, BLUETOOTH_GATT_FLAG_NONE);
            m_ValueChangeEventReg = NULL;
        }
    }

    m_ClientCount--;

    return hr;
}

HRESULT HealthHeartRateServiceContent::DispatchDeviceConnected(
    )
{
    HRESULT hr = S_OK;

    //
    // If not already set we must set the CCCD
    // Setting the CCCD here handles the important case that the device may not still be connected
    // when the driver is loaded and the Service Content is initialized
    //

    CComCritSecLock<CComAutoCriticalSection> Lock(m_CCCDSetCS);

    if (!m_bCCCDSet)
    {
        hr = SetCCCD();
        CHECK_HR(hr, "Failed to set the CCCD on the device connected event");

        if (SUCCEEDED(hr))
        {
            m_bCCCDSet = TRUE;
        }

        //
        // The CCCD only needs to be set once so if the device becomes disconnected by the time 
        // SetCCCD completes, then we can try again next time the device becomes connected
        //
        hr = S_OK;
    }

    //
    // If you need to handle the device connected state add the code for the connected event here
    //

    return hr;
}

HRESULT HealthHeartRateServiceContent::DispatchDeviceDisconnected(
    )
{
    HRESULT hr = S_OK;

    //
    // If you need to handle the device connected state add the code for the disconnected event here
    //

    return hr;
}


VOID WINAPI
HealthHeartRateServiceContent::s_HeartRateMeasurementEvent(
    _In_ BTH_LE_GATT_EVENT_TYPE EventType,
    _In_ PVOID EventOutParameter,
    _In_ PVOID Context
    )
{
    //
    // Redirect to the appropriate instance
    //
    ((HealthHeartRateServiceContent *)Context)->m_pBthLEDevice->GetGattService()
                ->HeartRateMeasurementEvent(EventType,
                                             EventOutParameter);
}


HRESULT HealthHeartRateServiceContent::SetCCCD(
    )
{
    HRESULT hr = S_OK;

    //
    // Client configuration Descriptor
    //
    PBTH_LE_GATT_DESCRIPTOR descriptors = NULL;
    USHORT numDesc = 0;
    USHORT numDescsActual = 0;
    BOOLEAN isFoundClientConfigDesc = FALSE;
    USHORT clientConfigDescIndex = 0;
    BTH_LE_GATT_DESCRIPTOR_VALUE clientConfigValue;

    ::ZeroMemory(&clientConfigValue, sizeof(clientConfigValue));

    //
    // Find the Client configuration descriptor
    //
    if (SUCCEEDED(hr))
    {
        // First call to determine buffer size.
        hr = BluetoothGATTGetDescriptors(
            m_hDeviceHandle,
            &m_HeartRateMeasurementCharacteristic,
            0,
            NULL,
            &numDesc,
            BLUETOOTH_GATT_FLAG_NONE);

        if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) == hr)
        {
            hr = S_OK;
        }
        else
        {
            CHECK_HR(hr, "Failed to get the Descriptor count - changing to E_FAIL");
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr)) 
    {    
        // Allocate memory for the descriptors buffer.
        descriptors = new BTH_LE_GATT_DESCRIPTOR[numDesc];
        if (NULL == descriptors) {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            ::ZeroMemory(descriptors, numDesc * sizeof(BTH_LE_GATT_DESCRIPTOR));
        }
        CHECK_HR(hr, "Failed to allocate the descriptor buffer");
    }

    if (SUCCEEDED(hr))
    {
        // Second call to retrieve the descriptors.
        hr = BluetoothGATTGetDescriptors(
            m_hDeviceHandle,
            &m_HeartRateMeasurementCharacteristic,
            numDesc,
            descriptors,
            &numDescsActual,
            BLUETOOTH_GATT_FLAG_NONE);
        CHECK_HR(hr, "Failed to get the descriptors");
    }

    if (SUCCEEDED(hr))
    {
        if (numDesc != numDescsActual) {
            hr = E_FAIL; 
        }
        CHECK_HR(hr, "Number of descriptors do not match");
    }

    if (SUCCEEDED(hr))
    {
        // Loop through the list of descriptors to see if the Client Characteristic
        // Configuration descriptor is present.
        for (USHORT i = 0; i < numDesc; i++) 
        {
            if (ClientCharacteristicConfiguration == descriptors[i].DescriptorType) {
                isFoundClientConfigDesc = true;
                clientConfigDescIndex = i;
                break;
            }
        }

        if (!isFoundClientConfigDesc)
        {
            hr = E_INVALIDARG;
        }
        CHECK_HR(hr, "Client Config descriptor not found");
    }

    if (SUCCEEDED(hr))
    {
        //
        // Set the client config descriptor to register for notifications
        //
        clientConfigValue.DescriptorType = ClientCharacteristicConfiguration;
        clientConfigValue.ClientCharacteristicConfiguration
            .IsSubscribeToNotification = TRUE;

        hr = BluetoothGATTSetDescriptorValue(
            m_hDeviceHandle,
            &descriptors[clientConfigDescIndex],
            &clientConfigValue,
            BLUETOOTH_GATT_FLAG_NONE);
        CHECK_HR(hr, "Failed to set the client config descriptor");
    }

    //
    // Cleanup
    //

    if (NULL != descriptors)
    {
        delete [] descriptors;
    }

    return hr;
}

