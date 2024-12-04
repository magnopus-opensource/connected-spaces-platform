# Logging

Logging is an essential process in software development and system management that involves recording events, messages, and errors during the execution of an application. These log entries provide valuable insights into the system's behavior, helping developers and administrators understand what is happening at various stages of the application's lifecycle.

1. **Debugging**: Logs are crucial for debugging purposes. When an error occurs, logs can provide detailed information about the state of the application, the sequence of operations, and the exact point of failure. This information is vital for diagnosing issues and finding solutions quickly.

2. **Monitoring**: Continuous monitoring of logs helps in identifying potential issues before they become critical. By analyzing log patterns, administrators can detect anomalies, performance bottlenecks, and security threats, allowing them to take proactive measures.

3. **Auditing**: Logs serve as an audit trail, recording significant events and actions within the system. This is important for compliance with regulatory requirements and for forensic analysis in the event of a security breach.

4. **Performance Analysis**: Logs provide data that can be analyzed to understand the system's performance characteristics. This helps in optimizing the application and ensuring it runs efficiently.

5. **User Behavior Analysis**: Logs can capture user interactions with the system, providing insights into user behavior and preferences. This information can be used to improve the user experience and guide future development.

## CSP-Specific Logging Features

The Connected Spaces Platform (CSP) incorporates a logging system designed to meet the needs of modern applications. Here are some key features:

1. **System-Level Logging**: CSP allows you to set the verbosity of logging at a system-wide level. This means you can control the amount of detail logged, from critical errors to informational debug messages.

   * `SetSystemLevel`: Adjusts the verbosity level for logging.

   * `GetSystemLevel`: Retrieves the current verbosity level.

   * `LoggingEnabled`: Checks if logging is enabled for a specified verbosity level.

2. **Event Logging**: CSP supports logging specific events, which can be used for monitoring significant occurrences within the system.

   * `LogEvent`: Records an event in the log.

3. **Marker Events**: CSP introduces the concept of marker events to indicate the beginning and end of specific processes, providing a clear outline of operations within the logs.

   * `BeginMarker`: Specifies the start of a marker event.

   * `EndMarker`: Indicates the end of a marker event.

4. **Callback System**: One of the standout features of CSP's logging system is the ability to set callbacks for various types of logs. This allows client applications to react to specific logs or events in real-time.

   * `SetLogCallback`: Sets a callback for handling log messages.

   * `SetEventCallback`: Sets a callback for handling event logs.

   * `SetBeginMarkerCallback`: Sets a callback for handling the start of marker events.

   * `SetEndMarkerCallback`: Sets a callback for handling the end of marker events.

   * `ClearAllCallbacks`: Clears all logging callbacks, providing a way to reset the logging behavior.

5. **Message Logging**: CSP also allows the client application to  provide log  messages at specific verbosity levels, providing detailed information based on the set logging level.

   * `LogMsg`: Logs a message with a specified verbosity level.

## CSP Log Categories

### System Logs

System logs capture and record the operational activities of  CSP. They help in diagnosing issues, tracking the platform's health, and auditing system behaviors.

**Examples of System Logs:**

* **Startup Logs:** Records details when the system starts.

* **Shutdown Logs:** Captures information during system shutdown.

* **Error Logs:** Logs system errors that occur during operation.

* **Performance Logs:** Tracks performance metrics and system efficiency.


### Event Logs

Event logs record specific actions or occurrences within CSP. They are essential for monitoring user activities, tracking events, and understanding system interactions.

**Examples of Event Logs:**

* **User Login Events:** Logs details when a user logs in.

* **File Upload Events:** Captures information about file uploads.

* **Data Access Events:** Records access to specific data or resources.

* **Transaction Events:** Tracks transactions within the system.


### Marker Logs

Marker logs are used to identify certain points or stages within a process. In the context of CSP, the process refers to any operation or sequence of actions within the platform that you want to monitor or debug. 

Here are some examples of processes where marker logs can be useful:

* **Data Processing:** Tracking the start and end of data processing tasks to ensure they complete successfully and within expected time frames.

* **API Calls:** Monitoring the flow of API requests and responses to identify any issues or delays.

* **User Workflows**: Following the steps a user takes through a particular workflow, such as a multi-step form submission.

* **Background Jobs:** Logging the phases of background tasks, like batch processing or scheduled maintenance activities.

* **Transaction Handling:** Marking the points in a financial or data transaction to ensure all steps are executed correctly.

**Usage Scenarios for Marker Logs:**

* **Debugging:** Use markers to pinpoint the beginning and end of a function to identify where issues may occur.

* **Performance Monitoring:** Track the duration of specific operations to optimize performance.

* **Process Tracking:** Follow the sequence of complex processes to ensure each step completes correctly.

## Setting Up Logging in Your Client Application

To effectively monitor and debug your application, you must set up logging. This process involves configuring various callback handlers that allow you to capture and manage log messages, events, and markers within the Connected Spaces Platform (CSP).

