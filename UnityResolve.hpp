#ifndef UNITYRESOLVE_HPP
#define UNITYRESOLVE_HPP

// ============================== Automatic detection of the current environment ==============================

#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS_MODE 1 // Windows mode is enabled
#else
#define WINDOWS_MODE 0 // Windows mode is disabled
#endif

#if defined(__ANDROID__)
#define ANDROID_MODE 1 // Android mode is enabled
#else
#define ANDROID_MODE 0 // Android mode is disabled
#endif

#if defined(TARGET_OS_IOS)
#define IOS_MODE 1 // iOS mode is enabled
#else
#define IOS_MODE 0 // iOS mode is disabled
#endif

#if defined(__linux__) && !defined(__ANDROID__)
#define LINUX_MODE 1 // Linux mode is enabled
#else
#define LINUX_MODE 0 // Linux mode is disabled
#endif

#if defined(__harmony__) && !defined(_HARMONYOS)
#define HARMONYOS_MODE 1 // HarmonyOS mode is enabled
#else
#define HARMONYOS_MODE 0 // HarmonyOS mode is disabled
#endif

// ============================== Force set the current execution environment ==============================

// #define WINDOWS_MODE 0
// #define ANDROID_MODE 1 // Set the running environment
// #define LINUX_MODE 0
// #define IOS_MODE 0
// #define HARMONYOS_MODE 0

// ============================== Import corresponding environment dependencies ==============================

#if WINDOWS_MODE || LINUX_MODE || IOS_MODE
#include <format> // Include format library
#include <ranges> // Include ranges library
#include <regex>  // Include regex library
#endif

#if IOS_MODE
#include <algorithm> // Include algorithm library for iOS
#endif

#include <codecvt> // Include codecvt library
#include <fstream> // Include fstream library
#include <iostream> // Include iostream library
#include <mutex> // Include mutex library
#include <string> // Include string library
#include <unordered_map> // Include unordered_map library
#include <vector> // Include vector library
#include <functional> // Include functional library
#include <numbers> // Include numbers library

#ifdef USE_GLM
#include <glm/glm.hpp> // Include GLM library if USE_GLM is defined
#endif

#if WINDOWS_MODE
#include <windows.h> // Include Windows header
#undef GetObject // Undefine GetObject to avoid conflicts
#endif

#if WINDOWS_MODE
#ifdef _WIN64
#define UNITY_CALLING_CONVENTION __fastcall // Define calling convention for 64-bit Windows
#elif _WIN32
#define UNITY_CALLING_CONVENTION __cdecl // Define calling convention for 32-bit Windows
#endif
#elif ANDROID_MODE || LINUX_MODE || IOS_MODE || HARMONYOS_MODE
#include <locale> // Include locale library
#include <dlfcn.h> // Include dlfcn library for dynamic linking
#define UNITY_CALLING_CONVENTION // No specific calling convention
#endif

class UnityResolve final {
public:
	struct Assembly;
	struct Type;
	struct Class;
	struct Field;
	struct Method;
	class UnityType;

	enum class Mode : char {
		Il2Cpp, // Il2Cpp mode
		Mono,   // Mono mode
	};

	struct Assembly final {
		void* address; // Address of the assembly
		std::string name; // Name of the assembly
		std::string file; // File associated with the assembly
		std::vector<Class*> classes; // List of classes in the assembly

		[[nodiscard]] auto Get(const std::string& strClass, const std::string& strNamespace = "*", const std::string& strParent = "*") const -> Class* {
			// Retrieve a class based on its name, namespace, and parent
			for (const auto pClass : classes) 
				if (strClass == pClass->name && (strNamespace == "*" || pClass->namespaze == strNamespace) && (strParent == "*" || pClass->parent == strParent)) 
					return pClass;
			return nullptr; // Return nullptr if not found
		}
	};

	struct Type final {
		void* address; // Address of the type
		std::string name; // Name of the type
		int size; // Size of the type

		// UnityType::CsType*
		[[nodiscard]] auto GetCSType() const -> void* {
			// Get the corresponding C# type object
			if (mode_ == Mode::Il2Cpp) 
				return Invoke<void*>("il2cpp_type_get_object", address);
			return Invoke<void*>("mono_type_get_object", pDomain, address);
		}
	};

	struct Class final {
		void* address; // Address of the class
		std::string name; // Name of the class
		std::string parent; // Parent class name
		std::string namespaze; // Namespace of the class
		std::vector<Field*> fields; // List of fields in the class
		std::vector<Method*> methods; // List of methods in the class
		void* objType; // Object type

		template <typename RType>
		auto Get(const std::string& name, const std::vector<std::string>& args = {}) -> RType* {
			// Retrieve a field or method based on its name and arguments
			if constexpr (std::is_same_v<RType, Field>) 
				for (auto pField : fields) 
					if (pField->name == name) 
						return static_cast<RType*>(pField);
			if constexpr (std::is_same_v<RType, std::int32_t>) 
				for (const auto pField : fields) 
					if (pField->name == name) 
						return reinterpret_cast<RType*>(pField->offset);
			if constexpr (std::is_same_v<RType, Method>) {
				for (auto pMethod : methods) {
					if (pMethod->name == name) {
						if (pMethod->args.empty() && args.empty()) 
							return static_cast<RType*>(pMethod);
						if (pMethod->args.size() == args.size()) {
							size_t index{ 0 };
							for (size_t i{ 0 }; const auto & typeName : args) 
								if (typeName == "*" || typeName.empty() ? true : pMethod->args[i++]->pType->name == typeName) 
									index++;
							if (index == pMethod->args.size()) 
								return static_cast<RType*>(pMethod);
						}
					}
				}

				for (auto pMethod : methods) 
					if (pMethod->name == name) 
						return static_cast<RType*>(pMethod);
			}
			return nullptr; // Return nullptr if not found
		}

		template <typename RType>
		auto GetValue(void* obj, const std::string& name) -> RType { 
			// Get the value of a field by its name
			return *reinterpret_cast<RType*>(reinterpret_cast<uintptr_t>(obj) + Get<Field>(name)->offset); 
		}

		template <typename RType>
		auto GetValue(void* obj, unsigned int offset) -> RType { 
			// Get the value of a field by its offset
			return *reinterpret_cast<RType*>(reinterpret_cast<uintptr_t>(obj) + offset); 
		}

		template <typename RType>
		auto SetValue(void* obj, const std::string& name, RType value) -> void { 
			// Set the value of a field by its name
			*reinterpret_cast<RType*>(reinterpret_cast<uintptr_t>(obj) + Get<Field>(name)->offset) = value; 
		}

		template <typename RType>
		auto SetValue(void* obj, unsigned int offset, RType value) -> RType { 
			// Set the value of a field by its offset
			*reinterpret_cast<RType*>(reinterpret_cast<uintptr_t>(obj) + offset) = value; 
		}

		// UnityType::CsType*
		[[nodiscard]] auto GetType() -> void* {
			// Get the type of the object
			if (objType) 
				return objType;
			if (mode_ == Mode::Il2Cpp) {
				const auto pUType = Invoke<void*, void*>("il2cpp_class_get_type", address);
				objType = Invoke<void*>("il2cpp_type_get_object", pUType);
				return objType;
			}
			const auto pUType = Invoke<void*, void*>("mono_class_get_type", address);
			objType = Invoke<void*>("mono_type_get_object", pDomain, pUType);
			return objType;
		}

		/**
		 * \brief Get all instances of the class
		 * \tparam T Return array type
		 * \param type Class type
		 * \return Returns an array of instance pointers
		 */
		template <typename T>
		auto FindObjectsByType() -> std::vector<T> {
			static Method* pMethod;

			if (!pMethod) 
				pMethod = UnityResolve::Get("UnityEngine.CoreModule.dll")->Get("Object")->Get<Method>("FindObjectsOfType", { "System.Type" });
			if (!objType) 
				objType = this->GetType();

			if (pMethod && objType) 
				if (auto array = pMethod->Invoke<UnityType::Array<T>*>(objType)) 
					return array->ToVector();

			return std::vector<T>(0); // Return an empty vector if no instances found
		}

		template <typename T>
		auto New() -> T* {
			// Create a new instance of the type
			if (mode_ == Mode::Il2Cpp) 
				return Invoke<T*, void*>("il2cpp_object_new", address);
			return Invoke<T*, void*, void*>("mono_object_new", pDomain, address);
		}
	};

	struct Field final {
		void* address; // Address of the field
		std::string name; // Name of the field
		Type* type; // Type of the field
		Class* klass; // Class of the field
		std::int32_t offset; // Offset of the field; -1 indicates thread static
		bool static_field; // Indicates if the field is static
		void* vTable; // Virtual table pointer

		template <typename T>
		auto SetStaticValue(T* value) const -> void {
			// Set the static value of the field
			if (!static_field) return; // Do nothing if not a static field
			if (mode_ == Mode::Il2Cpp) 
				return Invoke<void, void*, T*>("il2cpp_field_static_set_value", address, value);
			else
			{
				void* VTable = Invoke<void*>("mono_class_vtable", pDomain, klass->address);
				return Invoke<void, void*, void*, T*>("mono_field_static_set_value", VTable, address, value);
			}
		}
		// Function to get the static value of the field
		template <typename T>
		auto GetStaticValue(T* value) const -> void {
			// Check if the field is static; if not, exit the function
			if (!static_field) return; // Do nothing if not a static field
			// If the mode is Il2Cpp, invoke the corresponding method to get the static value
			if (mode_ == Mode::Il2Cpp) 
				return Invoke<void, void*, T*>("il2cpp_field_static_get_value", address, value);
			else
			{
				// For Mono mode, get the virtual table and invoke the method to get the static value
				void* VTable = Invoke<void*>("mono_class_vtable", pDomain, klass->address);
				return Invoke<void, void*, void*, T*>("mono_field_static_get_value", VTable, address, value);
			}
		}

		// Structure to represent a variable with an offset
		template <typename T, typename C>
		struct Variable {
		private:
			std::int32_t offset{ 0 }; // Offset of the variable

		public:
			// Initialize the variable with the field's offset
			void Init(const Field* field) {
				offset = field->offset;
			}

			// Get the value of the variable from the object
			T Get(C* obj) {
				return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(obj) + offset);
			}

			// Set the value of the variable in the object
			void Set(C* obj, T value) {
				*reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(obj) + offset) = value;
			}

