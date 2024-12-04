# Magnopus Cloud Services (MCS)
CSP-enabled applications rely on cloud-hosted services to function effectively. Magnopus offers a set of cloud-hosted services, known as Magnopus Cloud Services (MCS), specifically designed for the Connected Spaces Platform. These services act as an independent layer that supports both CSP and client applications. It is not required to use MCS with the Connected Spaces Platform.

It is however recommended to use MCS with CSP, as MCS helps developers simplify development by providing ready-to-use tools and services, reducing the complexity of building and maintaining applications. Selecting MCS as the backend service provider allows the developer to _not_ worry about the services themselves, and exclusively focus on building their spatial-web application using the CSP API.

### Importance of MCS in the CSP Ecosystem
* **Scalability**: MCS allows CSP-enabled applications to scale their operations seamlessly to accommodate growing users and applications without compromising performance. It adjusts resources dynamically to handle peak periods, keeping services running without interruptions.

* **Flexibility:** MCS components ensure CSP applications adjust to new needs and features. This flexibility lets the applications deploy updates fast, listen to user feedback, and stay updated with technology trends.

* **Interoperability:** MCS allows seamless integration with various external systems and services, hence offering both flexibility and usability of CSP. This interoperability allows CSP to connect with third-party applications, APIs, and tools, expanding the ecosystem's functionality and reach.

* **Increased Developer Experience:** MCS lets developers focus on building and innovating instead of managing infrastructure by providing comprehensive tools, APIs, and services. 

* **User engagement:** MCS provides an interactive environment through various services, including user management, social connections, and multi-user services. These services improve user satisfaction by allowing customized experiences, collaboration, and community-building.  

### What are the Core Components of MCS?

**1\. User Accounts & Management**

User Accounts & Management handles user authentication, authorization, and profile management. This component ensures that users can securely log in, manage their profiles, and access the appropriate services and features within the CSP environment.

**2\. Security & Access Control**

Security & Access Control is responsible for protecting data and ensuring that only authorized users can access certain parts of the platform. This component uses various security measures, such as encryption and permissions, to safeguard sensitive information and maintain privacy.

**3\. Groups, Organizations, and Social Connections**

This component facilitates social interactions and organizational structures within CSP. Users can form groups, join organizations, and connect with others to collaborate and share experiences. It helps build a community and enhances user engagement.

**4\. Referential Object Definitions and Instances**

Referential Object Definitions and Instances manage objects and their references within the platform. It creates, stores, and retrieves digital assets like 3D models, avatars, and virtual objects, ensuring they are consistently available and up-to-date for CSP-enabled applications.

**5\. Persistent Multiplayer Services**

Persistent Multiplayer Services support continuous, scalable and synchronized multi-user experiences. This component ensures that users can interact in real-time within shared virtual spaces, providing a seamless multi-user environment that is always available.

**6\. Geospatial Databases**

Geospatial Databases handle spatial data, which is crucial for accurate and reliable interactions in connected spaces. This component manages information about the physical location and positioning of objects and users within the virtual environment, enabling precise and consistent experiences across both the digital and spatial domains.

**7\. Economic Engine Interfaces**

Economic Engine Interfaces integrate economic models and transactions into CSP. This component allows for creating and managing e-commerce and e-ticketing platforms like Shopify and Eventbrite, providing a framework for monetization and financial interactions.

## Services Offered by MCS

### **User Service**

The User Service is the backbone of user management within the MCS platform. This service handles:

* User Registration: Allowing new users to create accounts.  
* Authentication: Verifying user identities during login.  
* Authorization: Assigning roles and permissions to users.  
* Profile Management: Enabling users to update their personal information.

Key Features:

* Secure access to the platform.  
* Maintains user data integrity.  
* Includes password management and session control.

### **Ranking Service**

The Ranking Service is designed to evaluate and compare user activities. This service helps in:

* Tracking Performance: Monitoring user activities and contributions.  
* Creating Leaderboards: Displaying rankings based on predefined criteria.  
* Awarding Badges: Recognizing achievements to motivate users.

