#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Common/String.h"


namespace csp::localscripts
{
    CSP_START_IGNORE
    const csp::common::String& EventBusScript = R"(

        export function createEventBus() {
        const eventGroups = new Map();
        const eventQueue = new Map();

        const addToQueue = (evtName, payload) => {
            let queue = eventQueue.get(evtName);
            if (!queue) {
            queue = {};
            eventQueue.set(evtName, queue);
            }
            Object.assign(queue, payload);
        };

        const fire = (name, payload) => {
            const events = eventGroups.get(name);
            if (events) {
            events.forEach((callback) => {
                callback(payload);
            });
            }
        };

        const flushQueue = () => {
            eventQueue.forEach((payload, evtName) => {
            fire(evtName, payload);
            });
            eventQueue.clear();
        };

        return {
            on: (name, callback) => {
            let events = eventGroups.get(name);
            if (!events) {
                events = new Map();
                eventGroups.set(name, events);
            }
            events.set(callback, callback);
            },

            once: (name, callback) => {
            let events = eventGroups.get(name);
            if (!events) {
                events = new Map();
                eventGroups.set(name, events);
            }
            events.set(callback, (props) => {
                callback(props);
                events.delete(callback);
            });
            },

            off: (name, callback) => {
            if (name && callback) {
                const events = eventGroups.get(name);
                if (events) {
                events.delete(callback);
                }
            }
            },

            clear: (name) => {
            eventGroups.delete(name);
            },

            fire,

            clearAll: () => {
            eventGroups.clear();
            },

            hasEvent: (name) => {
            const events = eventGroups.get(name);
            return !!(events && events.size > 0);
            },

            addToQueue,
            flushQueue
        };
        };
    )";
    CSP_END_IGNORE
}