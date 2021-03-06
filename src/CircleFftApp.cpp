#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Vbo.h"
#include "cinder/TriMesh.h"
#include "cinder/Triangulate.h"
#include "cinder/params/Params.h"

#include "BeatFactory.h"
#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CircleFftApp : public AppNative {
public:
	void prepareSettings( Settings* settings );
	void setup();
	void update();
	void draw();
    void shutdown();
    void resize();
    
    void toggleFullscreen();
    
    void mouseDown( MouseEvent event );
    void keyDown( KeyEvent event );
    
    TriMesh2d makeMesh( const vector<PolyLine2f> &polys );
    Shape2d getShapeFromPoints(vector<Vec2f> points);
    void addPolyFromShape( const Shape2d shape, vector<PolyLine2f> &polys);
    vector<Vec2f> getCirclePoints(float radius, Vec2f center);
    vector<Vec2f> getCirclePointsFromFFT(float radius, Vec2f center, float * fftData, int32_t fftDataSize);
    
    void initParams();
    
	gl::VboMesh             mVboMesh;
	params::InterfaceGl     mParams;
	float                   mZoom;
    float                   mBaseCircleRadius;
    float                   mCurrentCircleRaidus;
    bool                    mDrawWireframe;
    bool                    mDrawParams;
    bool                    mFullScreen = false;
    int                     mBaseCirclePoints = 360;
    int                     mCurrCircleIncriment = 36;
    
    Shape2d                 mBaseCircle;
    
    BeatFactoryRef          beatFactoryRef;
    
};

#pragma mark - ------------------
#pragma mark lifecycle
#pragma mark -

void CircleFftApp::prepareSettings( Settings* settings )
{
	settings->setWindowSize( 1280, 1080 );
    settings->enableHighDensityDisplay();
}

void CircleFftApp::setup()
{
    mZoom = 1.0f;
    mDrawWireframe = true;
    mDrawParams = true;
    mBaseCircleRadius = 1.0f;
    mCurrentCircleRaidus = 200.0f;
    
    beatFactoryRef = BeatFactory::create();
    beatFactoryRef->loadAudio(loadResource( RES_TRACK_2 ));
    beatFactoryRef->setup();
    
    vector<PolyLine2f> result;
    
    vector<Vec2f> points = getCirclePoints(mBaseCircleRadius, getWindowCenter());
    addPolyFromShape(getShapeFromPoints(points), result);
    
    points = getCirclePoints(mCurrentCircleRaidus, getWindowCenter());
    addPolyFromShape(getShapeFromPoints(points), result);
    
    mVboMesh = gl::VboMesh( makeMesh(result) );
    
    initParams();
}

void CircleFftApp::update()
{
    beatFactoryRef->update();
    
    // check to see if we have fft data, since that takes a sec
	if ( beatFactoryRef->hasFFTData() ) {
        
        vector<PolyLine2f> result;

        vector<Vec2f> points = getCirclePointsFromFFT(mCurrentCircleRaidus, getWindowCenter(), beatFactoryRef->getFftData(), beatFactoryRef->getDataSize());
        addPolyFromShape(getShapeFromPoints(points), result);
        
        points = getCirclePoints(mBaseCircleRadius, getWindowCenter());
        addPolyFromShape(getShapeFromPoints(points), result);
        
        mVboMesh = gl::VboMesh( makeMesh(result) );
    }
    
}

void CircleFftApp::draw()
{
    
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) );
    
	gl::pushModelView();
    
    if( mDrawWireframe ) {
        gl::enableWireframe();
        gl::color( Color::white() );
        gl::draw( mVboMesh );
        gl::disableWireframe();
    } else {
        gl::draw( mVboMesh );
    }
    
	gl::popModelView();
    if (mDrawParams)
        mParams.draw();
}

void CircleFftApp::shutdown()
{

}

void CircleFftApp::resize()
{
    // base circle
//    vector<Vec2f> points = getCirclePoints(mBaseCircleRadius, getWindowCenter());
//    mBaseCircle = getShapeFromPoints(points);
    
}

