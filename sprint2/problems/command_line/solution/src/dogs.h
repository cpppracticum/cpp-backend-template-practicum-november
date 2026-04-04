#pragma once

#include "dog.h"
#include <unordered_map>

namespace model {

class Dogs {
public:
    DogId AddDog(const std::string& name, const Point& position) {
        DogId id(next_id_++);
        dogs_.emplace(id, Dog(id, name, position));
        return id;
    }

    const Dog* GetDog(DogId id) const {
        auto it = dogs_.find(id);
        if (it != dogs_.end()) return &it->second;
        return nullptr;
    }

    Dog* GetDog(DogId id) {
        auto it = dogs_.find(id);
        if (it != dogs_.end()) return &it->second;
        return nullptr;
    }

    const std::unordered_map<DogId, Dog>& GetAllDogs() const { return dogs_; }
    
    std::unordered_map<DogId, Dog>& GetAllDogsForUpdate() { return dogs_; }

private:
    DogId::value_type next_id_ = 0;
    std::unordered_map<DogId, Dog> dogs_;
};

}  // namespace model
