using System.Collections;
using System.Collections.Generic;
using System.Reflection;
using System.Reflection.Emit;
using System.Text.RegularExpressions;

namespace System.Xml.Serialization;

internal class XmlSerializationILGen
{
	private int nextMethodNumber;

	private Hashtable methodNames = new Hashtable();

	private Dictionary<string, MethodBuilderInfo> methodBuilders = new Dictionary<string, MethodBuilderInfo>();

	internal Dictionary<string, Type> CreatedTypes = new Dictionary<string, Type>();

	internal Dictionary<string, MemberInfo> memberInfos = new Dictionary<string, MemberInfo>();

	private ReflectionAwareILGen raCodeGen;

	private TypeScope[] scopes;

	private TypeDesc stringTypeDesc;

	private TypeDesc qnameTypeDesc;

	private string className;

	private TypeMapping[] referencedMethods;

	private int references;

	private Hashtable generatedMethods = new Hashtable();

	private ModuleBuilder moduleBuilder;

	private TypeAttributes typeAttributes;

	protected TypeBuilder typeBuilder;

	protected CodeGenerator ilg;

	private static Dictionary<string, Regex> regexs = new Dictionary<string, Regex>();

	internal int NextMethodNumber
	{
		get
		{
			return nextMethodNumber;
		}
		set
		{
			nextMethodNumber = value;
		}
	}

	internal ReflectionAwareILGen RaCodeGen => raCodeGen;

	internal TypeDesc StringTypeDesc => stringTypeDesc;

	internal TypeDesc QnameTypeDesc => qnameTypeDesc;

	internal string ClassName => className;

	internal TypeScope[] Scopes => scopes;

	internal Hashtable MethodNames => methodNames;

	internal Hashtable GeneratedMethods => generatedMethods;

	internal ModuleBuilder ModuleBuilder
	{
		get
		{
			return moduleBuilder;
		}
		set
		{
			moduleBuilder = value;
		}
	}

	internal TypeAttributes TypeAttributes => typeAttributes;

	internal XmlSerializationILGen(TypeScope[] scopes, string access, string className)
	{
		this.scopes = scopes;
		if (scopes.Length != 0)
		{
			stringTypeDesc = scopes[0].GetTypeDesc(typeof(string));
			qnameTypeDesc = scopes[0].GetTypeDesc(typeof(XmlQualifiedName));
		}
		raCodeGen = new ReflectionAwareILGen();
		this.className = className;
		typeAttributes = TypeAttributes.Public;
	}

	internal static Regex NewRegex(string pattern)
	{
		Regex value;
		lock (regexs)
		{
			if (!regexs.TryGetValue(pattern, out value))
			{
				value = new Regex(pattern);
				regexs.Add(pattern, value);
			}
		}
		return value;
	}

	internal MethodBuilder EnsureMethodBuilder(TypeBuilder typeBuilder, string methodName, MethodAttributes attributes, Type returnType, Type[] parameterTypes)
	{
		if (!methodBuilders.TryGetValue(methodName, out var value))
		{
			value = new MethodBuilderInfo(typeBuilder.DefineMethod(methodName, attributes, returnType, parameterTypes), parameterTypes);
			methodBuilders.Add(methodName, value);
		}
		return value.MethodBuilder;
	}

	internal MethodBuilderInfo GetMethodBuilder(string methodName)
	{
		return methodBuilders[methodName];
	}

	internal virtual void GenerateMethod(TypeMapping mapping)
	{
	}

	internal void GenerateReferencedMethods()
	{
		while (references > 0)
		{
			TypeMapping mapping = referencedMethods[--references];
			GenerateMethod(mapping);
		}
	}

	internal string ReferenceMapping(TypeMapping mapping)
	{
		if (generatedMethods[mapping] == null)
		{
			referencedMethods = EnsureArrayIndex(referencedMethods, references);
			referencedMethods[references++] = mapping;
		}
		return (string)methodNames[mapping];
	}

	private TypeMapping[] EnsureArrayIndex(TypeMapping[] a, int index)
	{
		if (a == null)
		{
			return new TypeMapping[32];
		}
		if (index < a.Length)
		{
			return a;
		}
		TypeMapping[] array = new TypeMapping[a.Length + 32];
		Array.Copy(a, array, index);
		return array;
	}

