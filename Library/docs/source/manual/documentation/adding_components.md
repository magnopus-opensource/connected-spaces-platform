# Adding Components to CSP
A common practice when developing in CSP is adding new Component classes to the library. This page details this process so that all the relevant steps are tackled and the new feature contains all the functionality expected of a new component.

## Add a new Component class
The first thing to do will be adding the new class files. The header files are to be added to `Library/include/CSP/Multiplayer/Components/`, while CPP files should go in `Library/src/Multiplayer/Components/`.

Add your component code, using the other components as examples.

**MyComponent.h**
```cpp
enum class MyPropertyKeys
{
	ImageURL,
	Num
};

class CSP_API MyComponent : public ComponentBase
{
public:
	MyComponent(SpaceEntity* Parent);
	const csp::common::String& GetImageURL() const;
	void SetImageURL(const csp::common::String& Value);
};
```

**MyComponent.cpp**
```cpp
MyComponent::MyComponent(SpaceEntity* Parent) : ComponentBase(ComponentType::MyComponent, Parent)
{
	Properties[static_cast<uint32_t>(MyPropertyKeys::ImageURL)]				= "";

	SetScriptInterface(CSP_NEW MyComponentScriptInterface(this));
}

const csp::common::String& MyComponent::GetImageURL() const
{
	return GetStringProperty(static_cast<uint32_t>(MyPropertyKeys::ImageURL));
}

void MyComponent::SetImageURL(const csp::common::String& Value)
{
	SetProperty(static_cast<uint32_t>(MyPropertyKeys::ImageURL), Value);
}
```

## Add a ComponentType entry 
In `Library/include/CSP/Multiplayer/ComponentBase.h`, add an enum entry to the `ComponentType` enum, with your component name, ensuring it is added to the end of the enum, and not inserted!

```cpp
enum class ComponentType
{
	Invalid,
        MyComponent
}
```

## Add a component instantiation case
In `Library/src/Multiplayer/SpaceEntity.cpp`, in `InstantiateComponent`, add a case for your component type, so that it can be instantiated correctly.

```cpp
case ComponentType::MyComponent:
	Component = CSP_NEW MyComponent(this);
	break;
```

## Script Binding
This part is more involved, but very important to be done correctly.

First create a script interface header and cpp file both in this folder: `Library/src/Multiplayer/Script/ComponentBinding/`
The header file should contain declarations of script properties, using the macro you see in other interface files. The CPP follows a similar process. Use the existing script interfaces as an example.

**MyComponentScriptInterface.h**
```cpp
class MyComponentScriptInterface : public ComponentScriptInterface
{
public:
	MyComponentScriptInterface(MyComponent* InComponent = nullptr);

	DECLARE_SCRIPT_PROPERTY(std::string, ImageURL);
}
```
**MyComponentScriptInterface.cpp**
```cpp
MyComponentScriptInterface::MyComponentScriptInterface(MyComponent* InComponent) : ComponentScriptInterface(InComponent)
{
}

DEFINE_SCRIPT_PROPERTY_STRING(MyComponent, ImageURL);
```

Secondly, within `Library/src/Multiplayer/Script/EntityScriptBinding.cpp`, in the `Bind` and `BindComponents` functions, ensure that handling of your new component is added. This gives scripts access to properties and functions of the component.

```js
// BindComponents
    Module->class_<MyComponentScriptInterface>("MyComponent")
		.constructor<>()
		.base<ComponentScriptInterface>()
		.PROPERTY_GET_SET(MyComponent, ImageURL, "imageURL")

// Bind
.fun<&EntityScriptInterface::GetComponentsOfType<MyComponentScriptInterface, ComponentType::MyComponent>>("getMyComponents")
```

## Component Testing
New components need a new unit test file, in the following folder `Tests/src/PublicAPITests/ComponentTests/`, use the existing files as examples for contents. Typically components are only expected to test that their fields set and get correctly, though any additional functionality should be tested also, especially if there are systems that tie in to the component itself.