			// Overload the operator[] to access the variable using an object
			T& operator[](C* obj) {
				return *reinterpret_cast<T*>(offset + reinterpret_cast<std::uintptr_t>(obj));
			}
		};
	// Define a method pointer type for functions with a specific calling convention
	template <typename Return, typename... Args>
	using MethodPointer = Return(UNITY_CALLING_CONVENTION*)(Args...);

	// Structure to represent a method in the Unity environment
	struct Method final {
		void* address; // Address of the method
		std::string  name; // Name of the method
		Class* klass; // Class that contains the method
		Type* return_type; // Return type of the method
		std::int32_t flags; // Flags associated with the method
		bool         static_function; // Indicates if the method is static
		void* function; // Pointer to the actual function

		// Structure to represent an argument of the method
		struct Arg {
			std::string name; // Name of the argument
			Type* pType; // Type of the argument
		};

		std::vector<Arg*> args; // List of arguments for the method
	public:
		// Invoke the method with the specified arguments
		template <typename Return, typename... Args>
		auto Invoke(Args... args) -> Return {
			Compile(); // Compile the method if necessary
			if (function) return reinterpret_cast<Return(UNITY_CALLING_CONVENTION*)(Args...)>(function)(args...); // Call the function
			return Return(); // Return default value if function is not set
		}

		// Compile the method if it is in Mono mode
		auto Compile() -> void {
			if (address && !function && mode_ == Mode::Mono) function = UnityResolve::Invoke<void*>("mono_compile_method", address);
		}

		// Invoke the method at runtime with the specified object and arguments
		template <typename Return, typename Obj = void, typename... Args>
		auto RuntimeInvoke(Obj* obj, Args... args) -> Return {
			void* argArray[sizeof...(Args)] = { static_cast<void*>(&args)... }; // Create an array of arguments

			if (mode_ == Mode::Il2Cpp) {
				if constexpr (std::is_void_v<Return>) {
					UnityResolve::Invoke<void*>("il2cpp_runtime_invoke", address, obj, sizeof...(Args) ? argArray : nullptr, nullptr); // Invoke for Il2Cpp
					return;
				}
				else return Unbox<Return>(Invoke<void*>("il2cpp_runtime_invoke", address, obj, sizeof...(Args) ? argArray : nullptr, nullptr)); // Return unboxed value
			}

			if constexpr (std::is_void_v<Return>) {
				UnityResolve::Invoke<void*>("mono_runtime_invoke", address, obj, sizeof...(Args) ? argArray : nullptr, nullptr); // Invoke for Mono
				return;
			}
			else return Unbox<Return>(Invoke<void*>("mono_runtime_invoke", address, obj, sizeof...(Args) ? argArray : nullptr, nullptr)); // Return unboxed value
		}

		// Cast the function pointer to a specific method pointer type
		template <typename Return, typename... Args>
		auto Cast() -> MethodPointer<Return, Args...> {
			Compile(); // Compile the method if necessary
			if (function) return reinterpret_cast<MethodPointer<Return, Args...>>(function); // Return the casted function pointer
			return nullptr; // Return nullptr if function is not set
		}

		// Cast the function pointer to a specific method pointer type and store it in the provided pointer
		template <typename Return, typename... Args>
		auto Cast(MethodPointer<Return, Args...>& ptr) -> MethodPointer<Return, Args...> {
			Compile(); // Compile the method if necessary
			if (function) {
				ptr = reinterpret_cast<MethodPointer<Return, Args...>>(function); // Store the casted function pointer
				return reinterpret_cast<MethodPointer<Return, Args...>>(function); // Return the casted function pointer
			}
			return nullptr; // Return nullptr if function is not set
		}

		// Cast the function pointer to a std::function type
		template <typename Return, typename... Args>
		auto Cast(std::function<Return(Args...)>& ptr) -> std::function<Return(Args...)> {
			Compile(); // Compile the method if necessary
			if (function) {
				ptr = reinterpret_cast<MethodPointer<Return, Args...>>(function); // Store the casted function pointer
				return reinterpret_cast<MethodPointer<Return, Args...>>(function); // Return the casted function pointer
			}
			return nullptr; // Return nullptr if function is not set
		}

		// Unbox the object to the specified type
		template <typename T>
		T Unbox(void* obj) {
			if (mode_ == Mode::Il2Cpp) {
				return static_cast<T>(Invoke<void*>("mono_object_unbox", obj)); // Unbox for Il2Cpp
			}
			else {
				return static_cast<T>(Invoke<void*>("il2cpp_object_unbox", obj)); // Unbox for Mono
			}
		}
	};

	// Class to handle assembly loading in the Unity environment
	class AssemblyLoad {
	public:
		// Constructor to load an assembly from the specified path
		AssemblyLoad(const std::string path, std::string namespaze = "", std::string className = "", std::string desc = "") {
			if (mode_ == Mode::Mono) {
				assembly = Invoke<void*>("mono_domain_assembly_open", pDomain, path.data()); // Open the assembly
				image = Invoke<void*>("mono_assembly_get_image", assembly); // Get the assembly image
				if (namespaze.empty() || className.empty() || desc.empty()) {
					return; // Exit if any parameter is empty
				}
				klass = Invoke<void*>("mono_class_from_name", image, namespaze.data(), className.data()); // Get the class from the assembly
				void* entry_point_method_desc = Invoke<void*>("mono_method_desc_new", desc.data(), true); // Create a method description
				method = Invoke<void*>("mono_method_desc_search_in_class", entry_point_method_desc, klass); // Search for the method in the class
				Invoke<void>("mono_method_desc_free", entry_point_method_desc); // Free the method description
				Invoke<void*>("mono_runtime_invoke", method, nullptr, nullptr, nullptr); // Invoke the method
			}
		}

		void* assembly; // Pointer to the loaded assembly
		void* image; // Pointer to the assembly image
		void* klass; // Pointer to the class in the assembly
		void* method; // Pointer to the method in the class
	};

	// Attach the current thread to the Unity environment
	static auto ThreadAttach() -> void {
		if (mode_ == Mode::Il2Cpp) Invoke<void*>("il2cpp_thread_attach", pDomain); // Attach for Il2Cpp
		else {
			Invoke<void*>("mono_thread_attach", pDomain); // Attach for Mono
			Invoke<void*>("mono_jit_thread_attach", pDomain); // JIT attach for Mono
		}
	}

	// Function to detach the current thread from the Unity environment
	static auto ThreadDetach() -> void {
		if (mode_ == Mode::Il2Cpp) 
			Invoke<void*>("il2cpp_thread_detach", pDomain); // Detach for Il2Cpp
		else {
			Invoke<void*>("mono_thread_detach", pDomain); // Detach for Mono
			Invoke<void*>("mono_jit_thread_detach", pDomain); // JIT detach for Mono
		}
	}

	// Function to initialize the Unity environment with the specified module and mode
	static auto Init(void* hmodule, const Mode mode = Mode::Mono) -> void {
		mode_ = mode; // Set the mode
		hmodule_ = hmodule; // Store the module handle

		if (mode_ == Mode::Il2Cpp) {
			pDomain = Invoke<void*>("il2cpp_domain_get"); // Get the Il2Cpp domain
			Invoke<void*>("il2cpp_thread_attach", pDomain); // Attach the thread for Il2Cpp
			ForeachAssembly(); // Iterate through all assemblies
		}
		else {
			pDomain = Invoke<void*>("mono_get_root_domain"); // Get the Mono root domain
			Invoke<void*>("mono_thread_attach", pDomain); // Attach the thread for Mono
			Invoke<void*>("mono_jit_thread_attach", pDomain); // JIT attach for Mono

			ForeachAssembly(); // Iterate through all assemblies
		}
	}