Key Features:

* Enhances user engagement through gamification.  
* Supports various ranking metrics

### **Asset Service**

The Asset Service manages all digital assets within the platform. It includes:

* Storage: Keeping 3D models, images, audio files, and multimedia.  
* Retrieval: Fetching assets as needed by users or applications.  
* Versioning: Managing different versions of assets.  
* Distribution: Ensuring assets are available and up-to-date.

Key Features:

* Efficient management of digital resources.  
* Supports a wide range of file types.

### **Multiplayer Service**

The Multiplayer Service enables real-time interactions between users. This service is crucial for:

* Session Creation: Starting multi-user sessions.  
* User Connectivity: Connecting users to the same session.  
* State Synchronization: Ensuring all users see the same state in real time.

Key Features:

* Supports collaborative and competitive interactions.  
* Ensures a seamless multi-user experience.

### **Spatial Data Service**

The Spatial Data Service handles geospatial information within the platform. This service supports:

* Storage: Keeping geospatial data.  
* Retrieval: Accessing spatial data as needed.  
* Analysis: Performing spatial queries and analysis.

Key Features:

* Enables accurate positioning in virtual environments.  
* Supports mapping and location-based services.

### **Aggregation Service**

The Aggregation Service combines data from various sources to provide a unified view. This service performs:

* Data Collection: Gathering data from different systems.  
* Normalization: Standardizing data formats.  
* Transformation: Converting data to a usable form.  
* Aggregation: Summarizing data for analysis.

Key Features:

* Integrates data seamlessly from multiple sources.  
* Supports comprehensive data analysis.

## Callbacks and Result Objects in CSP

CSP relies on a set of cloud-hosted services. To perform asynchronous operations in CSP, like fetching and storing data from servers, there are two key patterns to be aware of: Callbacks and Result Objects.

### Callbacks

Callbacks are functions passed as arguments to other functions or methods. They allow a program to continue executing while waiting for an asynchronous operation to complete. When the operation finishes, the callback function is invoked, allowing the client application to handle the result.

In CSP, callbacks handle asynchronous responses from cloud-hosted services. An example of a callback in CSP is an instance of the AssetCollectionResultCallback class. This callback is primarily responsible for handling results from requests to retrieve asset collections.

```
/// @brief Callback containing asset collection.
/// @param Result AssetCollectionResult
std::function<void(const AssetCollectionResult& Result)>  AssetCollectionResultCallback;
```

The typedef (Type Definition) keyword declares an alias for an std::function in the code snippet.  Instances of this type store callable routines  such as functions or lambda expressions. The callable routine returns void and takes a const reference to an AssetCollectionResult object as an argument.

### Using Callbacks

One common technique for managing asynchronous tasks in MCS is through the use of callbacks. 

**Example: Retrieving an Asset Collection**  
Consider the method GetAssetCollectionById in the AssetSystem class, which demonstrates how callbacks are used to retrieve an asset collection by its AssetCollectionId. Within CSP, the method initiates a request to a service and, upon receiving a response, invokes a callback for the client application to handle the retrieved data.

```
void AssetSystem::GetAssetCollectionById(const String& AssetCollectionId, AssetCollectionResultCallback Callback)
{
    services::ResponseHandlerPtr ResponseHandler = PrototypeAPI->CreateHandler<AssetCollectionResultCallback, AssetCollectionResult, void, chs::PrototypeDto>(Callback, nullptr);
    static_cast<chs::PrototypeApi*>(PrototypeAPI)->apiV1PrototypesIdGet(AssetCollectionId, ResponseHandler);
}
```

* Within CSP, the GetAssetCollectionById method and the AssetCollectionId are used to fetch an AssetCollection from the services. 

* The Callback object is then invoked by the ResponseHandler (once it has converted an HttpResponse into a Result Object) to notify the client application and provide the result.

### Result Objects

A Result Object contains response data retrieved from an asynchronous operation in CSP. The object transforms raw data received from the services into interoperable, strongly typed formats.

