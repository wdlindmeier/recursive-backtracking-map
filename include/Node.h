//
//  Node.h
//  Interstate
//
//  Created by William Lindmeier on 9/20/13.
//
//

#pragma once

#include "cinder/Cinder.h"
#include "cinder/Rand.h"

class Node
{
    
public:
    
    Node(const ci::Vec2f & position, const std::string & name = "") :
    mColor(ci::Rand::randFloat(), ci::Rand::randFloat(), ci::Rand::randFloat()),
    mIsNamed(name != ""),
    mName(name),
    mRadius(20.0f),
    mPosition(position),
    mID(NextNodeID())
    {};
    
    ~Node(){};
    
    std::string getName();
    void setName(const std::string & name);

    ci::Vec2f getPosition();
    void setPosition(const ci::Vec2f & position);
    
    const int getID(){ return mID; };
    
    void draw(bool stroked);
    
    void addPathID(const int pathID)
    {
        // NOTE: Should this be a set or another container type that
        // maintains uniqueness?
        if (std::find(mPathIDs.begin(), mPathIDs.end(), pathID) == mPathIDs.end())
        {
            mPathIDs.push_back(pathID);
        }
    };
    const std::vector<int> getPathIDs(){ return mPathIDs; };

    static const int NextNodeID();
    
protected:
    
    ci::Vec2f mPosition;
    std::string mName;
    ci::Color mColor;
    float mRadius;
    bool mIsNamed;
    int mID;
    std::vector<int> mPathIDs;
};