	internal FieldBuilder GenerateHashtableGetBegin(string privateName, string publicName, TypeBuilder serializerContractTypeBuilder)
	{
		FieldBuilder fieldBuilder = serializerContractTypeBuilder.DefineField(privateName, typeof(Hashtable), FieldAttributes.Private);
		ilg = new CodeGenerator(serializerContractTypeBuilder);
		PropertyBuilder propertyBuilder = serializerContractTypeBuilder.DefineProperty(publicName, PropertyAttributes.None, CallingConventions.HasThis, typeof(Hashtable), null, null, null, null, null);
		ilg.BeginMethod(typeof(Hashtable), "get_" + publicName, CodeGenerator.EmptyTypeArray, CodeGenerator.EmptyStringArray, CodeGenerator.PublicOverrideMethodAttributes | MethodAttributes.SpecialName);
		propertyBuilder.SetGetMethod(ilg.MethodBuilder);
		ilg.Ldarg(0);
		ilg.LoadMember(fieldBuilder);
		ilg.Load(null);
		ilg.If(Cmp.EqualTo);
		ConstructorInfo constructor = typeof(Hashtable).GetConstructor(CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		LocalBuilder local = ilg.DeclareLocal(typeof(Hashtable), "_tmp");
		ilg.New(constructor);
		ilg.Stloc(local);
		return fieldBuilder;
	}

	internal void GenerateHashtableGetEnd(FieldBuilder fieldBuilder)
	{
		ilg.Ldarg(0);
		ilg.LoadMember(fieldBuilder);
		ilg.Load(null);
		ilg.If(Cmp.EqualTo);
		ilg.Ldarg(0);
		ilg.Ldloc(typeof(Hashtable), "_tmp");
		ilg.StoreMember(fieldBuilder);
		ilg.EndIf();
		ilg.EndIf();
		ilg.Ldarg(0);
		ilg.LoadMember(fieldBuilder);
		ilg.GotoMethodEnd();
		ilg.EndMethod();
	}

	internal FieldBuilder GeneratePublicMethods(string privateName, string publicName, string[] methods, XmlMapping[] xmlMappings, TypeBuilder serializerContractTypeBuilder)
	{
		FieldBuilder fieldBuilder = GenerateHashtableGetBegin(privateName, publicName, serializerContractTypeBuilder);
		if (methods != null && methods.Length != 0 && xmlMappings != null && xmlMappings.Length == methods.Length)
		{
			MethodInfo method = typeof(Hashtable).GetMethod("set_Item", CodeGenerator.InstanceBindingFlags, null, new Type[2]
			{
				typeof(object),
				typeof(object)
			}, null);
			for (int i = 0; i < methods.Length; i++)
			{
				if (methods[i] != null)
				{
					ilg.Ldloc(typeof(Hashtable), "_tmp");
					ilg.Ldstr(xmlMappings[i].Key);
					ilg.Ldstr(methods[i]);
					ilg.Call(method);
				}
			}
		}
		GenerateHashtableGetEnd(fieldBuilder);
		return fieldBuilder;
	}

	internal void GenerateSupportedTypes(Type[] types, TypeBuilder serializerContractTypeBuilder)
	{
		ilg = new CodeGenerator(serializerContractTypeBuilder);
		ilg.BeginMethod(typeof(bool), "CanSerialize", new Type[1] { typeof(Type) }, new string[1] { "type" }, CodeGenerator.PublicOverrideMethodAttributes);
		Hashtable hashtable = new Hashtable();
		foreach (Type type in types)
		{
			if (!(type == null) && (type.IsPublic || type.IsNestedPublic) && hashtable[type] == null && !type.IsGenericType && !type.ContainsGenericParameters)
			{
				hashtable[type] = type;
				ilg.Ldarg("type");
				ilg.Ldc(type);
				ilg.If(Cmp.EqualTo);
				ilg.Ldc(boolVar: true);
				ilg.GotoMethodEnd();
				ilg.EndIf();
			}
		}
		ilg.Ldc(boolVar: false);
		ilg.GotoMethodEnd();
		ilg.EndMethod();
	}

	internal string GenerateBaseSerializer(string baseSerializer, string readerClass, string writerClass, CodeIdentifiers classes)
	{
		baseSerializer = CodeIdentifier.MakeValid(baseSerializer);
		baseSerializer = classes.AddUnique(baseSerializer, baseSerializer);
		TypeBuilder typeBuilder = CodeGenerator.CreateTypeBuilder(moduleBuilder, CodeIdentifier.GetCSharpName(baseSerializer), TypeAttributes.Public | TypeAttributes.Abstract | TypeAttributes.BeforeFieldInit, typeof(XmlSerializer), CodeGenerator.EmptyTypeArray);
		ConstructorInfo constructor = CreatedTypes[readerClass].GetConstructor(CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg = new CodeGenerator(typeBuilder);
		ilg.BeginMethod(typeof(XmlSerializationReader), "CreateReader", CodeGenerator.EmptyTypeArray, CodeGenerator.EmptyStringArray, CodeGenerator.ProtectedOverrideMethodAttributes);
		ilg.New(constructor);
		ilg.EndMethod();
		ConstructorInfo constructor2 = CreatedTypes[writerClass].GetConstructor(CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg.BeginMethod(typeof(XmlSerializationWriter), "CreateWriter", CodeGenerator.EmptyTypeArray, CodeGenerator.EmptyStringArray, CodeGenerator.ProtectedOverrideMethodAttributes);
		ilg.New(constructor2);
		ilg.EndMethod();
		typeBuilder.DefineDefaultConstructor(MethodAttributes.Family | MethodAttributes.HideBySig | MethodAttributes.SpecialName | MethodAttributes.RTSpecialName);
		Type type = typeBuilder.CreateType();
		CreatedTypes.Add(type.Name, type);
		return baseSerializer;
	}

	internal string GenerateTypedSerializer(string readMethod, string writeMethod, XmlMapping mapping, CodeIdentifiers classes, string baseSerializer, string readerClass, string writerClass)
	{
		string text = CodeIdentifier.MakeValid(Accessor.UnescapeName(mapping.Accessor.Mapping.TypeDesc.Name));
		text = classes.AddUnique(text + "Serializer", mapping);
		TypeBuilder typeBuilder = CodeGenerator.CreateTypeBuilder(moduleBuilder, CodeIdentifier.GetCSharpName(text), TypeAttributes.Public | TypeAttributes.Sealed | TypeAttributes.BeforeFieldInit, CreatedTypes[baseSerializer], CodeGenerator.EmptyTypeArray);
		ilg = new CodeGenerator(typeBuilder);
		ilg.BeginMethod(typeof(bool), "CanDeserialize", new Type[1] { typeof(XmlReader) }, new string[1] { "xmlReader" }, CodeGenerator.PublicOverrideMethodAttributes);
		if (mapping.Accessor.Any)
		{
			ilg.Ldc(boolVar: true);
			ilg.Stloc(ilg.ReturnLocal);
			ilg.Br(ilg.ReturnLabel);
		}
		else
		{
			MethodInfo method = typeof(XmlReader).GetMethod("IsStartElement", CodeGenerator.InstanceBindingFlags, null, new Type[2]
			{
				typeof(string),
				typeof(string)
			}, null);
			ilg.Ldarg(ilg.GetArg("xmlReader"));
			ilg.Ldstr(mapping.Accessor.Name);
			ilg.Ldstr(mapping.Accessor.Namespace);
			ilg.Call(method);
			ilg.Stloc(ilg.ReturnLocal);
			ilg.Br(ilg.ReturnLabel);
		}
		ilg.MarkLabel(ilg.ReturnLabel);
		ilg.Ldloc(ilg.ReturnLocal);
		ilg.EndMethod();
		if (writeMethod != null)
		{
			ilg = new CodeGenerator(typeBuilder);
			ilg.BeginMethod(typeof(void), "Serialize", new Type[2]
			{
				typeof(object),
				typeof(XmlSerializationWriter)
			}, new string[2] { "objectToSerialize", "writer" }, CodeGenerator.ProtectedOverrideMethodAttributes);
			MethodInfo method2 = CreatedTypes[writerClass].GetMethod(writeMethod, CodeGenerator.InstanceBindingFlags, null, new Type[1] { (mapping is XmlMembersMapping) ? typeof(object[]) : typeof(object) }, null);
			ilg.Ldarg("writer");
			ilg.Castclass(CreatedTypes[writerClass]);
			ilg.Ldarg("objectToSerialize");
			if (mapping is XmlMembersMapping)
			{
				ilg.ConvertValue(typeof(object), typeof(object[]));
			}
			ilg.Call(method2);
			ilg.EndMethod();
		}
		if (readMethod != null)
		{
			ilg = new CodeGenerator(typeBuilder);
			ilg.BeginMethod(typeof(object), "Deserialize", new Type[1] { typeof(XmlSerializationReader) }, new string[1] { "reader" }, CodeGenerator.ProtectedOverrideMethodAttributes);
			MethodInfo method3 = CreatedTypes[readerClass].GetMethod(readMethod, CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldarg("reader");
			ilg.Castclass(CreatedTypes[readerClass]);
			ilg.Call(method3);
			ilg.EndMethod();
		}
		typeBuilder.DefineDefaultConstructor(CodeGenerator.PublicMethodAttributes);
		Type type = typeBuilder.CreateType();
		CreatedTypes.Add(type.Name, type);
		return type.Name;
	}

	private FieldBuilder GenerateTypedSerializers(Hashtable serializers, TypeBuilder serializerContractTypeBuilder)
	{
		string privateName = "typedSerializers";
		FieldBuilder fieldBuilder = GenerateHashtableGetBegin(privateName, "TypedSerializers", serializerContractTypeBuilder);
		MethodInfo method = typeof(Hashtable).GetMethod("Add", CodeGenerator.InstanceBindingFlags, null, new Type[2]
		{
			typeof(object),
			typeof(object)
		}, null);
		foreach (string key in serializers.Keys)
		{
			ConstructorInfo constructor = CreatedTypes[(string)serializers[key]].GetConstructor(CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
			ilg.Ldloc(typeof(Hashtable), "_tmp");
			ilg.Ldstr(key);
			ilg.New(constructor);
			ilg.Call(method);
		}
		GenerateHashtableGetEnd(fieldBuilder);
		return fieldBuilder;
	}

	private void GenerateGetSerializer(Hashtable serializers, XmlMapping[] xmlMappings, TypeBuilder serializerContractTypeBuilder)
	{
		ilg = new CodeGenerator(serializerContractTypeBuilder);
		ilg.BeginMethod(typeof(XmlSerializer), "GetSerializer", new Type[1] { typeof(Type) }, new string[1] { "type" }, CodeGenerator.PublicOverrideMethodAttributes);
		for (int i = 0; i < xmlMappings.Length; i++)
		{
			if (xmlMappings[i] is XmlTypeMapping)
			{
				Type type = xmlMappings[i].Accessor.Mapping.TypeDesc.Type;
				if (!(type == null) && (type.IsPublic || type.IsNestedPublic) && !type.IsGenericType && !type.ContainsGenericParameters)
				{
					ilg.Ldarg("type");
					ilg.Ldc(type);
					ilg.If(Cmp.EqualTo);
					ConstructorInfo constructor = CreatedTypes[(string)serializers[xmlMappings[i].Key]].GetConstructor(CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
					ilg.New(constructor);
					ilg.Stloc(ilg.ReturnLocal);
					ilg.Br(ilg.ReturnLabel);
					ilg.EndIf();
				}
			}
		}
		ilg.Load(null);
		ilg.Stloc(ilg.ReturnLocal);
		ilg.Br(ilg.ReturnLabel);
		ilg.MarkLabel(ilg.ReturnLabel);
		ilg.Ldloc(ilg.ReturnLocal);
		ilg.EndMethod();
	}

	internal void GenerateSerializerContract(string className, XmlMapping[] xmlMappings, Type[] types, string readerType, string[] readMethods, string writerType, string[] writerMethods, Hashtable serializers)
	{
		TypeBuilder typeBuilder = CodeGenerator.CreateTypeBuilder(moduleBuilder, "XmlSerializerContract", TypeAttributes.Public | TypeAttributes.BeforeFieldInit, typeof(XmlSerializerImplementation), CodeGenerator.EmptyTypeArray);
		ilg = new CodeGenerator(typeBuilder);
		PropertyBuilder propertyBuilder = typeBuilder.DefineProperty("Reader", PropertyAttributes.None, CallingConventions.HasThis, typeof(XmlSerializationReader), null, null, null, null, null);
		ilg.BeginMethod(typeof(XmlSerializationReader), "get_Reader", CodeGenerator.EmptyTypeArray, CodeGenerator.EmptyStringArray, CodeGenerator.PublicOverrideMethodAttributes | MethodAttributes.SpecialName);
		propertyBuilder.SetGetMethod(ilg.MethodBuilder);
		ConstructorInfo constructor = CreatedTypes[readerType].GetConstructor(CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg.New(constructor);
		ilg.EndMethod();
		ilg = new CodeGenerator(typeBuilder);
		PropertyBuilder propertyBuilder2 = typeBuilder.DefineProperty("Writer", PropertyAttributes.None, CallingConventions.HasThis, typeof(XmlSerializationWriter), null, null, null, null, null);
		ilg.BeginMethod(typeof(XmlSerializationWriter), "get_Writer", CodeGenerator.EmptyTypeArray, CodeGenerator.EmptyStringArray, CodeGenerator.PublicOverrideMethodAttributes | MethodAttributes.SpecialName);
		propertyBuilder2.SetGetMethod(ilg.MethodBuilder);
		constructor = CreatedTypes[writerType].GetConstructor(CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg.New(constructor);
		ilg.EndMethod();
		FieldBuilder memberInfo = GeneratePublicMethods("readMethods", "ReadMethods", readMethods, xmlMappings, typeBuilder);
		FieldBuilder memberInfo2 = GeneratePublicMethods("writeMethods", "WriteMethods", writerMethods, xmlMappings, typeBuilder);
		FieldBuilder memberInfo3 = GenerateTypedSerializers(serializers, typeBuilder);
		GenerateSupportedTypes(types, typeBuilder);
		GenerateGetSerializer(serializers, xmlMappings, typeBuilder);
		ConstructorInfo constructor2 = typeof(XmlSerializerImplementation).GetConstructor(CodeGenerator.InstanceBindingFlags, null, CodeGenerator.EmptyTypeArray, null);
		ilg = new CodeGenerator(typeBuilder);
		ilg.BeginMethod(typeof(void), ".ctor", CodeGenerator.EmptyTypeArray, CodeGenerator.EmptyStringArray, CodeGenerator.PublicMethodAttributes | MethodAttributes.RTSpecialName | MethodAttributes.SpecialName);
		ilg.Ldarg(0);
		ilg.Load(null);
		ilg.StoreMember(memberInfo);
		ilg.Ldarg(0);
		ilg.Load(null);
		ilg.StoreMember(memberInfo2);
		ilg.Ldarg(0);
		ilg.Load(null);
		ilg.StoreMember(memberInfo3);
		ilg.Ldarg(0);
		ilg.Call(constructor2);
		ilg.EndMethod();
		Type type = typeBuilder.CreateType();
		CreatedTypes.Add(type.Name, type);
	}

	internal static bool IsWildcard(SpecialMapping mapping)
	{
		if (mapping is SerializableMapping)
		{
			return ((SerializableMapping)mapping).IsAny;
		}
		return mapping.TypeDesc.CanBeElementValue;
	}

	internal void ILGenLoad(string source)
	{
		ILGenLoad(source, null);
	}

	internal void ILGenLoad(string source, Type type)
	{
		if (source.StartsWith("o.@", StringComparison.Ordinal))
		{
			MemberInfo memberInfo = memberInfos[source.Substring(3)];
			ilg.LoadMember(ilg.GetVariable("o"), memberInfo);
			if (type != null)
			{
				Type source2 = ((memberInfo.MemberType == MemberTypes.Field) ? ((FieldInfo)memberInfo).FieldType : ((PropertyInfo)memberInfo).PropertyType);
				ilg.ConvertValue(source2, type);
			}
		}
		else
		{
			new SourceInfo(source, null, null, null, ilg).Load(type);
		}
	}
}