An example of a Result Object is AssetCollectionResult.The AssetCollectionResult class is a result object that stores information about an asset collection.

```
class CSP_API AssetCollectionResult : public csp::systems::ResultBase
{
public:
	AssetCollection& GetAssetCollection();
	const AssetCollection& GetAssetCollection() const;
private:
	AssetCollectionResult(void*);
	void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;
	AssetCollection AssetCollection;
};
```

* The Result Object contains an associated object, AssetCollection, stored as a private member.

* The AssetCollectionResult defines two getters to get the AssetCollection by reference or by const reference.

* The `AssetCollectionResult` includes an `OnResponse` method that is used internally within CSP to transform the DTO (Data Transfer Object)  received from services into a strongly typed CSP object, `AssetCollection`.

## Interaction Between Callbacks and Result Objects

For asynchronous operations, the interaction between callbacks and result objects is essential. 

Here is an example of a workflow:

**1\. Client Code Initiates Request**:

```
csp::systems::AssetCollectionResultCallback GetAssetCollectionByIdCallback = [Callback, this](const csp::systems::AssetCollectionResult& Result)
{
	if (Response == EOKOResponse::Success)
	{
		const csp::systems::AssetCollection& RetrievedAssetCollection = Result.GetAssetCollection();
		// Process the retrieved asset collection...
	}
};

AssetSystem->GetAssetCollectionById(MyAssetCollectionId, GetAssetCollectionByIdCallback);
```

**2\. Request Handling**:

1. The client code defines a callback (`GetAssetCollectionByIdCallback`) as a lambda expression.

2. The client code calls the GetAssetCollectionById method, passing the asset collection ID and the callback as parameters.  
   

**3\. Services Response**:

1. The services process the request and send a response.

2. The CSP ResponseHandler generates an AssetCollectionResult object from the HTTP response.  
   

**4\. Callback Execution**:

1. Within CSP, the OnResponse method of AssetCollectionResult parses the DTO into a CSP object.

2. The callback is executed, allowing the client application to use the parsed data via the result object's getters.

## Mapping Magnopus Cloud Services to CSP Systems

Mapping MCS to CSP systems is crucial to understand how various services and functionalities are used within CSP. This process involves aligning the core components of MCS with the corresponding systems and subsystems in CSP to ensure they work together smoothly and effectively to establish a strong operational connection

The primary systems within CSP include:

**1\. User System**

The User System is responsible for managing user identities and credentials within CSP. It includes functionalities such as:

* **User Authentication and Authorization**: Ensures that only authenticated users can access the platform and have the appropriate permissions for their roles.  
* **Profile Management**: Allows users to create and update their profiles, including preferences and settings.  
* **User Sessions**: Manages user sessions to maintain a consistent and secure experience across different devices and applications.

**2\. Asset System**

The Asset System manages the digital assets within CSP. Key features include:

* **Asset Storage**: Securely stores digital assets, including 3D models, textures, audio files, and videos.  
* **Asset Retrieval**: Provides efficient mechanisms for retrieving assets as needed by different applications and users.  
* **Version Control**: Maintains different versions of assets to ensure that users can access the most up-to-date or preferred versions as required.

**3\. Space System**

The Space System manages the digital and physical spaces within CSP. It includes:

* **Space Creation and Management**: Facilitates the creation, configuration, and maintenance of virtual spaces.  
* **Access Control**: Ensures that spaces are accessible only to authorized users.  
* **Content Management**: Manages the content within these spaces, including layout, visibility, and interaction rules.

**4\. Multiplayer System**

The Multiplayer System supports real-time, synchronous interactions among users. Features include:

* **Session Management**: Manages multi-user sessions, ensuring users can join, interact, and leave sessions seamlessly.  
* **State Synchronization**: Keeps the state of the virtual environment consistent across all users, enabling a shared experience.  
* **Interaction Management**: Facilitates various forms of user interactions, such as communication, collaboration, and competition.

**5\. Scripting System**

The Scripting System allows developers to add custom behaviors and interactions within CSP. It provides:

