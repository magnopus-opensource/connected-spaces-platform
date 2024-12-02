# GraphQL

GraphQL is an open-source data query and manipulation language for APIs and a runtime for executing queries. GraphQL was developed internally by Meta in 2012 before being publicly released in 2015\. It allows the client's application to request specific data from a database. 

Unlike REST, where each API endpoint must return a fixed structure, GraphQL allows clients to define the structure of the response. GraphQL is designed to make APIs fast, flexible, and developer-friendly.

## Key Features

* **Query Language:** GraphQL uses a query language that allows clients to specify the data they need. This reduces over-fetching or under-fetching of data.

* **Types:** GraphQL types define the data structure and ensure that queries and responses follow a consistent format. They include various kinds: **scalar types, object types, enum type**s, and **input types.** 

* **Schema:** A GraphQL schema defines the data types that can be queried and how they are connected. This schema has strict rules, so queries are checked to ensure they are correct before executing. 

* **Resolvers:** Resolvers are functions that retrieve data for a particular field in a query or mutation. It executes the request sent from the client application and fetches data from the database based on the type of operation specified.

Compared to REST, GraphQL offers more flexibility by allowing clients to request only the specific data they need in a single request, reducing the number of requests and the amount of data transferred.

## Core Concepts

### Schema Definition

A GraphQL schema is a contract that defines the capabilities of the GraphQL server. It specifies the types of data available and the queries and mutations that can be performed on that data.

The schema is defined using the GraphQL Schema Definition Language (SDL) and includes types, fields, and operations (queries, mutations, and subscriptions).

### Scalar, Object, Custom, and Input Types

#### Scalar Types

Scalar Types are basic data types that represent a single value. GraphQL includes built-in scalar types such as:

*  `Int`: A signed 32-bit integer.

*  `Float`: A signed double-precision floating-point value.

*  `String`: A UTF-8 character sequence.

*  `Boolean`: A true or false value.

*  `ID`: A unique identifier, typically used for fetching objects or as keys for caches.

#### Object Types

**Object Types** are complex types that represent objects with multiple fields. Each field can have its own type, which can be a scalar type or another object type.

```text
type User {
	id: ID!
	name: String!
	email: String!
}
```

#### Custom Types

**Custom Types** are types defined by the user to represent complex structures. These can include:

* **Enums:** They are a special kind of scalar that restricts a field to a specified set of values.

```text
enum Role {
	ADMIN
	USER
	GUEST
}
```

#### Input Types 

**Input Types** are used to define complex arguments for queries and mutations. They cannot be used as output types.

```text
input UserInput {
	name: String!
	email: String!
}
```

Please Note: 

Output types in GraphQL are types that can be used in the response of a query or mutation. They define the structure of the data that will be returned to the client. Output types include:

* **Object Types**  
* **Scalar Types**  
* **Custom Types**

## Queries and Mutations

Queries are used to fetch data from the server. They specify what data is required and the structure of the response.

```text
query {
	user(id: "1") {
		id
		name
		email
	}
}
```

Queries can also include arguments to filter or modify the data returned.

```text
query getUser($id: ID!) {
	user(id: $id) {
		id
		name
		email
	}
}
```

### Mutations

Mutations are used to modify data on the server. They have a similar structure to queries but represent operations that change data (e.g., creating, updating, deleting).

```text
mutation {
	createUser(input: { name: "John", email: "john@example.com" }) {
		id
		name
		email
	}
}
```

Mutations can also accept arguments and return specific fields from the modified data.

```text
mutation createUser($input: UserInput!) {
	createUser(input: $input) {
		id
		name
		email
	}
}
```

### Differences Between Queries and Mutations

* Purpose: Queries are for reading data (fetching information), while mutations are for writing data (modifying information).

* Execution: Queries are typically idempotent (repeated executions yield the same result without side effects), whereas mutations are not (they cause changes to the state of the data).

