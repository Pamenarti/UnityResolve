> [!NOTE]\
> If you have new feature suggestions or encounter bugs, please submit them as issues. Additionally, you are encouraged to modify the code yourself and contribute your changes back to the repository. This collaborative approach not only helps improve the project but also allows you to gain valuable experience in coding and version control.
> > Examples of projects:
> > - [Phasmophobia Cheat](https://github.com/issuimo/PhasmophobiaCheat/tree/main)
> > - [Sausage Man](https://github.com/issuimo/SausageManCheat)

> If your compiler supports it, please enable the SEH (Structured Exception Handling) option. This feature is crucial for managing exceptions and errors that may occur during the execution of your program, particularly in environments where stability is paramount. Enabling SEH can help in gracefully handling crashes and providing a more robust user experience. \
> For potential issues related to crashes in higher version Android programs, please refer to the link [link](https://github.com/issuimo/UnityResolve.hpp/issues/11). This resource provides insights into common pitfalls and troubleshooting steps that can be taken to mitigate these issues. \
<hr>
<h3 align="center">Brief Overview</h3>
<hr>

# UnityResolve.hpp

> - [X] Windows
> - [X] Android
> - [X] Linux
> - [X] IOS
> - [X] HarmonyOS
> ### Type
> - [X] Camera
> - [X] Transform
> - [X] Component
> - [X] Object (Unity)
> - [X] LayerMask
> - [X] Rigidbody
> - [x] MonoBehaviour
> - [x] Renderer
> - [x] Mesh
> - [X] Behaviour
> - [X] Physics
> - [X] GameObject
> - [X] Collider
> - [X] Vector4
> - [X] Vector3
> - [X] Vector2
> - [X] Quaternion
> - [X] Bounds
> - [X] Plane
> - [X] Ray
> - [X] Rect
> - [X] Color
> - [X] Matrix4x4
> - [X] Array
> - [x] String
> - [x] Object (C#)
> - [X] Type (C#)
> - [X] List
> - [X] Dictionary
> - [X] Animator
> - [X] CapsuleCollider
> - [X] BoxCollider
> - [X] Time
> - [X] FieldInfo
> - More...
> ### Function
> - [X] Mono Injection
> - [X] DumpToFile
> - [X] Thread Attach / Detach
> - [X] Modifying the value of a static variable
> - [X] Obtaining an instance
> - [X] Create C# String
> - [X] Create C# Array
> - [X] Create C# instance
> - [X] WorldToScreenPoint/ScreenToWorldPoint
> - [X] Get the name of the inherited subclass
> - [X] Get the function address (variable offset) and invoke (modify/get)
> - [x] Get GameObject component
> - More...
<hr>
<h3 align="center">How to use</h3>
<hr>

#### GLM (use glm)
[GLM](https://github.com/g-truc/glm)
> ``` C++
> // This directive enables the use of the GLM library, which is a header-only C++ mathematics library for graphics software based on the OpenGL Shading Language (GLSL) specifications.
> #define USE_GLM
> #include "UnityResolve.hpp"
> ```

#### Change platform
> ``` c++
> // These preprocessor directives define the target platform for the application. 
> // Setting WINDOWS_MODE to 1 indicates that the application will run on Windows, while ANDROID_MODE and LINUX_MODE are set to 0, indicating those platforms are not targeted.
> #define WINDOWS_MODE 1 
> #define ANDROID_MODE 0
> #define LINUX_MODE 0
> ```

#### Initialization
> ``` c++
> // The following code initializes the UnityResolve library by loading the necessary DLLs based on the platform.
> // For Windows, it retrieves the handle of the GameAssembly.dll or mono.dll using GetModuleHandle.
> UnityResolve::Init(GetModuleHandle(L"GameAssembly.dll | mono.dll"), UnityResolve::Mode::Mono);
> 
> // For Linux or Android, it uses dlopen to dynamically load the GameAssembly.so or mono.so libraries.
> UnityResolve::Init(dlopen(L"GameAssembly.so | mono.so", RTLD_NOW), UnityResolve::Mode::Mono);
> ```
> // Parameter 1: DLL handle - This is a reference to the loaded dynamic link library.
> // Parameter 2: Usage mode - This specifies the mode in which the library will operate.
> // - Mode::Il2cpp: This mode is used for applications that utilize the IL2CPP scripting backend.
> // - Mode::Mono: This mode is used for applications that utilize the Mono scripting backend.

#### Thread Attach / Detach
> ``` c++
> // This function attaches the current thread to the C# garbage collector (GC). 
> // It is essential for ensuring that the managed objects created in this thread are properly tracked by the GC.
> UnityResolve::ThreadAttach();
> 
> // This function detaches the current thread from the C# garbage collector (GC). 
> // It is important to call this when the thread is no longer interacting with managed objects to prevent memory leaks and ensure proper garbage collection.
> UnityResolve::ThreadDetach();
> ```

# Start of Selection
#### Mono (Mono Inject)
> ``` c++
> // The following code demonstrates how to load a Mono assembly and invoke a specific method within it. 
> // The first line loads the assembly from the specified path, which is essential for accessing its types and methods.
> UnityResolve::AssemblyLoad assembly("./MonoCsharp.dll");
> 
> // The second line not only loads the assembly but also specifies the class and method to be invoked. 
> // This is particularly useful for injecting functionality into existing Mono behaviors, allowing for dynamic method execution.
> UnityResolve::AssemblyLoad assembly("./MonoCsharp.dll", "MonoCsharp", "Inject", "MonoCsharp.Inject:Load()");
> ```
# End of Selection

#### Get the function address (variable offset) and invoke (modify/get)
>  This section of code demonstrates how to retrieve the address of a function within a specified assembly, 
>  access its fields, and invoke methods dynamically. 

const auto assembly = UnityResolve::Get("assembly.dll | assembly name.dll"); // Retrieve the assembly by its name.
const auto pClass   = assembly->Get("className | class name"); // Get the class from the assembly.
>  The following commented lines provide alternative ways to retrieve the class, 
>  either by using a wildcard or specifying a namespace.
>  assembly->Get("className | class name", "*");
> assembly->Get("className | class name", "namespace | namespace");

const auto field       = pClass->Get<UnityResolve::Field>("Field Name | variable name"); // Access a specific field in the class.
const auto fieldOffset = pClass->Get<std::int32_t>("Field Name | variable name"); // Get the offset of the field within the class.
const int  time        = pClass->GetValue<int>(obj Instance | object address, "time"); // Retrieve the value of the 'time' field from an instance of the class.
>  The following commented line shows an alternative way to get the value from an instance.
>  pClass->GetValue(obj Instance*, name);
                        = pClass->SetValue<int>(obj Instance | object address, "time", 114514); // Set a new value for the 'time' field.
>  The following commented line shows an alternative way to set the value in an instance.
>  pClass->SetValue(obj Instance*, name, value);
> const auto method      = pClass->Get<UnityResolve::Method>("Method Name | function name"); // Retrieve a method from the class.
>  The following commented lines provide examples of how to retrieve methods with different parameter types.
>  pClass->Get<UnityResolve::Method>("Method Name | function name", { "System.String" });
>  pClass->Get<UnityResolve::Method>("Method Name | function name", { "*", "System.String" });
>  pClass->Get<UnityResolve::Method>("Method Name | function name", { "*", "", "System.String" });
>  pClass->Get<UnityResolve::Method>("Method Name | function name", { "*", "System.Int32", "System.String" });
>  pClass->Get<UnityResolve::Method>("Method Name | function name", { "*", "System.Int32", "System.String", "*" });
>  "*" == "" indicates that any type can be accepted as a parameter.
>  const auto functionPtr = method->function;

>  The following code retrieves two methods from the specified class within the assembly. 
>  The 'Get' function is used to access methods by their names, which allows for dynamic invocation later on. 
>  'method1' and 'method2' are placeholders for the actual method names that need to be specified.
>  This approach is essential for scenarios where methods need to be called at runtime, 
>  enabling flexibility and extensibility in the codebase.
>  const auto method1 = pClass->Get<UnityResolve::Method>("method name1 | function name 1");
>  const auto method2 = pClass->Get<UnityResolve::Method>("method name2 | function name 2");


>  The following code demonstrates how to invoke a method dynamically using the UnityResolve framework. 
>  The method1 is invoked with specific parameters, where the return type is specified as int. 
>  method1->Invoke<int>(114, 514, "114514"); 
>  This line illustrates the syntax for invoking a method with a specified return type and arguments.

>  The next section shows how to cast a method to a specific function pointer type. 
>  Here, method2 is cast to a MethodPointer that takes two parameters: an int and a bool, and returns void. 
>  const UnityResolve::MethodPointer<void, int, bool> ptr = method2->Cast<void, int, bool>(); 
>  The casted pointer is then called with the provided arguments.
>  ptr(114514, true);

>  This part demonstrates how to create a MethodPointer for method1 and assign it to a variable named add. 
>  UnityResolve::MethodPointer<void, int, bool> add; 
>  ptr = method1->Cast(add); 

>  A std::function is created to hold a callable that matches the signature of the method. 
>  std::function<void(int, bool)> add2; 
>  The method is then cast to this std::function type, allowing for easier invocation later.
>  method->Cast(add2);

>  The following code snippet illustrates how to work with fields in a class. 
>  A variable of type Vector3 associated with the Player class is initialized. 
>  UnityResolve::Field::Variable<Vector3, Player> syncPos; 
>  syncPos.Init(pClass->Get<UnityResolve::Field>("syncPos")); 
>  The position is then retrieved for a specific player instance.
>  auto pos = syncPos[playerInstance]; 
>  Alternatively, the position can be obtained using the Get method.
>  auto pos = syncPos.Get(playerInstance); 

#### Dumping to File (DumpToFile)
> ``` C++
> UnityResolve::DumpToFile("./output/");
> ```
This function allows you to export data to a specified file path. The `DumpToFile` method is part of the UnityResolve framework, which facilitates interaction with Unity's underlying structures. By providing a path, you can save relevant information, such as game state or configuration data, to a file for later analysis or debugging.

#### Creating a C# String (Create C# String)
> ``` c++
> const auto str     = UnityResolve::UnityType::String::New("string | string");
> std::string cppStr = str.ToString();
> ```
> In this snippet, a new C# string is created using the `UnityType::String::New` method. The string can be initialized with any valid string value. After creation, the `ToString` method is called to convert the C# string into a standard C++ string (`std::string`). This conversion is essential for > interoperability between C# and C++ code, allowing you to manipulate the string using C++ standard library functions.

#### Create C# Array
> ``` c++
> // The following code demonstrates how to create a C# array using the UnityResolve framework.
> // First, we retrieve the assembly that contains the desired class by specifying its name.
> const auto assembly = UnityResolve::Get("assembly.dll | assembly name.dll");
> // Next, we obtain a reference to the class we want to work with.
> const auto pClass   = assembly->Get("className | class name");
> // We then create a new C# array of type T, specifying the class reference and the desired size of the array.
> const auto array    = UnityResolve::UnityType::Array<T>::New(pClass, size);
> // Finally, we convert the C# array to a C++ vector for easier manipulation in C++.
> std::vector<T> cppVector = array.ToVector();
> ```

#### Creating a C# Instance
> ``` c++
> // First, we retrieve the assembly that contains the desired class by specifying its name.
> const auto assembly = UnityResolve::Get("assembly.dll | assembly name.dll");
> // Next, we obtain a reference to the class we want to instantiate.
> const auto pClass   = assembly->Get("className | class name");
> // We then create a new instance of the Game class using the New method.
> const auto pGame    = pClass->New<Game*>();
> ```

#### Obtaining an Instance
> ``` c++
> // Again, we start by retrieving the assembly that contains the class.
> const auto assembly = UnityResolve::Get("assembly.dll | assembly name.dll");
> // We obtain a reference to the class we want to work with.
> const auto pClass   = assembly->Get("className | class name");
> // We then find all objects of type Player in the scene and store them in a vector.
> std::vector<Player*> playerVector = pClass->FindObjectsByType<Player*>();
> // The FindObjectsByType method returns a vector of pointers to Player objects.
> // We can check the number of Player instances found using the size method.
> playerVector.size();
> ```

#### WorldToScreenPoint/ScreenToWorldPoint
> ``` c++
> // This code snippet demonstrates how to convert a 3D world point to a 2D screen point and vice versa using the UnityResolve framework.
> // First, we obtain a reference to the main camera in the scene.
> Camera* pCamera = UnityResolve::UnityType::Camera::GetMain();
> 
> // We then define a 3D point in the world space. The Vector3 class is used to represent 3D coordinates.
> Vector3 worldPoint = Vector3(x, y, z); // Replace x, y, z with actual coordinates.
> 
> // The WorldToScreenPoint method converts the 3D world point into a 2D screen point, taking into account the camera's perspective.
> Vector3 screenPoint = pCamera->WorldToScreenPoint(worldPoint, Eye::Left);
> 
> // Conversely, the ScreenToWorldPoint method converts the 2D screen point back into a 3D world point.
> Vector3 convertedWorldPoint = pCamera->ScreenToWorldPoint(screenPoint, Eye::Left);
> ```

#### Get the name of the inherited subclass
> ``` c++
> // First, we retrieve the assembly that contains the MonoBehaviour class.
> const auto assembly = UnityResolve::Get("UnityEngine.CoreModule.dll");
> // Next, we obtain a reference to the MonoBehaviour class.
> const auto pClass   = assembly->Get("MonoBehaviour");
> // We then find the first instance of the Parent class in the scene.
> Parent* pParent     = pClass->FindObjectsByType<Parent*>()[0];
> // Finally, we retrieve the full name of the type of the Parent instance.
> std::string child   = pParent->GetType()->GetFullName();
> ```

#### Get GameObject component
> ``` c++
> // This code snippet demonstrates how to retrieve components from a GameObject in Unity using the UnityResolve framework.
> // The GetComponents method retrieves all components of a specified type from the GameObject.
> std::vector<T*> objs = gameobj->GetComponents<T*>(UnityResolve::Get("assembly.dll")->Get("class"));
>                     // gameobj->GetComponents<return type>(Class* component)
> 
> // The GetComponentsInChildren method retrieves components of a specified type from the GameObject and its children.
> std::vector<T*> objs = gameobj->GetComponentsInChildren<T*>(UnityResolve::Get("assembly.dll")->Get("class"));
>                     // gameobj->GetComponentsInChildren<return type>(Class* component)
> 
> // The GetComponentsInParent method retrieves components of a specified type from the GameObject and its parents.
> std::vector<T*> objs = gameobj->GetComponentsInParent<T*>(UnityResolve::Get("assembly.dll")->Get("class"));
>                     // gameobj->GetComponentsInParent<return type>(Class* component)
> ```

* This project comprehensively performs object finding, component retrieval, and conversion between world and screen coordinates using the UnityResolve framework in the Unity game engine. 
* The project offers various methods to determine the types of objects derived from the MonoBehaviour class, as well as effective ways to retrieve GameObject components. 
* The methods used to convert 3D world points to 2D screen points ensure that objects in the scene are displayed correctly and enhance the user experience. 
* Additionally, the FindObjectsByType method used to locate objects in the scene simplifies object management and provides developers with greater control over the objects in the scene.
