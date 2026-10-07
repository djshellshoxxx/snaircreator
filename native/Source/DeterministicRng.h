#pragma once
#include <random>
#include <cstdint>

class DeterministicRng
{
public:
    explicit DeterministicRng(uint32_t seed):engine(seed){}
    float bipolar(){return std::uniform_real_distribution<float>(-1.0f,1.0f)(engine);}
    float uniform(){return std::uniform_real_distribution<float>(0.0f,1.0f)(engine);}
    int integer(int low,int high){return std::uniform_int_distribution<int>(low,high)(engine);}
private:
    std::mt19937 engine;
};
