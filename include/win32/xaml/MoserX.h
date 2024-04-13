#pragma once

#include <string>

class MoserX
{
public:
	void init();

	void* create(HWND hwnd);
	void expand(HWND hwnd, const RECT& r);

	void* load(HWND hwnd, const std::wstring& xaml);
	void* source(HWND hwnd);
	void* bridge(HWND hwnd);

	void activate(HWND hwnd, WPARAM wParam);
	void size(HWND hwnd, const RECT& r);

	bool translate( MSG& msg);

	void destroy(HWND hwnd);
	void shutdown();

private:

};

