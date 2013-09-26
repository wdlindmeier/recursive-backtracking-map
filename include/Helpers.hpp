//
//  Helpers.hpp
//  Interstate
//
//  Created by William Lindmeier on 9/25/13.
//
//

#pragma once

template <typename T>
bool VectorFind(const std::vector<T> & vec, T item)
{
    return std::find(vec.begin(), vec.end(), item) != vec.end();
}

template <typename T>
void VectorErase(std::vector<T> & vec, T item)
{
    vec.erase(std::remove(vec.begin(), vec.end(), item), vec.end());
}