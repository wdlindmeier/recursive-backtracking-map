//
//  Path.cpp
//  Interstate
//
//  Created by William Lindmeier on 9/24/13.
//
//

#include "Path.h"
#include "Node.h"
#include "cinder/gl/gl.h"
#include "cinder/Color.h"

using namespace ci;

void Path::draw(std::vector<Node> & nodes)
{
    // TODO: REMOVE
    if (nodes.size() > mFromNodeIdx &&
        nodes.size() > mToNodeIdx)
    {
        gl::drawLine(nodes[mFromNodeIdx].getPosition(),
                     nodes[mToNodeIdx].getPosition());
    }
}

const int Path::NextPathID()
{
    static int NextPathID = -1;
    NextPathID++;
    return NextPathID;
}