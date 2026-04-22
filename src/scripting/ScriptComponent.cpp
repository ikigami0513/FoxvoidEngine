#include "ScriptComponent.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "core/AssetRegistry.hpp"

#ifndef STANDALONE_MODE
#include "editor/EditorUI.hpp"
#include "editor/commands/CommandHistory.hpp"
#include "editor/commands/ModifyComponentCommand.hpp"
#endif

ScriptComponent::ScriptComponent(const std::string& moduleName, const std::string& className)
    : m_scriptName(moduleName), m_className(className)
{
    // Legacy support: Try to deduce the path and get the UUID
    std::string assumedPath = "assets/scripts/" + moduleName + ".py";
    UUID uuid = AssetRegistry::GetUUIDForPath(assumedPath);
    LoadScript(uuid, className);
}

ScriptComponent::~ScriptComponent() {
    if (m_instance) {
        try {
            Component* nativeComponent = m_instance.cast<Component*>();
            if (nativeComponent) {
                nativeComponent->owner = nullptr;
            }
            
            m_instance = py::none(); 
        } catch (...) {
            
        }
    }
}

void ScriptComponent::LoadScript(UUID scriptUUID, const std::string& className) {
    m_scriptUUID = scriptUUID;
    m_className = className;

    // On ne s'arrête plus si className est vide, on a juste besoin de l'UUID valide !
    if (m_scriptUUID == 0) return;

    // Resolve the UUID to the physical file path
    std::filesystem::path path = AssetRegistry::GetPathForUUID(m_scriptUUID);
    if (path.empty()) {
        std::cerr << "[ScriptComponent] Error: Could not resolve script UUID " << (uint64_t)m_scriptUUID << std::endl;
        return;
    }

    // Extract the module name (filename without extension) for Python import
    m_scriptName = path.stem().string();
    m_scriptFilePath = path.string();

    // CORRECTIF MAGIQUE : Si la classe est vide (vieille sauvegarde), on la déduit du fichier !
    if (m_className.empty()) {
        m_className = m_scriptName; // Fallback par défaut
        std::ifstream file(m_scriptFilePath);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                size_t classPos = line.find("class ");
                if (classPos != std::string::npos) {
                    size_t start = classPos + 6;
                    size_t end = line.find_first_of("(: \r\n", start);
                    if (end != std::string::npos) {
                        m_className = line.substr(start, end - start);
                        break; 
                    }
                }
            }
            file.close();
        }
    }

    try {
        py::module_ mod = py::module_::import(m_scriptName.c_str());
        py::object cls = mod.attr(m_className.c_str());
        
        m_instance = cls();
        
        // Immediately bind the owner pointer so Python knows its GameObject
        Component* nativeComponent = m_instance.cast<Component*>();
        if (nativeComponent && this->owner) {
            nativeComponent->owner = this->owner;
        }

        if (std::filesystem::exists(m_scriptFilePath)) {
            m_lastWriteTime = std::filesystem::last_write_time(m_scriptFilePath);
        }

    } catch (const py::error_already_set& e) {
        std::cerr << "[ScriptComponent] LoadScript Error (" << m_scriptName << "):\n" << e.what() << std::endl;
    }
}

void ScriptComponent::Start() {
    if (!m_instance) return;

    try {
        Component* nativeComponent = m_instance.cast<Component*>();

        if (nativeComponent) {
            // We bypass Python entirely and set the C++ pointer directly.
            // When Python scripts call 'self.game_object', it will read this exact pointer!
            nativeComponent->owner = this->owner;
        }

        // Call the start method in Python
        if (py::hasattr(m_instance, "start")) {
            m_instance.attr("start")();
        }
    } catch (const py::error_already_set& e) {
        std::cerr << "[ScriptComponent] Start Error:\n" << e.what() << std::endl;
    }
}