### Pre-requisites:

Before setting up logging in your client application, ensure you have:

* Integrated the CSP library into your project.

* A basic understanding of callbacks and logging mechanisms in CSP.

### Log Callback Handlers

Log callback handlers allow you to specify specific actions that should be performed when a log message is generated or a CSP event occurs. They help to capture and process log messages as they occur.

### How to Define and Set a Log Callback Handler 

1. **Define the Callback Handler:** Create a function that will handle log messages.

```
void MyLogCallback(const csp::common::String& logMessage)
{
	std::cout << "Log: " << logMessage << std::endl;
}
```

2. **Set the Callback Handler:** Use the SetLogCallback function to set your custom handler.

```
LogSystem.SetLogCallback(MyLogCallback);
```

### Event Callback Handlers

Event callback handlers identify and respond to specified events within CSP. They allow you to run a custom code when certain events occur.

### How to Define and Set an Event Callback Handler:

1. **Define the Callback Handler:** Create a function that will handle event messages.

```
void MyEventCallback(const csp::common::String& eventMessage) { 
	std::cout << "Event: " << eventMessage << std::endl; 
}
```

2\.  **Set the Callback Handler:** Use the SetEventCallback function to set your custom handler. 

```
LogSystem.SetEventCallback(MyEventCallback);
```

### Begin Marker Callback Handlers

Begin marker callback handlers mark the start of a specific process or phase. They help in tracking the beginning of operations for debugging or monitoring.

### How to Define and Set an Begin Marker Callback Handler:

1. **Define the Callback Handler:** Create a function that will handle the beginning of markers.

```
void MyBeginMarkerCallback(const csp::common::String& beginMarker) { 
	std::cout << "Begin Marker: " << beginMarker << std::endl; 
}
```

2\. **Set the Callback Handler:** Use the SetBeginMarkerCallback function to set your custom handler. 

```
LogSystem.SetBeginMarkerCallback(MyBeginMarkerCallback);
```

### End Marker Callback Handlers

End marker callback handlers mark the end of a specific process or phase. They help in tracking the completion of operations for debugging or monitoring.

### How to Define and Set an End Marker Callback Handler:

1. **Define the Callback Handler:** Create a function that will handle the end of markers.

```
void MyEndMarkerCallback() { 
	std::cout << "End Marker " << std::endl;
}
```

2\. **Set the Callback Handler:** Use the SetEndMarkerCallback function to set your custom handler. 

```
LogSystem.SetEndMarkerCallback(MyEndMarkerCallback);
```

By following these steps, you can effectively set up logging in your client application, enabling detailed monitoring and debugging of your CSP processes.

## Configuring Logging Verbosity

Understanding the various log levels is important for system monitoring and troubleshooting. Each log level has a specific semantic, and properly configuring the level will ensure that your log has the right information, allowing you to maintain and troubleshoot your system more quickly. 

Here's an overview of the log levels used in CSP systems:

1. **Error:** Describes critical issues that need immediate attention. These indicate severe problems that could cause the system to crash.

```
LogSystem.LogMsg(csp::systems::LogLevel::Error, "Critical error encountered.");
```

2. **Warning:** This represents potential issues that might not immediately affect the system but should be monitored. These help in identifying areas that might become problematic.

```
LogSystem.LogMsg(csp::systems::LogLevel::Warning, "Potential issue detected.");
```   

3. **Info:** This represents general information about the system's operation. These messages provide insights into the system's normal functioning.

```
LogSystem.LogMsg(csp::systems::LogLevel::Info, "System operation details.");
```   

4. **Debug:** It represents detailed information useful for debugging. Messages are more verbose and are typically used during development or troubleshooting.

```
LogSystem.LogMsg(csp::systems::LogLevel::Debug, "Debugging details.");
```   

By setting the appropriate log level, you can control the amount and type of information captured in your logs, making it easier to monitor and debug your system.

## System-Wide Log Level

Use the SetSystemLevel function to set the system's log level. This determines the verbosity of logs generated by the system. Higher verbosity levels include all lower levels, meaning setting a higher log level, like Debug, will also capture logs from Error, Warning, and Info levels. For example: 

```
LogSystem.SetSystemLevel(csp::systems::LogLevel::Debug);
```

### Retrieving the current log level

Use the GetSystemLevel function, to check the current log level. This helps you understand the current verbosity setting. For example: 

```
csp::systems::LogLevel currentLevel = LogSystem.GetSystemLevel();
```

### Checking the Logging Status of Specific Log Levels 

Use the LoggingEnabled function, to see if a specific log level is currently being logged. This ensures that your logs are being captured at the desired level. For example: 

```
bool isLogging = LogSystem.LoggingEnabled(csp::systems::LogLevel::Info);
```

## Summary

Logging is essential in software development and system management, capturing events, messages, and errors during application execution. It provides insights into system behavior, aiding in debugging, monitoring, auditing, performance analysis, and user behavior analysis.
