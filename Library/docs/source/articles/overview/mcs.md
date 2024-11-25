# Magnopus Cloud Services (MCS)
CSP-enabled applications rely on cloud-hosted services to function effectively. Magnopus offers a set of cloud-hosted services, known as Magnopus Cloud Services (MCS), specifically designed for the Connected Space Platform. These services act as an independent layer that supports both the CSP and client applications. It is not required to use MCS with the Connected Spaces platform.

It is however recommended to use MCS with CSP, as MCS helps developers simplify development by providing ready-to-use tools and services, reducing the complexity of building and maintaining applications. Selecting MCS as the backend service provider allows the developer to not worry about the services themselves, and exclusively focus on building their spatial-web application using the CSP API.

### Importance of MCS in the CSP Ecosystem
* **Scalability**: MCS allows CSP-enabled applications to scale their operations seamlessly to accommodate growing users and applications without compromising performance. It adjusts resources dynamically to handle peak periods, keeping services running without interruptions.

* **Flexibility:** MCS components ensure CSP applications adjust to new needs and features. This flexibility lets the applications deploy updates fast, listen to user feedback, and stay updated with technology trends.

* **Interoperability:** MCS allows seamless integration with various external systems and services, hence offering both flexibility and usability of CSP. This interoperability allows CSP to connect with third-party applications, APIs, and tools, expanding the ecosystem's functionality and reach.

* **Increased Developer Experience:** MCS lets developers focus on building and innovating instead of managing infrastructure by providing comprehensive tools, APIs, and services. 

* **User engagement:** MCS provides an interactive environment through various services, including user management, social connections, and multi-users services. These services improve user satisfaction by allowing customized experiences, collaboration, and community-building.  

### What are the Core Components of MCS?

**1\. User Accounts & Management**

User Accounts & Management handles user authentication, authorization, and profile management. This component ensures that users can securely log in, manage their profiles, and access the appropriate services and features within the CSP environment.

**2\. Security & Access Control**

Security & Access Control is responsible for protecting data and ensuring that only authorized users can access certain parts of the platform. This component uses various security measures, such as encryption and permissions, to safeguard sensitive information and maintain privacy.

**3\. Groups, Organizations, and Social Connections**

This component facilitates social interactions and organizational structures within the CSP. Users can form groups, join organizations, and connect with others to collaborate and share experiences. It helps build a community and enhances user engagement.

**4\. Referential Object Definitions and Instances**

Referential Object Definitions and Instances manage objects and their references within the platform. It creates, stores, and retrieves digital assets like 3D models, avatars, and virtual objects, ensuring they are consistently available and up-to-date for CSP-enabled applications.

**5\. Persistent Multiplayer Services**

Persistent Multiplayer Services support continuous, scalable and synchronized multi-users experiences. This component ensures that users can interact in real-time within shared virtual spaces, providing a seamless multi-user environment that is always available.

**6\. Geospatial Databases**

Geospatial Databases handle spatial data, which is crucial for accurate and reliable interactions in connected spaces. This component manages information about the physical location and positioning of objects and users within the virtual environment, enabling precise and consistent experiences across both the digital and spatial domains.

**7\. Economic Engine Interfaces**

Economic Engine Interfaces integrate economic models and transactions into the CSP. This component allows for creating and managing e-commerce and e-ticketing platforms like Shopify and Eventbrite, providing a framework for monetization and financial interactions.

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

* Session Creation: Starting multi-users sessions.  
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

CSP relies on a set of cloud-hosted services. To perform asynchronous operations in CSP, like fetching and storing data from servers, there are two key patterns to be aware of: Callbacks and the Result Object. 

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

* Within CSP, the GetAssetCollectionById method, the AssetCollectionId is used to fetch an AssetCollection from the services. 

* The Callback object is then invoked by the ResponseHandler (once it has converted an HttpResponse into a Result Object) to notify the client application and provide them with the result.

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