#pragma mark - ------------------
#pragma mark Events
#pragma mark -

void CircleFftApp::mouseDown( MouseEvent event )
{
}

void CircleFftApp::keyDown(KeyEvent event){
    switch( event.getCode() ) {
        case KeyEvent::KEY_ESCAPE:
            quit();
            break;
        case KeyEvent::KEY_f:
            toggleFullscreen();
            break;
        case KeyEvent::KEY_SPACE:
            mDrawParams = !mDrawParams;
            break;
    }
}


#pragma mark - ------------------
#pragma mark Helpers
#pragma mark -

TriMesh2d CircleFftApp::makeMesh( const vector<PolyLine2f> &polys )
{
	Triangulator triangulator;
	for( vector<PolyLine2f>::const_iterator polyIt = polys.begin(); polyIt != polys.end(); ++polyIt )
		triangulator.addPolyLine( *polyIt );
	
	return triangulator.calcMesh();
}

void CircleFftApp::addPolyFromShape(const cinder::Shape2d shape, vector<PolyLine2f> &polys)
{
	for( vector<Path2d>::const_iterator pathIt = shape.getContours().begin(); pathIt != shape.getContours().end(); ++pathIt ) {
		PolyLine2f contour( pathIt->subdivide() );
		polys.push_back( contour );
	}
}

Shape2d CircleFftApp::getShapeFromPoints(vector<Vec2f> points)
{
    Shape2d shape;
    shape.moveTo( points[0] );
    for( int i = 1; i < points.size(); i++ )
    {
        shape.quadTo( points[i-1], points[i] );
    }
    shape.close();
    
    return shape;
}

vector<Vec2f> CircleFftApp::getCirclePoints(float radius, Vec2f center)
{
    uint16_t numPoints = mBaseCirclePoints;
	float phi = (M_PI * 2.0f) / numPoints;
    vector<Vec2f> points;
    
    for (uint16_t i = 0; i<numPoints; i++)
    {
        float angle = phi * i;
        float x = center.x + radius * cos(angle);
        float y = center.y + radius * sin(angle);
        
        points.push_back(Vec2f(x,y));
    }
    
    return points;
    
}

vector<Vec2f> CircleFftApp::getCirclePointsFromFFT(float radius, Vec2f center, float *fftData, int32_t fftDataSize)
{
	float phi = (M_PI * 2.0f) / fftDataSize;
    vector<Vec2f> points;
    
    for (uint16_t i = 0; i<fftDataSize; i = i+mCurrCircleIncriment)
    {
        float angle = phi * i;
        float r = (radius + lmap(fftData[i], 0.0f, 0.8f, 0.0f, 500.0f));
        float x = center.x + r * cos(angle);
        float y = center.y + r * sin(angle);
        
        points.push_back(Vec2f(x,y));
    }
    
    return points;
}

void CircleFftApp::toggleFullscreen(){
    mFullScreen = !mFullScreen;
    setFullScreen( mFullScreen );
    
    resize();
}

#pragma mark - ------------------
#pragma mark inits
#pragma mark -

void CircleFftApp::initParams()
{
    mParams = params::InterfaceGl( "Parameters", Vec2i( 500, 300 ) );
	mParams.addParam( "Zoom", &mZoom, "min=0.01 max=20 keyIncr=z keyDecr=Z" );
	mParams.addParam( "Draw Wireframe", &mDrawWireframe, "min=1 max=2000 keyIncr== keyDecr=-" );
	mParams.addParam( "Base Circle Radius", &mBaseCircleRadius, "min=1 max=2000 keyIncr== keyDecr=-" );
	mParams.addParam( "Curr Circle Radius", &mCurrentCircleRaidus, "min=1 max=2000" );
	mParams.addParam( "Base Circle Points", &mBaseCirclePoints, "min=1 max=2000" );
   	mParams.addParam( "Curr Circle Increment", &mCurrCircleIncriment, "min=1 max=2000" );
    
}

CINDER_APP_NATIVE( CircleFftApp, RendererGl )
