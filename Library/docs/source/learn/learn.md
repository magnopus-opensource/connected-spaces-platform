# LEARN

Developing applications across today's fragmented internet is difficult. Most solutions require development across multiple platforms, making them expensive and complicated. An ideal approach should support open standards for seamless work across environments.

The Connected Spaces Platform (CSP) is an open-source, client-side library designed to make it easy for developers to create interoperable, cross-reality, multi-user applications. The main goal of CSP is to create an interoperability system for the spatial web, where different devices and applications can smoothly work together. Users can interact with any digital content without worrying about what technology or platform they're using. 

Architecturally, CSP functions as a bridge between the client applications and a set of cloud services.

## Prerequisites

Before diving into the details of CSP, it is beneficial to have a foundational understanding of the following key concepts and technologies:

* **Basic Networking Concepts:** Understanding of basic networking principles such as HTTP, WebSockets, and data routing.

* **Cloud Computing Fundamentals:** Familiarity with cloud services, especially concepts related to scalability, storage, and cloud-based applications.

* **Programming and Scripting:** Basic knowledge of programming languages like JavaScript, C\#, or C++ is helpful, particularly in understanding the API and CSP scripting engine.

* **Game Engines and Development Tools**: Awareness of game engines like Unity, Unreal Engine and development environments for AR/VR applications.

## General Architecture

Understanding the general architecture provides a valuable context on how various applications and cloud services interact with CSP.

![image info](../_static/stack.png)

### Apps, Plugins & Integration Layer

This layer represents client applications which use CSP. These may be in any number of forms, ranging from lightweight plugins and libraries for various software development environments and operating systems, to applications built with engines such as Unreal, Unity, Godot or PlayCanvas. They may be deployed on Windows, MacOS,  iOS, Android or VisionOS. Here are some features that they may include:

* **Creator Tools**: For developers to build and design applications.

* **Game Engine-Specific Plugins & Interfaces**: Custom plugins and interfaces for different game engines.

* **Connected Space Primitives**: Building blocks for many spatial web primitives, including avatars, 3D objects, images, and videos.

* **Available Cross-Platform Coherence**: Ensures consistent experiences across different platforms.

* **Event Listeners and Actors**: Mechanisms to handle events and interactions within the platform.

* **User Interface Abstractions**: Simplified UI components to enhance user interaction.

* **Support for Custom Engine Functionality for Rapid Prototyping:** Allows developers to create and test new features quickly.

### CSP Layer

Architecturally, the Connected Spaces Platform is an interoperability layer. It acts as a translator that allows different software and applications to communicate and share data, ensuring seamless integration and interaction across various systems.

Key components of this layer include:

* **Open Source SDK:** Helps developers create apps that work with different systems and services by providing reusable components and documentation.

* **Language and Engine-Specific Interfaces**: Interfaces for different programming languages and engines, such as JavaScript, C\#, and C++.

* **Logic Aggregations for Scene Graphs, Object Hierarchies, and Visibility Scopes**: Manages complex data structures and their relationships.

* **Event Routes**: Route events to the appropriate handlers.

* **JavaScript Scripting Engine:** Allows developers to write scripts that can interact with various components of CSP.

* **Data Marshaling between Cloud Services and Engines**: Converts data into a suitable format across different systems.

* **External API Connectors**: Integrates external data sources and services like IoT devices.

### Cloud Service Layer

The cloud service layer relies on a stable internet connection to provide users with seamless interaction with the Connected Spaces Platform.

Key components of the layer include:

* **User Accounts & Management:** Handles user authentication, authorization, and profile management.

* **Security & Access Control:** Ensures data security and proper access control measures.

* **Groups, Organizations, and Social Connections:** Facilitates social interactions and organizational structures.

* **Referential Object Definitions and Instances:** Manages objects and their references within the platform.

* **Persistent Multiplayer Services:** Supports continuous and synchronized multi-users experiences.

* **Geospatial Databases:** Manages spatial data for accurate and reliable virtual interactions.

* **Economic Engine Interfaces:** Integrates economic models and transactions.

## Overview

This series will introduce you to CSP and give you an overview of its features. 

You will learn about how CSP functions and whether it makes sense for your use-case. We will also go into detail on CSP's most essential concepts and features.

```eval_rst
.. toctree::
   :maxdepth: 1
   :titlesonly:

   overview/mcs
   overview/tenants
   overview/protocols
   overview/graphql
   overview/logging
   overview/maintenance_windows
```


## Users

This series covers how to reason about users in your application, using CSP.

We will cover user account creation, how to log in and out via the API, how users can manage their credentials, and user-specific settings.

```eval_rst
.. toctree::
   :maxdepth: 1
   :titlesonly:

   users/authentication
   users/management
   users/settings
```

## Spaces

This series introduces the notion of what a space is, in the context of CSP.

You will learn how spaces are structured, how content relates to them, and gain insights into space scene hierarchies. We will also cover multiplayer architectural design, event handling, and how entities, components, and avatars work together to provide a seamless multi-user experience within a space.

```eval_rst
.. toctree::
   :maxdepth: 1
   :titlesonly:

   spaces/management
   spaces/assets
   spaces/multiplayer_architecture
   spaces/entities
   spaces/events
```

## Physical and Digital Realities

This module will teach you how to bridge the physical and digital divide through augmented reality, digital twinning, and precise anchoring methods.

By the end of this module, you will learn how to use various tools for spatial positioning, including digital twinning for asset alignment, anchor management using Google Cloud Anchors (GCA), and fiducial markers. You will also understand how to establish and manage Points of Interest (POIs) in CSP, enhancing user experience by pinpointing significant locations within both physical and digital environments.

```eval_rst
.. toctree::
   :maxdepth: 1
   :titlesonly:

   physical_digital_realities/cross_reality_applications
   physical_digital_realities/anchoring
   physical_digital_realities/fiducial_markers
```