#include "wasapi-notify.hpp"

#include <obs-module.h>

#include <util/windows/win-version.h>

TRACELOGGING_DEFINE_PROVIDER(    // defines g_hOBSWASAPIProvider
	g_hOBSWASAPIProvider,    // Name of the provider handle
	"com.obsproject.Studio", // Human-readable name of the provider
	// ETW Control GUID: {f3c6d1a1-9615-47a8-a366-761d7600663b}
	(0xf3c6d1a1, 0x9615, 0x47a8, 0xa3, 0x66, 0x76, 0x1d, 0x76, 0x00, 0x66,
	 0x3b));

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-wasapi", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Windows WASAPI audio input/output sources";
}

void RegisterWASAPIInput();
void RegisterWASAPIDeviceOutput();
void RegisterWASAPIProcessOutput();

WASAPINotify *notify = nullptr;

static void default_device_changed_callback(EDataFlow flow, ERole, LPCWSTR)
{
	const char *id;
	obs_get_audio_monitoring_device(nullptr, &id);

	if (!id || strcmp(id, "default") != 0)
		return;

	if (flow != eRender)
		return;

	auto task = [](void *) {
		obs_reset_audio_monitoring();
	};

	obs_queue_task(OBS_TASK_UI, task, nullptr, false);
}

bool obs_module_load(void)
{
	/* MS says 20348, but process filtering seems to work earlier */
	struct win_version_info ver;
	get_win_ver(&ver);
	struct win_version_info minimum;
	minimum.major = 10;
	minimum.minor = 0;
	minimum.build = 19041;
	minimum.revis = 0;
	const bool process_filter_supported =
		win_version_compare(&ver, &minimum) >= 0;

	RegisterWASAPIInput();
	RegisterWASAPIDeviceOutput();
	if (process_filter_supported)
		RegisterWASAPIProcessOutput();

	notify = new WASAPINotify();
	notify->AddDefaultDeviceChangedCallback(
		obs_current_module(), default_device_changed_callback);

	TraceLoggingRegister(g_hOBSWASAPIProvider);

	return true;
}

void obs_module_unload(void)
{
	if (notify) {
		delete notify;
		notify = nullptr;
	}

	TraceLoggingUnregister(g_hOBSWASAPIProvider);
}

WASAPINotify *GetNotify()
{
	return notify;
}
