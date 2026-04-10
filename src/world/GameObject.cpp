#include "GameObject.hpp"
#include "ComponentRegistry.hpp"
#include <iostream>

void GameObject::Deserialize(nlohmann::json& j) {
    // Update the name if it exists in the JSON data
    if (j.contains("name")) {
        name = j["name"];
    }

    // Clear any default components that might have been added during creation
    // (like a default Transform2d) to avoid duplicates when loading from the prefab
    components.clear();

    // Iterate through the saved components array
    if (j.contains("components") && j["components"].is_array()) {
        for (const auto& compJson : j["components"]) {
            // Extract the exact type string (e.g., "SpriteRenderer")
            std::string type = compJson.value("type", "");
                    
            // Check if we have a factory registered for this type
            if (!type.empty() && ComponentRegistry::factories.count(type)) {
                        
                // Use the factory to allocate and attach the new component to this object
                ComponentRegistry::factories[type](*this);
                        
                // The factory automatically pushes the new component to the back of our vector.
                // We grab it and ask it to load its specific variables (textures, paths, etc.)
                if (!components.empty()) {
                    components.back()->Deserialize(compJson);
                }
            } else {
                std::cerr << "[GameObject] Warning: Could not deserialize unknown component type: " << type << std::endl;
            }
        }
    }
}
