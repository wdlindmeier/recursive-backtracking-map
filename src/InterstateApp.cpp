#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Surface.h"
#include "cinder/Utilities.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/params/Params.h"
#include "Node.h"
#include "Path.h"
#include "Data.h"
#include "Helpers.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;

typedef vector<int> Route;

class InterstateApp : public AppNative
{

public:
    
	void setup();
    void shutdown();

	void mouseDown( MouseEvent event );
	void mouseMove( MouseEvent event );
    int  closestNodeIndex( MouseEvent event );
    void keyDown(KeyEvent event);
    
	void update();
	void draw();
    
    void addNode(const Vec2f & mousePoint);
    void connectNodes(const int nodeIdxA, const int nodeIdxB);

    void findAllRoutes();
    bool travelNode(const int nodeID);
    void spawnSearch(int startingNodeIdx, int endingNodeIdx);
    
    gl::Texture         mMapTexture;
    Vec2f               mMapSize;
    float               mMapScale;
    
    vector<Node>        mNodes;
    vector<Path>        mPaths;
    
    Font				mFont;
    gl::TextureFontRef	mTextureFont;
    
    bool                mShouldDrawNetwork;
    bool                mShouldDrawMap;
    
    Route               mCurrentRoute;
    vector<int>         mVisitedNodeIDs;
    int                 mFewestRouteSteps;
    float               mShortestRouteLength;
    Route               mBestRoute;
    float               mCurrentRouteLength;

    bool                mIsSearchComplete;
    
    std::mutex          mPathMutex;
    std::thread         mSolveThread;
    
    int                 mSelectedNodeIdx;
    int                 mHighlightedNodeIdx;

    int                 mStartingNodeIDx;
    int                 mEndingNodeIDx;
    
    bool                mShouldCancel;
    
    float               mCalcDelay;
    params::InterfaceGl mParams;
    
};

void InterstateApp::setup()
{
    setFrameRate(60);

    mIsSearchComplete = true;
    
    mMapTexture = loadImage(loadResource("interstate.jpg"));
    mMapSize = mMapTexture.getSize();
    mMapScale = 0.34f;

    setWindowSize(mMapSize.x * mMapScale, mMapSize.y * mMapScale);
    setWindowPos(0, 0);

    mFont = Font( "Helvetica Bold", 38 );
    mTextureFont = gl::TextureFont::create( mFont );
    
    mStartingNodeIDx = -1;
    mEndingNodeIDx = -1;
    mSelectedNodeIdx = -1;
    mHighlightedNodeIdx = -1;
    
    // Add some delay so the search can be illustrated
    mCalcDelay = 0.15f;
    
    mParams = params::InterfaceGl(getWindow(), "Search Params", Vec2f(250,60));
    mParams.setOptions( "", "position='30 "+to_string(mMapScale * 1850.0f)+"'");
    mParams.addParam("Animation Delay", &mCalcDelay, "max=1.0 min=0 step=0.001");
    
    mNodes = GetAllNodes();
    mPaths = GetAllPaths();
    
    // Add all of the paths to their respective nodes.
    for (int i = 0; i < mPaths.size(); ++i)
    {
        Path & path = mPaths[i];
        int fromIDx = path.getFromNodeIdx();
        int toIDx = path.getToNodeIdx();
        
        Node & fromNode = mNodes[fromIDx];
        fromNode.addPathID(path.getID());
        
        Node & toNode = mNodes[toIDx];
        toNode.addPathID(path.getID());

        // Set the distance
        path.setDistance(fromNode.getPosition().distance(toNode.getPosition()));
    }
    
    mShouldDrawNetwork = true;
    mShouldDrawMap = true;
    
    // Make sure our IDs are the same as the vector indices
    assert(mNodes[100].getID() == 100);
    assert(mPaths[100].getID() == 100);
    assert(mNodes[10].getID() == 10);
    assert(mPaths[10].getID() == 10);
    
}

void InterstateApp::shutdown()
{
    if (mSolveThread.joinable())
    {
        mSolveThread.join();
    }
}

#pragma mark - Search

void InterstateApp::spawnSearch(int startingNodeIdx, int endingNodeIdx)
{
    if (mIsSearchComplete)
    {
        mSelectedNodeIdx = -1;
        mHighlightedNodeIdx = -1;

        mStartingNodeIDx = startingNodeIdx;
        mEndingNodeIDx = endingNodeIdx;
        
        mIsSearchComplete = false;
        if (mSolveThread.joinable())
        {
            mSolveThread.join();
        }
        // Solving on a different thread so we can watch the progress
        mSolveThread = std::thread(boost::bind(&InterstateApp::findAllRoutes, this));
    }
}

