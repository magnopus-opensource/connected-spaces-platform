# Tenants

A tenant is an instance dedicated to a specific customer. It segregates data from other tenants  secure resource and user management. Each tenant controls their own data, configurations, and policies, which isolates them from other tenants.

Tenants are necessary for securely organizing and managing resources. They help to isolate data and applications for users.

## What is an MCS Tenant?

A tenant is a dedicated instance within the services backend specifically designed to segregate users and their data from others. Each MCS tenant operates independently, ensuring that data and interactions remain isolated.

A developer building a CSP-powered application requires their own tenant to ensures that users of their application cannot interact with users of a different CSP-powered application.

## Multi-Tenancy in MCS

Multi-tenancy is an architecture where a single cloud hosted environment  and its supporting infrastructure are shared among two or more tenants at the same time. While each tenant's data is isolated from all other tenants' data, each tenant shares a single database and SaaS server with the others.

With MCS, this means that one environment can host multiple tenants, each with its own segregated data. 

**Benefits for Clients:**

* **Cost Savings:** Services are affordable as clients share the underlying infrastructure costs.

* **Customizability:** Each tenant can customize its environment to meet specific needs.

* **Security:** Isolation between tenants ensures that one tenant's data and activities do not affect another's.

## Requesting an MCS Tenant

### Prerequisites for Requesting a Tenant

Before requesting an MCS (Magnopus Cloud Services) tenant, ensure you have:

* **Company Name and Email:** A company name and email address is required. Personal email addresses are not accepted.

### Step-by-Step Process to Request an MCS Tenant

As a developer looking to use CSP and Magnopus Cloud Services, you must fill out an online form to request an MCS tenant. Follow these steps to complete your request:

1. Go to [Magnopus CSP for Developers](https://www.magnopus.com/csp/for-developers#tenant-id).  
2. Fill out the **Tenant Inquiry Form**  
3. Review all the information you entered to ensure accuracy and click the **Submit Inquiry** button to send your request. You will receive a confirmation email acknowledging receipt of your request.  
4. Tenant IDs requests are reviewed every two weeks, in line with Magnopus' release schedule.  
   If you have not heard back from the Magnopus team in two weeks confirming the creation of your tenant, please email [tenantkey@magnopus.com](mailto:tenantkey@magnopus.com).

## Initializing CSP with a Tenant

### How to Direct CSP to a Tenant

Tenants are uniquely identified by a string that Magnopus will share with you once your tenancy request has been approved. 

To direct CSP to a tenant, you need to specify the tenant string during the initialization of your application. This involves providing the tenant as an argument to the csp::CSPFoundation::Initialise function. This is expected to be the very first CSP invocation a client application makes in its lifecycle. 

**Example Code Snippet:**

Here is a code snippet demonstrating how to direct CSP to a specific tenant. 

```
int main()
{
	const csp::common::String Tenant = "MY_CSP_TENANT";
	const csp::common::String EndpointRootURI = "https://ogs-ostage.magnoboard.com";
	csp::CSPFoundation::Initialise(EndpointRootURI, Tenant);
	// Additional initialization and application code follows
}
```

**Explanation**

* **Define the Tenant:** The tenant is defined as a constant string Tenant with the value `MY_CSP_TENANT`. Replace `MY_CSP_TENANT` with your specific tenant identifier.

* **Define the Endpoint Root URI:** The endpoint root URI is defined as a constant string EndpointRootURI with the value `https://ogs-ostage.magnoboard.com`.

* **Initialize CSP:** The `csp::CSPFoundation::Initialise` function is called with `EndpointRootURI` and `Tenant` as arguments. This initializes CSP to use the specified custom tenant.

The following diagram illustrates the process flow for directing CSP to a custom tenant:

![image info](../../_static/tenants/tenant_flow_csp.png)

1. **Application Launched:** The application starts and launches successfully.

2. **Calls CSP Initialize:** The application calls CSP Initialize with the endpoint and tenant arguments.

3. **Registers with Services:** CSP uses the provided endpoint and tenant to register with the necessary services.

4. **Application Loop:** The application runs its main loop, performing its intended tasks.

5. **Invokes CSP Shutdown:** When the application is ready to terminate, it invokes the CSP shutdown process.

6. **Tears Down Systems:** CSP tears down all systems, cleaning up resources.

7. **Application Terminates:** The application completes its shutdown process and terminates.

## Relationship Between Environments and Tenants

The relationship between environments and tenants in CSP is foundational to maintaining a secure, compliant, and well-managed platform. Tenants provide the necessary isolation for users and their data, while environments ensure that different stages of application deployment are segregated and managed effectively. 

### What is an Environment?

An environment refers to the setting or context in which applications and services run. Environments can vary depending on their purpose, for example, development, testing, staging, or production. Each environment is isolated from the others; changes or issues in one environment do not affect the others.

### Types of Environments

* **Development Environment:** Used for building and developing applications.

* **Testing Environment:** Used for testing applications to identify and fix bugs.

* **Staging Environment:** Used for final testing before deployment to production.

* **Production Environment:** Used for live applications accessible by end-users.

### Differences between Tenants and Environments

Understanding the differences between tenants and environments is important for managing and optimizing your use of the CSP platform. Although both concepts play important roles in resource management and application lifecycle, they serve different purposes within the system.

|  | Environments | Tenants |
| :---- | :---- | :---- |
| Purpose | Serve different stages of application deployment, such as development, testing, staging, and production. | Provide isolation between different groups of users and their data, ensuring secure resource and user management. |
| Isolation | Each environment operates independently to ensure that issues in one environment do not affect others. | Each tenant operates independently with its resources, ensuring data and interaction isolation |
| Resource allocation | Resources are allocated to environments based on their purpose, ensuring appropriate resources for development, testing, or production. | Resources are allocated specifically for a tenant, ensuring each tenant has the necessary resources for its users. |
| Security | Enhances security by isolating various stages of application development and deployment.  | Ensure data and user isolation, helping meet compliance requirements by containing sensitive data within a tenant. |

### Mapping Tenants to Environments

Mapping tenants to environments involves linking specific tenants to particular environments within CSP. This mapping ensures that each environment operates within the context of a designated tenant, providing a structured and efficient framework for development and deployment processes.

### How Tenants are Linked to Specific Environments

In CSP, tenants and environments are linked through configuration settings during the setup of each environment. Here's how this is typically done:

1. **Tenant Identification:** Each tenant is assigned a unique identifier. This identifier is used to specify the tenant within configuration files and during the initialization of applications.

2. **Environment Configuration:** Each environment (development, testing, staging, production) is configured to use the tenant identifier. This configuration ensures that the environment operates within the context of the specified tenant.

3. **Initialization:** During the startup of an application, the CSP initialization process includes the tenant identifier as part of the setup. This links the application to the specific tenant and its associated environment.

### Example Configuration

```
const csp::common::String TenantID = "MY_CSP_TENANT";
const csp::common::String EndpointRootURI = "https://ogs-ostage.magnoboard.com";
csp::CSPFoundation::Initialise(EndpointRootURI, TenantID); |
```

In this example, TenantID specifies the tenant, and the initialization function links the application to this tenant within the given environment.

## Summary

A tenant is a dedicated instance that isolates users and their data to ensure security, compliance, and efficient resource management. Requesting a tenant involves filling out an online form. Tenant IDs are provided according to Magnopus' internal release schedule, typically every two weeks.

Directing CSP to a custom tenant involves specifying the tenant during the initialization of your application. This step ensures that your application operates within the context of your dedicated tenant, providing isolation and customized settings that cater to your specific needs.

Environments within a tenant, such as development, testing, staging, and production, are distinct backends that help manage different stages of the application lifecycle. Mapping tenants to these environments allows for efficient resource management, data isolation, and enhanced security and compliance.
