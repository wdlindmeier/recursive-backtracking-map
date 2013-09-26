//
//  Path.h
//  Interstate
//
//  Created by William Lindmeier on 9/24/13.
//
//

#pragma once

class Node;

class Path
{
    
public:
    
    Path(const int fromNodeIx, const int toNodeIdx) :
    mFromNodeIdx(fromNodeIx),
    mToNodeIdx(toNodeIdx),
    mID(Path::NextPathID())
    {};
    ~Path(){};
    
    std::string toString(){ return std::to_string(mFromNodeIdx) + "," + std::to_string(mToNodeIdx); };
    
    void draw(std::vector<Node> & nodes);
    
    int getFromNodeIdx(){ return mFromNodeIdx; };
    int getToNodeIdx(){ return mToNodeIdx; };
    int getID(){ return mID; }
    
    void setDistance(const float distance){ mDistance = distance; };
    float getDistance(){ return mDistance; };
    
    static const int NextPathID();
    
private:
    
    int mFromNodeIdx;
    int mToNodeIdx;
    float mDistance;
    int mID;
    
};