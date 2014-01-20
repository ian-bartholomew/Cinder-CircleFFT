#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CircleFftApp : public AppNative {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
};

void CircleFftApp::setup()
{
}

void CircleFftApp::mouseDown( MouseEvent event )
{
}

void CircleFftApp::update()
{
}

void CircleFftApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP_NATIVE( CircleFftApp, RendererGl )
