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
    
    void mouseDown( MouseEvent event );
    void keyDown( KeyEvent event );
    
    
    void recalcMesh();
    vector<Vec2f> getCirclePoints(float radius, Vec2f center);
    vector<Vec2f> getCirclePointsFromFFT(float radius, Vec2f center, float * fftData, int32_t fftDataSize);
    
    void initParams();
    
	gl::VboMesh			mVboMesh;
    float				mPrecision, mOldPrecision;
    Shape2d				mShape, mShapeB;
	params::InterfaceGl	mParams;
	float				mZoom;
	int32_t				mNumPoints;
    bool				mDrawWireframe;
    bool				mDrawParams;
    
    BeatFactoryRef      beatFactoryRef;
    
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
	mOldPrecision = mPrecision = 1.0f;
	mNumPoints = 0;
    
    beatFactoryRef = BeatFactory::create();
    beatFactoryRef->loadAudio(loadResource( RES_TRACK_2 ));
    beatFactoryRef->setup();
    
    vector<Vec2f> points = getCirclePoints(200.0f, getWindowCenter());
    mShape.moveTo( points[0]  );
    for( int i = 1; i < points.size(); i++ )
    {
        mShape.quadTo( points[i-1], points[i] );
    }
    mShape.close();
    
	// load VBO
	recalcMesh();
    
    initParams();
}

void CircleFftApp::update()
{
    beatFactoryRef->update();
    
    // check to see if we have fft data, since that takes a sec
	if ( beatFactoryRef->hasFFTData() ) {
        vector<Vec2f> points = getCirclePointsFromFFT(200.0f, getWindowCenter(), beatFactoryRef->getFftData(), beatFactoryRef->getDataSize());
        mShape.clear();
        mShape.moveTo( points[0] );
        for( int i = 1; i < points.size(); i++ )
        {
            mShape.quadTo( points[i-1], points[i] );
        }
        mShape.close();
        
        // load VBO
        recalcMesh();
    }
    
}

void CircleFftApp::draw()
{
    if( mOldPrecision != mPrecision )
		recalcMesh();
    
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) );
    
    
    if( mDrawWireframe ) {
        gl::enableWireframe();
        gl::color( Color::white() );
        gl::draw( mVboMesh );
        gl::disableWireframe();
    } else {
        gl::draw( mVboMesh );
    }
    if (mDrawParams)
        mParams.draw();
}

void CircleFftApp::shutdown()
{

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
//        case KeyEvent::KEY_f:
//            toggleFullscreen();
//            break;
        case KeyEvent::KEY_SPACE:
            mDrawParams = !mDrawParams;
            break;
    }
}


#pragma mark - ------------------
#pragma mark Helpers
#pragma mark -

void CircleFftApp::recalcMesh()
{
	TriMesh2d mesh = Triangulator( mShape, mPrecision ).calcMesh( Triangulator::WINDING_ODD );
	mNumPoints = mesh.getNumIndices();
	mVboMesh = gl::VboMesh( mesh );
	mOldPrecision = mPrecision;
}

vector<Vec2f> CircleFftApp::getCirclePoints(float radius, Vec2f center)
{
    uint16_t numPoints = 360;
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
    
    for (uint16_t i = 0; i<fftDataSize; i = i+12)
    {
        float angle = phi * i;
        float r = (radius + (fftData[i] * 100) );
        float x = center.x + r * cos(angle);
        float y = center.y + r * sin(angle);
        
        points.push_back(Vec2f(x,y));
    }
    
    return points;
}


#pragma mark - ------------------
#pragma mark inits
#pragma mark -

void CircleFftApp::initParams()
{
    mParams = params::InterfaceGl( "Parameters", Vec2i( 220, 170 ) );
	mParams.addParam( "Zoom", &mZoom, "min=0.01 max=20 keyIncr=z keyDecr=Z" );
	mParams.addParam( "Draw Wireframe", &mDrawWireframe, "min=1 max=2000 keyIncr== keyDecr=-" );
	mParams.addParam( "Precision", &mPrecision, "min=0.01 max=20 keyIncr=p keyDecr=P" );
	mParams.addParam( "Num Points", &mNumPoints, "", true );
}

CINDER_APP_NATIVE( CircleFftApp, RendererGl )