void InterstateApp::findAllRoutes()
{
    mPathMutex.lock();
    {
        // Clear out the previous search
        mCurrentRoute.clear();
        mVisitedNodeIDs.clear();
        mFewestRouteSteps = 16000; // Big #
        mCurrentRouteLength = 0.0f;
        mBestRoute.clear();
        mShouldCancel = false;
        
        // Optimization
        //
        // This is kind of cheating, because the code must "know" something
        // about the map, but we'll set the maximum allowable route length
        // to twice the distance between the starting and ending positions.
        //
        // This can make a HUGE difference eliminating a set of slow searches
        // that can't find an initial path to compare the others against for
        // a long time.
        Node & startNode = mNodes[mStartingNodeIDx];
        Node & endNode = mNodes[mEndingNodeIDx];
        mShortestRouteLength = startNode.getPosition().distance(endNode.getPosition()) * 2;
    }
    mPathMutex.unlock();
    
    // Kick it off from the starting position
    travelNode(mStartingNodeIDx);
    
    mPathMutex.lock();
    {
        mIsSearchComplete = true;
    }
    mPathMutex.unlock();
    console() << "Thread complete\n";
}

// The recursive function
bool InterstateApp::travelNode(const int nodeID)
{
    if (mCalcDelay > 0)
    {
        sleep(mCalcDelay);
    }
    
    if (mShouldCancel)
    {
        return false;
    }
    
    mVisitedNodeIDs.push_back(nodeID);

    if (nodeID == mEndingNodeIDx)
    {
        // This is the final node.
        return true;
    }
    
    Node & currentNode = mNodes[nodeID];
    vector<int> nodePathIDs = currentNode.getPathIDs();

    // Make sure that the current path length, plus whatever is left to cover
    // (in an optimal case) is less than the shortest known route.
    float distToGoal = currentNode.getPosition().distance(mNodes[mEndingNodeIDx].getPosition());
    if ((mCurrentRouteLength + distToGoal) >= mShortestRouteLength)
    {
        // This route isn't efficient enough.
        return false;
    }
    
    // Go down each path
    for (int i = 0; i < nodePathIDs.size(); ++i)
    {
        int pathID = nodePathIDs[i];

        Path & path = mPaths[pathID];
        int fromNodeID = path.getFromNodeIdx();
        int toNodeID = path.getToNodeIdx();
        int otherNodeID = (fromNodeID == nodeID) ? toNodeID : fromNodeID;
        
        // Don't go down this path if we've already visited the node at the other end
        if (!VectorFind(mVisitedNodeIDs, otherNodeID))
        {
            // Add the path to the current route
            mPathMutex.lock();
            {
                mCurrentRoute.push_back(pathID);
            }
            mCurrentRouteLength += path.getDistance();
            mPathMutex.unlock();
            
            if (travelNode(otherNodeID))
            {
                // Sweet. This is a good route.
                
                if (mCurrentRouteLength < mShortestRouteLength)
                {
                    mShortestRouteLength = mCurrentRouteLength;
                    mBestRoute = mCurrentRoute;
                }
            }
            
            // We've exhausted (and/or stored) every possible route.
            // Remove this path.
            VectorErase(mVisitedNodeIDs, otherNodeID);
            mPathMutex.lock();
            {
                mCurrentRoute.pop_back();
            }
            mCurrentRouteLength -= path.getDistance();
            mPathMutex.unlock();
        }
    }

    return false;
}

#pragma mark - Node Management

void InterstateApp::addNode(const Vec2f & mousePoint)
{
    console() << "Node at " << (mousePoint / mMapScale) << std::endl;
    Node node(mousePoint / mMapScale, std::to_string(mNodes.size()));
    //node.setPosition();
    node.setName(std::to_string(mNodes.size()));
    mNodes.push_back(node);
}

void InterstateApp::connectNodes(const int nodeIdxA, const int nodeIdxB)
{
    mPaths.push_back(Path(nodeIdxA, nodeIdxB));
}

#pragma mark - Input events

int InterstateApp::closestNodeIndex( MouseEvent event )
{
    // Find the node that the event is closest to
    float closestDist = 99999.0f;
    int closestNodeIdx = -1;
    for (int i = 0; i < mNodes.size(); ++i)
    {
        Node & node = mNodes[i];
        float nodeDist = node.getPosition().distance(Vec2f(event.getPos()) / mMapScale);
        if (nodeDist < closestDist)
        {
            closestDist = nodeDist;
            closestNodeIdx = i;
        }
    }
    assert(closestNodeIdx != -1);
    return closestNodeIdx;
}

void InterstateApp::mouseMove( MouseEvent event )
{
    if (!mIsSearchComplete)
    {
        return;
    }
    mHighlightedNodeIdx = closestNodeIndex(event);
}