* Keywords: Queries use the keyword `query`, while mutations use the keyword `mutation`.

## Performing Complex Queries with GraphQL

GraphQL is a query language that can handle complex queries that traditional REST APIs cannot. For many applications, GraphQL's flexibility and efficiency are essential because it enables the retrieval of the exact data needed in a single request, thereby reducing network traffic and improving performance. 

Here are some of the methods to perform complex queries in GraphQL.

### Nested Queries

Nested queries in GraphQL allow you to retrieve related data within a single query. This means you can fetch an object and its related objects in one request. For example, in CSP, nested queries can be used to fetch a space and its associated users and assets.

```text
{
space(id: "101") {
	name
	users {
		name
		role
	}
	assets {
		title
		type
	}
}
}
```

### Use Cases

* **Reducing Network Requests:** Instead of making multiple requests to fetch related data, nested queries allow retrieval of all necessary information with a single request to the API.

* **Improving Performance:** By minimizing the number of requests, nested queries can enhance the performance of the application.

* **Simplifying Code:** Nested queries reduce the complexity of client-side code, as there is no need to handle multiple asynchronous requests.

## Aliases and Fragments

Aliases in GraphQL allow you to rename the result of a field to avoid conflicts and make the response more readable. This is particularly useful when querying the same field multiple times with different arguments.

Here is a syntax of Aliases:

```text
{
	admin: user(id: "1") {
		name
	}
	guest: user(id: "2") {
		name
	}
}
```

In this example, two different users are queried, and aliases are used to distinguish them in the response.

### Simplify Queries with Fragments

In GraphQL, fragments are reusable query components. They eliminate duplication by allowing related fields to be defined in a fragment and reused across multiple queries.

Here is an example of a Fragment: 

```text
fragment userFields on User {
	id
	name
	email
	role
}

{
	admin: user(id: "1") {
		...userFields
	}
	guest: user(id: "2") {
		...userFields
	}
}
```

In this example, the `userFields` fragment is defined once and reused for both `admin` and `guest` queries, making the query simpler.

### Benefits of Fragments

* **Code Reusability:** Fragments allow you to define commonly used fields once and reuse them across multiple queries, reducing code duplication.

* **Maintainability:** Updating the fields in one place (the fragment) automatically updates all queries using that fragment, making the code easier to maintain.

* **Readability:** Fragments can make queries more readable by breaking down complex queries into smaller, manageable parts.

## Variables and Directives

Variables in GraphQL make queries more flexible and reusable. Instead of hardcoding values, you can use variables to pass values into queries dynamically. 

Here is an example: 

```text
query GetUser($id: ID!) {
	user(id: $id) {
		name
		email
	}
}

{ "id": "1" }
```

In this example, the `$id` variable is used to pass the user ID dynamically.

### Dynamic Query Execution with Directives

Directives in GraphQL control how queries can be executed based on a specific condition. The @include and @skip directives are commonly used for this purpose.

Here is an example: 

```text
	query GetUser($withEmail: Boolean!) {
	user(id: "1") {
		name
		email @include(if: $withEmail)
	}
}

{ "withEmail": true }
```

In this example, the `email` field is included only if `withEmail` is true.

### Benefits of Variables and Directives

* **Flexibility:** Using variables, values can be passed dynamically to make queries adaptable to different use cases.

* **Efficiency:** Directives control how queries are executed by retrieving only the necessary data based on predefined conditions.

* **Reusability:** Variables and directives reduce code duplication, making queries cleaner and more maintainable.

## GraphQL in CSP

GraphQL is considered an advanced feature for developers who have mastered the standard CSP systems. GraphQL plays a unique role in CSP by allowing client applications to create their own specific queries. Unlike other protocols where CSP defines the queries, GraphQL enables clients to specify the data they need. This flexibility is crucial for handling complex data retrieval scenarios that standard CSP APIs may not cover. CSP's GraphQL system is designed to accept query strings from client applications, transmit these queries securely via HTTPS, and return the requested data.

