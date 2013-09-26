//
//  Node.cpp
//  Interstate
//
//  Created by William Lindmeier on 9/20/13.
//
//

#include "Node.h"

using std::string;
using namespace ci;

std::string Node::getName()
{
    return mName;
}

void Node::setName(const std::string & name)
{
    mName = name;
}

Vec2f Node::getPosition()
{
    return mPosition;
}

void Node::setPosition(const ci::Vec2f & position)
{
    mPosition = position;
}

void Node::draw(bool stroked)
{
    if (stroked)
    {
        gl::drawStrokedCircle(mPosition, mRadius);
    }
    else
    {
        gl::drawSolidCircle(mPosition, mRadius);
    }
}

const int Node::NextNodeID()
{
    static int NextNodeID = -1;
    NextNodeID++;
    return NextNodeID;
}