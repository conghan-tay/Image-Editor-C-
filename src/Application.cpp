#include "precompiled.h"
//#include "Image.h"

namespace cs370
{
bool Application::OnInit()
{
#ifndef NDEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	//_CrtSetBreakAlloc(7277546);

	wxInitAllImageHandlers();

	Window* window = new Window(_("CS370 Framework"), wxDefaultPosition, wxDefaultSize);
	if (!window)
		return false;

	window->Show();
	SetTopWindow(window);
	
	return true;
}
} // namespace cs370