*It is important to note that CSP does not allow client applications to mutate data directly using GraphQL. This restriction ensures that CSP maintains control over the schema and ensures data consistency.*

### Example of a CSP GraphQL Query

In CSP, GraphQL queries can retrieve detailed information across multiple databases. For instance, a client application might query for user details along with their related projects and tasks in a single request. 

Here's an example of a GraphQL query in CSP:

```text
{
	prototype(filters: { groupIds: \["659bf45f5sf871cm7501c71c"\], types: \["Default"\] }) {
		items {
			id
			name
			type
			createdBy
			metadata {
				key
				value
			}
			assets(assetTypes: \["Thumbnail", "Model", "Video", "Image", "Audio",\]) {
				uri
				name
				assetType
				checksum
				fileName
				id
				mimeType
				prototypeId
				version
			}
		}   
	}
}
```

In the example, you can also observe that much of the Connected Spaces Platform domain language is absent. There are no references to spaces or asset collections, but in their place there are references to group IDs and prototypes.

This is an example of another limitation when directly using GraphQL queries. Since the client application is effectively speaking directly with the server when issuing queries, the query schema and terms available to the application can and do vary depending on the services that the developer has elected to use.

### Benefits and Limitations 

**Benefits:**

* **Flexibility:** GraphQL allows client applications to request the exact data needed, reducing over-fetching and under-fetching.

* **Efficiency:** Multiple related pieces of data can be retrieved in a single request, minimizing the number of API calls.

* **Specificity:** Developers can tailor queries to their exact needs, enabling more refined data retrieval.

**Limitations:**

* **Complexity:** Using GraphQL requires a deep understanding of the schema and the specific terms defined at the service level.

* **Responsibility:** Developers must create and manage their queries, which can be more challenging than standard CSP APIs.

* **No Data Mutation:** CSP restricts data mutations via GraphQL to maintain control over the schema and ensure data consistency.

* **No CSP Domain Language:** Since queries are issued directly to the server, the client application must reason about server-level domain language, which may differ from CSP-level domain language.

## Summary

* **GraphQL** is an open-source data query and manipulation language for APIs and a runtime for executing queries, developed by Meta. It allows clients to request specific data, offering flexibility and efficiency over traditional REST APIs.

**Key Characteristics of GraphQL**

* **Query Language**: Clients specify the exact data they need.  
* **Types**: Defines data structures, including scalar types, object types, enum types, and input types.  
* **Schema**: Defines data types and their relationships, ensuring query validation.  
* **Resolvers**: Functions that provide the logic for fetching and modifying data.

### Core Concepts of GraphQL

**Schema and Types**

* **Schema**: Defines the capabilities of the GraphQL server using the Schema Definition Language (SDL).

* **Types**:  
  * **Scalar Types**: Basic data types like `Int`, `Float`, `String`, `Boolean`, and `ID`.  
  * **Object Types**: Complex types with multiple fields.  
  * **Custom Types**: Enums and input types for complex structures.

**Queries and Mutations**

**Queries**: Fetch data from the server.

```text
query {
	user(id: "1") {
		id
		name
		email
	}
}
```

**Mutations**: Modify data on the server.

```text
mutation {
	createUser(input: { name: "John", email: "john@example.com" }) {
		id
		name
		email
	}
}
```

* **Differences**: Queries read data; mutations write data. Queries are idempotent; mutations are not.

### Complex Queries in GraphQL

* **Nested Queries**: Retrieve related data in a single query.  
* **Aliases and Fragments**: Aliases rename fields; fragments allow reusable query components.  
* **Variables and Directives**: Variables make queries flexible; directives control query execution based on conditions.

### GraphQL in CSP

* **Benefits**: Flexibility, efficiency, and specificity in data retrieval.  
* **Limitations**: Complexity, developer responsibility, loss of CSP domain language, and no data mutations to ensure schema control and data consistency.
