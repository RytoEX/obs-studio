#include <windows.h>
#include <strsafe.h>
#include <gdiplus.h>
#include <stdint.h>
#include <vector>

using namespace Gdiplus;

extern HINSTANCE dll_inst;
static std::vector<uint8_t> placeholder;
static wchar_t file[MAX_PATH] = {'\0'};
static bool initialized = false;

/* XXX: optimize this later.  or don't, it's only called once. */

static void convert_placeholder(const uint8_t *rgb_in, int width, int height)
{
	size_t size = width * height * 3;
	size_t linesize = width * 3;

	std::vector<uint8_t> yuv_out;
	yuv_out.resize(size);

	const uint8_t *in = rgb_in;
	const uint8_t *end = in + size;
	uint8_t *out = &yuv_out[0];

	while (in < end) {
		const int16_t b = *(in++);
		const int16_t g = *(in++);
		const int16_t r = *(in++);

		*(out++) = (uint8_t)(((66 * r + 129 * g + 25 * b + 128) >> 8) +
				     16);
		*(out++) = (uint8_t)(((-38 * r - 74 * g + 112 * b + 128) >> 8) +
				     128);
		*(out++) = (uint8_t)(((112 * r - 94 * g - 18 * b + 128) >> 8) +
				     128);
	}

	placeholder.resize(width * height * 3 / 2);

	in = &yuv_out[0];
	end = in + size;

	out = &placeholder[0];
	uint8_t *chroma = out + width * height;

	while (in < end) {
		const uint8_t *in2 = in + linesize;
		const uint8_t *end2 = in2;
		uint8_t *out2 = out + width;

		while (in < end2) {
			int16_t u;
			int16_t v;

			*(out++) = *(in++);
			u = *(in++);
			v = *(in++);

			*(out++) = *(in++);
			u += *(in++);
			v += *(in++);

			*(out2++) = *(in2++);
			u += *(in2++);
			v += *(in2++);

			*(out2++) = *(in2++);
			u += *(in2++);
			v += *(in2++);

			*(chroma++) = (uint8_t)(u / 4);
			*(chroma++) = (uint8_t)(v / 4);
		}

		in = in2;
		out = out2;
	}
}

static bool file_exists(const wchar_t *tmp_file)
{
	if (GetFileAttributes(tmp_file) == INVALID_FILE_ATTRIBUTES &&
	    GetLastError() == ERROR_FILE_NOT_FOUND) {
		return false;
	} else {
		return true;
	}
}

static bool get_file_path()
{
	wchar_t custom_file[MAX_PATH] = {'\0'};
	wchar_t old_file[MAX_PATH] = {'\0'};
	std::fill(std::begin(file), std::end(file), 0);
	if (!GetModuleFileNameW(dll_inst, file, MAX_PATH)) {
		return false;
	}

	wchar_t *slash = wcsrchr(file, '\\');
	if (!slash) {
		return false;
	}

	slash[1] = 0;

	StringCbCopy(old_file, sizeof(old_file), file);
	StringCbCopy(custom_file, sizeof(custom_file), file);
	StringCbCat(custom_file, sizeof(custom_file), L"custom-placeholder.png");

	if (file_exists(custom_file)) {
		StringCbCopy(file, sizeof(file), custom_file);
	} else {
		StringCbCat(file, sizeof(file), L"placeholder.png");
	}
	if (wcscmp(old_file, file) == 0) {
		initialized = true;
	} else {
		initialized = false;
	}

	return true;
}

static bool load_placeholder_internal()
{
	Status s;

	Bitmap bmp(file);
	if (bmp.GetLastStatus() != Status::Ok) {
		return false;
	}

	BitmapData bmd = {};
	Rect r(0, 0, bmp.GetWidth(), bmp.GetHeight());

	s = bmp.LockBits(&r, ImageLockModeRead, PixelFormat24bppRGB, &bmd);
	if (s != Status::Ok) {
		return false;
	}

	convert_placeholder((const uint8_t *)bmd.Scan0, bmp.GetWidth(),
			    bmp.GetHeight());

	bmp.UnlockBits(&bmd);
	return true;
}

static bool load_placeholder()
{
	GdiplusStartupInput si;
	ULONG_PTR token;
	GdiplusStartup(&token, &si, nullptr);

	bool success = load_placeholder_internal();

	GdiplusShutdown(token);
	return success;
}

const uint8_t *get_placeholder()
{
	static bool failed = false;

	get_file_path();

	if (wcslen(file) == 0 || !file_exists(file)) {
		initialized = false;
	}

	if (initialized) {
		return placeholder.data();
	} else if (failed) {
		return nullptr;
	}

	initialized = load_placeholder();
	failed = !initialized;

	return initialized ? placeholder.data() : nullptr;
}
