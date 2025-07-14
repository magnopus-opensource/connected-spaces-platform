#include "Multiplayer/Script/SpaceScriptInterface.h"
#include "Debug/Logging.h"
#include "quickjspp.hpp"

namespace csp::multiplayer
{

SpaceScriptInterface::SpaceScriptInterface(qjs::Context* context) : Context(context)
{
}

void SpaceScriptInterface::On(const std::string& eventName, qjs::Value callback)
{
	if (!JS_IsFunction(Context->ctx, callback.v))
	{
		CSP_LOG_ERROR_MSG("Callback supplied to on() is not a function.");
		return;
	}

	EventListeners[eventName].push_back(std::move(callback));
}

void SpaceScriptInterface::Off(const std::string& eventName, qjs::Value callback)
{
	if (!JS_IsFunction(Context->ctx, callback.v))
	{
		CSP_LOG_ERROR_MSG("Callback supplied to off() is not a function.");
		return;
	}

	auto it = EventListeners.find(eventName);
	if (it != EventListeners.end())
	{
		auto& listeners = it->second;
		for (auto i = listeners.begin(); i != listeners.end(); ++i)
		{
			// Use JS_VALUE_GET_PTR to compare function pointers safely
			if (JS_VALUE_GET_PTR(i->v) == JS_VALUE_GET_PTR(callback.v))
			{
				listeners.erase(i);
				break;
			}
		}
	}
}

void SpaceScriptInterface::Fire(const std::string& eventName, const qjs::Value& eventArgs)
{
	auto it = EventListeners.find(eventName);
	if (it != EventListeners.end())
	{
		auto& listeners = it->second;
		for (auto& listener : listeners)
		{
			JSValueConst args[] = {eventArgs.v};
			JSValue result	  = JS_Call(Context->ctx, listener.v, JS_UNDEFINED, 1, args);

			if (JS_IsException(result))
			{
				// Use the proper error handling API
				CSP_LOG_ERROR_MSG("Exception thrown in event handler");
			}

			JS_FreeValue(Context->ctx, result);
		}
	}
}

} // namespace csp::multiplayer