void InterstateApp::mouseDown( MouseEvent event )
{
    if (!mIsSearchComplete)
    {
        return;
    }

    mStartingNodeIDx = -1;
    mEndingNodeIDx = -1;

    int closestNodeIdx = closestNodeIndex(event);
    console() << "Clicked on " << closestNodeIdx << " " << mNodes[closestNodeIdx].getName() << std::endl;
    
    if (mSelectedNodeIdx == closestNodeIdx)
    {
        mSelectedNodeIdx = -1;
    }
    else if (mSelectedNodeIdx != -1)
    {
        // Search between these two nodes.
        console() << "Connect " << mSelectedNodeIdx << " + " << closestNodeIdx << std::endl;
        spawnSearch(mSelectedNodeIdx, closestNodeIdx);
        mSelectedNodeIdx = -1;
    }
    else
    {
        mSelectedNodeIdx = closestNodeIdx;
    }
}

void InterstateApp::keyDown(KeyEvent event)
{
    char c = event.getChar();
    if (c == 'n')
    {
        mShouldDrawNetwork = !mShouldDrawNetwork;
    }
    else if (c == 'm')
    {
        mShouldDrawMap = !mShouldDrawMap;
    }
    else if (event.getCode() == KeyEvent::KEY_ESCAPE)
    {
        mShouldCancel = true;
    }
}

#pragma mark - Draw loop

void InterstateApp::update()
{
}

void InterstateApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) );

    if (mShouldDrawMap)
    {
        if (mIsSearchComplete)
        {
            gl::color(1,1,1);
        }
        else
        {
            gl::color(1,1,1,0.5f);
        }
        gl::draw(mMapTexture, Rectf(0, 0, getWindowWidth(), getWindowHeight()));
    }

    if (!mShouldDrawNetwork)
    {
        return;
    }
    
    gl::pushMatrices();
    gl::enableAlphaBlending();
    gl::scale(Vec3f(mMapScale, mMapScale, mMapScale));
    
    // Draw all paths (green)
    /*
    gl::lineWidth(2);
    gl::color(Color(0,1,0));
    for (int i = 0; i < mPaths.size(); ++i)
    {
        Path & path = mPaths[i];
        path.draw(mNodes);
    }
    */
    
    // Draw the node fill
    for (int i = 0; i < mNodes.size(); ++i)
    {
        Node & node = mNodes[i];
        if (i == mSelectedNodeIdx || i == mStartingNodeIDx || i == mEndingNodeIDx)
        {
            gl::color(ColorA(1,1,1,1.0f));
        }
        else if (i == mHighlightedNodeIdx)
        {
            gl::color(ColorA(1,0,0,1));
        }
        else
        {
            gl::color(ColorA(0.65f,0.65f,0.65f,1));
        }
        
        node.draw(false);
    }
    
    if (mStartingNodeIDx != -1 && mEndingNodeIDx != -1)
    {
        mPathMutex.lock();
        {
            gl::lineWidth(6);

            // Only draw if we're not picking the next markers
            if (mSelectedNodeIdx == -1)
            {
                // Draw the current best route
                gl::color(ColorA(1.0,1.0,0,1.0f));
                for (int i = 0; i < mBestRoute.size(); ++i)
                {
                    int pathID = mBestRoute[i];
                    Path & path = mPaths[pathID];
                    path.draw(mNodes);
                }
            }
            
            if (!mIsSearchComplete)
            {
                // Draw the current search path
                gl::lineWidth(2);
                gl::color(Color(1.0,0,0));
                for (int j = 0; j < mCurrentRoute.size(); ++j)
                {
                    int pathID = mCurrentRoute[j];
                    Path & path = mPaths[pathID];
                    path.draw(mNodes);
                }
            }
        }
        mPathMutex.unlock();
    }
    
    // Draw the node outlines
    for (int i = 0; i < mNodes.size(); ++i)
    {
        Node & node = mNodes[i];
        gl::lineWidth(1.0f);
        gl::color(0,0,0);
        node.draw(true);
    }
    
    if (!mShouldDrawMap)
    {
        for (int i = 0; i < mNodes.size(); ++i)
        {
            Node & node = mNodes[i];

            std::string name = node.getName();
            Vec2f stringSize = mTextureFont->measureStringWrapped(name, Rectf(0,0,200,200));
            Vec2f stringOffset(20,-2);
            Vec2f rectOffset = node.getPosition() + stringOffset;
            Rectf drawRect(rectOffset.x - 10,
                           rectOffset.y - 10,
                           rectOffset.x + 20 + stringSize.x,
                           rectOffset.y + 20 + stringSize.y - mTextureFont->getAscent());
            gl::color(0.5,0.5,0.5);
            mTextureFont->drawStringWrapped(name,
                                            drawRect,
                                            Vec2f(0, mTextureFont->getAscent()) + stringOffset);
        }
    }

    gl::popMatrices();
    
    gl::pushMatrices();
    gl::translate(Vec2f(0,500));
    mParams.draw();
    gl::popMatrices();

}

CINDER_APP_NATIVE( InterstateApp, RendererGl )