void ScriptComponent::Update(float deltaTime) {
    // Hot reloading check
    try {
        // Ensure the path is not empty before checking the filesystem
        if (!m_scriptFilePath.empty() && std::filesystem::exists(m_scriptFilePath)) {
            auto currentWriteTime = std::filesystem::last_write_time(m_scriptFilePath);
            
            // If the file on disk is newer than our cached version
            if (currentWriteTime > m_lastWriteTime) {
                m_lastWriteTime = currentWriteTime;
                HotReload();
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[ScriptComponent] File system error: " << e.what() << std::endl;
    }

    // Standard update execution
    try {
        if (m_instance && py::hasattr(m_instance, "update")) {
            m_instance.attr("update")(deltaTime);
        }
    } catch (const py::error_already_set& e) {
        std::cerr << "[ScriptComponent] Update Error:\n" << e.what() << std::endl;
    }
}

void ScriptComponent::OnCollision(const Collision2D& collision) {
    if (!m_instance) return;

    try {
        // Look for 'on_collision' in the Python script
        if (py::hasattr(m_instance, "on_collision")) {
            // Call the python method passing the Collision2D struct
            m_instance.attr("on_collision")(collision);
        }
    } catch (const py::error_already_set& e) {
        std::cerr << "[ScriptComponent] OnCollision Error:\n" << e.what() << std::endl;
    }
}

void ScriptComponent::HotReload() {
    std::cout << "[ScriptEngine] File changed. Hot reloading: " << m_scriptName << "..." << std::endl;

    try {
        // Import Python's built-in importlib module
        py::module_ importlib = py::module_::import("importlib");
        py::module_ sys = py::module_::import("sys");
        py::dict modules = sys.attr("modules");

        // Check if our module is actually loaded in Python's cache
        if (modules.contains(m_scriptName)) {
            py::object currentModule = modules[m_scriptName.c_str()];
            
            // Force Python to re-read and re-compile the .py file
            importlib.attr("reload")(currentModule);
            
            // Fetch the newly compiled class
            py::object newClass = currentModule.attr(m_className.c_str());
            
            // We dynamically swap the class of our existing Python instance.
            // This replaces the methods (like update) but keeps all existing variables 
            // (like speed, health, transform references) perfectly intact!
            m_instance.attr("__class__") = newClass;

            // Changing the class doesn't re-run __init__. If the user added 'self.new_var = 10', 
            // it won't exist in m_instance. We create a temporary dummy instance to find new variables.
            py::object dummyInstance = newClass();
            py::dict dummyDict = dummyInstance.attr("__dict__");
            py::dict currentDict = m_instance.attr("__dict__");

            for (auto item : dummyDict) {
                // If the dummy has a variable that our running instance does NOT have, inject it!
                if (!currentDict.contains(item.first)) {
                    currentDict[item.first] = item.second;
                }
            }

            // Collect keys to remove to avoid iterator invalidation
            std::vector<py::object> keysToRemove;
            for (auto item : currentDict) {
                if (!dummyDict.contains(item.first)) {
                    keysToRemove.push_back(py::reinterpret_borrow<py::object>(item.first));
                }
            }

            // Safely remove deprecated variables
            for (const auto& key : keysToRemove) {
                currentDict.attr("pop")(key);
            }
            
            std::cout << "[ScriptEngine] Hot reload successful!" << std::endl;
        }
    } catch (const py::error_already_set& e) {
        // If you make a syntax error in your Python script and save it, 
        // the engine will catch it here without crashing, print the error, 
        // and keep running the old version of the script until you fix the typo!
        std::cerr << "[ScriptEngine] Failed to hot reload (Syntax Error?):\n" << e.what() << std::endl;
    }
}

py::object ScriptComponent::GetInstance() const {
    return m_instance;
}

std::string ScriptComponent::GetName() const {
    // Show the python class name in the Inspector (e.g., "PlayerController (Script)")
    return m_className + " (Script)";
}

#ifndef STANDALONE_MODE
void ScriptComponent::OnInspector() {
    // 1. Script Binding (Only visible if no script is currently loaded)
    if (!m_instance) {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "No valid Python instance.");
        
        std::string currentPath = m_scriptUUID != 0 ? AssetRegistry::GetPathForUUID(m_scriptUUID).string() : "Drop a .py file here";
        
        char pathBuffer[256];
        char clsBuffer[256];
        strncpy(pathBuffer, currentPath.c_str(), sizeof(pathBuffer));
        strncpy(clsBuffer, m_className.c_str(), sizeof(clsBuffer));

        // Make the path input read-only in the UI. Drag & Drop is the primary way to assign scripts now.
        ImGui::InputText("File", pathBuffer, sizeof(pathBuffer), ImGuiInputTextFlags_ReadOnly);
        
        // Drag and drop target for Python files
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
                std::string droppedPath = (const char*)payload->Data;
                std::filesystem::path fsPath(droppedPath);
                
                if (fsPath.extension() == ".py") {
                    nlohmann::json initialState = Serialize();

                    UUID newUUID = AssetRegistry::GetUUIDForPath(fsPath);
                    std::string parsedClassName = fsPath.stem().string(); // Default fallback

                    // Extract class name from the file
                    std::ifstream file(fsPath);
                    if (file.is_open()) {
                        std::string line;
                        while (std::getline(file, line)) {
                            size_t classPos = line.find("class ");
                            if (classPos != std::string::npos) {
                                size_t start = classPos + 6;
                                size_t end = line.find_first_of("(: \r\n", start);
                                if (end != std::string::npos) {
                                    parsedClassName = line.substr(start, end - start);
                                    break; 
                                }
                            }
                        }
                        file.close();
                    }

                    LoadScript(newUUID, parsedClassName);
                    CommandHistory::AddCommand(std::make_unique<ModifyComponentCommand>(this, initialState, Serialize()));
                }
            }
            ImGui::EndDragDropTarget();
        }

        // Allow manual class name edits (useful if multiple classes exist in one file)
        if (ImGui::InputText("Class", clsBuffer, sizeof(clsBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (m_scriptUUID != 0 && std::string(clsBuffer) != m_className) {
                nlohmann::json initialState = Serialize();
                LoadScript(m_scriptUUID, clsBuffer);
                CommandHistory::AddCommand(std::make_unique<ModifyComponentCommand>(this, initialState, Serialize()));
            }
        }

        ImGui::Separator();
        return; // Stop drawing here if no instance is loaded
    }

    // 2. Hot Reload Check for Editor Mode (Only runs if a script is loaded)
    try {
        if (!m_scriptFilePath.empty() && std::filesystem::exists(m_scriptFilePath)) {
            auto currentWriteTime = std::filesystem::last_write_time(m_scriptFilePath);
            if (currentWriteTime > m_lastWriteTime) {
                m_lastWriteTime = currentWriteTime;
                HotReload();
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[ScriptComponent] Inspector File system error: " << e.what() << std::endl;
    }

    // 3. Dynamic Variables Inspection
    try {
        py::dict attributes = m_instance.attr("__dict__");
        static nlohmann::json initialDynamicState;

        for (auto item : attributes) {
            std::string key = py::str(item.first);
            
            if (key.rfind("_", 0) == 0) continue;

            py::object value = py::reinterpret_borrow<py::object>(item.second);

            if (py::isinstance<Component>(value)) continue;

            if (py::isinstance<py::float_>(value)) {
                float v = value.cast<float>();
                if (EditorUI::DragFloat(key.c_str(), &v, 0.1f, this)) {
                    m_instance.attr(key.c_str()) = v;
                }
            }
            else if (py::isinstance<py::bool_>(value)) {
                bool v = value.cast<bool>();
                if (EditorUI::Checkbox(key.c_str(), &v, this)) {
                    m_instance.attr(key.c_str()) = v;
                }
            }
            else if (py::isinstance<py::int_>(value)) {
                int v = value.cast<int>();
                if (EditorUI::DragInt(key.c_str(), &v, 1.0f, this)) {
                    m_instance.attr(key.c_str()) = v;
                }
            }
            else if (py::isinstance<py::str>(value)) {
                std::string v = value.cast<std::string>();
                
                char buffer[256];
                strncpy(buffer, v.c_str(), sizeof(buffer));
                buffer[sizeof(buffer) - 1] = '\0';
                
                ImGui::InputText(key.c_str(), buffer, sizeof(buffer));

                if (ImGui::IsItemActivated()) {
                    initialDynamicState = Serialize();
                }
                if (ImGui::IsItemActive()) {
                    m_instance.attr(key.c_str()) = std::string(buffer);
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    CommandHistory::AddCommand(std::make_unique<ModifyComponentCommand>(this, initialDynamicState, Serialize()));
                }
            }
            else {
                std::string typeName = py::str(value.get_type().attr("__name__"));
                ImGui::TextDisabled("%s : [%s]", key.c_str(), typeName.c_str());
            }
        }
    } 
    catch (const py::error_already_set& e) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Python Error:");
        ImGui::TextWrapped("%s", e.what());
    }
}
#endif

nlohmann::json ScriptComponent::Serialize() const {
    nlohmann::json j;
    
    j["type"] = "ScriptComponent";

    // Save the core data needed to find and reload the script
    j["scriptUUID"] = (uint64_t)m_scriptUUID;
    j["className"] = m_className;

    // Automatically save all valid Python variables
    nlohmann::json properties;

    if (m_instance) {
        try {
            py::dict attributes = m_instance.attr("__dict__");

            for (auto item : attributes) {
                std::string key = py::str(item.first);
                
                // Skip private/internal variables
                if (key.rfind("_", 0) == 0) continue;

                py::object value = py::reinterpret_borrow<py::object>(item.second);

                // Skip component references (they are managed by the C++ engine, not the script state)
                if (py::isinstance<Component>(value)) continue;

                // Dynamically check types and save them to the JSON properties object
                if (py::isinstance<py::bool_>(value)) {
                    properties[key] = value.cast<bool>();
                } 
                else if (py::isinstance<py::float_>(value)) {
                    properties[key] = value.cast<float>();
                } 
                else if (py::isinstance<py::int_>(value)) {
                    properties[key] = value.cast<int>();
                } 
                else if (py::isinstance<py::str>(value)) {
                    properties[key] = value.cast<std::string>();
                }
            }
        } catch (const py::error_already_set& e) {
            std::cerr << "[ScriptComponent] Auto-Serialize Error:\n" << e.what() << std::endl;
        }
    }

    j["properties"] = properties;

    return j;
}

void ScriptComponent::Deserialize(const nlohmann::json& j) {
    // Restore the script identifiers
    if (j.contains("scriptName")) m_scriptName = j["scriptName"].get<std::string>();
    
    // Backward compatibility & UUID loading
    if (j.contains("scriptUUID")) {
        LoadScript(UUID(j["scriptUUID"].get<uint64_t>()), m_className);
    } 
    else if (j.contains("scriptName")) {
        std::string sName = j["scriptName"].get<std::string>();
        std::string assumedPath = "assets/scripts/" + sName + ".py";
        LoadScript(AssetRegistry::GetUUIDForPath(assumedPath), m_className);
    }
    
    // If this component was created via deserialization (empty), we need to instantiate Python now
    if (!m_instance && !m_scriptName.empty() && !m_className.empty()) {
        try {
            py::module_ mod = py::module_::import(m_scriptName.c_str());
            py::object cls = mod.attr(m_className.c_str());
            m_instance = cls();
            
            // IMPORTANT: Immediately bind the owner pointer so Python knows its GameObject
            Component* nativeComponent = m_instance.cast<Component*>();
            if (nativeComponent) {
                nativeComponent->owner = this->owner;
            }

            // Initialize hot reload paths when loading a scene
            m_scriptFilePath = "assets/scripts/" + m_scriptName + ".py";
            if (std::filesystem::exists(m_scriptFilePath)) {
                m_lastWriteTime = std::filesystem::last_write_time(m_scriptFilePath);
            }
        } catch (const py::error_already_set& e) {
            std::cerr << "[ScriptComponent] Deserialize Instantiation Error:\n" << e.what() << std::endl;
            return;
        }
    }

    // Automatically restore all saved variables into the Python instance
    if (j.contains("properties") && m_instance) {
        auto props = j["properties"];
        
        for (auto& el : props.items()) {
            std::string key = el.key();
            try {
                // Determine the JSON type and push it directly into the Python dictionary
                if (el.value().is_boolean()) {
                    m_instance.attr(key.c_str()) = el.value().get<bool>();
                } 
                else if (el.value().is_number_float()) {
                    m_instance.attr(key.c_str()) = el.value().get<float>();
                } 
                else if (el.value().is_number_integer()) {
                    m_instance.attr(key.c_str()) = el.value().get<int>();
                } 
                else if (el.value().is_string()) {
                    m_instance.attr(key.c_str()) = el.value().get<std::string>();
                }
            } catch (const py::error_already_set& e) {
                std::cerr << "[ScriptComponent] Failed to restore property '" << key << "':\n" << e.what() << std::endl;
            }
        }
    }
}
