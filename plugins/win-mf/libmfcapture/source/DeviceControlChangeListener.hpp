#pragma once

#include "framework.hpp"

#include <util/windows/ComPtr.hpp>

#include <mfidl.h>
#include <string>

typedef enum _WS_CONTROL_TYPE {
	WS_EFFECT_UNKNOWN = -1,
	WS_BACKGROUND_STANDARD = 0,
	WS_BACKGROUND_PORTRAIT = 1,
	WS_BACKGROUND_MASK = 2,
	WS_BACKGROUND_NONE = 3,
	WS_AUTO_FRAMING = 4,
	WS_EYE_CONTACT = 5,
} WS_CONTROL_TYPE;

typedef enum _WS_CONTROL_VALUE {
	WS_EFFECT_OFF = 0,
	WS_EFFECT_ON = 1,
} WS_CONTROL_VALUE;

class DeviceControlChangeListener : public IMFCameraControlNotify {
	long m_cRef = 1;
	CRITICAL_SECTION m_lock;

	std::wstring m_wsDevId;
	ComPtr<IMFCameraControlMonitor> m_spMonitor = nullptr;
	ComPtr<IKsControl> m_spKsControl = nullptr;
	ComPtr<IMFExtendedCameraController> m_spExtCamController =
		nullptr;
	void *m_cbdata = nullptr;

	bool m_bStarted = false;

private:
	HRESULT HandleControlSet_ExtendedCameraControl(UINT32 id);

public:
	DeviceControlChangeListener();
	~DeviceControlChangeListener();

	// IUnknown methods
	IFACEMETHODIMP_(ULONG) AddRef(void) override;
	IFACEMETHODIMP_(ULONG) Release(void) override;
	IFACEMETHODIMP QueryInterface(_In_ REFIID riid,
				      _Out_ LPVOID *ppvObject) override;

	// IMFCameraControlNotify methods
	void STDMETHODCALLTYPE OnChange(_In_ REFGUID controlSet,
					_In_ UINT32 id);
	void STDMETHODCALLTYPE OnError(_In_ HRESULT hrStatus);

	HRESULT Start(const wchar_t *devId,
		      IMFExtendedCameraController *pExtCamController);

	HRESULT Stop();
};
