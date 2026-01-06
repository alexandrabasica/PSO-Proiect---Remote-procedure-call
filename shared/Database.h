#ifndef DATABASE_H
#define DATABASE_H
#include "tinyxml2.h"
#include <string>
#include <mutex>

class Database {
public:
    Database(const std::string& filename = "database.xml");
    void load();
    void save();
    void addLog(const std::string& message);
    void setStateVariable(const std::string& name, const std::string& value);
    std::string getStateVariable(const std::string& name);

private:
    tinyxml2::XMLDocument doc;
    std::string filename;
    std::recursive_mutex m_mutex;
};
#endif
