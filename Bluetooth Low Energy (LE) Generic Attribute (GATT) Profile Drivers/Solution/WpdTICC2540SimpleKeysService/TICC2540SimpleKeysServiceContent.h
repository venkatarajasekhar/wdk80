/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    TICC2540SimpleKeysServiceContent.h

Abstract:

    Contains TI CC2540 Simple Keys Service Declaration.

--*/


#pragma once

class TICC2540SimpleKeysServiceContent : public AbstractDeviceContent
{
public:
    TICC2540SimpleKeysServiceContent()
    {
        ObjectID                    = SERVICE_OBJECT_ID;
        PersistentUniqueID          = SERVICE_PERSISTENT_UNIQUE_ID;
        ParentID                    = WPD_DEVICE_OBJECT_ID;
        Name                        = SERVICE_OBJECT_NAME_VALUE;
        ContentType                 = WPD_CONTENT_TYPE_UNSPECIFIED;
        Format                      = WPD_OBJECT_FORMAT_UNSPECIFIED;
        ParentPersistentUniqueID    = WPD_DEVICE_OBJECT_ID;
        ContainerFunctionalObjectID = WPD_DEVICE_OBJECT_ID;
        Version                     = SERVICE_VERSION;
        FunctionalCategory          = SERVICE_TICC2540SimpleKeysService;
        HumanReadableName           = SERVICE_HUMAN_READABLE_NAME;
        RequiredScope               = BLUETOOTH_GATT_SERVICE_ACCESS;
        m_Value                     = 0;

        CComCritSecLock<CComAutoCriticalSection> Lock(m_CCCDSetCS);
        m_bCCCDSet                  = FALSE;
    }

    TICC2540SimpleKeysServiceContent(const AbstractDeviceContent& src) : m_ValueChangeEventReg(NULL)
    {
        *this = src;
    }

    virtual ~TICC2540SimpleKeysServiceContent();

    virtual HRESULT InitializeContent(
        _In_ IWDFDevice* pDevice,
        _Inout_ DWORD *pdwLastObjectID,
        _In_ HANDLE hDeviceHandle,
        _In_ BthLEDevice * pBthLEDevice);

    virtual HRESULT CreatePropertiesOnlyObject(
        _In_    IPortableDeviceValues* pObjectProperties,
        _Inout_ DWORD*                 pdwLastObjectID,
        _Inout_ AbstractDeviceContent**          ppNewObject);

    virtual HRESULT GetSupportedProperties(
        _Inout_ IPortableDeviceKeyCollection *pKeys);

    virtual HRESULT GetPropertyAttributes(
                REFPROPERTYKEY         Key,
        _Inout_ IPortableDeviceValues* pAttributes);

    virtual HRESULT GetValue(
                REFPROPERTYKEY         Key,
        _Inout_ IPortableDeviceValues* pStore);

    virtual HRESULT DispatchClientArrival();

    virtual HRESULT DispatchClientDeparture();

    virtual HRESULT DispatchClientApplicationActivated();

    virtual HRESULT DispatchClientApplicationSuspended();

    virtual HRESULT DispatchDeviceConnected();

    virtual HRESULT DispatchDeviceDisconnected();

    static VOID WINAPI s_ValueChangeEvent(
        _In_ BTH_LE_GATT_EVENT_TYPE EventType,
        _In_ PVOID EventOutParameter,
        _In_ PVOID Context
        );

    VOID ValueChangeEvent(
        _In_ BTH_LE_GATT_EVENT_TYPE EventType,
        _In_ PVOID EventOutParameter
        );

    HRESULT SetCCCD();

public:
    CAtlStringW                 Version;
    CAtlStringW                 HumanReadableName;
    GUID                        FunctionalCategory;

private:
    HANDLE                      m_hDeviceHandle;

    BTH_LE_GATT_CHARACTERISTIC  m_KeyPressStateCharacteristic;
    BLUETOOTH_GATT_EVENT_HANDLE m_ValueChangeEventReg;
    CComPtr<IWpdSerializer>     m_pWpdSerializer;
    CComPtr<IWDFDevice>         m_pDevice;
    BthLEDevice *               m_pBthLEDevice;

    CComAutoCriticalSection     m_ValueCS;
    BYTE                        m_Value;

    BOOLEAN                     m_bCCCDSet;
    CComAutoCriticalSection     m_CCCDSetCS;
};