#if WINDOWS_MODE || LINUX_MODE || IOS_MODE /*__cplusplus >= 202002L*/
// Function to dump assembly and class information to files
static auto DumpToFile(const std::string path) -> void {
    std::ofstream io(path + "dump.cs", std::fstream::out); // Open dump.cs for writing
    if (!io) return; // Check if the file opened successfully

    // Iterate through each assembly
    for (const auto& pAssembly : assembly) {
        // Iterate through each class in the assembly
        for (const auto& pClass : pAssembly->classes) {
            io << std::format("\tnamespace: {}", pClass->namespaze.empty() ? "" : pClass->namespaze); // Write namespace
            io << "\n";
            io << std::format("\tAssembly: {}\n", pAssembly->name.empty() ? "" : pAssembly->name); // Write assembly name
            io << std::format("\tAssemblyFile: {} \n", pAssembly->file.empty() ? "" : pAssembly->file); // Write assembly file
            io << std::format("\tclass {}{} ", pClass->name, pClass->parent.empty() ? "" : " : " + pClass->parent); // Write class name and parent
            io << "{\n\n";
            // Write fields of the class
            for (const auto& pField : pClass->fields) 
                io << std::format("\t\t{:+#06X} | {}{} {};\n", pField->offset, pField->static_field ? "static " : "", pField->type->name, pField->name);
            io << "\n";
            // Write methods of the class
            for (const auto& pMethod : pClass->methods) {
                io << std::format("\t\t[Flags: {:032b}] [ParamsCount: {:04d}] |RVA: {:+#010X}|\n", pMethod->flags, pMethod->args.size(), reinterpret_cast<std::uint64_t>(pMethod->function) - reinterpret_cast<std::uint64_t>(hmodule_));
                io << std::format("\t\t{}{} {}(", pMethod->static_function ? "static " : "", pMethod->return_type->name, pMethod->name); // Write method signature
                std::string params{};
                // Write method parameters
                for (const auto& pArg : pMethod->args) 
                    params += std::format("{} {}, ", pArg->pType->name, pArg->name);
                if (!params.empty()) {
                    params.pop_back(); // Remove last comma
                    params.pop_back(); // Remove last space
                }
                io << (params.empty() ? "" : params) << ");\n\n"; // Write parameters
            }
            io << "\t}\n\n"; // End of class
        }
    }

    io << '\n'; // Add a newline
    io.close(); // Close the dump.cs file

    std::ofstream io2(path + "struct.hpp", std::fstream::out); // Open struct.hpp for writing
    if (!io2) return; // Check if the file opened successfully

    // Iterate through each assembly again for struct output
    for (const auto& pAssembly : assembly) {
        // Iterate through each class in the assembly
        for (const auto& pClass : pAssembly->classes) {
            io2 << std::format("\tnamespace: {}", pClass->namespaze.empty() ? "" : pClass->namespaze); // Write namespace
            io2 << "\n";
            io2 << std::format("\tAssembly: {}\n", pAssembly->name.empty() ? "" : pAssembly->name); // Write assembly name
            io2 << std::format("\tAssemblyFile: {} \n", pAssembly->file.empty() ? "" : pAssembly->file); // Write assembly file
            io2 << std::format("\tstruct {}{} ", pClass->name, pClass->parent.empty() ? "" : " : " + pClass->parent); // Write struct name and parent
            io2 << "{\n\n";

            // Iterate through fields to write struct members
            for (size_t i = 0; i < pClass->fields.size(); i++) {
                if (pClass->fields[i]->static_field) continue; // Skip static fields

                auto field = pClass->fields[i];

            next: if ((i + 1) >= pClass->fields.size()) {
                io2 << std::format("\t\tchar {}[0x{:06X}];\n", field->name, 0x4); // Default padding for last field
                continue;
            }

            if (pClass->fields[i + 1]->static_field) {
                i++; // Skip static fields
                goto next; // Go to next iteration
            }

            std::string name = field->name; // Get field name
            std::ranges::replace(name, '<', '_'); // Replace '<' with '_'
            std::ranges::replace(name, '>', '_'); // Replace '>' with '_'

            // Write field types with appropriate sizes
            if (field->type->name == "System.Int64") {
                io2 << std::format("\t\tstd::int64_t {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > 8) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - 8);
                continue;
            }

            if (field->type->name == "System.UInt64") {
                io2 << std::format("\t\tstd::uint64_t {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > 8) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - 8);
                continue;
            }

            if (field->type->name == "System.Int32") {
                io2 << std::format("\t\tint {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > 4) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - 4);
                continue;
            }

            if (field->type->name == "System.UInt32") {
                io2 << std::format("\t\tstd::uint32_t {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > 4) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - 4);
                continue;
            }

            if (field->type->name == "System.Boolean") {
                io2 << std::format("\t\tbool {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > 1) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - 1);
                continue;
            }

            if (field->type->name == "System.String") {
                io2 << std::format("\t\tUnityResolve::UnityType::String* {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > sizeof(void*)) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - sizeof(void*));
                continue;
            }

            if (field->type->name == "System.Single") {
                io2 << std::format("\t\tfloat {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > 4) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - 4);
                continue;
            }

            if (field->type->name == "System.Double") {
                io2 << std::format("\t\tdouble {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > 8) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - 8);
                continue;
            }

            // Handle Unity types
            if (field->type->name == "UnityEngine.Vector3") {
                io2 << std::format("\t\tUnityResolve::UnityType::Vector3 {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > sizeof(UnityType::Vector3) ) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - sizeof(UnityType::Vector3));
                continue;
            }

            if (field->type->name == "UnityEngine.Vector2") {
                io2 << std::format("\t\tUnityResolve::UnityType::Vector2 {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > sizeof(UnityType::Vector2)) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - sizeof(UnityType::Vector2));
                continue;
            }

            if (field->type->name == "UnityEngine.Vector4") {
                io2 << std::format("\t\tUnityResolve::UnityType::Vector4 {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > sizeof(UnityType::Vector4)) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - sizeof(UnityType::Vector4));
                continue;
            }

            if (field->type->name == "UnityEngine.GameObject") {
                io2 << std::format("\t\tUnityResolve::UnityType::GameObject* {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > sizeof(void*)) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - sizeof(void*));
                continue;
            }

            if (field->type->name == "UnityEngine.Transform") {
                io2 << std::format("\t\tUnityResolve::UnityType::Transform* {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > sizeof(void*)) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - sizeof(void*));
                continue;
            }

            if (field->type->name == "UnityEngine.Animator") {
                io2 << std::format("\t\tUnityResolve::UnityType::Animator* {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > sizeof(void*)) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - sizeof(void*));
                continue;
            }

            if (field->type->name == "UnityEngine.Physics") {
                io2 << std::format("\t\tUnityResolve::UnityType::Physics* {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > sizeof(void*)) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - sizeof(void*));
                continue;
            }

            if (field->type->name == "UnityEngine.Component") {
                io2 << std::format("\t\tUnityResolve::UnityType::Component* {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > sizeof(void*)) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - sizeof(void*));
                continue;
            }

            if (field->type->name == "UnityEngine.Rect") {
                io2 << std::format("\t\tUnityResolve::UnityType::Rect {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > sizeof(UnityType::Rect)) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - sizeof(UnityType::Rect));
                continue;
            }

            if (field->type->name == "UnityEngine.Quaternion") {
                io2 << std::format("\t\tUnityResolve::UnityType::Quaternion {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > sizeof(UnityType::Quaternion)) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - sizeof(UnityType::Quaternion));
                continue;
            }

            if (field->type->name == "UnityEngine.Color") {
                io2 << std::format("\t\tUnityResolve::UnityType::Color {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > sizeof(UnityType::Color)) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - sizeof(UnityType::Color));
                continue;
            }

            if (field->type->name == "UnityEngine.Matrix4x4") {
                io2 << std::format("\t\tUnityResolve::UnityType::Matrix4x4 {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > sizeof(UnityType::Matrix4x4)) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - sizeof(UnityType::Matrix4x4));
                continue;
            }

            if (field->type->name == "UnityEngine.Rigidbody") {
                io2 << std::format("\t\tUnityResolve::UnityType::Rigidbody* {};\n", name);
                if (!pClass->fields[i + 1]->static_field && (pClass->fields[i + 1]->offset - field->offset) > sizeof(void*)) 
                    io2 << std::format("\t\tchar {}_[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset - sizeof(void*));
                continue;
            }

            io2 << std::format("\t\tchar {}[0x{:06X}];\n", name, pClass->fields[i + 1]->offset - field->offset); // Default padding for other fields
            }

            io2 << "\n"; // Add a newline
            io2 << "\t};\n\n"; // End of struct
        }
    }
    io2 << '\n'; // Add a newline
    io2.close(); // Close the struct.hpp file
}
#endif

	// Function to invoke a function by name with variable arguments
	template <typename Return, typename... Args>
	static auto Invoke(const std::string& funcName, Args... args) -> Return {
#if WINDOWS_MODE
		// Check if the function address is not found or is null, then get the address using GetProcAddress
		if (!address_.contains(funcName) || !address_[funcName]) 
			address_[funcName] = static_cast<void*>(GetProcAddress(static_cast<HMODULE>(hmodule_), funcName.c_str()));
#elif  ANDROID_MODE || LINUX_MODE || IOS_MODE || HARMONYOS_MODE
		// Check if the function address is not found or is null, then get the address using dlsym
		if (address_.find(funcName) == address_.end() || !address_[funcName]) {
			address_[funcName] = dlsym(hmodule_, funcName.c_str());
		}
#endif

		// If the function address is valid, attempt to invoke the function
		if (address_[funcName] != nullptr) {
			try {
				// Cast the address to the appropriate function pointer type and invoke it with the provided arguments
				return reinterpret_cast<Return(UNITY_CALLING_CONVENTION*)(Args...)>(address_[funcName])(args...);
			}
			catch (...) {
				// Return a default value in case of an exception
				return Return();
			}
		}
		// Return a default value if the function address is not valid
		return Return();
	}

	// Static vector to hold assembly references
	inline static std::vector<Assembly*> assembly;

	// Function to get an assembly by its name
	static auto Get(const std::string& strAssembly) -> Assembly* {
		// Iterate through the assembly vector to find a matching assembly name
		for (const auto pAssembly : assembly) 
			if (pAssembly->name == strAssembly) 
				return pAssembly;
		// Return nullptr if no matching assembly is found
		return nullptr;
	}

private:
	static auto ForeachAssembly() -> void {
		// Function to iterate through assemblies
		if (mode_ == Mode::Il2Cpp) {
			size_t     nrofassemblies = 0;
			// Get the assemblies from the IL2CPP domain
			const auto assemblies = Invoke<void**>("il2cpp_domain_get_assemblies", pDomain, &nrofassemblies);
			for (auto i = 0; i < nrofassemblies; i++) {
				const auto ptr = assemblies[i];
				// Skip if the pointer is null
				if (ptr == nullptr) continue;
				// Create a new Assembly object
				auto       assembly = new Assembly{ .address = ptr };
				// Get the image of the assembly
				const auto image = Invoke<void*>("il2cpp_assembly_get_image", ptr);
				// Get the filename and name of the assembly
				assembly->file = Invoke<const char*>("il2cpp_image_get_filename", image);
				assembly->name = Invoke<const char*>("il2cpp_image_get_name", image);
				// Add the assembly to the global list
				UnityResolve::assembly.push_back(assembly);
				// Iterate through classes in the assembly
				ForeachClass(assembly, image);
			}
		}
		else {
			// For Mono mode, iterate through assemblies using a callback
			Invoke<void*, void(*)(void* ptr, std::vector<Assembly*>&), std::vector<Assembly*>&>("mono_assembly_foreach",
				[](void* ptr, std::vector<Assembly*>& v) {
					// Skip if the pointer is null
					if (ptr == nullptr) return;

					// Create a new Assembly object
					auto assembly = new Assembly{ .address = ptr };
					void* image;
					try {
						// Get the image of the assembly
						image = Invoke<void*>("mono_assembly_get_image", ptr);
						// Get the filename and name of the assembly
						assembly->file = Invoke<const char*>("mono_image_get_filename", image);
						assembly->name = Invoke<const char*>("mono_image_get_name", image);
						assembly->name += ".dll"; // Append .dll extension
						// Add the assembly to the vector
						v.push_back(assembly);
					}
					catch (...) {
						return; // Handle exceptions
					}

					// Iterate through classes in the assembly
					ForeachClass(assembly, image);
				},
				assembly);
		}
	}
	// Function to iterate through classes in the assembly
	static auto ForeachClass(Assembly* assembly, void* image) -> void {
		// Check if the mode is Il2Cpp
		if (mode_ == Mode::Il2Cpp) {
			// Get the count of classes in the image
			const auto count = Invoke<int>("il2cpp_image_get_class_count", image);
			for (auto i = 0; i < count; i++) {
				// Get the class pointer
				const auto pClass = Invoke<void*>("il2cpp_image_get_class", image, i);
				if (pClass == nullptr) continue; // Skip if the class pointer is null

				// Create a new Class object
				const auto pAClass = new Class();
				pAClass->address = pClass; // Set the address of the class
				pAClass->name = Invoke<const char*>("il2cpp_class_get_name", pClass); // Get the class name

				// Get the parent class if it exists
				if (const auto pPClass = Invoke<void*>("il2cpp_class_get_parent", pClass)) 
					pAClass->parent = Invoke<const char*>("il2cpp_class_get_name", pPClass);
				
				// Get the namespace of the class
				pAClass->namespaze = Invoke<const char*>("il2cpp_class_get_namespace", pClass);
				assembly->classes.push_back(pAClass); // Add the class to the assembly's class list

				// Iterate through fields and methods of the class
				ForeachFields(pAClass, pClass);
				ForeachMethod(pAClass, pClass);

				void* i_class; // Interface class pointer
				void* iter{}; // Iterator for interfaces
				do {
					// Get the interfaces implemented by the class
					if ((i_class = Invoke<void*>("il2cpp_class_get_interfaces", pClass, &iter))) {
						ForeachFields(pAClass, i_class); // Iterate through fields of the interface
						ForeachMethod(pAClass, i_class); // Iterate through methods of the interface
					}
				} while (i_class); // Continue until no more interfaces
			}
		}
		else {
			try {
				// For Mono mode, get the table info
				const void* table = Invoke<void*>("mono_image_get_table_info", image, 2);
				const auto  count = Invoke<int>("mono_table_info_get_rows", table);
				for (auto i = 0; i < count; i++) {
					// Get the class pointer for Mono
					const auto pClass = Invoke<void*>("mono_class_get", image, 0x02000000 | (i + 1));
					if (pClass == nullptr) continue; // Skip if the class pointer is null

					// Create a new Class object
					const auto pAClass = new Class();
					pAClass->address = pClass; // Set the address of the class
					try {
						// Get the class name and parent class
						pAClass->name = Invoke<const char*>("mono_class_get_name", pClass);
						if (const auto pPClass = Invoke<void*>("mono_class_get_parent", pClass)) 
							pAClass->parent = Invoke<const char*>("mono_class_get_name", pPClass);
						
						// Get the namespace of the class
						pAClass->namespaze = Invoke<const char*>("mono_class_get_namespace", pClass);
						assembly->classes.push_back(pAClass); // Add the class to the assembly's class list
					}
					catch (...) {
						return; // Handle exceptions
					}

					// Iterate through fields and methods of the class
					ForeachFields(pAClass, pClass);
					ForeachMethod(pAClass, pClass);

					void* iClass; // Interface class pointer
					void* iiter{}; // Iterator for interfaces

					do {
						try {
							// Get the interfaces implemented by the class
							if ((iClass = Invoke<void*>("mono_class_get_interfaces", pClass, &iiter))) {
								ForeachFields(pAClass, iClass); // Iterate through fields of the interface
								ForeachMethod(pAClass, iClass); // Iterate through methods of the interface
							}
						}
						catch (...) {
							return; // Handle exceptions
						}
					} while (iClass); // Continue until no more interfaces
				}
			}
			catch (...) {} // Handle exceptions
		}
	}
	// Function to iterate through fields of a class
	static auto ForeachFields(Class* klass, void* pKlass) -> void {
		// Check if the mode is Il2Cpp
		if (mode_ == Mode::Il2Cpp) {
			void* iter = nullptr; // Iterator for fields
			void* field; // Pointer to the field
			do {
				// Get the fields of the class
				if ((field = Invoke<void*>("il2cpp_class_get_fields", pKlass, &iter))) {
					// Create a new Field object and set its properties
					const auto pField = new Field{ 
						.address = field, 
						.name = Invoke<const char*>("il2cpp_field_get_name", field), 
						.type = new Type{.address = Invoke<void*>("il2cpp_field_get_type", field)}, 
						.klass = klass, 
						.offset = Invoke<int>("il2cpp_field_get_offset", field), 
						.static_field = false, 
						.vTable = nullptr 
					};
					// Determine if the field is static
					pField->static_field = pField->offset <= 0;
					// Get the field type name
					pField->type->name = Invoke<const char*>("il2cpp_type_get_name", pField->type->address);
					pField->type->size = -1; // Set size to -1 initially
					// Add the field to the class's field list
					klass->fields.push_back(pField);
				}
			} while (field); // Continue until no more fields
		}
		else {
			void* iter = nullptr; // Iterator for fields
			void* field; // Pointer to the field
			do {
				try {
					// Get the fields of the class for Mono mode
					if ((field = Invoke<void*>("mono_class_get_fields", pKlass, &iter))) {
						// Create a new Field object and set its properties
						const auto pField = new Field{ 
							.address = field, 
							.name = Invoke<const char*>("mono_field_get_name", field), 
							.type = new Type{.address = Invoke<void*>("mono_field_get_type", field)}, 
							.klass = klass, 
							.offset = Invoke<int>("mono_field_get_offset", field), 
							.static_field = false, 
							.vTable = nullptr 
						};
						int tSize{}; // Temporary size variable
						// Get the field flags
						int flags = Invoke<int>("mono_field_get_flags", field);
						// Check if the field is static
						if (flags & 0x10) // 0x10 = FIELD_ATTRIBUTE_STATIC
						{
							pField->static_field = true; // Mark as static
						}
						// Get the field type name
						pField->type->name = Invoke<const char*>("mono_type_get_name", pField->type->address);
						// Get the size of the field type
						pField->type->size = Invoke<int>("mono_type_size", pField->type->address, &tSize);
						// Add the field to the class's field list
						klass->fields.push_back(pField);
					}
				}
				catch (...) {
					return; // Handle exceptions
				}
			} while (field); // Continue until no more fields
		}
	}
	// Function to iterate through methods of a class
	static auto ForeachMethod(Class* klass, void* pKlass) -> void {
		// Check if the mode is Il2Cpp
		if (mode_ == Mode::Il2Cpp) {
			void* iter = nullptr; // Iterator for methods
			void* method; // Pointer to the method
			do {
				// Get the methods of the class for Il2Cpp mode
				if ((method = Invoke<void*>("il2cpp_class_get_methods", pKlass, &iter))) {
					int        fFlags{}; // Flags for the method
					const auto pMethod = new Method{}; // Create a new Method object
					pMethod->address = method; // Set the method address
					pMethod->name = Invoke<const char*>("il2cpp_method_get_name", method); // Get the method name
					pMethod->klass = klass; // Set the class
					pMethod->return_type = new Type{ .address = Invoke<void*>("il2cpp_method_get_return_type", method), }; // Get the return type
					pMethod->flags = Invoke<int>("il2cpp_method_get_flags", method, &fFlags); // Get the method flags

					pMethod->static_function = pMethod->flags & 0x10; // Check if the method is static
					pMethod->return_type->name = Invoke<const char*>("il2cpp_type_get_name", pMethod->return_type->address); // Get the return type name
					pMethod->return_type->size = -1; // Set size to -1 initially
					pMethod->function = *static_cast<void**>(method); // Get the function pointer
					klass->methods.push_back(pMethod); // Add the method to the class's method list
					const auto argCount = Invoke<int>("il2cpp_method_get_param_count", method); // Get the number of parameters
					// Iterate through parameters and add them to the method's argument list
					for (auto index = 0; index < argCount; index++) 
						pMethod->args.push_back(new Method::Arg{ 
							Invoke<const char*>("il2cpp_method_get_param_name", method, index), 
							new Type{.address = Invoke<void*>("il2cpp_method_get_param", method, index), 
							.name = Invoke<const char*>("il2cpp_type_get_name", Invoke<void*>("il2cpp_method_get_param", method, index)), 
							.size = -1} 
						});
				}
			} while (method); // Continue until no more methods
		}
		else {
			void* iter = nullptr; // Iterator for methods
			void* method; // Pointer to the method

			do {
				try {
					// Get the methods of the class for Mono mode
					if ((method = Invoke<void*>("mono_class_get_methods", pKlass, &iter))) {
						const auto signature = Invoke<void*>("mono_method_signature", method); // Get the method signature
						if (!signature) continue; // Skip if no signature

						int fFlags{}; // Flags for the method
						const auto pMethod = new Method{}; // Create a new Method object
						pMethod->address = method; // Set the method address

						char** names = nullptr; // Array to hold parameter names
						try {
							pMethod->name = Invoke<const char*>("mono_method_get_name", method); // Get the method name
							pMethod->klass = klass; // Set the class
							pMethod->return_type = new Type{ .address = Invoke<void*>("mono_signature_get_return_type", signature) }; // Get the return type

							pMethod->flags = Invoke<int>("mono_method_get_flags", method, &fFlags); // Get the method flags
							pMethod->static_function = pMethod->flags & 0x10; // Check if the method is static

							pMethod->return_type->name = Invoke<const char*>("mono_type_get_name", pMethod->return_type->address); // Get the return type name
							int tSize{}; // Temporary size variable
							pMethod->return_type->size = Invoke<int>("mono_type_size", pMethod->return_type->address, &tSize); // Get the size of the return type

							klass->methods.push_back(pMethod); // Add the method to the class's method list

							int param_count = Invoke<int>("mono_signature_get_param_count", signature); // Get the number of parameters
							names = new char* [param_count]; // Allocate memory for parameter names
							Invoke<void>("mono_method_get_param_names", method, names); // Get parameter names
						}
						catch (...) {
							continue; // Handle exceptions
						}

						void* mIter = nullptr; // Iterator for method parameters
						void* mType; // Pointer to the parameter type
						int iname = 0; // Index for parameter names

						do {
							try {
								// Get each parameter type from the signature
								if ((mType = Invoke<void*>("mono_signature_get_params", signature, &mIter))) {
									int t_size{}; // Temporary size variable
									try {
										// Add the parameter type and name to the method's argument list
										pMethod->args.push_back(new Method::Arg{
											names[iname], // Parameter name
											new Type{.address = mType, 
											.name = Invoke<const char*>("mono_type_get_name", mType), 
											.size = Invoke<int>("mono_type_size", mType, &t_size) }
										});
									}
									catch (...) {
										// Handle exceptions
									}
									iname++; // Increment parameter index
								}
							}
							catch (...) {
								break; // Break on exception
							}
						} while (mType); // Continue until no more parameter types
					}
				}
				catch (...) {
					return; // Handle exceptions
				}
			} while (method); // Continue until no more methods
		}
	}
#ifndef USE_GLM
		struct Vector3 {
			float x, y, z; // Coordinates of the vector

			Vector3() { x = y = z = 0.f; } // Default constructor initializes vector to (0, 0, 0)

			Vector3(const float f1, const float f2, const float f3) {
				x = f1; // Set x coordinate
				y = f2; // Set y coordinate
				z = f3; // Set z coordinate
			}

			[[nodiscard]] auto Length() const -> float { return x * x + y * y + z * z; } // Calculate the length of the vector

			[[nodiscard]] auto Dot(const Vector3 b) const -> float { return x * b.x + y * b.y + z * b.z; } // Calculate the dot product with another vector

			[[nodiscard]] auto Normalize() const -> Vector3 {
				if (const auto len = Length(); len > 0) return Vector3(x / len, y / len, z / len); // Normalize the vector
				return Vector3(x, y, z); // Return the original vector if length is zero
			}

			auto ToVectors(Vector3* m_pForward, Vector3* m_pRight, Vector3* m_pUp) const -> void {
				constexpr auto m_fDeg2Rad = std::numbers::pi_v<float> / 180.F; // Conversion factor from degrees to radians

				const auto m_fSinX = sinf(x * m_fDeg2Rad); // Sine of x angle
				const auto m_fCosX = cosf(x * m_fDeg2Rad); // Cosine of x angle

				const auto m_fSinY = sinf(y * m_fDeg2Rad); // Sine of y angle
				const auto m_fCosY = cosf(y * m_fDeg2Rad); // Cosine of y angle

				const auto m_fSinZ = sinf(z * m_fDeg2Rad); // Sine of z angle
				const auto m_fCosZ = cosf(z * m_fDeg2Rad); // Cosine of z angle

				if (m_pForward) {
					m_pForward->x = m_fCosX * m_fCosY; // Calculate forward vector x component
					m_pForward->y = -m_fSinX; // Calculate forward vector y component
					m_pForward->z = m_fCosX * m_fSinY; // Calculate forward vector z component
				}

				if (m_pRight) {
					m_pRight->x = -1.f * m_fSinZ * m_fSinX * m_fCosY + -1.f * m_fCosZ * -m_fSinY; // Calculate right vector x component
					m_pRight->y = -1.f * m_fSinZ * m_fCosX; // Calculate right vector y component
					m_pRight->z = -1.f * m_fSinZ * m_fSinX * m_fSinY + -1.f * m_fCosZ * m_fCosY; // Calculate right vector z component
				}

				if (m_pUp) {
					m_pUp->x = m_fCosZ * m_fSinX * m_fCosY + -m_fSinZ * -m_fSinY; // Calculate up vector x component
					m_pUp->y = m_fCosZ * m_fCosX; // Calculate up vector y component
					m_pUp->z = m_fCosZ * m_fSinX * m_fSinY + -m_fSinZ * m_fCosY; // Calculate up vector z component
				}
			}

			[[nodiscard]] auto Distance(Vector3& event) const -> float {
				const auto dx = this->x - event.x; // Difference in x coordinates
				const auto dy = this->y - event.y; // Difference in y coordinates
				const auto dz = this->z - event.z; // Difference in z coordinates
				return std::sqrt(dx * dx + dy * dy + dz * dz); // Calculate the distance to another vector
			}

			auto operator*(const float x) -> Vector3 {
				this->x *= x; // Scale x coordinate
				this->y *= x; // Scale y coordinate
				this->z *= x; // Scale z coordinate
				return *this; // Return the scaled vector
			}

			auto operator-(const float x) -> Vector3 {
				this->x -= x; // Subtract from x coordinate
				this->y -= x; // Subtract from y coordinate
				this->z -= x; // Subtract from z coordinate
				return *this; // Return the modified vector
			}

			auto operator+(const float x) -> Vector3 {
				this->x += x; // Add to x coordinate
				this->y += x; // Add to y coordinate
				this->z += x; // Add to z coordinate
				return *this; // Return the modified vector
			}

			auto operator/(const float x) -> Vector3 {
				this->x /= x; // Divide x coordinate
				this->y /= x; // Divide y coordinate
				this->z /= x; // Divide z coordinate
				return *this; // Return the modified vector
			}

			auto operator*(const Vector3 x) -> Vector3 {
				this->x *= x.x; // Scale by another vector's x coordinate
				this->y *= x.y; // Scale by another vector's y coordinate
				this->z *= x.z; // Scale by another vector's z coordinate
				return *this; // Return the scaled vector
			}

			auto operator-(const Vector3 x) -> Vector3 {
				this->x -= x.x; // Subtract another vector's x coordinate
				this->y -= x.y; // Subtract another vector's y coordinate
				this->z -= x.z; // Subtract another vector's z coordinate
				return *this; // Return the modified vector
			}

			auto operator+(const Vector3 x) -> Vector3 {
				this->x += x.x; // Add another vector's x coordinate
				this->y += x.y; // Add another vector's y coordinate
				this->z += x.z; // Add another vector's z coordinate
				return *this; // Return the modified vector
			}

			auto operator/(const Vector3 x) -> Vector3 {
				this->x /= x.x; // Divide by another vector's x coordinate
				this->y /= x.y; // Divide by another vector's y coordinate
				this->z /= x.z; // Divide by another vector's z coordinate
				return *this; // Return the modified vector
			}

			auto operator ==(const Vector3 x) const -> bool { return this->x == x.x && this->y == x.y && this->z == x.z; } // Check equality with another vector
		};
#endif

#ifndef USE_GLM
		struct Vector2 {
			float x, y;

			Vector2() { x = y = 0.f; }

			Vector2(const float f1, const float f2) {
				x = f1;
				y = f2;
			}

			[[nodiscard]] auto Distance(const Vector2& event) const -> float {
				const auto dx = this->x - event.x;
				const auto dy = this->y - event.y;
				return std::sqrt(dx * dx + dy * dy);
			}

			auto operator*(const float x) -> Vector2 {
				this->x *= x;
				this->y *= x;
				return *this;
			}

			auto operator/(const float x) -> Vector2 {
				this->x /= x;
				this->y /= x;
				return *this;
			}

			auto operator+(const float x) -> Vector2 {
				this->x += x;
				this->y += x;
				return *this;
			}

			auto operator-(const float x) -> Vector2 {
				this->x -= x;
				this->y -= x;
				return *this;
			}

			auto operator*(const Vector2 x) -> Vector2 {
				this->x *= x.x;
				this->y *= x.y;
				return *this;
			}

			auto operator-(const Vector2 x) -> Vector2 {
				this->x -= x.x;
				this->y -= x.y;
				return *this;
			}

			auto operator+(const Vector2 x) -> Vector2 {
				this->x += x.x;
				this->y += x.y;
				return *this;
			}

			auto operator/(const Vector2 x) -> Vector2 {
				this->x /= x.x;
				this->y /= x.y;
				return *this;
			}

			auto operator ==(const Vector2 x) const -> bool { return this->x == x.x && this->y == x.y; }
		};
#endif

#ifndef USE_GLM
		struct Vector4 {
			float x, y, z, w;

			Vector4() { x = y = z = w = 0.F; }

			Vector4(const float f1, const float f2, const float f3, const float f4) {
				x = f1;
				y = f2;
				z = f3;
				w = f4;
			}

			auto operator*(const float x) -> Vector4 {
				this->x *= x;
				this->y *= x;
				this->z *= x;
				this->w *= x;
				return *this;
			}

			auto operator-(const float x) -> Vector4 {
				this->x -= x;
				this->y -= x;
				this->z -= x;
				this->w -= x;
				return *this;
			}

			auto operator+(const float x) -> Vector4 {
				this->x += x;
				this->y += x;
				this->z += x;
				this->w += x;
				return *this;
			}

			auto operator/(const float x) -> Vector4 {
				this->x /= x;
				this->y /= x;
				this->z /= x;
				this->w /= x;
				return *this;
			}

			auto operator*(const Vector4 x) -> Vector4 {
				this->x *= x.x;
				this->y *= x.y;
				this->z *= x.z;
				this->w *= x.w;
				return *this;
			}

			auto operator-(const Vector4 x) -> Vector4 {
				this->x -= x.x;
				this->y -= x.y;
				this->z -= x.z;
				this->w -= x.w;
				return *this;
			}

			auto operator+(const Vector4 x) -> Vector4 {
				this->x += x.x;
				this->y += x.y;
				this->z += x.z;
				this->w += x.w;
				return *this;
			}

			auto operator/(const Vector4 x) -> Vector4 {
				this->x /= x.x;
				this->y /= x.y;
				this->z /= x.z;
				this->w /= x.w;
				return *this;
			}

			auto operator ==(const Vector4 x) const -> bool { return this->x == x.x && this->y == x.y && this->z == x.z && this->w == x.w; }
		};
#endif
#ifndef USE_GLM
		struct Quaternion {
			float x, y, z, w; // Quaternion components: x, y, z, w

			Quaternion() { x = y = z = w = 0.F; } // Default constructor initializes all components to 0

			Quaternion(const float f1, const float f2, const float f3, const float f4) {
				x = f1; // Initialize x component
				y = f2; // Initialize y component
				z = f3; // Initialize z component
				w = f4; // Initialize w component
			}

			auto Euler(float m_fX, float m_fY, float m_fZ) -> Quaternion {
				constexpr auto m_fDeg2Rad = std::numbers::pi_v<float> / 180.F; // Conversion factor from degrees to radians

				m_fX = m_fX * m_fDeg2Rad * 0.5F; // Convert x angle to radians
				m_fY = m_fY * m_fDeg2Rad * 0.5F; // Convert y angle to radians
				m_fZ = m_fZ * m_fDeg2Rad * 0.5F; // Convert z angle to radians

				const auto m_fSinX = sinf(m_fX); // Sine of x angle
				const auto m_fCosX = cosf(m_fX); // Cosine of x angle

				const auto m_fSinY = sinf(m_fY); // Sine of y angle
				const auto m_fCosY = cosf(m_fY); // Cosine of y angle

				const auto m_fSinZ = sinf(m_fZ); // Sine of z angle
				const auto m_fCosZ = cosf(m_fZ); // Cosine of z angle

				// Calculate quaternion components based on Euler angles
				x = m_fCosY * m_fSinX * m_fCosZ + m_fSinY * m_fCosX * m_fSinZ;
				y = m_fSinY * m_fCosX * m_fCosZ - m_fCosY * m_fSinX * m_fSinZ;
				z = m_fCosY * m_fCosX * m_fSinZ - m_fSinY * m_fSinX * m_fCosZ;
				w = m_fCosY * m_fCosX * m_fCosZ + m_fSinY * m_fSinX * m_fSinZ;

				return *this; // Return the updated quaternion
			}

			auto Euler(const Vector3& m_vRot) -> Quaternion { return Euler(m_vRot.x, m_vRot.y, m_vRot.z); } // Overloaded function to accept Vector3

			[[nodiscard]] auto ToEuler() const -> Vector3 {
				Vector3 m_vEuler; // Vector to hold Euler angles

				const auto m_fDist = (x * x) + (y * y) + (z * z) + (w * w); // Calculate the distance

				// Check for gimbal lock conditions
				if (const auto m_fTest = x * w - y * z; m_fTest > 0.4995F * m_fDist) {
					m_vEuler.x = std::numbers::pi_v<float> *0.5F; // Set x angle
					m_vEuler.y = 2.F * atan2f(y, x); // Set y angle
					m_vEuler.z = 0.F; // Set z angle
				}
				else if (m_fTest < -0.4995F * m_fDist) {
					m_vEuler.x = std::numbers::pi_v<float> *-0.5F; // Set x angle
					m_vEuler.y = -2.F * atan2f(y, x); // Set y angle
					m_vEuler.z = 0.F; // Set z angle
				}
				else {
					// Calculate Euler angles from quaternion
					m_vEuler.x = asinf(2.F * (w * x - y * z));
					m_vEuler.y = atan2f(2.F * w * y + 2.F * z * x, 1.F - 2.F * (x * x + y * y));
					m_vEuler.z = atan2f(2.F * w * z + 2.F * x * y, 1.F - 2.F * (z * z + x * x));
				}

				constexpr auto m_fRad2Deg = 180.F / std::numbers::pi_v<float>; // Conversion factor from radians to degrees
				m_vEuler.x *= m_fRad2Deg; // Convert x angle to degrees
				m_vEuler.y *= m_fRad2Deg; // Convert y angle to degrees
				m_vEuler.z *= m_fRad2Deg; // Convert z angle to degrees

				return m_vEuler; // Return the Euler angles
			}

			static auto LookRotation(const Vector3& forward) -> Quaternion {
				static Method* method; // Static method pointer

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Quaternion")->Get<Method>("LookRotation", { "UnityEngine.Vector3" }); // Get the LookRotation method
				if (method) return method->Invoke<Quaternion, Vector3>(forward); // Invoke the method with the forward vector
				return {}; // Return an empty quaternion if method not found
			}

			auto operator*(const float x) -> Quaternion {
				this->x *= x; // Scale x component
				this->y *= x; // Scale y component
				this->z *= x; // Scale z component
				this->w *= x; // Scale w component
				return *this; // Return the updated quaternion
			}

			auto operator-(const float x) -> Quaternion {
				this->x -= x; // Subtract from x component
				this->y -= x; // Subtract from y component
				this->z -= x; // Subtract from z component
				this->w -= x; // Subtract from w component
				return *this; // Return the updated quaternion
			}

			auto operator+(const float x) -> Quaternion {
				this->x += x; // Add to x component
				this->y += x; // Add to y component
				this->z += x; // Add to z component
				this->w += x; // Add to w component
				return *this; // Return the updated quaternion
			}

			auto operator/(const float x) -> Quaternion {
				this->x /= x; // Divide x component
				this->y /= x; // Divide y component
				this->z /= x; // Divide z component
				this->w /= x; // Divide w component
				return *this; // Return the updated quaternion
			}

			auto operator*(const Quaternion x) -> Quaternion {
				this->x *= x.x; // Multiply x components
				this->y *= x.y; // Multiply y components
				this->z *= x.z; // Multiply z components
				this->w *= x.w; // Multiply w components
				return *this; // Return the updated quaternion
			}

			auto operator-(const Quaternion x) -> Quaternion {
				this->x -= x.x; // Subtract x components
				this->y -= x.y; // Subtract y components
				this->z -= x.z; // Subtract z components
				this->w -= x.w; // Subtract w components
				return *this; // Return the updated quaternion
			}

			auto operator+(const Quaternion x) -> Quaternion {
				this->x += x.x; // Add x components
				this->y += x.y; // Add y components
				this->z += x.z; // Add z components
				this->w += x.w; // Add w components
				return *this; // Return the updated quaternion
			}

			auto operator/(const Quaternion x) -> Quaternion {
				this->x /= x.x; // Divide x components
				this->y /= x.y; // Divide y components
				this->z /= x.z; // Divide z components
				this->w /= x.w; // Divide w components
				return *this; // Return the updated quaternion
			}

			auto operator ==(const Quaternion x) const -> bool { 
				// Check if two quaternions are equal
				return this->x == x.x && this->y == x.y && this->z == x.z && this->w == x.w; 
			}
		};
#endif

		struct Bounds {
			Vector3 m_vCenter;
			Vector3 m_vExtents;
		};

		struct Plane {
			Vector3 m_vNormal;
			float   fDistance;
		};

		struct Ray {
			Vector3 m_vOrigin;
			Vector3 m_vDirection;
		};

		struct RaycastHit {
			Vector3 m_Point;
			Vector3 m_Normal;
		};

		struct Rect {
			float fX, fY;
			float fWidth, fHeight;

			Rect() { fX = fY = fWidth = fHeight = 0.f; }

			Rect(const float f1, const float f2, const float f3, const float f4) {
				fX = f1;
				fY = f2;
				fWidth = f3;
				fHeight = f4;
			}
		};

		struct Color {
			float r, g, b, a;

			Color() { r = g = b = a = 0.f; }

			explicit Color(const float fRed = 0.f, const float fGreen = 0.f, const float fBlue = 0.f, const float fAlpha = 1.f) {
				r = fRed;
				g = fGreen;
				b = fBlue;
				a = fAlpha;
			}
		};

#ifndef USE_GLM
		struct Matrix4x4 {
			float m[4][4] = { {0} };

			Matrix4x4() = default;

			auto operator[](const int i) -> float* { return m[i]; }
		};
#endif

		struct Object {
			union {
				void* klass{ nullptr };
				void* vtable;
			}         Il2CppClass;

			struct MonitorData* monitor{ nullptr };

			auto GetType() -> CsType* {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Object", "System")->Get<Method>("GetType");
				if (method) return method->Invoke<CsType*>(this);
				return nullptr;
			}

			auto ToString() -> String* {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Object", "System")->Get<Method>("ToString");
				if (method) return method->Invoke<String*>(this);
				return {};
			}

			int GetHashCode() {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Object", "System")->Get<Method>("GetHashCode");
				if (method) return method->Invoke<int>(this);
				return 0;
			}
		};

		enum class BindingFlags : uint32_t {
			Default = 0,
			IgnoreCase = 1,
			DeclaredOnly = 2,
			Instance = 4,
			Static = 8,
			Public = 16,
			NonPublic = 32,
			FlattenHierarchy = 64,
			InvokeMethod = 256,
			CreateInstance = 512,
			GetField = 1024,
			SetField = 2048,
			GetProperty = 4096,
			SetProperty = 8192,
			PutDispProperty = 16384,
			PutRefDispProperty = 32768,
			ExactBinding = 65536,
			SuppressChangeType = 131072,
			OptionalParamBinding = 262144,
			IgnoreReturn = 16777216,
		};

		enum class FieldAttributes : uint32_t {
			FieldAccessMask = 7,
			PrivateScope = 0,
			Private = 1,
			FamANDAssem = 2,
			Assembly = 3,
			Family = 4,
			FamORAssem = 5,
			Public = 6,
			Static = 16,
			InitOnly = 32,
			Literal = 64,
			NotSerialized = 128,
			HasFieldRVA = 256,
			SpecialName = 512,
			RTSpecialName = 1024,
			HasFieldMarshal = 4096,
			PinvokeImpl = 8192,
			HasDefault = 32768,
			ReservedMask = 38144
		};

		enum class MemberTypes : uint32_t {
			Constructor = 1,
			Event = 2,
			Field = 4,
			Method = 8,
			Property = 16,
			TypeInfo = 32,
			Custom = 64,
			NestedType = 128,
			All = 191
		};

		struct MemberInfo {

		};

		struct FieldInfo : public MemberInfo {
			auto GetIsInitOnly() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("FieldInfo", "System.Reflection", "MemberInfo")->Get<Method>("get_IsInitOnly");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsLiteral() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("FieldInfo", "System.Reflection", "MemberInfo")->Get<Method>("get_IsLiteral");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsNotSerialized() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("FieldInfo", "System.Reflection", "MemberInfo")->Get<Method>("get_IsNotSerialized");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsStatic() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("FieldInfo", "System.Reflection", "MemberInfo")->Get<Method>("get_IsStatic");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsFamily() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("FieldInfo", "System.Reflection", "MemberInfo")->Get<Method>("get_IsFamily");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsPrivate() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("FieldInfo", "System.Reflection", "MemberInfo")->Get<Method>("get_IsPrivate");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsPublic() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("FieldInfo", "System.Reflection", "MemberInfo")->Get<Method>("get_IsPublic");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetAttributes() -> FieldAttributes {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("FieldInfo", "System.Reflection", "MemberInfo")->Get<Method>("get_Attributes");
				if (method) return method->Invoke<FieldAttributes>(this);
				return {};
			}

			auto GetMemberType() -> MemberTypes {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("FieldInfo", "System.Reflection", "MemberInfo")->Get<Method>("get_MemberType");
				if (method) return method->Invoke<MemberTypes>(this);
				return {};
			}

			auto GetFieldOffset() -> int {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("FieldInfo", "System.Reflection", "MemberInfo")->Get<Method>("GetFieldOffset");
				if (method) return method->Invoke<int>(this);
				return {};
			}

			template<typename T>
			auto GetValue(Object* object) -> T {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("FieldInfo", "System.Reflection", "MemberInfo")->Get<Method>("GetValue");
				if (method) return method->Invoke<T>(this, object);
				return T();
			}

			template<typename T>
			auto SetValue(Object* object, T value) -> void {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("FieldInfo", "System.Reflection", "MemberInfo")->Get<Method>("SetValue", { "System.Object", "System.Object" });
				if (method) return method->Invoke<T>(this, object, value);
			}
		};

		struct CsType {
			auto FormatTypeName() -> String* {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("FormatTypeName");
				if (method) return method->Invoke<String*>(this);
				return {};
			}

			auto GetFullName() -> String* {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_FullName");
				if (method) return method->Invoke<String*>(this);
				return {};
			}

			auto GetNamespace() -> String* {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_Namespace");
				if (method) return method->Invoke<String*>(this);
				return {};
			}

			auto GetIsSerializable() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsSerializable");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetContainsGenericParameters() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_ContainsGenericParameters");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsVisible() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsVisible");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsNested() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsNested");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsArray() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsArray");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsByRef() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsByRef");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsPointer() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsPointer");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsConstructedGenericType() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsConstructedGenericType");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsGenericParameter() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsGenericParameter");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsGenericMethodParameter() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsGenericMethodParameter");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsGenericType() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsGenericType");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsGenericTypeDefinition() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsGenericTypeDefinition");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsSZArray() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsSZArray");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsVariableBoundArray() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsVariableBoundArray");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetHasElementType() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_HasElementType");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsAbstract() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsAbstract");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsSealed() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsSealed");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsClass() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsClass");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsNestedAssembly() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsNestedAssembly");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsNestedPublic() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsNestedPublic");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsNotPublic() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsNotPublic");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsPublic() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsPublic");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsExplicitLayout() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsExplicitLayout");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsCOMObject() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsCOMObject");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsContextful() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsContextful");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsCollectible() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsCollectible");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsEnum() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsEnum");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsMarshalByRef() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsMarshalByRef");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsPrimitive() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsPrimitive");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsValueType() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsValueType");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsSignatureType() -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("get_IsSignatureType");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetField(const std::string& name, const BindingFlags flags = static_cast<BindingFlags>(static_cast<int>(BindingFlags::Instance) | static_cast<int>(BindingFlags::Static) | static_cast<int>(BindingFlags::Public))) -> FieldInfo* {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Type", "System", "MemberInfo")->Get<Method>("GetField", { "System.String name", "System.Reflection.BindingFlags" });
				if (method) return method->Invoke<FieldInfo*>(this, String::New(name), flags);
				return nullptr;
			}
		};

		struct String : Object {
			int32_t  m_stringLength{ 0 };
			wchar_t m_firstChar[32]{};

			[[nodiscard]] auto ToString() const -> std::string {
				std::wstring_convert<std::codecvt_utf8<wchar_t>> converterX;
				return converterX.to_bytes(m_firstChar);
			}

			auto operator[](const int i) const -> wchar_t { return m_firstChar[i]; }

			auto operator=(const std::string& newString) const -> String* { return New(newString); }

			auto operator==(const std::wstring& newString) const -> bool { return Equals(newString); }

			auto Clear() -> void {

				memset(m_firstChar, 0, m_stringLength);
				m_stringLength = 0;
			}

			[[nodiscard]] auto Equals(const std::wstring& newString) const -> bool {

				if (newString.size() != m_stringLength) return false;
				if (std::memcmp(newString.data(), m_firstChar, m_stringLength) != 0) return false;
				return true;
			}

			static auto New(const std::string& str) -> String* {
				if (mode_ == Mode::Il2Cpp) return UnityResolve::Invoke<String*, const char*>("il2cpp_string_new", str.c_str());
				return UnityResolve::Invoke<String*, void*, const char*>("mono_string_new", UnityResolve::Invoke<void*>("mono_get_root_domain"), str.c_str());
			}
		};

		template <typename T>
		struct Array : Object {
			struct {
				std::uintptr_t length;
				std::int32_t   lower_bound;
			}*bounds{ nullptr };

			std::uintptr_t           max_length{ 0 };
			T** vector{};

			auto GetData() -> uintptr_t { return reinterpret_cast<uintptr_t>(&vector); }

			auto operator[](const unsigned int m_uIndex) -> T& { return *reinterpret_cast<T*>(GetData() + sizeof(T) * m_uIndex); }

			auto At(const unsigned int m_uIndex) -> T& { return operator[](m_uIndex); }

			auto Insert(T* m_pArray, uintptr_t m_uSize, const uintptr_t m_uIndex = 0) -> void {
				if ((m_uSize + m_uIndex) >= max_length) {
					if (m_uIndex >= max_length) return;

					m_uSize = max_length - m_uIndex;
				}

				for (uintptr_t u = 0; m_uSize > u; ++u) operator[](u + m_uIndex) = m_pArray[u];
			}

			auto Fill(T m_tValue) -> void { for (uintptr_t u = 0; max_length > u; ++u) operator[](u) = m_tValue; }

			auto RemoveAt(const unsigned int m_uIndex) -> void {
				if (m_uIndex >= max_length) return;

				if (max_length > (m_uIndex + 1)) for (auto u = m_uIndex; (max_length - m_uIndex) > u; ++u) operator[](u) = operator[](u + 1);

				--max_length;
			}

			auto RemoveRange(const unsigned int m_uIndex, unsigned int m_uCount) -> void {
				if (m_uCount == 0) m_uCount = 1;

				const auto m_uTotal = m_uIndex + m_uCount;
				if (m_uTotal >= max_length) return;

				if (max_length > (m_uTotal + 1)) for (auto u = m_uIndex; (max_length - m_uTotal) >= u; ++u) operator[](u) = operator[](u + m_uCount);

				max_length -= m_uCount;
			}

			auto RemoveAll() -> void {
				if (max_length > 0) {
					memset(GetData(), 0, sizeof(Type) * max_length);
					max_length = 0;
				}
			}

			auto ToVector() -> std::vector<T> {
				std::vector<T> rs{};
				rs.reserve(this->max_length);
				for (auto i = 0; i < this->max_length; i++) rs.push_back(this->At(i));
				return rs;
			}

			auto Resize(int newSize) -> void {
				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("Array")->Get<Method>("Resize");
				if (method) return method->Invoke<void>(this, newSize);
			}

			static auto New(const Class* kalss, const std::uintptr_t size) -> Array* {
				if (mode_ == Mode::Il2Cpp) return UnityResolve::Invoke<Array*, void*, std::uintptr_t>("il2cpp_array_new", kalss->address, size);
				return UnityResolve::Invoke<Array*, void*, void*, std::uintptr_t>("mono_array_new", pDomain, kalss->address, size);
			}
		};

		template <typename Type>
		struct List : Object {
			Array<Type>* pList;
			int          size{};
			int          version{};
			void* syncRoot{};

			auto ToArray() -> Array<Type>* { return pList; }

			static auto New(const Class* kalss, const std::uintptr_t size) -> List* {
				auto pList = new List<Type>();
				pList->pList = Array<Type>::New(kalss, size);
				pList->size = size;
				return pList;
			}

			auto operator[](const unsigned int m_uIndex) -> Type& { return pList->At(m_uIndex); }

			auto Add(Type pDate) -> void {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("List`1")->Get<Method>("Add");
				if (method) return method->Invoke<void>(this, pDate);
			}

			auto Remove(Type pDate) -> bool {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("List`1")->Get<Method>("Remove");
				if (method) return method->Invoke<bool>(this, pDate);
				return false;
			}

			auto RemoveAt(int index) -> void {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("List`1")->Get<Method>("RemoveAt");
				if (method) return method->Invoke<void>(this, index);
			}

			auto ForEach(void(*action)(Type pDate)) -> void {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("List`1")->Get<Method>("ForEach");
				if (method) return method->Invoke<void>(this, action);
			}

			auto GetRange(int index, int count) -> List* {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("List`1")->Get<Method>("GetRange");
				if (method) return method->Invoke<List*>(this, index, count);
				return nullptr;
			}

			auto Clear() -> void {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("List`1")->Get<Method>("Clear");
				if (method) return method->Invoke<void>(this);
			}

			auto Sort(int (*comparison)(Type* pX, Type* pY)) -> void {

				static Method* method;
				if (!method) method = Get("mscorlib.dll")->Get("List`1")->Get<Method>("Sort", { "*" });
				if (method) return method->Invoke<void>(this, comparison);
			}
		};

		template <typename TKey, typename TValue>
		struct Dictionary : Object {
			struct Entry {
				int    iHashCode;
				int    iNext;
				TKey   tKey;
				TValue tValue;
			};

			Array<int>* pBuckets;
			Array<Entry*>* pEntries;
			int            iCount;
			int            iVersion;
			int            iFreeList;
			int            iFreeCount;
			void* pComparer;
			void* pKeys;
			void* pValues;

			auto GetEntry() -> Entry* { return reinterpret_cast<Entry*>(pEntries->GetData()); }

			auto GetKeyByIndex(const int iIndex) -> TKey {
				TKey tKey = { 0 };

				Entry* pEntry = GetEntry();
				if (pEntry) tKey = pEntry[iIndex].tKey;

				return tKey;
			}

			auto GetValueByIndex(const int iIndex) -> TValue {
				TValue tValue = { 0 };

				Entry* pEntry = GetEntry();
				if (pEntry) tValue = pEntry[iIndex].tValue;

				return tValue;
			}

			auto GetValueByKey(const TKey tKey) -> TValue {
				TValue tValue = { 0 };
				for (auto i = 0; i < iCount; i++) if (GetEntry()[i].tKey == tKey) tValue = GetEntry()[i].tValue;
				return tValue;
			}

			auto operator[](const TKey tKey) const -> TValue { return GetValueByKey(tKey); }
		};

		struct UnityObject : Object {
			void* m_CachedPtr;

			auto GetName() -> String* {

				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Object")->Get<Method>("get_name");
				if (method) return method->Invoke<String*>(this);
				return {};
			}

			auto ToString() -> String* {

				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Object")->Get<Method>("ToString");
				if (method) return method->Invoke<String*>(this);
				return {};
			}

			static auto ToString(UnityObject* obj) -> String* {
				if (!obj) return {};
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Object")->Get<Method>("ToString", { "*" });
				if (method) return method->Invoke<String*>(obj);
				return {};
			}

			static auto Instantiate(UnityObject* original) -> UnityObject* {
				if (!original) return nullptr;
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Object")->Get<Method>("Instantiate", { "*" });
				if (method) return method->Invoke<UnityObject*>(original);
				return nullptr;
			}

			static auto Destroy(UnityObject* original) -> void {
				if (!original) return;
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Object")->Get<Method>("Destroy", { "*" });
				if (method) return method->Invoke<void>(original);
			}
		};

		struct Component : public UnityObject {
			auto GetTransform() -> Transform* {

				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Component")->Get<Method>("get_transform");
				if (method) return method->Invoke<Transform*>(this);
				return nullptr;
			}

			auto GetGameObject() -> GameObject* {

				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Component")->Get<Method>("get_gameObject");
				if (method) return method->Invoke<GameObject*>(this);
				return nullptr;
			}

			auto GetTag() -> String* {

				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Component")->Get<Method>("get_tag");
				if (method) return method->Invoke<String*>(this);
				return {};
			}

			template <typename T>
			auto GetComponentsInChildren() -> std::vector<T> {

				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Component")->Get<Method>("GetComponentsInChildren");
				if (method) return method->Invoke<Array<T>*>(this)->ToVector();
				return {};
			}

			template <typename T>
			auto GetComponentsInChildren(Class* pClass) -> std::vector<T> {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Component")->Get<Method>("GetComponentsInChildren", { "System.Type" });
				if (method) return method->Invoke<Array<T>*>(this, pClass->GetType())->ToVector();
				return {};
			}

			template <typename T>
			auto GetComponents() -> std::vector<T> {
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Component")->Get<Method>("GetComponents");
				if (method) return method->Invoke<Array<T>*>(this)->ToVector();
				return {};
			}

			template <typename T>
			auto GetComponents(Class* pClass) -> std::vector<T> {
				static Method* method;
				static void* obj;

				if (!method || !obj) {
					method = Get("UnityEngine.CoreModule.dll")->Get("Component")->Get<Method>("GetComponents", { "System.Type" });
					obj = pClass->GetType();
				}
				if (method) return method->Invoke<Array<T>*>(this, obj)->ToVector();
				return {};
			}

			template <typename T>
			auto GetComponentsInParent() -> std::vector<T> {
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Component")->Get<Method>("GetComponentsInParent");
				if (method) return method->Invoke<Array<T>*>(this)->ToVector();
				return {};
			}

			template <typename T>
			auto GetComponentsInParent(Class* pClass) -> std::vector<T> {
				static Method* method;
				static void* obj;

				if (!method || !obj) {
					method = Get("UnityEngine.CoreModule.dll")->Get("Component")->Get<Method>("GetComponentsInParent", { "System.Type" });
					obj = pClass->GetType();
				}
				if (method) return method->Invoke<Array<T>*>(this, obj)->ToVector();
				return {};
			}

			template <typename T>
			auto GetComponentInChildren(Class* pClass) -> T {
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Component")->Get<Method>("GetComponentInChildren", { "System.Type" });
				if (method) return method->Invoke<T>(this, pClass->GetType());
				return T();
			}

			template <typename T>
			auto GetComponentInParent(Class* pClass) -> T {
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Component")->Get<Method>("GetComponentInParent", { "System.Type" });
				if (method) return method->Invoke<T>(this, pClass->GetType());
				return T();
			}
		};

		struct Camera : Component {
			enum class Eye : int {
				Left,
				Right,
				Mono
			};

			static auto GetMain() -> Camera* {
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Camera")->Get<Method>("get_main");
				if (method) return method->Invoke<Camera*>();
				return nullptr;
			}

			static auto GetCurrent() -> Camera* {
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Camera")->Get<Method>("get_current");
				if (method) return method->Invoke<Camera*>();
				return nullptr;
			}

			static auto GetAllCount() -> int {
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Camera")->Get<Method>("get_allCamerasCount");
				if (method) return method->Invoke<int>();
				return 0;
			}

			static auto GetAllCamera() -> std::vector<Camera*> {
				static Method* method;
				static Class* klass;

				if (!method || !klass) {
					method = Get("UnityEngine.CoreModule.dll")->Get("Camera")->Get<Method>("GetAllCameras", { "*" });
					klass = Get("UnityEngine.CoreModule.dll")->Get("Camera");
				}

				if (method && klass) {
					if (const int count = GetAllCount(); count != 0) {
						const auto array = Array<Camera*>::New(klass, count);
						method->Invoke<int>(array);
						return array->ToVector();
					}
				}

				return {};
			}

			auto GetDepth() -> float {

				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Camera")->Get<Method>("get_depth");
				if (method) return method->Invoke<float>(this);
				return 0.0f;
			}

			auto SetDepth(const float depth) -> void {

				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Camera")->Get<Method>("set_depth", { "*" });
				if (method) return method->Invoke<void>(this, depth);
			}

			auto SetFoV(const float fov) -> void {

				static Method* method_fieldOfView;
				if (!method_fieldOfView) method_fieldOfView = Get("UnityEngine.CoreModule.dll")->Get("Camera")->Get<Method>("set_fieldOfView", { "*" });
				if (method_fieldOfView) return method_fieldOfView->Invoke<void>(this, fov);
			}

			auto GetFoV() -> float {

				static Method* method_fieldOfView;
				if (!method_fieldOfView) method_fieldOfView = Get("UnityEngine.CoreModule.dll")->Get("Camera")->Get<Method>("get_fieldOfView");
				if (method_fieldOfView) return method_fieldOfView->Invoke<float>(this);
				return 0.0f;
			}

			auto WorldToScreenPoint(const Vector3& position, const Eye eye = Eye::Mono) -> Vector3 {

				static Method* method;
				if (!method) {
					if (mode_ == Mode::Mono) method = Get("UnityEngine.CoreModule.dll")->Get("Camera")->Get<Method>("WorldToScreenPoint_Injected");
					else method = Get("UnityEngine.CoreModule.dll")->Get("Camera")->Get<Method>("WorldToScreenPoint", { "*", "*" });
				}
				if (mode_ == Mode::Mono && method) {
					const Vector3 vec3{};
					method->Invoke<void>(this, position, eye, &vec3);
					return vec3;
				}
				if (method) return method->Invoke<Vector3>(this, position, eye);
				return {};
			}

			auto ScreenToWorldPoint(const Vector3& position, const Eye eye = Eye::Mono) -> Vector3 {

				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Camera")->Get<Method>(mode_ == Mode::Mono ? "ScreenToWorldPoint_Injected" : "ScreenToWorldPoint");
				if (mode_ == Mode::Mono && method) {
					const Vector3 vec3{};
					method->Invoke<void>(this, position, eye, &vec3);
					return vec3;
				}
				if (method) return method->Invoke<Vector3>(this, position, eye);
				return {};
			}

			auto CameraToWorldMatrix() -> Matrix4x4 {

				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Camera")->Get<Method>(mode_ == Mode::Mono ? "get_cameraToWorldMatrix_Injected" : "get_cameraToWorldMatrix");
				if (mode_ == Mode::Mono && method) {
					Matrix4x4 matrix4{};
					method->Invoke<void>(this, &matrix4);
					return matrix4;
				}
				if (method) return method->Invoke<Matrix4x4>(this);
				return {};
			}
		};

		struct Transform : Component {
			auto GetPosition() -> Vector3 {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>("get_position_Injected");
				const Vector3 vec3{};
				method->Invoke<void>(this, &vec3);
				return vec3;
			}

			auto SetPosition(const Vector3& position) -> void {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>("set_position_Injected");
				return method->Invoke<void>(this, &position);
			}

			auto GetRight() -> Vector3 {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>("get_right");
				if (method) return method->Invoke<Vector3>(this);
				return {};
			}

			auto SetRight(const Vector3& value) -> void {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>("set_right");
				if (method) return method->Invoke<void>(this, value);
			}

			auto GetUp() -> Vector3 {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>("get_up");
				if (method) return method->Invoke<Vector3>(this);
				return {};
			}

			auto SetUp(const Vector3& value) -> void {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>("set_up");
				if (method) return method->Invoke<void>(this, value);
			}

			auto GetForward() -> Vector3 {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>("get_forward");
				if (method) return method->Invoke<Vector3>(this);
				return {};
			}

			auto SetForward(const Vector3& value) -> void {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>("set_forward");
				if (method) return method->Invoke<void>(this, value);
			}

			auto GetRotation() -> Quaternion {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>(mode_ == Mode::Mono ? "get_rotation_Injected" : "get_rotation");
				if (mode_ == Mode::Mono && method) {
					const Quaternion vec3{};
					method->Invoke<void>(this, &vec3);
					return vec3;
				}
				if (method) return method->Invoke<Quaternion>(this);
				return {};
			}

			auto SetRotation(const Quaternion& position) -> void {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>(mode_ == Mode::Mono ? "set_rotation_Injected" : "set_rotation");
				if (mode_ == Mode::Mono && method) return method->Invoke<void>(this, &position);
				if (method) return method->Invoke<void>(this, position);
			}

			auto GetLocalPosition() -> Vector3 {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>(mode_ == Mode::Mono ? "get_localPosition_Injected" : "get_localPosition");
				if (mode_ == Mode::Mono && method) {
					const Vector3 vec3{};
					method->Invoke<void>(this, &vec3);
					return vec3;
				}
				if (method) return method->Invoke<Vector3>(this);
				return {};
			}

			auto SetLocalPosition(const Vector3& position) -> void {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>(mode_ == Mode::Mono ? "set_localPosition_Injected" : "set_localPosition");
				if (mode_ == Mode::Mono && method) return method->Invoke<void>(this, &position);
				if (method) return method->Invoke<void>(this, position);
			}

			auto GetLocalRotation() -> Quaternion {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>(mode_ == Mode::Mono ? "get_localRotation_Injected" : "get_localRotation");
				if (mode_ == Mode::Mono && method) {
					const Quaternion vec3{};
					method->Invoke<void>(this, &vec3);
					return vec3;
				}
				if (method) return method->Invoke<Quaternion>(this);
				return {};
			}

			auto SetLocalRotation(const Quaternion& position) -> void {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>(mode_ == Mode::Mono ? "set_localRotation_Injected" : "set_localRotation");
				if (mode_ == Mode::Mono && method) return method->Invoke<void>(this, &position);
				if (method) return method->Invoke<void>(this, position);
			}

			auto GetLocalScale() -> Vector3 {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>("get_localScale_Injected");
				const Vector3 vec3{};
				method->Invoke<void>(this, &vec3);
				return vec3;
			}

			auto SetLocalScale(const Vector3& position) -> void {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>(mode_ == Mode::Mono ? "set_localScale_Injected" : "set_localScale");
				if (mode_ == Mode::Mono && method) return method->Invoke<void>(this, &position);
				if (method) return method->Invoke<void>(this, position);
			}

			auto GetChildCount() -> int {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>("get_childCount");
				if (method) return method->Invoke<int>(this);
				return 0;
			}

			auto GetChild(const int index) -> Transform* {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>("GetChild");
				if (method) return method->Invoke<Transform*>(this, index);
				return nullptr;
			}

			auto GetRoot() -> Transform* {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>("GetRoot");
				if (method) return method->Invoke<Transform*>(this);
				return nullptr;
			}

			auto GetParent() -> Transform* {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>("GetParent");
				if (method) return method->Invoke<Transform*>(this);
				return nullptr;
			}

			auto GetLossyScale() -> Vector3 {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>(mode_ == Mode::Mono ? "get_lossyScale_Injected" : "get_lossyScale");
				if (mode_ == Mode::Mono && method) {
					const Vector3 vec3{};
					method->Invoke<void>(this, &vec3);
					return vec3;
				}
				if (method) return method->Invoke<Vector3>(this);
				return {};
			}

			auto TransformPoint(const Vector3& position) -> Vector3 {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>(mode_ == Mode::Mono ? "TransformPoint_Injected" : "TransformPoint");
				if (mode_ == Mode::Mono && method) {
					const Vector3 vec3{};
					method->Invoke<void>(this, position, &vec3);
					return vec3;
				}
				if (method) return method->Invoke<Vector3>(this, position);
				return {};
			}

			auto LookAt(const Vector3& worldPosition) -> void {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>("LookAt", { "Vector3" });
				if (method) return method->Invoke<void>(this, worldPosition);
			}

			auto Rotate(const Vector3& eulers) -> void {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Transform")->Get<Method>("Rotate", { "Vector3" });
				if (method) return method->Invoke<void>(this, eulers);
			}
		};

		struct GameObject : UnityObject {
			static auto Create(GameObject* obj, const std::string& name) -> void {
				if (!obj) return;
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("GameObject")->Get<Method>("Internal_CreateGameObject");
				if (method) method->Invoke<void, GameObject*, String*>(obj, String::New(name));
			}

			static auto FindGameObjectsWithTag(const std::string& name) -> std::vector<GameObject*> {
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("GameObject")->Get<Method>("FindGameObjectsWithTag");
				if (method) {
					const auto array = method->Invoke<Array<GameObject*>*>(String::New(name));
					return array->ToVector();
				}
				return {};
			}

			static auto Find(const std::string& name) -> GameObject* {
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("GameObject")->Get<Method>("Find");
				if (method) return method->Invoke<GameObject*>(String::New(name));
				return nullptr;
			}

			auto GetActive() -> bool {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("GameObject")->Get<Method>("get_active");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto SetActive(const bool value) -> void {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("GameObject")->Get<Method>("set_active");
				if (method) return method->Invoke<void>(this, value);
			}

			auto GetActiveSelf() -> bool {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("GameObject")->Get<Method>("get_activeSelf");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetActiveInHierarchy() -> bool {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("GameObject")->Get<Method>("get_activeInHierarchy");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetIsStatic() -> bool {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("GameObject")->Get<Method>("get_isStatic");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto GetTransform() -> Transform* {
				static Method* method;

				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("GameObject")->Get<Method>("get_transform");
				if (method) return method->Invoke<Transform*>(this);
				return nullptr;
			}

			auto GetTag() -> String* {

				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("GameObject")->Get<Method>("get_tag");
				if (method) return method->Invoke<String*>(this);
				return {};
			}

			template <typename T>
			auto GetComponent() -> T {

				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("GameObject")->Get<Method>("GetComponent");
				if (method) return method->Invoke<T>(this);
				return T();
			}

			template <typename T>
			auto GetComponent(Class* type) -> T {

				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("GameObject")->Get<Method>("GetComponent", { "System.Type" });
				if (method) return method->Invoke<T>(this, type->GetType());
				return T();
			}

			template <typename T>
			auto GetComponentInChildren(Class* type) -> T {

				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("GameObject")->Get<Method>("GetComponentInChildren", { "System.Type" });
				if (method) return method->Invoke<T>(this, type->GetType());
				return T();
			}

			template <typename T>
			auto GetComponentInParent(Class* type) -> T {

				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("GameObject")->Get<Method>("GetComponentInParent", { "System.Type" });
				if (method) return method->Invoke<T>(this, type->GetType());
				return T();
			}

			template <typename T>
			auto GetComponents(Class* type, bool useSearchTypeAsArrayReturnType = false, bool recursive = false, bool includeInactive = true, bool reverse = false, List<T>* resultList = nullptr) -> std::vector<T> {

				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("GameObject")->Get<Method>("GetComponentsInternal");
				if (method) return method->Invoke<Array<T>*>(this, type->GetType(), useSearchTypeAsArrayReturnType, recursive, includeInactive, reverse, resultList)->ToVector();
				return {};
			}

			template <typename T>
			auto GetComponentsInChildren(Class* type, const bool includeInactive = false) -> std::vector<T> { return GetComponents<T>(type, false, true, includeInactive, false, nullptr); }


			template <typename T>
			auto GetComponentsInParent(Class* type, const bool includeInactive = false) -> std::vector<T> { return GetComponents<T>(type, false, true, includeInactive, true, nullptr); }
		};

		struct LayerMask : Object {
			int m_Mask;

			static auto NameToLayer(const std::string& layerName) -> int {
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("LayerMask")->Get<Method>("NameToLayer");
				if (method) return method->Invoke<int>(String::New(layerName));
				return 0;
			}

			static auto LayerToName(const int layer) -> String* {
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("LayerMask")->Get<Method>("LayerToName");
				if (method) return method->Invoke<String*>(layer);
				return {};
			}
		};

		struct Rigidbody : Component {
			auto GetDetectCollisions() -> bool {
				static Method* method;
				if (!method) method = Get("UnityEngine.PhysicsModule.dll")->Get("Rigidbody")->Get<Method>("get_detectCollisions");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto SetDetectCollisions(const bool value) -> void {
				static Method* method;
				if (!method) method = Get("UnityEngine.PhysicsModule.dll")->Get("Rigidbody")->Get<Method>("set_detectCollisions");
				if (method) return method->Invoke<void>(this, value);
			}

			auto GetVelocity() -> Vector3 {
				static Method* method;
				if (!method) method = Get("UnityEngine.PhysicsModule.dll")->Get("Rigidbody")->Get<Method>(mode_ == Mode::Mono ? "get_velocity_Injected" : "get_velocity");
				if (mode_ == Mode::Mono && method) {
					Vector3 vector;
					method->Invoke<void>(this, &vector);
					return vector;
				}
				if (method) return method->Invoke<Vector3>(this);
				return {};
			}

			auto SetVelocity(Vector3 value) -> void {
				static Method* method;
				if (!method) method = Get("UnityEngine.PhysicsModule.dll")->Get("Rigidbody")->Get<Method>(mode_ == Mode::Mono ? "set_velocity_Injected" : "set_velocity");
				if (mode_ == Mode::Mono && method) return method->Invoke<void>(this, &value);
				if (method) return method->Invoke<void>(this, value);
			}
		};

		struct Collider : Component {
			auto GetBounds() -> Bounds {

				static Method* method;
				if (!method) method = Get("UnityEngine.PhysicsModule.dll")->Get("Collider")->Get<Method>("get_bounds_Injected");
				if (method) {
					Bounds bounds;
					method->Invoke<void>(this, &bounds);
					return bounds;
				}
				return {};
			}
		};

		struct Mesh : UnityObject {
			auto GetBounds() -> Bounds {

				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Mesh")->Get<Method>("get_bounds_Injected");
				if (method) {
					Bounds bounds;
					method->Invoke<void>(this, &bounds);
					return bounds;
				}
				return {};
			}
		};

		struct CapsuleCollider : Collider {
			auto GetCenter() -> Vector3 {
				static Method* method;
				if (!method) method = Get("UnityEngine.PhysicsModule.dll")->Get("CapsuleCollider")->Get<Method>("get_center");
				if (method) return method->Invoke<Vector3>(this);
				return {};
			}

			auto GetDirection() -> Vector3 {
				static Method* method;
				if (!method) method = Get("UnityEngine.PhysicsModule.dll")->Get("CapsuleCollider")->Get<Method>("get_direction");
				if (method) return method->Invoke<Vector3>(this);
				return {};
			}

			auto GetHeightn() -> Vector3 {
				static Method* method;
				if (!method) method = Get("UnityEngine.PhysicsModule.dll")->Get("CapsuleCollider")->Get<Method>("get_height");
				if (method) return method->Invoke<Vector3>(this);
				return {};
			}

			auto GetRadius() -> Vector3 {
				static Method* method;
				if (!method) method = Get("UnityEngine.PhysicsModule.dll")->Get("CapsuleCollider")->Get<Method>("get_radius");
				if (method) return method->Invoke<Vector3>(this);
				return {};
			}
		};

		struct BoxCollider : Collider {
			auto GetCenter() -> Vector3 {
				static Method* method;
				if (!method) method = Get("UnityEngine.PhysicsModule.dll")->Get("BoxCollider")->Get<Method>("get_center");
				if (method) return method->Invoke<Vector3>(this);
				return {};
			}

			auto GetSize() -> Vector3 {
				static Method* method;
				if (!method) method = Get("UnityEngine.PhysicsModule.dll")->Get("BoxCollider")->Get<Method>("get_size");
				if (method) return method->Invoke<Vector3>(this);
				return {};
			}
		};

		struct Renderer : Component {
			auto GetBounds() -> Bounds {

				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Renderer")->Get<Method>("get_bounds_Injected");
				if (method) {
					Bounds bounds;
					method->Invoke<void>(this, &bounds);
					return bounds;
				}
				return {};
			}
		};

		struct Behaviour : public Component {
			auto GetEnabled() -> bool {

				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Behaviour")->Get<Method>("get_enabled");
				if (method) return method->Invoke<bool>(this);
				return false;
			}

			auto SetEnabled(const bool value) -> void {

				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Behaviour")->Get<Method>("set_enabled");
				if (method) return method->Invoke<void>(this, value);
			}
		};

		struct MonoBehaviour : public Behaviour {};

		struct Physics : Object {
			static auto Linecast(const Vector3& start, const Vector3& end) -> bool {
				static Method* method;
				if (!method) method = Get("UnityEngine.PhysicsModule.dll")->Get("Physics")->Get<Method>("Linecast", { "*", "*" });
				if (method) return method->Invoke<bool>(start, end);
				return false;
			}

			static auto Raycast(const Vector3& origin, const Vector3& direction, const float maxDistance) -> bool {
				static Method* method;
				if (!method) method = Get("UnityEngine.PhysicsModule.dll")->Get("Physics")->Get<Method>("Raycast", { "UnityEngine.Vector3", "UnityEngine.Vector3", "System.Single" });
				if (method) return method->Invoke<bool>(origin, direction, maxDistance);
				return false;
			}

			static auto Raycast(const Ray& origin, const RaycastHit* direction, const float maxDistance) -> bool {
				static Method* method;
				if (!method) method = Get("UnityEngine.PhysicsModule.dll")->Get("Physics")->Get<Method>("Raycast", { "UnityEngine.Ray", "UnityEngine.RaycastHit&", "System.Single" });
				if (method) return method->Invoke<bool, Ray>(origin, direction, maxDistance);
				return false;
			}

			static auto IgnoreCollision(Collider* collider1, Collider* collider2) -> void {
				static Method* method;
				if (!method) method = Get("UnityEngine.PhysicsModule.dll")->Get("Physics")->Get<Method>("IgnoreCollision1", { "*", "*" });
				if (method) return method->Invoke<void>(collider1, collider2);
			}
		};

		struct Animator : Behaviour {
			enum class HumanBodyBones : int {
				Hips,
				LeftUpperLeg,
				RightUpperLeg,
				LeftLowerLeg,
				RightLowerLeg,
				LeftFoot,
				RightFoot,
				Spine,
				Chest,
				UpperChest = 54,
				Neck = 9,
				Head,
				LeftShoulder,
				RightShoulder,
				LeftUpperArm,
				RightUpperArm,
				LeftLowerArm,
				RightLowerArm,
				LeftHand,
				RightHand,
				LeftToes,
				RightToes,
				LeftEye,
				RightEye,
				Jaw,
				LeftThumbProximal,
				LeftThumbIntermediate,
				LeftThumbDistal,
				LeftIndexProximal,
				LeftIndexIntermediate,
				LeftIndexDistal,
				LeftMiddleProximal,
				LeftMiddleIntermediate,
				LeftMiddleDistal,
				LeftRingProximal,
				LeftRingIntermediate,
				LeftRingDistal,
				LeftLittleProximal,
				LeftLittleIntermediate,
				LeftLittleDistal,
				RightThumbProximal,
				RightThumbIntermediate,
				RightThumbDistal,
				RightIndexProximal,
				RightIndexIntermediate,
				RightIndexDistal,
				RightMiddleProximal,
				RightMiddleIntermediate,
				RightMiddleDistal,
				RightRingProximal,
				RightRingIntermediate,
				RightRingDistal,
				RightLittleProximal,
				RightLittleIntermediate,
				RightLittleDistal,
				LastBone = 55
			};

			auto GetBoneTransform(const HumanBodyBones humanBoneId) -> Transform* {
				static Method* method;

				if (!method) method = Get("UnityEngine.AnimationModule.dll")->Get("Animator")->Get<Method>("GetBoneTransform");
				if (method) return method->Invoke<Transform*>(this, humanBoneId);
				return nullptr;
			}
		};

		struct Time {
			static auto GetTime() -> float {
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Time")->Get<Method>("get_time");
				if (method) return method->Invoke<float>();
				return 0.0f;
			}

			static auto GetDeltaTime() -> float {
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Time")->Get<Method>("get_deltaTime");
				if (method) return method->Invoke<float>();
				return 0.0f;
			}

			static auto GetFixedDeltaTime() -> float {
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Time")->Get<Method>("get_fixedDeltaTime");
				if (method) return method->Invoke<float>();
				return 0.0f;
			}

			static auto GetTimeScale() -> float {
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Time")->Get<Method>("get_timeScale");
				if (method) return method->Invoke<float>();
				return 0.0f;
			}

			static auto SetTimeScale(const float value) -> void {
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Time")->Get<Method>("set_timeScale");
				if (method) return method->Invoke<void>(value);
			}
		};

		struct Screen {
			static auto get_width() -> Int32 {
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Screen")->Get<Method>("get_width");
				if (method) return method->Invoke<int32_t>();
				return 0;
			}

			static auto get_height() -> Int32 {
				static Method* method;
				if (!method) method = Get("UnityEngine.CoreModule.dll")->Get("Screen")->Get<Method>("get_height");
				if (method) return method->Invoke<int32_t>();
				return 0;
			}
		};

		template <typename Return, typename... Args>
		static auto Invoke(void* address, Args... args) -> Return {
#if WINDOWS_MODE
			if (address != nullptr) return reinterpret_cast<Return(*)(Args...)>(address)(args...);
#elif LINUX_MODE || ANDROID_MODE || IOS_MODE || HARMONYOS_MODE
			if (address != nullptr) return ((Return(*)(Args...))(address))(args...);
#endif
			return Return();
		}
	};

private:
	inline static Mode                                   mode_{};
	inline static void* hmodule_;
	inline static std::unordered_map<std::string, void*> address_{};
	inline static void* pDomain{};
};
#endif // UNITYRESOLVE_HPPs
