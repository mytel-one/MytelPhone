#pragma once
#include <map>
#include <memory>
#include <string>

class TestDirectoryConfig {
public:
	std::string getString(const std::string &section, const std::string &key, const std::string &fallback) const {
		const auto found = values.find(section + "/" + key);
		return found == values.end() ? fallback : found->second;
	}
	void setString(const std::string &section, const std::string &key, const std::string &value) {
		values[section + "/" + key] = value;
	}

private:
	std::map<std::string, std::string> values;
};

class TestDirectoryCore {
public:
	std::shared_ptr<TestDirectoryConfig> getConfig() {
		return config;
	}

private:
	std::shared_ptr<TestDirectoryConfig> config = std::make_shared<TestDirectoryConfig>();
};

class CoreModel {
public:
	static CoreModel *getInstance() {
		static CoreModel model;
		return &model;
	}
	std::shared_ptr<TestDirectoryCore> getCore() {
		return core;
	}

private:
	std::shared_ptr<TestDirectoryCore> core = std::make_shared<TestDirectoryCore>();
};