* **Script Execution**: Executes scripts that define how objects and environments behave.  
* **Event Handling**: Manages events triggered by user actions or system changes, allowing for dynamic and responsive interactions.  
* **Customization**: Supports customization of interactions and functionalities to meet specific application needs.

**6\. Anchoring System**

The Anchoring System links digital spaces to real-world locations, providing spatial accuracy. It includes:

* **Geospatial Mapping**: Maps virtual objects to physical locations with high precision.  
* **Spatial Anchoring**: Ensures that digital objects remain in their designated positions relative to the real world, even as users move around.  
* **Augmented Reality Integration**: Integrates with AR systems to enhance the realism and utility of virtual overlays on physical spaces.

## How Specific MCS Services Map to CSP Systems

**User Service**

![image info](../../_static/mcs/user_service.png)

The user service is integral to the CSP user system. It handles the creation and storage of user profiles, groups, and space information. Additionally, it provides secure authentication tokens for accessing CSP and premium services such as Agora, Eventbrite, Shopify, and single sign-on. This service ensures a structured framework for managing user identities, securing access, and facilitating efficient group interactions within the CSP environment.

**Ranking Service**

![image info](../../_static/mcs/ranking_service.png)

The Ranking Service plays a crucial role within the CSP User System. It stores user preferences and settings, allowing applications to customize experiences for individual users. This service enables users to rank resources along application-specific dimensions, supporting features like "likes" and "favorites." The persistent nature of this information ensures a familiar yet unique user experience across all platforms.

**Asset Service**

![image info](../../_static/mcs/asset_service.png)

The Asset Service is a key component of the CSP Asset System. It manages the storage and distribution of various digital assets, including metadata, 2D and 3D art, video, and audio files. The service also supports advanced features like Level of Detail (LOD) model generation and object capturing, which optimize content delivery and simplify asset management. By storing assets in the cloud, they become accessible from anywhere, ensuring efficient content management.

**Multiplayer Service**

![image info](../../_static/mcs/multiplayer_service.png)

The Multiplayer Service enhances the CSP Multiplayer System by facilitating real-time networked experiences. It enables users to interact with each other and their environment in real time, using SignalR technology for low-latency, stable connections globally. Supported by services like Redis Backplane and MongoDB, this service ensures seamless interactions, regardless of users' physical locations. Additionally, it provides persistence, ensuring that user interactions extend beyond individual sessions, fostering a dynamic and interconnected community.

**Spatial Data Service**

![image info](../../_static/mcs/spatialdata_service.png)

The Spatial Data Service is essential for the CSP Space and Anchoring Systems. It stores and accesses real-world geographical data, enabling location-based experiences. This service allows spaces, objects, and events to be geolocated and anchored to physical locations, bridging the gap between the digital and physical worlds. The spatial data service empowers users with personalized, interactive, and contextually relevant encounters in their surroundings, enhancing their experiences across various contexts.

**Aggregation Service**

![image info](../../_static/mcs/aggregation_service.png)

The Aggregation Service simplifies data retrieval within the CSP User and Space Systems. Built with GraphQL, it optimizes querying by consolidating multiple API calls into single queries. This service streamlines data retrieval, reduces network overhead, and enhances performance, providing a seamless user experience.

## Summary

This topic explored how Magnopus Cloud Services (MCS) is essential to the  Connected Spaces Platform. It outlined key MCS components like user management, security, social connections, asset management, multiplayer services, geospatial databases, and economic interfaces. These components ensure that CSP-enabled applications can offer scalable, flexible, and reliable services.

This topic also detailed how specific services, such as User Service, Ranking Service, Asset Service, Multiplayer Service, Spatial Data Service, and Aggregation Service, can enhance functionality and user experience in CSP.

Callbacks and result objects are crucial for handling asynchronous operations in MCS. These functionalities enable efficient data retrieval and processing, ensuring smooth interactions in the platform.

Overall, cloud-hosted services play a vital role in providing infrastructure and services to meet the dynamic needs of the CSP ecosystem, driving innovation and improving user experiences.
