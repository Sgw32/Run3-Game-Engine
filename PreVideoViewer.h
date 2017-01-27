#include <dshow.h>
#include <xstring>

using namespace std;

class PreVideoViewer
{
public:
	PreVideoViewer();
	~PreVideoViewer();
	void showVideo(string vid);
private:
 IGraphBuilder *pGraphBuilder;
 IMediaControl *pMediaControl;
 IMediaEvent *pMediaEvent;
 long eventCode;
// string videoFileName;
 IVideoWindow *pVideoWindow;
};