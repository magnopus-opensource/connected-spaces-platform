#pragma once

#include "quickjspp.hpp"
#include <map>
#include <string>
#include <vector>

namespace qjs
{
class Value;
}

namespace csp::multiplayer
{

class SpaceScriptInterface
{
public:
	SpaceScriptInterface(qjs::Context* context);

	void On(const std::string& eventName, qjs::Value callback);
	void Off(const std::string& eventName, qjs::Value callback);
	void Fire(const std::string& eventName, const qjs::Value& eventArgs);

	static const int js_class_id = 234234;

private:
	qjs::Context* Context;
	std::map<std::string, std::vector<qjs::Value>> EventListeners;
};

} // namespace csp::multiplayer
