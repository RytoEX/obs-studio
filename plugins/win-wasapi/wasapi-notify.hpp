#pragma once

#include <util/windows/ComPtr.hpp>
#include <Mmdeviceapi.h>

#include <unordered_map>
#include <functional>
#include <mutex>

#include <TraceLoggingProvider.h>

TRACELOGGING_DECLARE_PROVIDER(g_hOBSWASAPIProvider);

typedef std::function<void(EDataFlow, ERole, LPCWSTR)>
	WASAPINotifyDefaultDeviceChangedCallback;

class NotificationClient;

class WASAPINotify {
public:
	WASAPINotify();
	~WASAPINotify();

	void AddDefaultDeviceChangedCallback(
		void *handle, WASAPINotifyDefaultDeviceChangedCallback cb);
	void RemoveDefaultDeviceChangedCallback(void *handle);

private:
	void OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR id);

	std::mutex mutex;

	ComPtr<IMMDeviceEnumerator> enumerator;
	ComPtr<NotificationClient> notificationClient;

	std::unordered_map<void *, WASAPINotifyDefaultDeviceChangedCallback>
		defaultDeviceChangedCallbacks;
};
