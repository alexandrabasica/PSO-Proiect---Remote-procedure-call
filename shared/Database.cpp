#include "Database.h"
#include <iostream>
#include <chrono>
#include <ctime>

Database::Database(const std::string& filename) : filename(filename) {
    load();
}

void Database::load() {
    if (doc.LoadFile(filename.c_str()) != tinyxml2::XML_SUCCESS) {
        std::cout << "database.xml not found, creating new one.\n";
        auto* root = doc.NewElement("applicationData");
        doc.InsertFirstChild(root);

        auto* settings = doc.NewElement("settings");
        root->InsertEndChild(settings);

        auto* state = doc.NewElement("state");
        root->InsertEndChild(state);

        auto* logs = doc.NewElement("logs");
        root->InsertEndChild(logs);

        save();
    }
}

void Database::save() {
    doc.SaveFile(filename.c_str());
}

void Database::addLog(const std::string& message) 
{
    auto* root = doc.RootElement();
    auto* logs = root->FirstChildElement("logs");
    if (!logs) {
        logs = doc.NewElement("logs");
        root->InsertEndChild(logs);
    }

    auto* log = doc.NewElement("log");
    
    // adaugam timestamp
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", std::localtime(&t));
    log->SetAttribute("timestamp", buf);
    log->SetText(message.c_str());
    logs->InsertEndChild(log);

    save();
}

void Database::setStateVariable(const std::string& name, const std::string& value) {
    auto* root = doc.RootElement();
    auto* state = root->FirstChildElement("state");
    if (!state) {
        state = doc.NewElement("state");
        root->InsertEndChild(state);
    }

    tinyxml2::XMLElement* var = state->FirstChildElement("variable");
    while (var) {
        if (name == var->Attribute("name")) {
            var->SetText(value.c_str());
            save();
            return;
        }
        var = var->NextSiblingElement("variable");
    }

    // daca nu exista, adaugam unul nou
    auto* newVar = doc.NewElement("variable");
    newVar->SetAttribute("name", name.c_str());
    newVar->SetText(value.c_str());
    state->InsertEndChild(newVar);
    save();
}

std::string Database::getStateVariable(const std::string& name) {
    auto* root = doc.RootElement();
    auto* state = root->FirstChildElement("state");
    if (!state) return "";

    tinyxml2::XMLElement* var = state->FirstChildElement("variable");
    while (var) {
        if (name == var->Attribute("name")) {
            return var->GetText() ? var->GetText() : "";
        }
        var = var->NextSiblingElement("variable");
    }
    return "";
}
