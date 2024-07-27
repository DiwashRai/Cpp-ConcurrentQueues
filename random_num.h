#ifndef RANDOM_NUM_H
#define RANDOM_NUM_H

#include <random>
#include <memory>

class RandomNum {
public:
    RandomNum(const RandomNum&) = delete;
    RandomNum& operator=(const RandomNum&) = delete;
    RandomNum(RandomNum&&) = delete;
    RandomNum& operator=(RandomNum&&) = delete;

    static int randomInt(const int min, const int max) {
        std::uniform_int_distribution dist(min, max);
        return dist(getInstance().gen_);
    }

    static std::vector<int> randomIntVec(const int min, const int max, const std::size_t size) {
        std::uniform_int_distribution dist(min, max);
        std::vector<int> vec;
        vec.reserve(size);
        for (std::size_t i = 0; i < size; ++i) {
            vec.emplace_back(dist(getInstance().gen_));
        }
        return vec;
    }

private:
    RandomNum() : gen_(std::random_device{}()) {}

    static RandomNum& getInstance() {
        static RandomNum instance;
        return instance;
    }

    std::mt19937 gen_;
};

#endif //RANDOM_NUM_H
