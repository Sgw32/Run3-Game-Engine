#include "PreVideoViewer.h"
#include <comutil.h>

PreVideoViewer::PreVideoViewer()
{
}

PreVideoViewer::~PreVideoViewer()
{
}

void PreVideoViewer::showVideo(string vid)
{
 IGraphBuilder *pGraphBuilder;
 IMediaControl *pMediaControl;
 IMediaEvent *pMediaEvent;
 long eventCode;
 IVideoWindow *pVideoWindow;

 CoInitialize(NULL);

 CoCreateInstance(CLSID_FilterGraph,
	NULL,
	CLSCTX_INPROC,
	IID_IGraphBuilder,
	(LPVOID *)&pGraphBuilder);

 pGraphBuilder->QueryInterface(IID_IMediaControl,
	(LPVOID *)&pMediaControl);

 pGraphBuilder->QueryInterface(IID_IMediaEvent,
	(LPVOID *)&pMediaEvent);

 pMediaControl->RenderFile(_com_util::ConvertStringToBSTR(vid.c_str()));//vid.c_str());

 pGraphBuilder->QueryInterface(IID_IVideoWindow,
	(LPVOID *)&pVideoWindow);

 // Start Full Screen
 pVideoWindow->put_FullScreenMode(OATRUE);

 pMediaControl->Run();

 pMediaEvent->WaitForCompletion(-1, &eventCode);

 // End Full Screen
 pVideoWindow->put_FullScreenMode(OAFALSE);

 pVideoWindow->Release();
 pMediaEvent->Release();
 pMediaControl->Release();
 pGraphBuilder->Release();
 CoUninitialize();
}